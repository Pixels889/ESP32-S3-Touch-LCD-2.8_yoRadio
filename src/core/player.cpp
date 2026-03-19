// 添加编译优化
#pragma GCC optimize("O3")

// 调试开关
#define PLAYER_DEBUG 0
#if PLAYER_DEBUG
  #define PLOG(...) Serial.printf(__VA_ARGS__)
#else
  #define PLOG(...)
#endif


#include "options.h"
#include "esp_task_wdt.h"
#include "player.h"
#include "config.h"
#include "telnet.h"
#include "display.h"
#include "sdmanager.h"
#include "netserver.h"
#include "timekeeper.h"
#include "../displays/tools/l10n.h"
#include "../pluginsManager/pluginsManager.h"
#ifdef USE_NEXTION
#include "../displays/nextion.h"
#endif




Player player;
QueueHandle_t playerQueue;

#if VS1053_CS!=255 && !I2S_INTERNAL
  #if VS_HSPI
    Player::Player(): Audio(VS1053_CS, VS1053_DCS, VS1053_DREQ, &SPI2),_currentIndex(0), _currentMode(PM_WEB) {}
  #else
    Player::Player(): Audio(VS1053_CS, VS1053_DCS, VS1053_DREQ, &SPI),_currentIndex(0), _currentMode(PM_WEB) {}
  #endif
  void ResetChip(){
    pinMode(VS1053_RST, OUTPUT);
    digitalWrite(VS1053_RST, LOW);
    delay(30);
    digitalWrite(VS1053_RST, HIGH);
    delay(100);
  }
#else
  #if !I2S_INTERNAL
    Player::Player() : _currentIndex(0), _currentMode(PM_WEB) {}
  #else
    Player::Player(): Audio(true, I2S_DAC_CHANNEL_BOTH_EN),_currentIndex(0), _currentMode(PM_WEB)  {}
  #endif
#endif



void Player::init() {
  Serial.print("##[BOOT]#\tplayer.init\t");
  playerQueue=NULL;
  _resumeFilePos = 0;
  _hasError=false;
  playerQueue = xQueueCreate( 5, sizeof( playerRequestParams_t ) );
  setOutputPins(false);
  delay(50);
#ifdef MQTT_ROOT_TOPIC
  memset(burl, 0, MQTT_BURL_SIZE);
#endif
  if(MUTE_PIN!=255) pinMode(MUTE_PIN, OUTPUT);
  #if I2S_DOUT!=255
    #if !I2S_INTERNAL
      setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    #endif
  #else
    SPI.begin();
    if(VS1053_RST>0) ResetChip();
    begin();
  #endif
  setBalance(config.store.balance);
  setTone(config.store.bass, config.store.middle, config.store.trebble);
  setVolume(0);
  _status = STOPPED;
  _volTimer=false;
  //randomSeed(analogRead(0));
  #if PLAYER_FORCE_MONO
    forceMono(true);
  #endif
  _loadVol(config.store.volume);
  setConnectionTimeout(CONNECTION_TIMEOUT, CONNECTION_TIMEOUT_SSL);
  Serial.println("done");
}

void Player::sendCommand(playerRequestParams_t request){
  if(playerQueue==NULL) return;

  //xQueueSend(playerQueue, &request, PLQ_SEND_DELAY);
   // 使用 0 超时，队列满时直接返回，避免阻塞
   xQueueSend(playerQueue, &request, 0);  // 确保是0超时
    // 可选的日志，避免刷屏
    // log_w("playerQueue full, request type %d dropped", request.type);
 }


void Player::resetQueue(){
  if(playerQueue!=NULL) xQueueReset(playerQueue);
}

void Player::stopInfo() {
  config.setSmartStart(0);
  netserver.requestOnChange(MODE, 0);
}

void Player::setError(const char *e){
    _hasError = true;
    strlcpy(config.tmpBuf, e, sizeof(config.tmpBuf));
    config.setTitle(config.tmpBuf);
    telnet.printf("##ERROR#:\t%s\n", config.tmpBuf);
}
// 如果有无参版本被其他地方调用，可保留并重定向
void Player::setError(){
    setError("Connection failed");
}

