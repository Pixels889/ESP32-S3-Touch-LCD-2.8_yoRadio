#ifndef touchscreen_h
#define touchscreen_h

#include "common.h"

enum tsDirection_e { TSD_STAY, TSD_LEFT, TSD_RIGHT, TSD_UP, TSD_DOWN, TDS_REQUEST };

class TouchScreen {
  public:
    TouchScreen() : _enabled(true), _lastTouchTime(0) {}
    
    void init(uint16_t w, uint16_t h);
    void loop();
    void flip();
    void enable(bool en) { _enabled = en; }
    void resetTimeout() { _lastTouchTime = millis(); }
    
    // 中断标志
    static volatile bool touchAvailable;

  private:
    uint16_t _width, _height;
    uint32_t _lastTouchTime;
    bool _enabled;
    
    bool _checklpdelay(int m, uint32_t &tstamp);
    tsDirection_e _tsDirection(uint16_t x, uint16_t y);
    bool _istouched();
};

extern TouchScreen touchscreen;

#endif