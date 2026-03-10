#ifndef rtcsupport_h
#define rtcsupport_h

#include <Arduino.h>
#include <time.h>

#define RTCSUPPORTED (RTC_SDA!=255 && RTC_SCL!=255 && (RTC_MODULE==DS3231 || RTC_MODULE==DS1307 || RTC_MODULE==PCF85063))

#if RTCSUPPORTED

#if RTC_MODULE==DS3231 || RTC_MODULE==DS1307
#include "RTClib.h"
#endif

#if RTC_MODULE==PCF85063
#include <PCF85063TP.h>
#endif

class RTC {
public:
    bool init();
    bool isRunning();
    void getTime(struct tm* tinfo);
    void setTime(struct tm* tinfo);
    
    // 强制刷新缓存（例如时间同步后调用）
    void forceRefresh() { _lastReadTime = 0; }

private:
    static uint32_t _lastReadTime;
    static struct tm _cachedTime;
};

extern RTC rtc;

#endif // RTCSUPPORTED
#endif