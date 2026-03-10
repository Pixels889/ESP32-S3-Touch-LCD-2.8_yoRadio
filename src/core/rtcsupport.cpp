#include "options.h"
#include "rtcsupport.h"

#if RTCSUPPORTED

#include <Wire.h>

// 定义静态成员
uint32_t RTC::_lastReadTime = 0;
struct tm RTC::_cachedTime;

#if RTC_MODULE==DS3231 || RTC_MODULE==DS1307

static RTC_DS3231 rtc_ds3231;
static RTC_DS1307 rtc_ds1307;
TwoWire RTCWire = TwoWire(0);

bool RTC::init() {
    RTCWire.begin(RTC_SDA, RTC_SCL);
    RTCWire.setClock(100000); // 降低 I2C 速度提高稳定性
#if RTC_MODULE==DS3231
    return rtc_ds3231.begin(&RTCWire);
#else
    return rtc_ds1307.begin(&RTCWire);
#endif
}

bool RTC::isRunning() {
#if RTC_MODULE==DS3231
    return !rtc_ds3231.lostPower();
#else
    return rtc_ds1307.isrunning();
#endif
}

void RTC::getTime(struct tm* tinfo) {
    // 如果上次读取时间在1秒内，使用缓存的值
    if (millis() - _lastReadTime < 1000) {
        memcpy(tinfo, &_cachedTime, sizeof(struct tm));
        return;
    }

    DateTime nowTm;
#if RTC_MODULE==DS3231
    nowTm = rtc_ds3231.now();
#else
    nowTm = rtc_ds1307.now();
#endif
    
    _cachedTime.tm_sec  = nowTm.second();
    _cachedTime.tm_min  = nowTm.minute();
    _cachedTime.tm_hour = nowTm.hour();
    _cachedTime.tm_wday = nowTm.dayOfTheWeek();
    _cachedTime.tm_mday = nowTm.day();
    _cachedTime.tm_mon  = nowTm.month() - 1;
    _cachedTime.tm_year = nowTm.year() - 1900;
    
    memcpy(tinfo, &_cachedTime, sizeof(struct tm));
    _lastReadTime = millis();
}

void RTC::setTime(struct tm* tinfo) {
    DateTime newTime(tinfo->tm_year + 1900, tinfo->tm_mon + 1, tinfo->tm_mday,
                     tinfo->tm_hour, tinfo->tm_min, tinfo->tm_sec);
#if RTC_MODULE==DS3231
    rtc_ds3231.adjust(newTime);
#else
    rtc_ds1307.adjust(newTime);
#endif
    // 设置后立即更新缓存
    getTime(&_cachedTime);
    _lastReadTime = millis();
}

#elif RTC_MODULE==PCF85063

#include <PCF85063TP.h>
PCD85063TP rtc_pcf;
static bool _rtc_ok = false;

bool RTC::init() {
    Wire.begin(RTC_SDA, RTC_SCL);
    Wire.setClock(100000); // 降低 I2C 速度提高稳定性
    rtc_pcf.begin();

    // 尝试读取时间以验证通信
    for (int i = 0; i < 3; i++) {
        rtc_pcf.getTime();
        if (rtc_pcf.year >= 0 && rtc_pcf.year <= 99) {
            _rtc_ok = true;
            break;
        }
        delay(10);
    }
    return _rtc_ok;
}

bool RTC::isRunning() {
    if (!_rtc_ok) return false;
    
    // 如果上次读取时间在1秒内，直接返回 true
    if (millis() - _lastReadTime < 1000) return true;
    
    rtc_pcf.getTime();
    _lastReadTime = millis();
    return (rtc_pcf.year >= 0 && rtc_pcf.year <= 99);
}

void RTC::getTime(struct tm* tinfo) {
    if (!_rtc_ok) return;

    // 如果上次读取时间在1秒内，使用缓存的值
    if (millis() - _lastReadTime < 1000) {
        memcpy(tinfo, &_cachedTime, sizeof(struct tm));
        return;
    }

    rtc_pcf.getTime();
    _lastReadTime = millis();
    
    _cachedTime.tm_sec  = rtc_pcf.second;
    _cachedTime.tm_min  = rtc_pcf.minute;
    _cachedTime.tm_hour = rtc_pcf.hour;
    _cachedTime.tm_wday = rtc_pcf.dayOfWeek;
    _cachedTime.tm_mday = rtc_pcf.dayOfMonth;
    _cachedTime.tm_mon  = rtc_pcf.month - 1;
    _cachedTime.tm_year = rtc_pcf.year + 100;
    
    memcpy(tinfo, &_cachedTime, sizeof(struct tm));
}

void RTC::setTime(struct tm* tinfo) {
    if (!_rtc_ok) return;
    
    rtc_pcf.fillByYMD(tinfo->tm_year + 1900, tinfo->tm_mon + 1, tinfo->tm_mday);
    rtc_pcf.fillByHMS(tinfo->tm_hour, tinfo->tm_min, tinfo->tm_sec);
    rtc_pcf.fillDayOfWeek(tinfo->tm_wday);
    rtc_pcf.setTime();
    rtc_pcf.startClock();
    
    // 设置后立即更新缓存
    getTime(&_cachedTime);
    _lastReadTime = millis();
}

#endif // RTC_MODULE

RTC rtc;

#endif // RTCSUPPORTED