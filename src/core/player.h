#ifndef player_h
#define player_h
#include "config.h"

#if I2S_DOUT!=255 || I2S_INTERNAL
  #include "../audioI2S/AudioEx.h"
#else
  #include "../audioVS1053/audioVS1053Ex.h"
#endif

#ifndef MQTT_BURL_SIZE
  #define MQTT_BURL_SIZE  512
#endif

#ifndef PLQ_SEND_DELAY
  #define PLQ_SEND_DELAY pdMS_TO_TICKS(1000) //portMAX_DELAY
#endif

enum playerRequestType_e : uint8_t { 
    PR_PLAY = 1, 
    PR_STOP = 2, 
    PR_PREV = 3, 
    PR_NEXT = 4, 
    PR_VOL = 5, 
    PR_CHECKSD = 6, 
    PR_VUTONUS = 7, 
    PR_BURL = 8, 
    PR_TOGGLE = 9 
};

struct playerRequestParams_t
{
  playerRequestType_e type;
  int payload;
};

enum plStatus_e : uint8_t { PLAYING = 1, STOPPED = 2 };

class Player: public Audio {
  private:
    uint32_t    _volTicks;      // 延迟音量保存计时
    bool        _volTimer;      // 延迟音量保存标志
    uint32_t    _resumeFilePos; // 文件恢复位置
    plStatus_e  _status;        // 当前状态
    uint16_t    _currentIndex;  // 当前播放索引
    playMode_e  _currentMode;   // 当前播放模式
    bool        _hasError;      // 错误标志
    uint8_t     _consecutiveFails = 0;  // 会话级连续失败计数（不随连接成功清零，只稳定播放30秒后清零）
    uint32_t    _playStartTime = 0;     // 当前播放开始时间（用于判断稳定播放）
    
    // 音量映射表（声压级到人耳听感曲线）
    static const uint8_t _volumeMap[255];

  private:
    void _stop(bool alreadyStopped = false);
    void _play(uint16_t stationId);
    void _loadVol(uint8_t volume);
    
  public:
    // 公共变量
    bool lockOutput = true;
    bool resumeAfterUrl = false;
    volatile bool connproc = true;
    uint32_t sd_min, sd_max;
    
    #ifdef MQTT_ROOT_TOPIC
    char burl[MQTT_BURL_SIZE];  // 浏览URL缓冲区
    #endif
    
  public:
    // 构造函数
    Player();
    
    // 初始化
    void init();
    
    // 主循环
    void loop();
    
    // 初始化文件头
    void initHeaders(const char *file);
    
    // 错误处理
    void setError();
    void setError(const char *e);
    
    // 队列命令
    void sendCommand(playerRequestParams_t request);
    void resetQueue();
    
    #ifdef MQTT_ROOT_TOPIC
    void browseUrl();
    #endif
    
    // 状态相关
    plStatus_e status() const { return _status; }
    bool isPlaying() const { return _status == PLAYING; }
    uint16_t getCurrentIndex() const { return _currentIndex; }
    playMode_e getCurrentMode() const { return _currentMode; }
    
    // 远程电台名标志
    bool remoteStationName = false;
    
    // 控制函数
    void prev();
    void next();
    void toggle();
    void stepVol(bool up);
    void setVol(uint8_t volume);
    uint8_t volToI2S(uint8_t volume);
    void stopInfo();
    void setOutputPins(bool isPlaying);
    void setResumeFilePos(uint32_t pos) { _resumeFilePos = pos; }
    uint8_t getConsecutiveFails() const { return _consecutiveFails; }
};

// 全局对象声明
extern Player player;

// 弱回调函数声明
extern __attribute__((weak)) void player_on_start_play();
extern __attribute__((weak)) void player_on_stop_play();
extern __attribute__((weak)) void player_on_track_change();
extern __attribute__((weak)) void player_on_station_change();

#endif