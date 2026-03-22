// 添加编译优化
#pragma GCC optimize("O2")

#include "Arduino.h"
#include "options.h"
#include "WiFi.h"
#include "time.h"
#include "config.h"
#include "display.h"
#include "player.h"
#include "network.h"
#include "netserver.h"
#include "timekeeper.h"
#include "../pluginsManager/pluginsManager.h"
#include "../displays/dspcore.h"
#include "../displays/widgets/widgets.h"
#include "../displays/widgets/pages.h"
#include "../displays/tools/l10n.h"
#include "common.h"
#include "BAT_Driver.h"
#include "touchscreen.h"

#include <U8g2_for_Adafruit_GFX.h>
#include <u8g2_fonts.h>
#include "../fonts/u8g2_font_alibabapuhuitiregular22t.h"
#include "esp_task_wdt.h"

U8G2_FOR_ADAFRUIT_GFX u8g2Font; 

Display display;
#ifdef USE_NEXTION
#include "../displays/nextion.h"
Nextion nextion;
#endif

#ifndef CORE_STACK_SIZE
  #define CORE_STACK_SIZE  1024*4
#endif
#ifndef DSP_TASK_PRIORITY
  #define DSP_TASK_PRIORITY  2
#endif
#ifndef DSP_TASK_CORE_ID
  #define DSP_TASK_CORE_ID  0
#endif
#ifndef DSP_TASK_DELAY
  #define DSP_TASK_DELAY pdMS_TO_TICKS(30)
#endif

#define DSP_QUEUE_TICKS 0
// RSSI功能已启用（显示四档竖条信号强度）

QueueHandle_t displayQueue;

// Display 构造函数
Display::Display() 
    : _meta(nullptr), _title1(nullptr), _plcurrent(nullptr), _weather(nullptr),
      _title2(nullptr), _plwidget(nullptr), _fullbitrate(nullptr), 
      _metabackground(nullptr), _plbackground(nullptr), _volbar(nullptr), 
      _heapbar(nullptr), _pager(nullptr), _footer(nullptr), _playlistFooter(nullptr), 
      _vuwidget(nullptr), _nums(nullptr), _clock(nullptr), _boot(nullptr), 
      _bootstring(nullptr), _volip(nullptr), _voltxt(nullptr), _rssi(nullptr), 
      _bitrate(nullptr), _batteryVolt(nullptr), _lastBatteryRead(0), 
      _bootStep(0), _locked(false) {}

static void loopDspTask(void * pvParameters) {
    esp_task_wdt_add(NULL);
    while(true) {
        esp_task_wdt_reset();
#ifndef DUMMYDISPLAY
        if(displayQueue == NULL) break;
        if(timekeeper.loop0()) {
            esp_task_wdt_reset();
            display.loop();
            esp_task_wdt_reset();
#ifndef NETSERVER_LOOP1
            netserver.loop();
            esp_task_wdt_reset();
#endif
        }
#else
        timekeeper.loop0();
#ifndef NETSERVER_LOOP1
        netserver.loop();
        esp_task_wdt_reset();
#endif
#endif
        vTaskDelay(DSP_TASK_DELAY);
    }
    esp_task_wdt_delete(NULL);
    vTaskDelete(NULL);
}

void Display::_createDspTask() {
    xTaskCreatePinnedToCore(loopDspTask, "DspTask", CORE_STACK_SIZE, NULL, 
                            DSP_TASK_PRIORITY, NULL, DSP_TASK_CORE_ID);
}


void Display::_clearBootTextArea() {
    if (!_bootstring) return;
    
    // 获取U8g2字体的实际高度
    u8g2Font.setFont(u8g2_font_wqy16_t_gb2312);  // 使用启动屏使用的字体
    int16_t ascent = u8g2Font.getFontAscent();
    int16_t descent = u8g2Font.getFontDescent();
    int16_t fontHeight = ascent - descent;  // 字体实际高度
    
    // 计算文本的实际绘制区域
    int16_t textY = bootstrConf.top + (bootstrConf.textsize * CHARHEIGHT) - 2;  // U8g2的基线Y坐标
    int16_t textTop = textY - ascent;  // 文本顶部Y坐标
    int16_t textBottom = textY + descent;  // 文本底部Y坐标
    int16_t textHeight = textBottom - textTop;  // 实际高度
    
     // 根据实际测试，可能需要调整清除范围
    // 如果还有轻微残影，可以增加下面的数值
    int16_t extraTop = 5;    // 向上额外清除像素
    int16_t extraBottom = 5; // 向下额外清除像素
    
    int16_t clearTop = textTop - extraTop;
    int16_t clearHeight = textHeight + extraTop + extraBottom;
    
    // 确保不超出屏幕范围
    if (clearTop < 0) {
        clearHeight += clearTop;
        clearTop = 0;
    }
    if (clearTop + clearHeight > dsp.height()) {
        clearHeight = dsp.height() - clearTop;
    }
    
    // 清除整个宽度的区域
    dsp.fillRect(0, clearTop, dsp.width(), clearHeight, ST77XX_BLACK);
}


