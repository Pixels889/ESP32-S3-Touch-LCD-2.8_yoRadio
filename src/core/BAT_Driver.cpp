// BAT_Driver.cpp
#include "BAT_Driver.h"

float BAT_analogVolts = 0;
bool BAT_isLow = false;
bool BAT_isCritical = false;

static float _filteredVoltage = 0;

void BAT_Init(void) {
    analogReadResolution(12);
    _filteredVoltage = BAT_Get_Volts();
}

float BAT_Get_Volts(void) {
    int raw = analogReadMilliVolts(BAT_ADC_PIN); // 单位 mV
    // 计算公式： (raw / 1000) * 分压比 * 校准系数
    return (float)(raw * BAT_DIVIDER_RATIO * BAT_CALIB_OFFSET / 1000.0);
}

void BAT_Update(void) {
    float raw = BAT_Get_Volts();
    // 一阶低通滤波
    _filteredVoltage = _filteredVoltage * (1 - BAT_FILTER_ALPHA) + raw * BAT_FILTER_ALPHA;
    
    BAT_analogVolts = _filteredVoltage;
    BAT_isLow = (_filteredVoltage < BAT_LOW_VOLTAGE);
    BAT_isCritical = (_filteredVoltage < BAT_CRITICAL_VOLTAGE);
}

bool BAT_IsLow(void) {
    return BAT_isLow;
}

bool BAT_IsCritical(void) {
    return BAT_isCritical;
}