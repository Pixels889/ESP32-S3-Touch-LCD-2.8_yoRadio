/*************************************************************************************
    ST7789 320x240 displays CUSTOM configuration file.
    自定义配置 - 优化中文显示和滚动速度
    More info on https://github.com/e2002/yoradio/wiki/Widgets#widgets-description
*************************************************************************************/

#ifndef displayST7789conf_h
#define displayST7789conf_h

#define DSP_WIDTH       320
#define TFT_FRAMEWDT    8
#define MAX_WIDTH       DSP_WIDTH-TFT_FRAMEWDT*2
//#define PLMITEMS        11
//#define PLMITEMLENGHT   40
//#define PLMITEMHEIGHT   22


#if BITRATE_FULL
  #define TITLE_FIX 44
#else
  #define TITLE_FIX 60 //预留媒体信息位置
#endif
#define bootLogoTop     68

/* SROLLS  */                            /* {{ left, top, fontsize, align }, buffsize, uppercase, width, scrolldelay, scrolldelta, scrolltime } */
/*
  参数说明：
  - scrolldelay: 开始滚动前的延迟时间（毫秒）
  - scrolldelta: 每次滚动移动的像素数（原来4-5，改为1-2会更慢更平滑）
  - scrolltime: 滚动间隔时间（毫秒，原来30，改为50-80会更慢）
*/

// 电台名称滚动 - 减慢速度：scrolldelta 5→2, scrolltime 30→60
const ScrollConfig metaConf       PROGMEM = {{ TFT_FRAMEWDT, TFT_FRAMEWDT, 3, WA_LEFT }, 140, true, MAX_WIDTH + TFT_FRAMEWDT, 5000, 4, 300 };

// 歌曲标题1滚动 - 减慢速度：scrolldelta 4→2, scrolltime 30→60
const ScrollConfig title1Conf     PROGMEM = {{ TFT_FRAMEWDT, 52, 2, WA_LEFT }, 140, true, MAX_WIDTH - TITLE_FIX, 5000, 4, 300 };

// 歌曲标题2滚动 - 减慢速度：scrolldelta 4→2, scrolltime 30→60
const ScrollConfig title2Conf     PROGMEM = {{ TFT_FRAMEWDT, 76, 2, WA_LEFT }, 140, true, MAX_WIDTH - TITLE_FIX, 5000, 4, 300 };

// 播放列表滚动 - 减慢速度：scrolldelta 2→1, scrolltime 30→50
const ScrollConfig playlistConf   PROGMEM = {{ TFT_FRAMEWDT, 112, 2, WA_LEFT }, 140, true, MAX_WIDTH, 1000, 2, 300 };

// AP标题滚动
const ScrollConfig apTitleConf    PROGMEM = {{ TFT_FRAMEWDT, TFT_FRAMEWDT, 3, WA_CENTER }, 140, false, MAX_WIDTH, 0, 2, 300 };

// AP设置滚动
const ScrollConfig apSettConf     PROGMEM = {{ TFT_FRAMEWDT, 240-TFT_FRAMEWDT-16, 2, WA_LEFT }, 140, false, MAX_WIDTH, 0, 2, 300 };

// 天气滚动
const ScrollConfig weatherConf    PROGMEM = {{ 8, 87, 2, WA_LEFT }, 140, true, MAX_WIDTH, 0, 2, 300 };

/* BACKGROUNDS  */                       /* {{ left, top, fontsize, align }, width, height, outlined } */
const FillConfig   metaBGConf     PROGMEM = {{ 0, 0, 0, WA_LEFT }, DSP_WIDTH, 42, false };
const FillConfig   metaBGConfInv  PROGMEM = {{ 0, 42, 0, WA_LEFT }, DSP_WIDTH, 1, false };
const FillConfig   volbarConf     PROGMEM = {{ TFT_FRAMEWDT, 240-TFT_FRAMEWDT-6, 0, WA_LEFT }, MAX_WIDTH, 6, true };
const FillConfig  playlBGConf     PROGMEM = {{ 0, 107, 0, WA_LEFT }, DSP_WIDTH, 24, false };
const FillConfig  heapbarConf     PROGMEM = {{ 0, 239, 0, WA_LEFT }, DSP_WIDTH, 1, false };

