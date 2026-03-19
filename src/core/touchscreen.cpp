// 编译优化
#pragma GCC optimize("O2")

// 调试开关
#define TOUCH_DEBUG 1
#if TOUCH_DEBUG
  #define TLOG(...) Serial.printf(__VA_ARGS__)
#else
  #define TLOG(...)
#endif

#ifndef ABS
#define ABS(x) ((x) > 0 ? (x) : -(x))
#endif

#include "options.h"
#if (TS_MODEL != TS_MODEL_UNDEFINED) && (DSP_MODEL != DSP_DUMMY)
#include "Arduino.h"
#include "touchscreen.h"
#include "config.h"
#include "controls.h"
#include "display.h"
#include "player.h"

// 定义静态成员变量
volatile bool TouchScreen::touchAvailable = false;

#ifndef TS_X_MIN
  #define TS_X_MIN              400
#endif
#ifndef TS_X_MAX
  #define TS_X_MAX              3800
#endif
#ifndef TS_Y_MIN
  #define TS_Y_MIN              260
#endif
#ifndef TS_Y_MAX
  #define TS_Y_MAX              3800
#endif
#ifndef TS_STEPS
  #define TS_STEPS              40
#endif

// 滑动阈值（累积位移，像素）
#ifndef SLIDE_STEP
  #define SLIDE_STEP            30
#endif

// 稳定起始点采样数量
#ifndef STABLE_SAMPLES
  #define STABLE_SAMPLES        3
#endif

// 超时时间（毫秒）
#ifndef LIST_TIMEOUT
  #define LIST_TIMEOUT          5000
#endif
#ifndef VOL_TIMEOUT
  #define VOL_TIMEOUT           3000
#endif

// 长按进入列表时间（毫秒）
#ifndef LONG_PRESS_LIST
  #define LONG_PRESS_LIST       1500
#endif

#if TS_MODEL == TS_MODEL_XPT2046
  #ifdef TS_SPIPINS
    SPIClass TSSPI(HSPI);
  #endif
  #include <XPT2046_Touchscreen.h>
  XPT2046_Touchscreen ts(TS_CS);
  typedef TS_Point TSPoint;

#elif TS_MODEL == TS_MODEL_GT911
  #include "../GT911_Touchscreen/TAMC_GT911.h"
  TAMC_GT911 ts = TAMC_GT911(TS_SDA, TS_SCL, TS_INT, TS_RST, 0, 0);
  typedef TP_Point TSPoint;

#elif TS_MODEL == TS_MODEL_CST328
  #include "CSE_CST328.h"
  // 注意：CST328 构造函数参数是 (width, height)
  // 屏幕实际尺寸是 320x240，但触摸屏的原始方向可能是240x320
  // 需要根据实际硬件方向调整
  CSE_CST328 ts = CSE_CST328(320, 240, &Wire1, TS_RST, TS_INT);
  typedef TS_Point TSPoint;
#endif


// 中断服务函数（必须放在 IRAM 中）
static void IRAM_ATTR touchISR() {
  TouchScreen::touchAvailable = true;
}


void TouchScreen::init(uint16_t w, uint16_t h) {
  
#if TS_MODEL == TS_MODEL_XPT2046
  #ifdef TS_SPIPINS
    TSSPI.begin(TS_SPIPINS);
    ts.begin(TSSPI);
  #else
    #if TS_HSPI
      ts.begin(SPI2);
    #else
      bool ok = ts.begin();
    #endif
  #endif
  Serial.printf("XPT2046 init: %s\n", ok ? "OK" : "FAIL");
  ts.setRotation(config.store.fliptouch ? 3 : 1);
#endif

#if TS_MODEL == TS_MODEL_GT911
  bool ok = ts.begin();
  Serial.printf("GT911 init: %s\n", ok ? "OK" : "FAIL");
  ts.setRotation(config.store.fliptouch ? 0 : 2);
#endif

  _width = w;
  _height = h;

#if TS_MODEL == TS_MODEL_GT911
  ts.setResolution(_width, _height);
#endif

#if TS_MODEL == TS_MODEL_CST328
  Wire1.begin(TS_SDA, TS_SCL);
  bool ok = ts.begin(); {
    Serial.printf("CST328 init: %s\n", ok ? "OK" : "FAIL");
    // 设置初始旋转：90度（纠正90度顺时针偏移）
    // 当 fliptouch 为 true 时，旋转270度（180度翻转 + 90度纠正）
    ts.setRotation(config.store.fliptouch ? 3 : 1);
  
   // === 新增：配置中断 ===
    if (ok) {
      pinMode(TS_INT, INPUT_PULLUP);  // CST328 INT 通常是开漏输出，需要上拉
      attachInterrupt(digitalPinToInterrupt(TS_INT), touchISR, FALLING);  // 下降沿触发
      touchAvailable = false;
      Serial.println("CST328 interrupt enabled");
    }
    // ====================
  }
#endif
}

