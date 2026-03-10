#ifndef DISPLAY_H
#define DISPLAY_H

#include "common.h"

// 前置声明依赖类型
#ifndef DISPLAY_MODE_E_DEFINED
enum displayMode_e;
#define DISPLAY_MODE_E_DEFINED
#endif

#ifndef DISPLAY_REQUEST_TYPE_E_DEFINED
enum displayRequestType_e;
#define DISPLAY_REQUEST_TYPE_E_DEFINED
#endif

#if DSP_MODEL == DSP_DUMMY
#define DUMMYDISPLAY
#endif

// 前置声明所有Widget类
class ScrollWidget;
class PlayListWidget;
class BitrateWidget;
class FillWidget;
class SliderWidget;
class Pager;
class Page;
class VuWidget;
class NumWidget;
class ClockWidget;
class TextWidget;

class Display {
public:
    uint16_t currentPlItem = 0;
    uint16_t numOfNextStation = 0;
    displayMode_e _mode;

public:
    Display();
    ~Display() = default;

    displayMode_e mode() const { return _mode; }
    void mode(displayMode_e m) { _mode = m; }
    
    // 公共接口
    void init();
    void loop();
    void _start();
    bool ready() const { return _bootStep == 2; }
    void resetQueue();
    void putRequest(displayRequestType_e type, int payload = 0);
    void flip();
    void invert();
    bool deepsleep();
    void wakeup();
    void setContrast();
    void lock() { _locked = true; }
    void unlock() { _locked = false; }
    uint16_t width() const;
    uint16_t height() const;

    // 统一接口
    void centerText(const char* text, uint8_t y, uint16_t fg, uint16_t bg);
    void rightText(const char* text, uint8_t y, uint16_t fg, uint16_t bg);
    void printPLitem(uint8_t pos, const char* item);

private:
    // Widget指针成员（按使用频率排序）
    ScrollWidget *_meta, *_title1, *_plcurrent, *_weather, *_title2;
    PlayListWidget *_plwidget;
    Pager *_pager;
    Page *_footer, *_playlistFooter, *_boot;
    ClockWidget *_clock;
    VuWidget *_vuwidget;
    NumWidget *_nums;
    TextWidget *_bootstring, *_volip, *_voltxt, *_rssi, *_bitrate, *_batteryVolt;
    BitrateWidget *_fullbitrate;
    FillWidget *_metabackground, *_plbackground;
    SliderWidget *_volbar, *_heapbar;
    
    uint32_t _lastBatteryRead;
    uint8_t _bootStep;
    bool _locked;

    // 私有方法
    void _time(bool redraw = false);
    void _apScreen();
    void _switchMode(displayMode_e newmode);
    void _drawPlaylist();
    void _volume();
    void _title();
    void _station();
    void _drawNextStationNum(uint16_t num);
    void _createDspTask();
    void _showDialog(const char *title);
    void _buildPager();
    void _bootScreen();
    void _layoutChange(bool played);
    void _setRSSI(int rssi);
    void _updateBatteryVoltage(bool force = false);
    void _clearBootTextArea();
};

extern Display display;

#endif // DISPLAY_H