void Player::_stop(bool alreadyStopped) {
  if(_status == STOPPED) return;
  
  static bool inStop = false;
  if(inStop) return;
  inStop = true;

  PLOG("_stop called\n");
  
  if(config.getMode() == PM_SDCARD && !alreadyStopped) {
    config.sdResumePos = getFilePos();
  }
  
  _status = STOPPED;
  setOutputPins(false);
  
  if(!_hasError) {
    const char* title = (display.mode() == LOST || display.mode() == UPDATING) ? 
                        "" : LANG::const_PlStopped;
    config.setTitle(title);
  }
  
  config.station.bitrate = 0;
  config.setBitrateFormat(BF_UNKNOWN);
  
  #ifdef USE_NEXTION
    nextion.bitrate(config.station.bitrate);
  #endif
  
  setDefaults();
  
  if(!alreadyStopped) stopSong();
  
  netserver.requestOnChange(BITRATE, 0);
  display.putRequest(DBITRATE);
  display.putRequest(PSTOP);
  
  if(!lockOutput) stopInfo();
  
  if (player_on_stop_play) player_on_stop_play();
  pm.on_stop_play();

  inStop = false;
}

void Player::initHeaders(const char *file) {
  if(strlen(file)==0 || true) return; //TODO Read TAGs
  connecttoFS(sdman,file);
  eofHeader = false;
  while(!eofHeader) Audio::loop();
  //netserver.requestOnChange(SDPOS, 0);
  setDefaults();
}
void resetPlayer(){
  if(!config.store.watchdog) return;
  player.resetQueue();
  player.sendCommand({PR_STOP, 0});
  player.loop();
}

#ifndef PL_QUEUE_TICKS
  #define PL_QUEUE_TICKS 0
#endif
#ifndef PL_QUEUE_TICKS_ST
  #define PL_QUEUE_TICKS_ST 15
#endif


void Player::loop() {
  if(playerQueue==NULL) return;
  
  // 使用静态变量缓存状态，减少函数调用
  static bool lastRunningState = false;
  bool isRunningNow = isRunning();
  
  playerRequestParams_t requestP;
  if(xQueueReceive(playerQueue, &requestP, isRunningNow ? PL_QUEUE_TICKS : PL_QUEUE_TICKS_ST)){
    switch (requestP.type){
      case PR_STOP: 
        _stop(); 
        break;
        
      case PR_PLAY: {
        uint16_t num = abs(requestP.payload);
        _currentIndex = num;
        _currentMode = (playMode_e)config.getMode();
        
        if (requestP.payload > 0) {
          config.setLastStation(num);
        }
        
        _play(num);
        
        if (player_on_station_change) player_on_station_change(); 
        pm.on_station_change();
        break;
      }
      
      case PR_TOGGLE: 
        toggle(); 
        break;

      case PR_VOL:
      {
        // requestP.payload 是原始值 (0-254)
        uint8_t original_volume = requestP.payload;

        // 保存原始值用于显示
        config.setVolume(original_volume);

        // 应用音量映射表（声压级到人耳听感曲线）
        uint8_t hardware_volume = _volumeMap[original_volume];

        // 设置硬件音量（映射后的值）
        Audio::setVolume(volToI2S(hardware_volume));

        break;
      }

#ifdef USE_SD
      case PR_CHECKSD: {
        if(config.getMode() == PM_SDCARD && !sdman.cardPresent()){
          sdman.stop();
          config.changeMode(PM_WEB);
        }
        break;
      }
      #endif
      
      case PR_VUTONUS: 
        if(config.vuThreshold > 10) config.vuThreshold -= 10;
        break;
        
      #ifdef MQTT_ROOT_TOPIC
      case PR_BURL: 
        if(strlen(burl) > 0) browseUrl();
        break;
      #endif
          
      default: break;
    }
  }
  
  // Audio 循环
  Audio::loop();
  
  // 状态变化检测
  bool isRunningNowCheck = isRunning();
  if (lastRunningState != isRunningNowCheck) {
    if (!isRunningNowCheck && _status == PLAYING) {
      _stop(true);
    }
    lastRunningState = isRunningNowCheck;
  }
  
  // 音量保存定时器
  if(_volTimer && (millis() - _volTicks) > 3000){
    config.saveVolume();
    _volTimer = false;
  }
}