#ifndef DUMMYDISPLAY
//============================================================================================================================
DspCore dsp;

Page *pages[] = { new Page(), new Page(), new Page(), new Page() };

#if !((DSP_MODEL==DSP_ST7735 && DTYPE==INITR_BLACKTAB) || DSP_MODEL==DSP_ST7789 || DSP_MODEL==DSP_ST7796 || DSP_MODEL==DSP_ILI9488 \
 || DSP_MODEL==DSP_ILI9486 || DSP_MODEL==DSP_ILI9341 || DSP_MODEL==DSP_ILI9225 || DSP_MODEL==DSP_ST7789_170)
  #undef  BITRATE_FULL
  #define BITRATE_FULL     false
#endif




void returnPlayer(){
  display.putRequest(NEWMODE, PLAYER);
}





void Display::init() {
  Serial.print("##[BOOT]#\tdisplay.init\t");
#ifdef USE_NEXTION
  nextion.begin();
#endif
#if LIGHT_SENSOR!=255
  analogSetAttenuation(ADC_0db);
#endif
  _bootStep = 0;
  dsp.initDisplay();

  u8g2Font.begin(dsp);
  u8g2Font.setFont(u8g2_font_wqy16_t_gb2312);  // 默认字体

  u8g2Font.setForegroundColor(ST77XX_WHITE);
  u8g2Font.setBackgroundColor(ST77XX_BLACK);
  u8g2Font.setFontMode(1);  // 透明模式

  displayQueue=NULL;
  displayQueue = xQueueCreate( 5, sizeof( requestParams_t ) );
  while(displayQueue==NULL){;}
  _createDspTask();
  while(!_bootStep==0) { delay(10); }
  _pager = new Pager();
  _footer = new Page();
  _playlistFooter = new Page();  // 初始化播放列表专用底部栏
  _plwidget = new PlayListWidget();
  _nums = new NumWidget();
  _clock = new ClockWidget();
  _meta = new ScrollWidget();       // 后续在_buildPager中init
  _title1 = new ScrollWidget();
  _plcurrent = new ScrollWidget();
  Serial.println("done");
}

uint16_t Display::width() const { 
    return dsp.width(); 
}

uint16_t Display::height() const { 
    return dsp.height(); 
}

#if TIME_SIZE>19
  #if DSP_MODEL==DSP_SSD1322
    #define BOOT_PRG_COLOR    WHITE
    #define BOOT_TXT_COLOR    WHITE
    #define PINK              WHITE
  #elif DSP_MODEL==DSP_SSD1327
    #define BOOT_PRG_COLOR    0x07
    #define BOOT_TXT_COLOR    0x3f
    #define PINK              0x02
  #else
    #define BOOT_PRG_COLOR    0xE68B
    #define BOOT_TXT_COLOR    0xFFFF
    #define PINK              0xF97F
  #endif
#endif

void Display::_bootScreen(){
  _boot = new Page();
  // ProgressWidget 需要字体参数
  _boot->addWidget(new ProgressWidget(bootWdtConf, bootPrgConf, u8g2_font_wqy16_t_gb2312, BOOT_PRG_COLOR, 0));
  // TextWidget 需要字体参数
  _bootstring = (TextWidget*) &_boot->addWidget(new TextWidget(bootstrConf, 50, true, u8g2_font_wqy16_t_gb2312, BOOT_TXT_COLOR, 0));
  _pager->addPage(_boot);
  _pager->setPage(_boot, true);
  dsp.drawLogo(bootLogoTop);
  _bootStep = 1;
}


