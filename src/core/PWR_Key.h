#pragma once
#include "Arduino.h"
#include "../displays/displayST7789.h"

#define PWR_KEY_Input_PIN   6
#define PWR_Control_PIN     7

#define Measurement_offset 0.990476     
#define EXAMPLE_BAT_TICK_PERIOD_MS 50

// 时间阈值（单位：毫秒）
#define SHORT_PRESS_TIME    500   // 短按（<500ms）
#define SHUTDOWN_TIME      2000   // 长按关机（≥2秒）
#define DEBOUNCE_TIME       50    // 按键防抖时间

// 函数声明
void Shutdown(void);
void PWR_Init(void);
void PWR_Loop(void);