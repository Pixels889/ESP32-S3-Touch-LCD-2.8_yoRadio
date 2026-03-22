#ifndef widgetsconfig_h
#define widgetsconfig_h

#include <stdint.h>

enum WidgetAlign { WA_LEFT, WA_CENTER, WA_RIGHT };
enum BitrateFormat { BF_UNKNOWN, BF_MP3, BF_AAC, BF_FLAC, BF_OGG, BF_WAV };

struct WidgetConfig {
  uint16_t left; 
  uint16_t top; 
  uint16_t textsize;
  WidgetAlign align;
};

struct ScrollConfig {
  WidgetConfig widget;
  uint16_t buffsize;
  bool uppercase;
  uint16_t width;
  uint16_t startscrolldelay;
  uint8_t scrolldelta;
  uint16_t scrolltime;
};

struct FillConfig {
  WidgetConfig widget;
  uint16_t width;
  uint16_t height;
  bool outlined;
};

struct ProgressConfig {
  uint16_t speed;
  uint16_t width;
  uint16_t barwidth;
};

struct VUBandsConfig {
  uint16_t width;
  uint16_t height;
  uint8_t  space;
  uint8_t  vspace;
  uint8_t  perheight;
  uint8_t  fadespeed;
};

struct MoveConfig {
  uint16_t x;
  uint16_t y;
  int16_t width;
};

struct BitrateConfig {
  WidgetConfig widget;
  uint16_t dimension;
};

// RSSI竖条配置结构
struct RSSIBarConfig {
  WidgetConfig widget;   // 基础位置配置
  uint8_t barCount;      // 竖条数量（默认4）
  uint8_t barWidth;      // 每个竖条宽度
  uint8_t barSpacing;    // 竖条间距
  uint8_t maxHeight;     // 最高竖条的高度
};

#endif