/* WIDGETS  */                           /* { left, top, fontsize, align } */
const WidgetConfig bootstrConf    PROGMEM = { 0, 182, 1, WA_CENTER };
const WidgetConfig bitrateConf    PROGMEM = { 70, 191, 1, WA_LEFT };
//const WidgetConfig voltxtConf     PROGMEM = { 0, 214, 1, WA_CENTER };
const WidgetConfig voltxtConf     PROGMEM = { 130, 214, 1, WA_LEFT };
const WidgetConfig  iptxtConf     PROGMEM = { TFT_FRAMEWDT, 214, 1, WA_LEFT };
//const WidgetConfig   rssiConf     PROGMEM = { 220,214, 1, WA_LEFT };
const WidgetConfig   rssiConf     PROGMEM = { 290,214, 1, WA_LEFT };
/* RSSI BAR CONFIG */
const RSSIBarConfig rssiBarConf PROGMEM = {
    { 310, 210, 1, WA_RIGHT },  // WidgetConfig
    4,                          // barCount (4档竖条)
    3,                          // barWidth (每个竖条4像素宽)
    2,                          // barSpacing (竖条间距2像素)
    10                          // maxHeight (最高竖条16像素高)
};
const WidgetConfig numConf        PROGMEM = { 0, 120+30, 0, WA_CENTER };
const WidgetConfig apNameConf     PROGMEM = { TFT_FRAMEWDT, 66, 2, WA_CENTER };
const WidgetConfig apName2Conf    PROGMEM = { TFT_FRAMEWDT, 90, 2, WA_CENTER };
const WidgetConfig apPassConf     PROGMEM = { TFT_FRAMEWDT, 130, 2, WA_CENTER };
const WidgetConfig apPass2Conf    PROGMEM = { TFT_FRAMEWDT, 154, 2, WA_CENTER };
const WidgetConfig  clockConf     PROGMEM = { 8, 176, 0, WA_RIGHT };
const WidgetConfig vuConf         PROGMEM = { TFT_FRAMEWDT, 100, 1, WA_LEFT };
//const WidgetConfig batteryVoltConf PROGMEM = { 0, 214, 1, WA_RIGHT };
const WidgetConfig batteryVoltConf PROGMEM = { 200, 214, 1, WA_LEFT };

const WidgetConfig bootWdtConf    PROGMEM = { 0, 162, 1, WA_CENTER };
const ProgressConfig bootPrgConf  PROGMEM = { 90, 14, 4 };
const BitrateConfig fullbitrateConf PROGMEM = {{DSP_WIDTH-TFT_FRAMEWDT-34, 47, 2, WA_LEFT}, 42 };

/* BANDS  */                             /* { onebandwidth, onebandheight, bandsHspace, bandsVspace, numofbands, fadespeed } */
const VUBandsConfig bandsConf     PROGMEM = { 24, 100, 4, 2, 10, 2 };

/* STRINGS  */
const char         numtxtFmt[]    PROGMEM = "%d";
const char           rssiFmt[]    PROGMEM = "WiFi %d";
const char          iptxtFmt[]    PROGMEM = "IP: %s";
const char         voltxtFmt[]    PROGMEM = "Vol:%d";
const char        bitrateFmt[]    PROGMEM = "%d kBs";

/* MOVES  */                             /* { left, top, width } */
const MoveConfig    clockMove     PROGMEM = { 8, 180, -1 };
const MoveConfig   weatherMove    PROGMEM = { 8, 97, MAX_WIDTH };
const MoveConfig   weatherMoveVU  PROGMEM = { 70, 97, MAX_WIDTH-70+TFT_FRAMEWDT };

#endif
