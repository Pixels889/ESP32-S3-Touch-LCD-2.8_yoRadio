// sdmanager.h 中，SDFS 需要完整类型
#ifndef sdmanager_h
#define sdmanager_h

#include <FS.h>  // 需要添加这个

class SDManager : public SDFS {
  public:
    bool ready;
    uint32_t _sdFCount;      // 改为 public 或添加 getter
    uint32_t _totalSize;      // 改为 public 或添加 getter
    
  public:
    SDManager(FSImplPtr impl) : SDFS(impl) {}
    bool start();
    void stop();
    bool cardPresent();
    void listSD(File &plSDfile, File &plSDindex, const char * dirname, uint8_t levels);
    void indexSDPlaylist();    
    bool hasSDChanged();
        // 新增：全面检查SD卡状态
    bool checkAndRebuildIfNeeded();
    
    // 新增：验证播放列表与实际文件是否一致
    bool validatePlaylist();
    
    // 添加 getter 方法
    uint32_t getFileCount() { return _sdFCount; }
    uint32_t getTotalSize() { return _totalSize; }
    
  private:
    bool _checkNoMedia(const char* path);
    bool _endsWith (const char* base, const char* str);
    void _quickScan(File dir, uint32_t &count, uint32_t &size, uint8_t levels);
};

extern SDManager sdman;
#if defined(SD_SPIPINS) || SD_HSPI
extern SPIClass  SDSPI;
#endif
#endif