void Display::_buildPager(){
  // 为不同小部件指定不同字体
  _meta->init("*", metaConf, u8g2_font_alibabapuhuitiregular22t, config.theme.meta, config.theme.metabg);
  _title1->init("*", title1Conf, u8g2_font_wqy16_t_gb2312, config.theme.title1, config.theme.background);
  _clock->init(clockConf, 0, 0);
  #if DSP_MODEL==DSP_NOKIA5110
    _plcurrent->init("*", playlistConf, u8g2_font_wqy12_t_gb2312, 0, 1);
  #else
    _plcurrent->init("*", playlistConf, u8g2_font_wqy12_t_gb2312, config.theme.plcurrent, config.theme.plcurrentbg);
  #endif
  _plwidget->init(_plcurrent);
  #if !defined(DSP_LCD)
    _plcurrent->moveTo({TFT_FRAMEWDT, (uint16_t)(_plwidget->currentTop()), (int16_t)playlistConf.width});
  #endif
  #ifndef HIDE_TITLE2
    _title2 = new ScrollWidget("*", title2Conf, u8g2_font_wqy14_t_gb2312, config.theme.title2, config.theme.background);
  #endif
  #if !defined(DSP_LCD) && DSP_MODEL!=DSP_NOKIA5110
    _plbackground = new FillWidget(playlBGConf, config.theme.plcurrentfill);
    #if DSP_INVERT_TITLE || defined(DSP_OLED)
      _metabackground = new FillWidget(metaBGConf, config.theme.metafill);
    #else
      _metabackground = new FillWidget(metaBGConfInv, config.theme.metafill);
    #endif
  #endif
  #if DSP_MODEL==DSP_NOKIA5110
    _plbackground = new FillWidget(playlBGConf, 1);
  #endif
  #ifndef HIDE_VU
    _vuwidget = new VuWidget(vuConf, bandsConf, config.theme.vumax, config.theme.vumin, config.theme.background);
    // 初始时应该锁定（不显示）
    if(_vuwidget) _vuwidget->lock();
  #endif
  #ifndef HIDE_VOLBAR
    _volbar = new SliderWidget(volbarConf, config.theme.volbarin, config.theme.background, 254, config.theme.volbarout);
  #endif
  #ifndef HIDE_HEAPBAR
    _heapbar = new SliderWidget(heapbarConf, config.theme.buffer, config.theme.background, psramInit()?300000:1600 * config.store.abuff);
  #endif
  #ifndef HIDE_VOL
    _voltxt = new TextWidget(voltxtConf, 20, false, u8g2_font_wqy12_t_gb2312, config.theme.vol, config.theme.background);
  #endif
  #ifndef HIDE_IP
    _volip = new TextWidget(iptxtConf, 30, false, u8g2_font_wqy12_t_gb2312, config.theme.ip, config.theme.background);
  #endif
  #ifndef HIDE_RSSI
    _rssi = new RSSIBarWidget(rssiBarConf, config.theme.rssi, config.theme.background, config.theme.background);
  #endif
  _nums->init(numConf, 10, false, config.theme.digit, config.theme.background);
  #ifndef HIDE_WEATHER
    _weather = new ScrollWidget("\007", weatherConf, u8g2_font_wqy14_t_gb2312, config.theme.weather, config.theme.background);
  #endif
  
  // ===== 主底部栏（用于播放器页面）= 包含所有widget =====
  if(_volbar)   _footer->addWidget( _volbar);
  if(_voltxt)   _footer->addWidget( _voltxt);
  if(_volip)    _footer->addWidget( _volip);
  if(_rssi)     _footer->addWidget( _rssi);
  if(_heapbar)  _footer->addWidget( _heapbar);
  
  #ifdef BAT_ADC_PIN
  {
    _batteryVolt = new TextWidget(batteryVoltConf, 20, false, u8g2_font_wqy12_t_gb2312,
                                   config.theme.vol, config.theme.background);
    BAT_Init();
    _updateBatteryVoltage(false);
    _footer->addWidget(_batteryVolt);
  }
  #endif

  // ===== 播放列表底部栏（只放时钟和必要信息）= 避免RSSI干扰 =====
  // 可以只放时钟，或者根据需要添加其他widget
  //_playlistFooter->addWidget(_clock);
  // 如果您想在播放列表页也显示电池，可以取消下面的注释
  // #ifdef BAT_ADC_PIN
  // if(_batteryVolt) _playlistFooter->addWidget(_batteryVolt);
  // #endif

  // ===== 添加页面内容 =====
  if(_metabackground) pages[PG_PLAYER]->addWidget( _metabackground);
  pages[PG_PLAYER]->addWidget(_meta);
  pages[PG_PLAYER]->addWidget(_title1);
  if(_title2) pages[PG_PLAYER]->addWidget(_title2);
  if(_weather) pages[PG_PLAYER]->addWidget(_weather);
  #if BITRATE_FULL
    _fullbitrate = new BitrateWidget(fullbitrateConf, config.theme.bitrate, config.theme.background);
    pages[PG_PLAYER]->addWidget( _fullbitrate);
  #else
    _bitrate = new TextWidget(bitrateConf, 30, false, u8g2_font_wqy14_t_gb2312, config.theme.bitrate, config.theme.background);
    pages[PG_PLAYER]->addWidget( _bitrate);
  #endif
  if(_vuwidget) pages[PG_PLAYER]->addWidget( _vuwidget);
  pages[PG_PLAYER]->addWidget(_clock);
  pages[PG_SCREENSAVER]->addWidget(_clock);
  
  // 为不同页面添加不同的底部栏
  pages[PG_PLAYER]->addPage(_footer);           // 播放器页面用完整底部栏
  //pages[PG_PLAYLIST]->addPage(_playlistFooter); // 播放列表页面用简化底部栏

  if(_metabackground) pages[PG_DIALOG]->addWidget( _metabackground);
  pages[PG_DIALOG]->addWidget(_meta);
  pages[PG_DIALOG]->addWidget(_nums);
  
  #if !defined(DSP_LCD) && DSP_MODEL!=DSP_NOKIA5110
    //pages[PG_DIALOG]->addPage(_footer);
    //pages[PG_DIALOG]->addPage(_playlistFooter);  // 改用简化底部栏
  #endif
  #if !defined(DSP_LCD)
  if(_plbackground) {
    pages[PG_PLAYLIST]->addWidget( _plbackground);
    _plbackground->setHeight(_plwidget->itemHeight());
    _plbackground->moveTo({0,(uint16_t)(_plwidget->currentTop()-playlistConf.widget.textsize*2), (int16_t)playlBGConf.width});
  }
  #endif
  
  pages[PG_PLAYLIST]->addWidget(_plcurrent);
  pages[PG_PLAYLIST]->addWidget(_plwidget);
  
  // 最后将所有页面添加到 pager
  for(const auto& p: pages) _pager->addPage(p);
}




