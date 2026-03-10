// BAT_Driver.h
#pragma once
#include <Arduino.h> 

#define BAT_ADC_PIN         8
#define BAT_REF_VOLTAGE     3.3        // ESP32 ADC参考电压
#define BAT_DIVIDER_RATIO   3.0        // 理论分压比 (R1+R2)/R2
#define BAT_CALIB_OFFSET    0.990476   // 实际校准系数（保留原值）
#define BAT_FILTER_ALPHA     0.1        // 低通滤波系数
#define BAT_LOW_VOLTAGE      3.5        // 低电压阈值
#define BAT_CRITICAL_VOLTAGE 3.3        // 严重低电压阈值

extern float BAT_analogVolts;
extern bool BAT_isLow;
extern bool BAT_isCritical;

void BAT_Init(void);
float BAT_Get_Volts(void);
void BAT_Update(void);
bool BAT_IsLow(void);
bool BAT_IsCritical(void);