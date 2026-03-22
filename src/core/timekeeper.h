#ifndef timekeeper_h
#define timekeeper_h
#pragma once

void _syncTask(void * pvParameters);
void _getWeather();

class TimeKeeper {
  public:
    volatile bool forceWeather;
    volatile bool forceTimeSync;
    volatile bool busy;
    char *weatherBuf;
    
  public:
    TimeKeeper();
    bool loop0();  // core0 (display)
    bool loop1();  // core1 (player)
    void timeTask();
    void weatherTask();
    void waitAndReturnPlayer(uint8_t time_s);
    void waitAndDo(uint8_t time_s, void (*callback)());

  private:
    uint32_t _returnPlayerTime;
    uint32_t _doAfterTime;
    void (*_aftercallback)();
    
    void _upRSSI();
    void _upSDPos();
    void _upClock();
    void _upScreensaver();
    void _returnPlayer();
    void _doAfterWait();
};

extern TimeKeeper timekeeper;

#endif