void Display::_apScreen() {
  if(_boot) _pager->removePage(_boot);
  #ifndef DSP_LCD
    _boot = new Page();
    #if DSP_MODEL!=DSP_NOKIA5110
      #if DSP_INVERT_TITLE || defined(DSP_OLED)
      _boot->addWidget(new FillWidget(metaBGConf, config.theme.metafill));
      #else
      _boot->addWidget(new FillWidget(metaBGConfInv, config.theme.metafill));
      #endif
    #endif
    // ScrollWidget 需要字体参数
    ScrollWidget *bootTitle = (ScrollWidget*) &_boot->addWidget(
        new ScrollWidget("*", apTitleConf, u8g2_font_wqy16_t_gb2312, config.theme.meta, config.theme.metabg));
    // TextWidget 需要字体参数
    TextWidget *apname = (TextWidget*) &_boot->addWidget(
        new TextWidget(apNameConf, 30, false, u8g2_font_wqy16_t_gb2312, config.theme.title1, config.theme.background));
    apname->setText(LANG::apNameTxt);
    TextWidget *apname2 = (TextWidget*) &_boot->addWidget(
        new TextWidget(apName2Conf, 30, false, u8g2_font_wqy16_t_gb2312, config.theme.clock, config.theme.background));
    apname2->setText(apSsid);
    TextWidget *appass = (TextWidget*) &_boot->addWidget(
        new TextWidget(apPassConf, 30, false, u8g2_font_wqy16_t_gb2312, config.theme.title1, config.theme.background));
    appass->setText(LANG::apPassTxt);
    TextWidget *appass2 = (TextWidget*) &_boot->addWidget(
        new TextWidget(apPass2Conf, 30, false, u8g2_font_wqy16_t_gb2312, config.theme.clock, config.theme.background));
    appass2->setText(apPassword);
    ScrollWidget *bootSett = (ScrollWidget*) &_boot->addWidget(
        new ScrollWidget("*", apSettConf, u8g2_font_wqy16_t_gb2312, config.theme.title2, config.theme.background));
    bootSett->setText(config.ipToStr(WiFi.softAPIP()), LANG::apSettFmt);
    _pager->addPage(_boot);
    _pager->setPage(_boot);
  #else
    dsp.apScreen();
  #endif
}

