#include "options.h"
#if SDC_CS!=255
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "vfs_api.h"
#include "sd_diskio.h"
//#define USE_SD
#include "config.h"
#include "sdmanager.h"
#include "display.h"
#include "player.h"

#if defined(SD_SPIPINS) || SD_HSPI
SPIClass  SDSPI(HSPI);
#define SDREALSPI SDSPI
#else
  #define SDREALSPI SPI
#endif

#ifndef SDSPISPEED
  #define SDSPISPEED 20000000
#endif

SDManager sdman(FSImplPtr(new VFSImpl()));
//-------------------------------------
bool SDManager::start(){
  // 设置 SPI 引脚（参数顺序：SCLK, MISO, MOSI, CS）
  SDREALSPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SDC_CS);

  ready = begin(SDC_CS, SDREALSPI, SDSPISPEED);
  vTaskDelay(10);
  if(!ready) ready = begin(SDC_CS, SDREALSPI, SDSPISPEED);
  vTaskDelay(20);
  if(!ready) ready = begin(SDC_CS, SDREALSPI, SDSPISPEED);
  vTaskDelay(50);
  if(!ready) ready = begin(SDC_CS, SDREALSPI, SDSPISPEED);
  return ready;
}
//--------------------------------------


bool SDManager::validatePlaylist() {
  if (!exists(PLAYLIST_SD_PATH)) return false;
  
  File playlist = open(PLAYLIST_SD_PATH, "r");
  if (!playlist) return false;
  
  bool valid = true;
  int lineNum = 0;
  
  while (playlist.available()) {
    lineNum++;
    String line = playlist.readStringUntil('\n');
    int tabPos = line.indexOf('\t');
    if (tabPos < 0) {
      valid = false;
      break;
    }
    
    String fileName = line.substring(0, tabPos);
    String filePath = line.substring(tabPos + 1, line.indexOf('\t', tabPos + 1));
    filePath.trim();
    
    // 检查文件是否存在
    if (!exists(filePath.c_str())) {
      Serial.printf("Playlist validation: missing file %s\n", filePath.c_str());
      valid = false;
      break;
    }
  }
  
  playlist.close();
  return valid;
}

bool SDManager::checkAndRebuildIfNeeded() {
  bool needRebuild = false;
  
  // 1. 检查必需文件是否存在
  bool hasSig = exists("/sd.sig");
  bool hasPlaylist = exists(PLAYLIST_SD_PATH);
  bool hasIndex = exists(INDEX_SD_PATH);
  
  if (!hasSig || !hasPlaylist || !hasIndex) {
    Serial.println("Missing SD files, rebuilding...");
    needRebuild = true;
  } else {
    // 2. 读取数据总量记录
    uint32_t savedCount = 0, savedSize = 0;
    File sig = open("/sd.sig", "r");
    if (sig) {
      sig.read((uint8_t*)&savedCount, 4);
      sig.read((uint8_t*)&savedSize, 4);
      sig.close();
    }
    
    // 3. 快速扫描实际文件
    uint32_t curCount = 0, curSize = 0;
    File root = open("/");
    if (root) {
      _quickScan(root, curCount, curSize, SD_MAX_LEVELS);
      root.close();
    }
    
    // 4. 比对数据总量
    if (curCount != savedCount || curSize != savedSize) {
      Serial.printf("SD size mismatch: saved(%lu,%lu) vs actual(%lu,%lu)\n", 
                    savedCount, savedSize, curCount, curSize);
      needRebuild = true;
    } else {
      // 5. 验证播放列表内容
      if (!validatePlaylist()) {
        Serial.println("Playlist validation failed, rebuilding...");
        needRebuild = true;
      }
    }
  }
  
  if (needRebuild) {
    indexSDPlaylist();  // 重建所有文件
    return true;
  }
  
  return false;
}


void SDManager::stop(){
  end();
  ready = false;
}
#include "diskio_impl.h"
bool SDManager::cardPresent() {
    if(!ready) return false;
    
    // 简单的验证：尝试打开根目录
    File root = sdman.open("/");
    if (!root) return false;
    root.close();
    return true;
}


bool SDManager::_checkNoMedia(const char* path){
  if (path[strlen(path) - 1] == '/')
    snprintf(config.tmpBuf, sizeof(config.tmpBuf), "%s%s", path, ".nomedia");
  else
    snprintf(config.tmpBuf, sizeof(config.tmpBuf), "%s/%s", path, ".nomedia");
  bool nm = exists(config.tmpBuf);
  return nm;
}

bool SDManager::_endsWith(const char* base, const char* str) {
    size_t blen = strlen(base);
    size_t slen = strlen(str);
    if (blen < slen) return false;
    
    // 直接比较，避免重复计算
    const char* baseEnd = base + blen - slen;
    return strcasecmp(baseEnd, str) == 0;
}