void Player::setOutputPins(bool isPlaying) {
  if(REAL_LEDBUILTIN!=255) digitalWrite(REAL_LEDBUILTIN, LED_INVERT?!isPlaying:isPlaying);
  bool _ml = MUTE_LOCK?!MUTE_VAL:(isPlaying?!MUTE_VAL:MUTE_VAL);
  if(MUTE_PIN!=255) digitalWrite(MUTE_PIN, _ml);
}


void Player::_play(uint16_t stationId) {
  PLOG("_play: stationId=%d, mode=%d\n", stationId, config.getMode());

  _currentIndex = stationId;
  _currentMode = (playMode_e)config.getMode();
  _hasError = false;
  _status = STOPPED;
  
  setDefaults();
  setOutputPins(false);
  remoteStationName = false;
  
  if(!config.prepareForPlaying(stationId)) return;
  
  _loadVol(config.store.volume);
  
  bool isConnected = false;
  uint32_t connectStart = millis();
  
  // 根据模式选择连接方式
  if(config.getMode() == PM_SDCARD && SDC_CS != 255) {
    uint32_t resumePos = (config.sdResumePos == 0) ? _resumeFilePos : (config.sdResumePos - player.sd_min);
    isConnected = connecttoFS(sdman, config.station.url, resumePos);
  } else {
    // 确保是 WEB 模式
    if(config.getMode() != PM_WEB) {
      config.saveValue(&config.store.play_mode, static_cast<uint8_t>(PM_WEB));
    }
    
    connproc = false;
    esp_task_wdt_reset();
    isConnected = connecttohost(config.station.url);
    esp_task_wdt_reset();
    connproc = true;
  }
  
  uint32_t connectTime = millis() - connectStart;
  PLOG("Connect time: %lu ms, result: %d\n", connectTime, isConnected);
  
  if(isConnected) {
    _status = PLAYING;
    config.configPostPlaying(stationId);
    setOutputPins(true);
    
    if (player_on_start_play) player_on_start_play();
    pm.on_start_play();
    
    display.putRequest(PSTART);
    display.putRequest(NEWSTATION);
    
    if (strlen(config.station.title) > 0) {
      display.putRequest(NEWTITLE);
    }
  } else {
    // ========== 播放失败处理 ==========
    if(config.getMode() == PM_SDCARD) {
      // SD卡文件播放失败
      telnet.printf("##ERROR#:\tSD file playback failed: %s\n", config.station.url);
      
      // 显示错误信息
      char errMsg[64];
      snprintf(errMsg, sizeof(errMsg), "文件不存在或损坏，重启初始化列表");
      config.setTitle(errMsg);
      
      // 延时2秒让用户看到错误信息
      vTaskDelay(2000);
      
      // 提示用户重启
      display.putRequest(SHUTDOWN_PROMPT, 0);
      vTaskDelay(3000);
      
      // 重启系统
      ESP.restart();
    } else {
      // WEB模式播放失败
      telnet.printf("##ERROR#:\tNetwork connection failed\n");
      setError("Connection failed");
      
      if(_status != STOPPED) {
        _stop(true);
      }
    }
    // ================================
  }
}

#ifdef MQTT_ROOT_TOPIC
void Player::browseUrl(){
  _hasError=false;
  remoteStationName = true;
  config.setDspOn(1);
  resumeAfterUrl = _status==PLAYING;
  display.putRequest(PSTOP);
  setOutputPins(false);
  config.setTitle(LANG::const_PlConnect);
  if (connecttohost(burl)){
    _status = PLAYING;
    config.setTitle("");
    netserver.requestOnChange(MODE, 0);
    setOutputPins(true);
    display.putRequest(PSTART);
    if (player_on_start_play) player_on_start_play();
    pm.on_start_play();
  }else{
    telnet.printf("##ERROR#:\tError connecting to %.128s\n", burl);
    snprintf(config.tmpBuf, sizeof(config.tmpBuf), "Error connecting to %.128s", burl); setError();
    _stop(true);
  }
  //memset(burl, 0, MQTT_BURL_SIZE);
}
#endif