void Display::_start() {
  if(_boot) _pager->removePage(_boot);
  #ifdef USE_NEXTION
    nextion.wake();
  #endif
  if (network.status != CONNECTED && network.status != SDREADY) {
    _apScreen();
    #ifdef USE_NEXTION
      nextion.apScreen();
    #endif
    _bootStep = 2;
    return;
  }
  #ifdef USE_NEXTION
    nextion.start();
  #endif
  _buildPager();
  _mode = PLAYER;
  config.setTitle(LANG::const_PlReady);
  
  if(_heapbar)  _heapbar->lock(!config.store.audioinfo);
  
  if(_weather)  _weather->lock(!config.store.showweather);
  if(_weather && config.store.showweather)  _weather->setText(LANG::const_getWeather);

  if(_vuwidget) _vuwidget->lock();
  if(_rssi)     _setRSSI(WiFi.RSSI());
  #ifndef HIDE_IP
    if(_volip) _volip->setText(config.ipToStr(WiFi.localIP()), iptxtFmt);
  #endif
  _pager->setPage( pages[PG_PLAYER]);
  _volume();
  _station();
  _time(false);
  _bootStep = 2;
  // 在所有 widget 创建完成后，请求应用 VU 表状态
  display.putRequest(SHOWVUMETER);

  pm.on_display_player();
}

void Display::_showDialog(const char *title){
  dsp.setScrollId(NULL);
  _pager->setPage( pages[PG_DIALOG]);
  #ifdef META_MOVE
    _meta->moveTo(metaMove);
  #endif
  _meta->setAlign(WA_CENTER);
  _meta->setText(title);
}

void Display::_switchMode(displayMode_e newmode) {
    displayMode_e oldmode = _mode;
    
#ifdef USE_NEXTION
    nextion.putRequest({NEWMODE, newmode});
#endif
    if (newmode == _mode || (network.status != CONNECTED && network.status != SDREADY)) return;

    // 唤醒处理
    if (newmode == PLAYER && oldmode == SCREENBLANK) {
        dsp.wake();
#if BRIGHTNESS_PIN != 255
        analogWrite(BRIGHTNESS_PIN, config.getBrightnessPWM());
#endif
        touchscreen.enable(true);
    }
    
    _mode = newmode;
    dsp.setScrollId(NULL);

    // 根据模式处理
    switch (newmode) {
        case PLAYER:
            if (player.isRunning())
                (clockMove.width < 0) ? _clock->moveBack() : _clock->moveTo(clockMove);
            else
                _clock->moveBack();
#ifdef DSP_LCD
            dsp.clearDsp();
#endif
            numOfNextStation = 0;
#ifdef META_MOVE
            _meta->moveBack();
#endif
            _meta->setAlign(metaConf.widget.align);
            _meta->setText(config.station.name);
            _nums->setText("");
            config.isScreensaver = false;
            _pager->setPage(pages[PG_PLAYER]);
            config.setDspOn(config.store.dspon, false);
            pm.on_display_player();
            break;
            
        case SCREENSAVER:
        case SCREENBLANK:
            _pager->setPage(pages[PG_SCREENSAVER]);
            if (newmode == SCREENBLANK) {
                config.isScreensaver = true;
                _clock->clear();
                config.setDspOn(false, false);
                touchscreen.enable(false);
#if BRIGHTNESS_PIN != 255
                analogWrite(BRIGHTNESS_PIN, 0);
#endif
            }
            break;
            
        case VOL:
            touchscreen.resetTimeout();
#ifndef HIDE_IP
            _showDialog(LANG::const_DlgVolume);
#else
            _showDialog(config.ipToStr(WiFi.localIP()));
#endif
            _nums->setText(config.store.volume, numtxtFmt);
            break;
            
        case STATIONS:
            // SD卡模式处理
            if (config.getMode() == PM_SDCARD) {
                config.initSDPlaylist();
            }
            
            if (config.playlistLength() == 0) {
                dsp.fillRect(0, 0, dsp.width(), dsp.height(), config.theme.background);
                dsp.setCursor(10, 20);
                dsp.setTextColor(config.theme.meta, config.theme.background);
                dsp.print("No files on SD card, returning...");
                delay(3000);
                _switchMode(PLAYER);
                return;
            }
            
            _pager->setPage(pages[PG_PLAYLIST]);
            _plcurrent->setText("");
            currentPlItem = config.lastStation();
            _drawPlaylist();
            touchscreen.resetTimeout();
            break;
            
        default:
            // 其他对话框模式
            if (newmode == LOST) _showDialog(LANG::const_DlgLost);
            else if (newmode == UPDATING) _showDialog(LANG::const_DlgUpdate);
            else if (newmode == SLEEPING) _showDialog("SLEEPING");
            else if (newmode == SDCHANGE) _showDialog(LANG::const_waitForSD);
            else if (newmode == INFO || newmode == SETTINGS || newmode == TIMEZONE || newmode == WIFI) 
                _showDialog(LANG::const_DlgNextion);
            else if (newmode == NUMBERS) _showDialog("");
            break;
    }
}


void Display::resetQueue(){
  if(displayQueue!=NULL) xQueueReset(displayQueue);
}