void SDManager::listSD(File &plSDfile, File &plSDindex, const char* dirname, uint8_t levels) {
    Serial.printf("Scanning directory: %s\n", dirname);

    File root = sdman.open(dirname);
    if (!root || !root.isDirectory()) {
        if(root) root.close();
        Serial.println("##[ERROR]#\tFailed to open directory");
        return;
    }

    // 使用静态缓冲区避免重复 malloc/free
    static char filePath[256];
    
    while (true) {
        vTaskDelay(2);
        player.loop();
        
        bool isDir;
        String fileName = root.getNextFileName(&isDir);
        if (fileName.isEmpty()) break;
        
        // 复制到静态缓冲区
        strlcpy(filePath, fileName.c_str(), sizeof(filePath));
        const char* fn = strrchr(filePath, '/') + 1;
        
        if (isDir) {
            if (levels && !_checkNoMedia(filePath)) {
                listSD(plSDfile, plSDindex, filePath, levels - 1);
            }
        } else {
            if (_endsWith(fn, ".mp3") || _endsWith(fn, ".m4a") || 
                _endsWith(fn, ".aac") || _endsWith(fn, ".wav") || 
                _endsWith(fn, ".flac")) {
                
                uint32_t pos = plSDfile.position();
                plSDfile.printf("%s\t%s\t0\n", fn, filePath);
                plSDindex.write((uint8_t*)&pos, 4);
                
                // 获取文件大小
                File f = sdman.open(filePath, "r");
                if (f) {
                    _totalSize += f.size();
                    f.close();
                }

                Serial.print(".");
                if(display.mode() == SDCHANGE) {
                    display.putRequest(SDFILEINDEX, _sdFCount + 1);
                }
                _sdFCount++;
                if (_sdFCount % 64 == 0) Serial.println();
            }
        }
    }
    root.close();
}
void SDManager::indexSDPlaylist() {

  _sdFCount = 0;
  _totalSize = 0;  // 新增：重置总大小
  // 删除原打印行

  if (exists(PLAYLIST_SD_PATH)) remove(PLAYLIST_SD_PATH);
  if (exists(INDEX_SD_PATH)) remove(INDEX_SD_PATH);

  File playlist = open(PLAYLIST_SD_PATH, "w", true);
  if (!playlist) {
    Serial.printf("Failed to open %s for writing\n", PLAYLIST_SD_PATH);
    Serial.printf("SD ready: %d, cardPresent: %d\n", ready, cardPresent());
    return;
  }
  File index = open(INDEX_SD_PATH, "w", true);
  if (!index) {
    Serial.println("Failed to open index file");
    playlist.close();
    return;
  }

  listSD(playlist, index, "/", SD_MAX_LEVELS);

    // 新增：保存签名
  File sig = open("/sd.sig", "w");
  if (sig) {
    sig.write((uint8_t*)&_sdFCount, 4);
    sig.write((uint8_t*)&_totalSize, 4);
    sig.close();
  }

  index.flush();
  index.close();
  playlist.flush();
  playlist.close();

  // 新增：打印最终计数
  //Serial.printf("SD index found %d audio files\n", _sdFCount);
  Serial.printf("SD index found %d audio files, total size %u bytes\n", _sdFCount, _totalSize);
  delay(50);
}


void SDManager::_quickScan(File dir, uint32_t &count, uint32_t &size, uint8_t levels) {
    if (levels == 0) return;
    
    // 使用静态缓冲区
    static char filePath[256];
    
    while (true) {
        vTaskDelay(1);
        
        bool isDir;
        String fileName = dir.getNextFileName(&isDir);
        if (fileName.isEmpty()) break;
        
        strlcpy(filePath, fileName.c_str(), sizeof(filePath));
        const char* fn = strrchr(filePath, '/') + 1;
        
        if (isDir) {
            if (!_checkNoMedia(filePath)) {
                File subDir = open(filePath);
                if (subDir) {
                    _quickScan(subDir, count, size, levels - 1);
                    subDir.close();
                }
            }
        } else {
            if (_endsWith(fn, ".mp3") || _endsWith(fn, ".m4a") || 
                _endsWith(fn, ".aac") || _endsWith(fn, ".wav") || 
                _endsWith(fn, ".flac")) {
                count++;
                File f = open(filePath, "r");
                if (f) {
                    size += f.size();
                    f.close();
                }
            }
        }
    }
}


bool SDManager::hasSDChanged() {
    // 读取上次保存的签名
    uint32_t savedCount = 0, savedSize = 0;
    File sig = open("/sd.sig", "r");
    if (sig) {
        sig.read((uint8_t*)&savedCount, 4);
        sig.read((uint8_t*)&savedSize, 4);
        sig.close();
    } else {
        // 没有签名文件，视为需要重建
        return true;
    }

    // 快速扫描当前SD卡
    uint32_t curCount = 0, curSize = 0;
    File root = open("/");
    if (!root) return true;
    _quickScan(root, curCount, curSize, SD_MAX_LEVELS);
    root.close();

    // 比对
    return (curCount != savedCount) || (curSize != savedSize);
}

#endif