void Player::prev() {
  uint16_t lastStation = config.lastStation();
  uint16_t listLength = config.playlistLength();
  
  if (listLength == 0) return;
  
  if (config.getMode() == PM_WEB || !config.store.sdsnuffle) {
    // 顺序播放
    lastStation = (lastStation == 1) ? listLength : lastStation - 1;
  } else {
    // 随机播放
    lastStation = random(1, listLength + 1);
  }
  
  config.lastStation(lastStation);
  sendCommand({PR_PLAY, lastStation});
}

void Player::next() {
  uint16_t lastStation = config.lastStation();
  uint16_t listLength = config.playlistLength();
  
  if (listLength == 0) return;
  
  if (config.getMode() == PM_WEB || !config.store.sdsnuffle) {
    // 顺序播放
    lastStation = (lastStation == listLength) ? 1 : lastStation + 1;
  } else {
    // 随机播放
    lastStation = random(1, listLength + 1);
  }
  
  config.lastStation(lastStation);
  sendCommand({PR_PLAY, lastStation});
}

void Player::toggle() {
  if (_status == PLAYING) {
    sendCommand({PR_STOP, 0});
  } else {
    sendCommand({PR_PLAY, config.lastStation()});
  }
}

void Player::stepVol(bool up) {
  if (up) {
    if (config.store.volume <= 254 - config.store.volsteps) {
      setVol(config.store.volume + config.store.volsteps);
    }else{
      setVol(254);
    }
  } else {
    if (config.store.volume >= config.store.volsteps) {
      setVol(config.store.volume - config.store.volsteps);
    }else{
      setVol(0);
    }
  }
}

// 音量映射表定义（声压级到人耳听感曲线）
// 0-40 → 0-10（每4对应1）
// 41-70 → 10-20（每3对应1）
// 71-100 → 20-30（每3对应1）
// 101-130 → 30-50
// 131-160 → 50-80
// 161-190 → 80-110
// 191-220 → 110-140
// 221-250 → 140-200
// 251-254 → 200-254
const uint8_t Player::_volumeMap[255] = {
    // 0-40 → 0-10（每4对应1） - 41个元素
    0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4,
    5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 9, 10,

    // 41-70 → 10-20（每3对应1） - 30个元素
    10, 10, 11, 11, 11, 12, 12, 12, 13, 13, 13, 14, 14, 14, 15, 15, 15,
    16, 16, 16, 17, 17, 17, 18, 18, 18, 19, 19, 19, 20,

    // 71-100 → 20-30（每3对应1） - 30个元素
    20, 20, 21, 21, 21, 22, 22, 22, 23, 23, 23, 24, 24, 24, 25, 25, 25,
    26, 26, 26, 27, 27, 27, 28, 28, 28, 29, 29, 29, 30,

    // 101-130 → 30-50 - 30个元素
    30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46,
    47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59,

    // 131-160 → 50-80 - 30个元素
    50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66,
    67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 80,

    // 161-190 → 80-110 - 30个元素
    80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96,
    97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 110,

    // 191-220 → 110-140 - 30个元素
    110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123,
    124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137,
    138, 140,

    // 221-250 → 140-200 - 30个元素
    140, 142, 144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 166,
    168, 170, 172, 174, 176, 178, 180, 182, 184, 186, 188, 190, 192, 194,
    196, 200,

    // 251-254 → 200-254 - 4个元素
    200, 210, 230, 254
};

uint8_t Player::volToI2S(uint8_t volume) {
  return constrain(
    map(volume, 0, 254 - config.station.ovol * 3, 0, 254),
    0, 254
  );
}

void Player::_loadVol(uint8_t volume) {
  // 应用音量映射表（声压级到人耳听感曲线）
  uint8_t hardware_volume = _volumeMap[volume];
  // 应用 I2S 音量映射
  setVolume(volToI2S(hardware_volume));
}


void Player::setVol(uint8_t volume) {
  _volTicks = millis();
  _volTimer = true;
  player.sendCommand({PR_VOL, volume});
}