void Display::_drawPlaylist() {
  _plwidget->drawPlaylist(currentPlItem);
  timekeeper.waitAndReturnPlayer(3);
}

void Display::_drawNextStationNum(uint16_t num) {
  timekeeper.waitAndReturnPlayer(30);
  _meta->setText(config.stationByNum(num));
  _nums->setText(num, "%d");
}

void Display::putRequest(displayRequestType_e type, int payload){
  if(displayQueue==NULL) return;
    // 队列满时直接丢弃，绝不阻塞
  if(uxQueueSpacesAvailable(displayQueue) == 0) return;
  requestParams_t request;
  request.type = type;
  request.payload = payload;
  xQueueSend(displayQueue, &request, 0);

#ifdef USE_NEXTION
    nextion.putRequest(request);
#endif
}

void Display::_layoutChange(bool played){
  // 只处理布局移动，不改变 VU 表的锁定状态
  if(_vuwidget){
    // 注意：这里不修改 _vuwidget 的锁定状态
    // 锁定状态完全由 SHOWVUMETER 控制
  }
  
  // 布局移动逻辑（保持不变）
  if(config.store.vumeter && _vuwidget){
    if(played){
      if(clockMove.width<0) _clock->moveBack(); else _clock->moveTo(clockMove);
      if(_weather) _weather->moveTo(weatherMoveVU);
    }else{
      _clock->moveBack();
      if(_weather) _weather->moveBack();
    }
  }else{
    if(played){
      if(clockMove.width<0) _clock->moveBack(); else _clock->moveTo(clockMove);
      if(_weather) _weather->moveTo(weatherMove);
    }else{
      if(_weather) _weather->moveBack();
      _clock->moveBack();
    }
  }
}