tsDirection_e TouchScreen::_tsDirection(uint16_t x, uint16_t y) {
  return TDS_REQUEST;
}

void TouchScreen::flip() {
#if TS_MODEL == TS_MODEL_XPT2046
  ts.setRotation(config.store.fliptouch ? 3 : 1);
#endif
#if TS_MODEL == TS_MODEL_GT911
  ts.setRotation(config.store.fliptouch ? 0 : 2);
#endif
#if TS_MODEL == TS_MODEL_CST328
  ts.setRotation(config.store.fliptouch ? 3 : 1);
#endif
}

void TouchScreen::loop()
{
  if (!_enabled) return; // 触摸禁用时直接返回

    // === 中断模式：没有触摸直接返回 ===
  if (!touchAvailable) return;
  touchAvailable = false;  // 清除标志
  // ================================

  static enum { TS_IDLE, TS_PRESSING, TS_DRAGGING, TS_LONGPRESSED } state = TS_IDLE;
  static uint32_t pressStartTime;
  static uint16_t refX, refY;
  static int16_t accDx, accDy;
  static int sampleCount;
  static uint32_t sumX, sumY;
  static bool slid = false;

  //if (!_checklpdelay(20, _touchdelay)) return;

#if TS_MODEL == TS_MODEL_GT911
  ts.read();
#elif TS_MODEL == TS_MODEL_CST328
  ts.readData();
#endif

  bool istouched = _istouched();
   // 如果没有触摸且状态为 IDLE，直接返回
  if (!istouched && state == TS_IDLE) return;

  uint16_t curX = 0, curY = 0;

  if (istouched)
  {
#if TS_MODEL == TS_MODEL_XPT2046
    TSPoint p = ts.getPoint();
    curX = map(p.x, TS_X_MIN, TS_X_MAX, 0, _width);
    curY = map(p.y, TS_Y_MIN, TS_Y_MAX, 0, _height);
#elif TS_MODEL == TS_MODEL_GT911
    TSPoint p = ts.points[0];
    curX = p.x;
    curY = p.y;
#elif TS_MODEL == TS_MODEL_CST328
    TSPoint p = ts.getPoint();
    // CST328 库已经处理了旋转，返回的是屏幕坐标
    TLOG("CST328 point: p.x=%d, p.y=%d, width=%d, height=%d\n", p.x, p.y, _width, _height);
    curX = p.x;
    curY = p.y;
    // 确保坐标在屏幕范围内
    if (curX >= _width) curX = _width - 1;
    if (curY >= _height) curY = _height - 1;
#endif

    // 如果触摸屏翻转，调整坐标
    // 注意：CST328 库已经在 setRotation 中处理了翻转，所以不需要额外处理
#if TS_MODEL == TS_MODEL_CST328
    // CST328 库已经处理了旋转，不需要额外翻转
    TLOG("CST328: curX=%d, curY=%d, fliptouch=%d (already rotated by library)\n", curX, curY, config.store.fliptouch);
#else
    if (config.store.fliptouch) {
      TLOG("Before flip: curX=%d, curY=%d, width=%d, height=%d\n", curX, curY, _width, _height);
      curX = _width - curX;
      curY = _height - curY;
      TLOG("After flip: curX=%d, curY=%d\n", curX, curY);
    } else {
      TLOG("No flip: curX=%d, curY=%d, fliptouch=%d\n", curX, curY, config.store.fliptouch);
    }
#endif
  }

  // 长按锁定状态处理
  if (state == TS_LONGPRESSED) {
    if (!istouched) {
      state = TS_IDLE;
      slid = false;
    }
    return;
  }

  // 每次触摸时更新超时基准时间
  if (istouched) {
    _lastTouchTime = millis();
  }

  displayMode_e currentMode = display.mode();

    // 边界值只计算一次
  static uint16_t cachedUpperBound = 0;
  static uint16_t cachedRightBound = 0;
  static uint16_t cachedHalfWidth = 0;
  static uint16_t lastHeight = 0;
  static uint16_t lastWidth = 0;
  
  
  // 当屏幕尺寸或翻转状态变化时重新计算
  if (lastHeight != _height || lastWidth != _width) {
    cachedUpperBound = _height * 3 / 4;
    cachedRightBound = _width * 2 / 3;
    cachedHalfWidth = _width / 2;
    // 添加串口输出
  //  Serial.print("_height: ");
  //  Serial.print(_height);
 //   Serial.print(", cachedUpperBound: ");
 //   Serial.print(cachedUpperBound);
 //   Serial.print(" (上部区域: Y < ");
 //   Serial.print(cachedUpperBound);
//    Serial.print(", 下部区域: Y >= ");
//    Serial.print(cachedUpperBound);
 //   Serial.println(")");

    lastHeight = _height;
    lastWidth = _width;
  }
  
  // 调试：打印当前触摸坐标和区域判断
 // if (istouched) {
 //   Serial.print("Touch Y=");
 //   Serial.print(curY);
 //   Serial.print(", upperBound=");
 //   Serial.print(cachedUpperBound);
 //   Serial.print(" -> ");
//    Serial.println(curY < cachedUpperBound ? "上部区域" : "下部区域");
 // }
  
  // 使用缓存值
  uint16_t upperBound = cachedUpperBound;
  uint16_t rightBound = cachedRightBound;
  uint16_t halfWidth = cachedHalfWidth;

  // ---------- 超时处理 ----------
  if (currentMode == STATIONS && !istouched) {
    if (millis() - _lastTouchTime > LIST_TIMEOUT) {
      display.putRequest(PLAYLIST_TIMEOUT, 0);
      _lastTouchTime = millis();
    }
  }
  if (currentMode == VOL && !istouched) {
    if (millis() - _lastTouchTime > VOL_TIMEOUT) {
      display.putRequest(NEWMODE, PLAYER);
      _lastTouchTime = millis();
    }
  }

  // ---------- 触摸结束处理（手指抬起）----------
// ---------- 触摸结束处理（手指抬起）----------
if (!istouched) {
  if (state != TS_IDLE) {
    uint32_t pressDuration = millis() - pressStartTime;
    if (!slid) {
      uint16_t touchX, touchY;
      if (sampleCount > 0) {
        touchX = sumX / sampleCount;
        touchY = sumY / sampleCount;
      } else {
        touchX = curX;
        touchY = curY;
      }

      // ✅ 优化点：先统一判断短按条件
      if (pressDuration > 50 && pressDuration < 500) {
        
        // 再根据不同的模式执行不同的操作
        switch (currentMode) {
          
          case PLAYER:
            if (touchY < upperBound) {
              onBtnClick(EVT_BTNCENTER);
              Serial.println("Upper click: play/pause");
            }
            break;
            
          case VOL:
            display.putRequest(NEWMODE, PLAYER);
            Serial.println("VOL short press: return to PLAYER");
            break;
            
          case STATIONS:
            display.putRequest(CLOSEPLAYLIST, display.currentPlItem);
            Serial.printf("Playlist item %d clicked\n", display.currentPlItem);
            break;
            
          default:
            // 其他模式暂时不处理短按
            break;
        }
      }
    }
    state = TS_IDLE;
    slid = false;
  }
  return;
}

  // ---------- 有触摸，根据模式处理 ----------
  switch (currentMode)
  {
  case PLAYER:
    switch (state)
    {
    case TS_IDLE:
      state = TS_PRESSING;
      pressStartTime = millis();
      sampleCount = 0;
      sumX = sumY = 0;
      slid = false;
      sumX += curX;
      sumY += curY;
      sampleCount = 1;
      break;

    case TS_PRESSING:
      if (sampleCount < STABLE_SAMPLES)
      {
        sumX += curX;
        sumY += curY;
        sampleCount++;
      }
      if (sampleCount >= STABLE_SAMPLES)
      {
        uint16_t stableY = sumY / sampleCount;
        refX = sumX / sampleCount;
        refY = stableY;
        accDx = accDy = 0;
        state = TS_DRAGGING;
      }
      break;

    case TS_DRAGGING:
    {
      int16_t dx = curX - refX;
      int16_t dy = curY - refY;
      accDx += dx;
      accDy += dy;
      refX = curX;
      refY = curY;

      // 检测滑动
      if (ABS(accDx) > SLIDE_STEP || ABS(accDy) > SLIDE_STEP)
      {
        slid = true;
      }

      // **功能3：屏幕右侧1/3区域上下滑动调亮度**
      if (curX >= rightBound && abs(accDy) > SLIDE_STEP)
      {
        const int BRIGHTNESS_STEP = 10;
        int newBrightness = config.store.brightness;
        if (accDy > 0) {
          // 向下滑动：降低亮度
          newBrightness -= BRIGHTNESS_STEP;
          if (newBrightness < 0) newBrightness = 0;
          Serial.println("Brightness down (right edge)");
        } else {
          // 向上滑动：增加亮度
          newBrightness += BRIGHTNESS_STEP;
          if (newBrightness > 100) newBrightness = 100;
          Serial.println("Brightness up (right edge)");
        }
        if (newBrightness != config.store.brightness) {
          config.store.brightness = newBrightness;
          config.setBrightness(true);
        }
        accDy = 0;
      }

      // **功能1：上半部长按进入列表（左右分区）**
      if (!slid && (millis() - pressStartTime) > LONG_PRESS_LIST && curY < upperBound)
      {

        if (curX < halfWidth)
        {
          // 左边：网络电台列表
          if (config.getMode() == PM_SDCARD)
            config.changeMode(PM_WEB);
          display.putRequest(NEWMODE, STATIONS);
          Serial.println("Long press left: web radio list");
          
        }
        else
        {
          // 右边：SD卡文件列表
          config.changeMode(PM_SDCARD);
          display.putRequest(NEWMODE, STATIONS);
          Serial.println("Long press right: SD card list");
        }
        state = TS_LONGPRESSED;
        slid = true;
        return;
      }

      // **功能4：上半部左右滑动切换歌曲**
      if (curY < upperBound && abs(accDx) > SLIDE_STEP)
      {
        if (accDx > 0) {
          // 右滑：上一曲/上一电台
          player.prev();
          Serial.println("Upper swipe right: previous");
        } else {
          // 左滑：下一曲/下一电台
          player.next();
          Serial.println("Upper swipe left: next");
        }
        accDx = 0;
      }

      // **功能5：下半部左右滑动调音量**
      if (curY >= upperBound && abs(accDx) > SLIDE_STEP)
      {
        if (accDx > 0) {
          player.stepVol(true); // 右滑：增加音量
          Serial.println("Lower swipe right: volume up");
        } else {
          player.stepVol(false); // 左滑：减少音量
          Serial.println("Lower swipe left: volume down");
        }
        accDx = 0;
      }
      break;
    }
    }
    break;

  case STATIONS:
    switch (state)
    {
    case TS_IDLE:
      state = TS_PRESSING;
      pressStartTime = millis();
      sampleCount = 0;
      sumX = sumY = 0;
      slid = false;
      sumX += curX;
      sumY += curY;
      sampleCount = 1;
      break;

    case TS_PRESSING:
      if (sampleCount < STABLE_SAMPLES)
      {
        sumX += curX;
        sumY += curY;
        sampleCount++;
      }
      if (sampleCount >= STABLE_SAMPLES)
      {
        refX = sumX / sampleCount;
        refY = sumY / sampleCount;
        accDx = accDy = 0;
        state = TS_DRAGGING;
      }
      break;

    case TS_DRAGGING:
    {
      int16_t dx = curX - refX;
      int16_t dy = curY - refY;
      accDx += dx;
      accDy += dy;
      refX = curX;
      refY = curY;

      if (ABS(accDy) > SLIDE_STEP)
      {
        if (accDy > 0)
        {
          controlsEvent(false); // 向下
        }
        else
        {
          controlsEvent(true); // 向上
        }
        accDy = 0;
        slid = true;
      }
      break;      
    }
    }
    break;

  case VOL:
    switch (state)
    {
    case TS_IDLE:
      state = TS_PRESSING;
      pressStartTime = millis();
      sampleCount = 0;
      sumX = sumY = 0;
      slid = false;
      sumX += curX;
      sumY += curY;
      sampleCount = 1;
      break;

    case TS_PRESSING:
      if (sampleCount < STABLE_SAMPLES)
      {
        sumX += curX;
        sumY += curY;
        sampleCount++;
      }
      if (sampleCount >= STABLE_SAMPLES)
      {
        refX = sumX / sampleCount;
        refY = sumY / sampleCount;
        accDx = accDy = 0;
        state = TS_DRAGGING;
      }
      break;

    case TS_DRAGGING:
    {
      int16_t dx = curX - refX;
      int16_t dy = curY - refY;
      accDx += dx;
      accDy += dy;
      refX = curX;
      refY = curY;

      if (ABS(accDx) > SLIDE_STEP)
      {
        if (accDx > 0)
        {
          controlsEvent(true); // 增加音量
        }
        else
        {
          controlsEvent(false); // 减少音量
        }
        accDx = 0;
        slid = true;
      }
      break;
    }
    }
    break;

  default:
    break;
  }
}


bool TouchScreen::_checklpdelay(int m, uint32_t &tstamp) {
  if (millis() - tstamp > m) {
    tstamp = millis();
    return true;
  }
  return false;
}

bool TouchScreen::_istouched() {
#if TS_MODEL == TS_MODEL_XPT2046
  return ts.touched();
#elif TS_MODEL == TS_MODEL_GT911
  return ts.isTouched;
#elif TS_MODEL == TS_MODEL_CST328
  return ts.isTouched();
#endif
}

#endif

TouchScreen touchscreen;  // 全局对象定义