void Display::loop() {
  if(_bootStep==0) {
    _pager->begin();
    _bootScreen();
    return;
  }
  if(displayQueue==NULL || _locked) return;
  _pager->loop();
#ifdef USE_NEXTION
  nextion.loop();
#endif
// 电池电压更新（每10秒）
if (_batteryVolt && millis() - _lastBatteryRead > 10000) {
    _updateBatteryVoltage(false);
}
  requestParams_t request;
  if(xQueueReceive(displayQueue, &request, DSP_QUEUE_TICKS)){
    bool pm_result = true;
    pm.on_display_queue(request, pm_result);
    if(pm_result) {
      switch (request.type) {

        case NEWMODE:
          _switchMode((displayMode_e)request.payload);
          break;

        case CLOSEPLAYLIST: {
          uint16_t selected = request.payload;
          uint16_t current = player.getCurrentIndex();
          playMode_e currentMode = player.getCurrentMode();

          if (selected == current && config.getMode() == currentMode) {
            _switchMode(PLAYER);
          } else {
            player.sendCommand({PR_PLAY, selected});
          }
          break;
        }

        case CLOCK:
          if(_mode==PLAYER || _mode==SCREENSAVER) _time(request.payload==1);
          break;

        case NEWTITLE:
          _title();
          break;

        case NEWSTATION:
          _station();
          break;

        case NEXTSTATION:
          _drawNextStationNum(request.payload);
          break;

        case DRAWPLAYLIST:
          _drawPlaylist();
          break;

        case DRAWVOL:
          _volume();
          break;

        case DBITRATE: {
          char buf[20];
          snprintf(buf, 20, bitrateFmt, config.station.bitrate);
          if(_bitrate) { _bitrate->setText(config.station.bitrate==0?"":buf); }
          if(_fullbitrate) {
            _fullbitrate->setBitrate(config.station.bitrate);
            _fullbitrate->setFormat(config.configFmt);
          }
          break;
        }

        case AUDIOINFO:
          if(_heapbar) {
            _heapbar->lock(!config.store.audioinfo);
            _heapbar->setValue(player.inBufferFilled());
          }
          break;

        case SHOWVUMETER:
        {
          if (_vuwidget)
          {
            // 直接根据开关状态设置锁定
            if (config.store.vumeter)
            {
              _vuwidget->unlock(); // 开关开：解锁
            }
            else
            {
              _vuwidget->lock(); // 开关关：锁定
            }
            // 重新布局（传入当前播放状态）
            _layoutChange(player.isRunning());
          }
          break;
        }

        case SHOWWEATHER: {
          if(_weather) _weather->lock(!config.store.showweather);
          if(!config.store.showweather){
            #ifndef HIDE_IP
            if(_volip) _volip->setText(config.ipToStr(WiFi.localIP()), iptxtFmt);
            #endif
          } else {
            if(_weather) _weather->setText(LANG::const_getWeather);
          }
          break;
        }

        case NEWWEATHER: {
          if(_weather && timekeeper.weatherBuf) _weather->setText(timekeeper.weatherBuf);
          break;
        }

        case BOOTSTRING: {
          if(_bootstring){
          _clearBootTextArea();  // 精确清除启动屏文本区域
          _bootstring->setText(config.ssids[request.payload].ssid, LANG::bootstrFmt);
          }
          break;
        }

        case WAITFORSD: {          
          if(_bootstring) {
            _clearBootTextArea();  // 精确清除启动屏文本区域
            _bootstring->setText(LANG::const_waitForSD);
          }
          break;
        }

        case SDFILEINDEX: {
          if(_mode == SDCHANGE) _nums->setText(request.payload, "%d");
          break;
        }

        case DSPRSSI:
          if(_rssi) _setRSSI(request.payload);
          if (_heapbar && config.store.audioinfo) _heapbar->setValue(player.isRunning()?player.inBufferFilled():0);
          break;

        case PSTART:
          _layoutChange(true);
          break;

        case PSTOP:
          _layoutChange(false);
          break;

        case DSP_START:
          _start();
          break;

        case NEWIP: {
          #ifndef HIDE_IP
          if(_volip) _volip->setText(config.ipToStr(WiFi.localIP()), iptxtFmt);
          #endif
          break;
        }

        case PLAYLIST_TIMEOUT: {
          if (_mode == STATIONS) {
            uint16_t selected = currentPlItem;
            uint16_t current = player.getCurrentIndex();
            playMode_e currentMode = player.getCurrentMode();
            if (selected == current && config.getMode() == currentMode) {
              _switchMode(PLAYER);
              // 手动刷新
              display.putRequest(NEWSTATION);
              display.putRequest(DBITRATE);
              if (strlen(config.station.title) > 0) {
                display.putRequest(NEWTITLE);
              }
            } else {
              player.sendCommand({PR_PLAY, selected});
            }
           }
           break;
          }
           // ===== 在这里添加 SHUTDOWN_PROMPT 处理 =====
         case SHUTDOWN_PROMPT:{
           // 清空屏幕
           dsp.fillScreen(config.theme.background);

           // 设置字体
           u8g2Font.setFont(u8g2_font_wqy16_t_gb2312b);
           u8g2Font.setForegroundColor(ST77XX_WHITE);
           u8g2Font.setBackgroundColor(config.theme.background);
           u8g2Font.setFontMode(0);  // 非透明模式，绘制背景

           // 显示提示信息
           const char *msg = "请松开按键关机！";
           int16_t x = (dsp.width() - u8g2Font.getUTF8Width(msg)) / 2;
           int16_t y = dsp.height() / 2;
           u8g2Font.drawUTF8(x, y, msg);

           // 暂停其他显示更新
           _locked = true;
           break;
          }
           // ========================================
          default:
          break;
         }
    }

    if (uxQueueMessagesWaiting(displayQueue))
      return;
  }

  dsp.loop();

}

/*
void Display::_setRSSI(int rssi) {
  if(!_rssi) return;
#if RSSI_DIGIT
  _rssi->setText(rssi, rssiFmt);
  return;
#endif
  char rssiG[3];
  int rssi_steps[] = {RSSI_STEPS};
  if(rssi >= rssi_steps[0]) strlcpy(rssiG, "\004\006", 3);
  if(rssi >= rssi_steps[1] && rssi < rssi_steps[0]) strlcpy(rssiG, "\004\005", 3);
  if(rssi >= rssi_steps[2] && rssi < rssi_steps[1]) strlcpy(rssiG, "\004\002", 3);
  if(rssi >= rssi_steps[3] && rssi < rssi_steps[2]) strlcpy(rssiG, "\003\002", 3);
  if(rssi <  rssi_steps[3] || rssi >=  0) strlcpy(rssiG, "\001\002", 3);
  _rssi->setText(rssiG);
}
*/
void Display::_setRSSI(int rssi) {
    if(!_rssi) return;
    
    // 直接调用RSSIBarWidget的setRSSI方法
    _rssi->setRSSI(rssi);
    
    _rssi->setActive(true);
    _rssi->unlock();
}

void Display::_station() {
  _meta->setAlign(metaConf.widget.align);
  _meta->setText(config.station.name);
}


char *split(char *str, const char *delim) {
  char *dmp = strstr(str, delim);
  if (dmp == NULL) return NULL;
  *dmp = '\0'; 
  return dmp + strlen(delim);
}

void Display::_title() {
  if (strlen(config.station.title) > 0) {
    char tmpbuf[strlen(config.station.title)+1];
    strlcpy(tmpbuf, config.station.title, strlen(config.station.title)+1);
    char *stitle = split(tmpbuf, " - ");
    if(stitle && _title2){
      _title1->setText(tmpbuf);
      _title2->setText(stitle);
    }else{
      _title1->setText(config.station.title);
      if(_title2) _title2->setText("");
    }
  }else{
    _title1->setText("");
    if(_title2) _title2->setText("");
  }
  if (player_on_track_change) player_on_track_change();
  pm.on_track_change();
}

void Display::_time(bool redraw) {
  
#if LIGHT_SENSOR!=255
  if(config.store.dspon) {
    config.store.brightness = AUTOBACKLIGHT(analogRead(LIGHT_SENSOR));
    config.setBrightness();
  }
#endif
  if(config.isScreensaver && network.timeinfo.tm_sec % 60 == 0){
    #if TIME_SIZE<19
      uint16_t ft=static_cast<uint16_t>(random(TFT_FRAMEWDT, (dsp.height()-TIME_SIZE*CHARHEIGHT-TFT_FRAMEWDT)));
    #else
      uint16_t ft=static_cast<uint16_t>(random(TFT_FRAMEWDT+TIME_SIZE, (dsp.height()-_clock->dateSize()-TFT_FRAMEWDT*2)));
    #endif
    uint16_t lt=static_cast<uint16_t>(random(TFT_FRAMEWDT, (dsp.width()-_clock->clockWidth()-TFT_FRAMEWDT)));
    if(clockConf.align==WA_CENTER) lt-=(dsp.width()-_clock->clockWidth())/2;
    _clock->moveTo({lt, ft, 0});
  }
  _clock->draw(redraw);
}

void Display::_volume() {
  if(_volbar) _volbar->setValue(config.store.volume);
  #ifndef HIDE_VOL
     if(_voltxt) {
    char volBuf[12];
    snprintf(volBuf, sizeof(volBuf), "音量: %d", config.store.volume);
    _voltxt->setText(volBuf);
    }
  #endif
  if(_mode==VOL) {
    timekeeper.waitAndReturnPlayer(3);
    _nums->setText(config.store.volume, numtxtFmt);
  }
}

void Display::flip(){ dsp.flip(); }

void Display::invert(){ dsp.invert(); }

void  Display::setContrast(){
  #if DSP_MODEL==DSP_NOKIA5110
    dsp.setContrast(config.store.contrast);
  #endif
}

bool Display::deepsleep(){
#if defined(LCD_I2C) || defined(DSP_OLED) || BRIGHTNESS_PIN!=255
  dsp.sleep();
  return true;
#endif
  return false;
}


void Display::wakeup() {
  #if defined(LCD_I2C) || defined(DSP_OLED) || BRIGHTNESS_PIN!=255
    dsp.wake();
  #endif
}
//void Display::wakeup(){
//#if defined(LCD_I2C) || defined(DSP_OLED) || BRIGHTNESS_PIN!=255
//  dsp.wake();
//#endif
//}

// 新增：更新电池电压显示
void Display::_updateBatteryVoltage(bool force) {
    if (!_batteryVolt) return;
    
    float v = BAT_Get_Volts();
    char buf[13];  // 小缓冲区
    
    snprintf(buf, sizeof(buf), "电池: %.1fV", v);  // 只显示数字，不加符号
    
    _batteryVolt->setText(buf);
    _lastBatteryRead = millis();
}
#endif

//============================================================================================================================
#ifdef DUMMYDISPLAY // !DUMMYDISPLAY
//============================================================================================================================
void Display::init(){
  _createDspTask();
  #ifdef USE_NEXTION
  nextion.begin(true);
  #endif
}
void Display::_start(){
  #ifdef USE_NEXTION
  nextion.start();
  #endif
  config.setTitle(LANG::const_PlReady);
}

void Display::putRequest(displayRequestType_e type, int payload){
  if(type==DSP_START) _start();
  #ifdef USE_NEXTION
    requestParams_t request;
    request.type = type;
    request.payload = payload;
    nextion.putRequest(request);
  #else
    if(type==NEWMODE) mode((displayMode_e)payload);
  #endif
}


void Display::centerText(const char* text, uint8_t y, uint16_t fg, uint16_t bg) {}
void Display::rightText(const char* text, uint8_t y, uint16_t fg, uint16_t bg) {}
void Display::printPLitem(uint8_t pos, const char* item) {}
void Display::loop() {
    // 实际循环逻辑
}
//============================================================================================================================
#endif // DUMMYDISPLAY

