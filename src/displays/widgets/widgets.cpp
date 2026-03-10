// 添加编译优化
#pragma GCC optimize("O2")

#include "../../core/options.h"
#include "../../core/BAT_Driver.h"
#include "esp_task_wdt.h"


#include <U8g2_for_Adafruit_GFX.h>

extern U8G2_FOR_ADAFRUIT_GFX u8g2Font;   // 声明外部对象，不要重复定义

#if DSP_MODEL!=DSP_DUMMY
#include "../dspcore.h"
#include "Arduino.h"
#include "widgets.h"
#include "../../core/player.h"    //  for VU widget
#include "../../core/network.h"   //  for Clock widget
#include "../../core/config.h"
#include "../tools/l10n.h"
#include "../tools/psframebuffer.h"




// 删除最后一个UTF-8字符（从字符串末尾向前找到完整的字符起始）
static void remove_last_utf8_char(char* str) {
  int len = strlen(str);
  if (len == 0) return;
  int i = len - 1;
  // 跳过后续字节（10xxxxxx）
  while (i >= 0 && (str[i] & 0xC0) == 0x80) i--;
  if (i >= 0) str[i] = '\0';
}


/************************
      FILL WIDGET
 ************************/
void FillWidget::init(FillConfig conf, uint16_t bgcolor){
  Widget::init(conf.widget, bgcolor, bgcolor);
  _width = conf.width;
  _height = conf.height;
}

void FillWidget::_draw(){
  if(!_active) return;
  dsp.fillRect(_config.left, _config.top, _width, _height, _bgcolor);
}

void FillWidget::setHeight(uint16_t newHeight){
  _height = newHeight;
}

/************************
      TEXT WIDGET
 ************************/
TextWidget::~TextWidget() {
  free(_text);
  free(_oldtext);
}

void TextWidget::_charSize(uint8_t textsize, uint8_t& width, uint16_t& height){
#ifndef DSP_LCD
  width = textsize * CHARWIDTH;
  height = textsize * CHARHEIGHT;
#else
  width = 1;
  height = 1;
#endif
}

// 修改：增加字体参数
void TextWidget::init(WidgetConfig wconf, uint16_t buffsize, bool uppercase, const uint8_t* font, uint16_t fgcolor, uint16_t bgcolor) {
  Widget::init(wconf, fgcolor, bgcolor);
  _buffsize = buffsize;
  _text = (char *) malloc(sizeof(char) * _buffsize);
  memset(_text, 0, _buffsize);
  _oldtext = (char *) malloc(sizeof(char) * _buffsize);
  memset(_oldtext, 0, _buffsize);
  _charSize(_config.textsize, _charWidth, _textheight);
  _textwidth = _oldtextwidth = _oldleft = 0;
  _uppercase = uppercase;
  _font = font;  // 存储字体指针
}

void TextWidget::setText(const char* txt) {
  strlcpy(_text, txt, _buffsize);
  _textwidth = strlen(_text) * _charWidth;
  if (strcmp(_oldtext, _text) == 0) return;
 
  if (_active) {
    // 提前计算好需要清除的区域，减少重复计算
    uint16_t left = (_oldleft == 0) ? _realLeft() : min(_oldleft, _realLeft());
    uint16_t width = max(_oldtextwidth, _textwidth) + 10;
    uint16_t height = _textheight + 6;
    int16_t clearTop = _config.top > 5 ? _config.top - 5 : 0;
    
    dsp.fillRect(left, clearTop, width, height, _bgcolor);
  }

  _oldtextwidth = _textwidth;
  _oldleft = _realLeft();
  if (_active) _draw();
}

void TextWidget::setText(int val, const char *format){
  char buf[_buffsize];
  snprintf(buf, _buffsize, format, val);
  setText(buf);
}

void TextWidget::setText(const char* txt, const char *format){
  char buf[_buffsize];
  snprintf(buf, _buffsize, format, txt);
  setText(buf);
}

uint16_t TextWidget::_realLeft(bool w_fb) {
  uint16_t realwidth = (_width>0 && w_fb)?_width:dsp.width();
  switch (_config.align) {
    case WA_CENTER: return (realwidth - _textwidth) / 2; break;
    case WA_RIGHT: return (realwidth - _textwidth - (!w_fb?_config.left:0)); break;
    default: return !w_fb?_config.left:0; break;
  }
}

void TextWidget::_draw() {
  if(!_active) return;
  // 使用U8g2绘制中文，先设置专属字体
  u8g2Font.setFont(_font);
  dsp.setClipping({_config.left, _config.top, _width>0?_width:dsp.width(), _textheight});
  u8g2Font.setForegroundColor(_fgcolor);
  u8g2Font.setBackgroundColor(_bgcolor);
  u8g2Font.setFontMode(1);                // 透明模式
  // U8g2使用基线坐标，需要调整为从顶部绘制
  u8g2Font.drawUTF8(_realLeft(), _config.top + _textheight - 2, _text);
  dsp.clearClipping();
  strlcpy(_oldtext, _text, _buffsize);
}

/************************
      SCROLL WIDGET
 ************************/


// 修改构造函数，增加字体参数
ScrollWidget::ScrollWidget(const char* separator, ScrollConfig conf, const uint8_t* font, uint16_t fgcolor, uint16_t bgcolor) {
  init(separator, conf, font, fgcolor, bgcolor);
}

/**
 * 析构函数 - 释放ScrollWidget分配的内存资源
 * 
 * 释放为滚动文本分配的帧缓冲区(_fb)、分隔符(_sep)和滚动窗口(_window)内存，
 * 防止内存泄漏
 */
ScrollWidget::~ScrollWidget() {
  free(_fb);
  free(_sep);
  free(_window);
}

// 修改init，增加字体参数，并传递给父类
void ScrollWidget::init(const char* separator, ScrollConfig conf, const uint8_t* font, uint16_t fgcolor, uint16_t bgcolor) {

 // Serial.printf("ScrollWidget init: conf.width = %d\n", conf.width);  // 新增调试

  TextWidget::init(conf.widget, conf.buffsize, conf.uppercase, font, fgcolor, bgcolor);
  _sep = (char *) malloc(sizeof(char) * 4);
  memset(_sep, 0, 4);
  snprintf(_sep, 4, " %.*s ", 1, separator);
  _x = conf.widget.left;
  _startscrolldelay = conf.startscrolldelay;
  _scrolldelta = conf.scrolldelta;
  _scrolltime = conf.scrolltime;
  _charSize(_config.textsize, _charWidth, _textheight);
  _sepwidth = strlen(_sep) * _charWidth;
  _width = conf.width;
  _backMove.width = _width;
 
  // 使用U8g2获取实际字体宽度来计算窗口大小
  u8g2Font.setFont(_font);
  _u8g2CharWidth = u8g2Font.getUTF8Width("好");  // 用中文字符测试获取实际宽度
  if (_u8g2CharWidth == 0) _u8g2CharWidth = _charWidth;  // 回退到默认值

   // === 新增：计算区域能显示的字符数（向下取整，至少为1）===
  _displayChars = _width / _u8g2CharWidth;
  if (_displayChars == 0) _displayChars = 1;
  _charStart = 0;  // 初始化起始索引
  // =====================================================
  
  _window = (char*) malloc(SCROLL_BUFFER_SIZE);
  if (_window) memset(_window, 0, SCROLL_BUFFER_SIZE);
  _doscroll = true;  // 启用滚动（可根据需要配置）
  _pixelOffset = 0;   // <--- 新增  
  #ifdef PSFBUFFER
  _fb = new psFrameBuffer(dsp.width(), dsp.height());
  uint16_t _rl = (_config.align==WA_CENTER)?(dsp.width()-_width)/2:_config.left;
  _fb->begin(&dsp, _rl, _config.top, _width, _textheight, _bgcolor);
  #endif

 // 在 init() 末尾添加：
  _lastCharStart = 0xFFFF; // 确保第一次绘制时不相等
  _lastDisplayBuf[0] = '\0';
  _needRedraw = true;
  
}

void ScrollWidget::_setTextParams() {
  if (_config.textsize == 0) return;
  if(_fb->ready()){
  #ifdef PSFBUFFER
    _fb->setTextSize(_config.textsize);
    _fb->setTextColor(_fgcolor, _bgcolor);
  #endif
  }else{
    // 不再设置dsp的文本参数，U8g2自行处理
  }
}


bool ScrollWidget::_checkIsScrollNeeded() {
  // 使用U8g2实际宽度计算文本是否需要滚动
  if (_text && strlen(_text) > 0) {
    u8g2Font.setFont(_font);
    _textwidth = u8g2Font.getUTF8Width(_text);
//    Serial.printf("[滚动] 部件=%p 文本=\"%s\" 文本宽=%d 区域宽=%d 是否需要滚动=%s\n",
//                  this, _text, _textwidth, _width,
//                  (_textwidth > _width) ? "是" : "否");
  }
  return _textwidth > _width; // 根据实际宽度判断是否需要滚动
}

void ScrollWidget::setText(const char* txt) {

 // Serial.print("[ScrollWidget] setText called. New full text: \"");
  //Serial.print(txt);
 // Serial.print("\", Old full text: \"");
 // Serial.print(_oldtext);
 // Serial.println("\"");

  strlcpy(_text, txt, _buffsize - 1);

   _needRedraw = true;   // 新增：文本更新，需要重绘

  if (strcmp(_oldtext, _text) == 0) return;
  // 使用U8g2获取实际宽度
  if (_text && strlen(_text) > 0) {
    u8g2Font.setFont(_font);
    _textwidth = u8g2Font.getUTF8Width(_text);
  } else {
    _textwidth = 0;
  }
  _x = _fb->ready()?0:_config.left;
  _doscroll = _checkIsScrollNeeded();
  _pixelOffset = 0;   // <--- 新增：新文本从头开始滚动
   _charStart = 0;     // 新增：新文本从头开始-----------------
  _scrolldelay = millis();
  if (_active) {
    _setTextParams();
    // 文本内容已更新，将在下次_draw或loop中绘制
    strlcpy(_oldtext, _text, _buffsize);
    // 立即绘制（非滚动模式或初始显示）
    if (_active) _draw();
  }
}

void ScrollWidget::setText(const char* txt, const char *format){
  char buf[_buffsize];
  snprintf(buf, _buffsize, format, txt);
  setText(buf);
}

void ScrollWidget::loop() {
  if(_locked) return;
  if (!_doscroll || _config.textsize == 0) return;
  void* scrollId = dsp.getScrollId();
  if (scrollId != NULL && scrollId != this) return;

  // 使用 _scrolltime 作为滚动间隔
  if (_checkDelay(_scrolltime, _scrolldelay)) {

    uint16_t oldStart = _charStart;        // <--- 先保存旧值

    // 字符级滚动：每次移动一个字符
    _charStart++;
    uint16_t len = strlen(_text);
    if (len == 0) return;
    // 如果超过文本长度，则回到开头（循环）
    if (_charStart >= len) {
      _charStart = 0;
    }
    //Serial.printf("loop: _charStart %d -> %d (len=%d)\n", oldStart, _charStart, len);
    
    if (_active) _draw();
  }
}


void ScrollWidget::_clear(){
  if(_fb->ready()){
    #ifdef PSFBUFFER
    _fb->fillRect(0, 0, _width, _textheight, _bgcolor);
    _fb->display();
    #endif
  } else {
    dsp.fillRect(_config.left, _config.top, _width, _textheight + 2, _bgcolor);
  }
}


// 获取UTF-8字符的字节长度（1-4字节）
static inline uint8_t utf8_char_len(uint8_t c) {
  if (c < 0x80) return 1;
  else if ((c & 0xE0) == 0xC0) return 2;
  else if ((c & 0xF0) == 0xE0) return 3;
  else if ((c & 0xF8) == 0xF0) return 4;
  return 1; // 安全回退
}

void ScrollWidget::_draw() {
  if(!_active || _locked) return;
  _setTextParams();

  if (_doscroll) {
    uint16_t totalLen = strlen(_text);
    if (totalLen == 0) return;

    char displayBuf[SCROLL_BUFFER_SIZE];
    displayBuf[0] = '\0';
    int16_t currentWidth = 0;

    // 定位到第 _charStart 个字符
    const char* src = _text;
    for (uint16_t i = 0; i < _charStart && *src; i++) {
      src += utf8_char_len(*src);
    }
    if (*src == '\0') src = _text;

    // 快速构建显示字符串
    const char* p = src;
    while (*p && currentWidth < _width - 2) {
      uint8_t len = utf8_char_len(*p);
      if (len <= 4) {
        char temp[5] = {0};
        memcpy(temp, p, len);  // 用 memcpy 代替 strncpy
        int16_t w = u8g2Font.getUTF8Width(temp);
        if (currentWidth + w <= _width - 2) {
          strcat(displayBuf, temp);
          currentWidth += w;
          p += len;
        } else break;
      } else break;
    }

    // 确保不超宽
    int16_t finalWidth = u8g2Font.getUTF8Width(displayBuf);
    while (finalWidth > _width && strlen(displayBuf) > 0) {
      remove_last_utf8_char(displayBuf);
      finalWidth = u8g2Font.getUTF8Width(displayBuf);
    }

    // 跳过相同内容的绘制
    if (!_needRedraw && _charStart == _lastCharStart && 
        strcmp(displayBuf, _lastDisplayBuf) == 0) {
      return;
    }
    
    _needRedraw = false;
    _lastCharStart = _charStart;
    strlcpy(_lastDisplayBuf, displayBuf, SCROLL_BUFFER_SIZE);

    // 绘制
    if (_fb->ready()) {
      #ifdef PSFBUFFER
      _fb->fillRect(0, 0, _width, _textheight, _bgcolor);
      int16_t x = (_config.align == WA_CENTER) ? (_width - finalWidth) / 2 :
                  (_config.align == WA_RIGHT) ? _width - finalWidth : 0;
      _fb->setCursor(x, 0);
      _fb->print(displayBuf);
      _fb->display();
      #endif
    } else {
      u8g2Font.setFont(_font);
      dsp.fillRect(_config.left, _config.top, _width, _textheight + 2, _bgcolor);
      u8g2Font.setForegroundColor(_fgcolor);
      u8g2Font.setBackgroundColor(_bgcolor);
      u8g2Font.setFontMode(1);

      int16_t drawX = _config.left;
      if (_config.align == WA_CENTER) {
        drawX = _config.left + (_width - finalWidth) / 2;
      } else if (_config.align == WA_RIGHT) {
        drawX = _config.left + _width - finalWidth;
      }
      
      u8g2Font.drawUTF8(drawX, _config.top + _textheight - 2, displayBuf);
    }
  } else {
    // 非滚动模式保持不变
    if (_fb->ready()) {
      #ifdef PSFBUFFER
      _fb->fillRect(0, 0, _width, _textheight, _bgcolor);
      _fb->setCursor(_realLeft(true), 0);
      _fb->print(_text);
      _fb->display();
      #endif
    } else {
      u8g2Font.setFont(_font);
      dsp.fillRect(_config.left, _config.top, _width, _textheight + 2, _bgcolor);
      u8g2Font.setForegroundColor(_fgcolor);
      u8g2Font.setBackgroundColor(_bgcolor);
      u8g2Font.setFontMode(1);
      u8g2Font.drawUTF8(_realLeft(), _config.top + _textheight - 2, _text);
    }
  }
}

/*
void ScrollWidget::_calcX() {
  if (!_doscroll || _config.textsize == 0) return;
  _pixelOffset += _scrolldelta;   // 向左滚动，增加偏移

  // 计算最大偏移量：文本宽度 + 分隔符宽度
  int maxOffset = _textwidth + _sepwidth;
  // 循环滚动：超过最大值则回到开头
  if (_pixelOffset >= maxOffset) {
    _pixelOffset = 0;
  }
}
*/

bool ScrollWidget::_checkDelay(int m, uint32_t &tstamp) {
  if (millis() - tstamp > m) {
    tstamp = millis();
    return true;
  } else {
    return false;
  }
}

void ScrollWidget::_reset(){
  _x = _fb->ready()?0:_config.left;
  _scrolldelay = millis();
  _doscroll = _checkIsScrollNeeded();
  _pixelOffset = 0;   // <--- 新增
  #ifdef PSFBUFFER
  _fb->freeBuffer();
  uint16_t _rl = (_config.align==WA_CENTER)?(dsp.width()-_width)/2:_config.left;
  _fb->begin(&dsp, _rl, _config.top, _width, _textheight, _bgcolor);
  #endif
}

/************************
      SLIDER WIDGET
 ************************/
void SliderWidget::init(FillConfig conf, uint16_t fgcolor, uint16_t bgcolor, uint32_t maxval, uint16_t oucolor) {
  Widget::init(conf.widget, fgcolor, bgcolor);
  _width = conf.width; _height = conf.height; _outlined = conf.outlined; _oucolor = oucolor, _max = maxval;
  _oldvalwidth = _value = 0;
}

void SliderWidget::setValue(uint32_t val) {
  _value = val;
  if (_active && !_locked) _drawslider();
}

void SliderWidget::_drawslider() {
  uint16_t valwidth = map(_value, 0, _max, 0, _width - _outlined * 2);
  if (_oldvalwidth == valwidth) return;
  dsp.fillRect(_config.left + _outlined + min(valwidth, _oldvalwidth), _config.top + _outlined, abs(_oldvalwidth - valwidth), _height - _outlined * 2, _oldvalwidth > valwidth ? _bgcolor : _fgcolor);
  _oldvalwidth = valwidth;
}

void SliderWidget::_draw() {
  if(_locked) return;
  _clear();
  if(!_active) return;
  if (_outlined) dsp.drawRect(_config.left, _config.top, _width, _height, _oucolor);
  uint16_t valwidth = map(_value, 0, _max, 0, _width - _outlined * 2);
  dsp.fillRect(_config.left + _outlined, _config.top + _outlined, valwidth, _height - _outlined * 2, _fgcolor);
}

void SliderWidget::_clear() {
  dsp.fillRect(_config.left, _config.top, _width, _height, _bgcolor);
}
void SliderWidget::_reset() {
  _oldvalwidth = 0;
}

/************************
      VU WIDGET
 ************************/
#if !defined(DSP_LCD) && !defined(DSP_OLED)
VuWidget::~VuWidget() {
  if(_canvas) free(_canvas);
}

void VuWidget::init(WidgetConfig wconf, VUBandsConfig bands, uint16_t vumaxcolor, uint16_t vumincolor, uint16_t bgcolor) {
  Widget::init(wconf, bgcolor, bgcolor);
  _vumaxcolor = vumaxcolor;
  _vumincolor = vumincolor;
  _bands = bands;
  _canvas = new Canvas(_bands.width * 2 + _bands.space, _bands.height);
}

void VuWidget::_draw(){
  if(!_active || _locked) return;
  static uint16_t measL, measR;
  uint16_t bandColor;
  uint16_t dimension = _config.align?_bands.width:_bands.height;
  uint16_t vulevel = player.get_VUlevel(dimension);
  
  uint8_t L = (vulevel >> 8) & 0xFF;
  uint8_t R = vulevel & 0xFF;
  
  bool played = player.isRunning();
  if(played){
    measL=(L>=measL)?measL + _bands.fadespeed:L;
    measR=(R>=measR)?measR + _bands.fadespeed:R;
  }else{
    if(measL<dimension) measL += _bands.fadespeed;
    if(measR<dimension) measR += _bands.fadespeed;
  }
  if(measL>dimension) measL=dimension;
  if(measR>dimension) measR=dimension;
  uint8_t h=(dimension/_bands.perheight)-_bands.vspace;
  _canvas->fillRect(0,0,_bands.width * 2 + _bands.space,_bands.height, _bgcolor);
  for(int i=0; i<dimension; i++){
    if(i%(dimension/_bands.perheight)==0){
      if(_config.align){
        #ifndef BOOMBOX_STYLE
          bandColor = (i>_bands.width-(_bands.width/_bands.perheight)*4)?_vumaxcolor:_vumincolor;
          _canvas->fillRect(i, 0, h, _bands.height, bandColor);
          _canvas->fillRect(i + _bands.width + _bands.space, 0, h, _bands.height, bandColor);
        #else
          bandColor = (i>(_bands.width/_bands.perheight))?_vumincolor:_vumaxcolor;
          _canvas->fillRect(i, 0, h, _bands.height, bandColor);
          bandColor = (i>_bands.width-(_bands.width/_bands.perheight)*3)?_vumaxcolor:_vumincolor;
          _canvas->fillRect(i + _bands.width + _bands.space, 0, h, _bands.height, bandColor);
        #endif
      }else{
        bandColor = (i<(_bands.height/_bands.perheight)*3)?_vumaxcolor:_vumincolor;
        _canvas->fillRect(0, i, _bands.width, h, bandColor);
        _canvas->fillRect(_bands.width + _bands.space, i, _bands.width, h, bandColor);
      }
    }
  }
  if(_config.align){
    #ifndef BOOMBOX_STYLE
      _canvas->fillRect(_bands.width-measL, 0, measL, _bands.width, _bgcolor);
      _canvas->fillRect(_bands.width * 2 + _bands.space - measR, 0, measR, _bands.width, _bgcolor);
      dsp.drawRGBBitmap(_config.left, _config.top, _canvas->getBuffer(), _bands.width * 2 + _bands.space, _bands.height);
    #else
      _canvas->fillRect(0, 0, _bands.width-(_bands.width-measL), _bands.width, _bgcolor);
      _canvas->fillRect(_bands.width * 2 + _bands.space - measR, 0, measR, _bands.width, _bgcolor);
      dsp.startWrite();
      dsp.setAddrWindow(_config.left, _config.top, _bands.width * 2 + _bands.space, _bands.height);
      dsp.writePixels((uint16_t*)_canvas->getBuffer(), (_bands.width * 2 + _bands.space)*_bands.height);
      dsp.endWrite();
    #endif
  }else{
    _canvas->fillRect(0, 0, _bands.width, measL, _bgcolor);
    _canvas->fillRect(_bands.width + _bands.space, 0, _bands.width, measR, _bgcolor);
    dsp.startWrite();
    dsp.setAddrWindow(_config.left, _config.top, _bands.width * 2 + _bands.space, _bands.height);
    dsp.writePixels((uint16_t*)_canvas->getBuffer(), (_bands.width * 2 + _bands.space)*_bands.height);
    dsp.endWrite();
  }
}

void VuWidget::loop(){
  if(_active && !_locked) _draw();
}

void VuWidget::_clear(){
  dsp.fillRect(_config.left, _config.top, _bands.width * 2 + _bands.space, _bands.height, _bgcolor);
}
#else // DSP_LCD
VuWidget::~VuWidget() { }
void VuWidget::init(WidgetConfig wconf, VUBandsConfig bands, uint16_t vumaxcolor, uint16_t vumincolor, uint16_t bgcolor) {
  Widget::init(wconf, bgcolor, bgcolor);
}
void VuWidget::_draw(){ }
void VuWidget::loop(){ }
void VuWidget::_clear(){ }
#endif

/************************
      NUM & CLOCK
 ************************/
#if !defined(DSP_LCD)
  #if TIME_SIZE<19 //19->NOKIA
  const GFXfont* Clock_GFXfontPtr = nullptr;
  #define CLOCKFONT5x7
  #else
  const GFXfont* Clock_GFXfontPtr = &Clock_GFXfont;
  #endif
#endif //!defined(DSP_LCD)

#if !defined(CLOCKFONT5x7) && !defined(DSP_LCD)
  inline GFXglyph *pgm_read_glyph_ptr(const GFXfont *gfxFont, uint8_t c) {
    return gfxFont->glyph + c;
  }
  uint8_t _charWidth(unsigned char c){
    GFXglyph *glyph = pgm_read_glyph_ptr(&Clock_GFXfont, c - 0x20);
    return pgm_read_byte(&glyph->xAdvance);
  }
  uint16_t _textHeight(){
    GFXglyph *glyph = pgm_read_glyph_ptr(&Clock_GFXfont, '8' - 0x20);
    return pgm_read_byte(&glyph->height);
  }
#else //!defined(CLOCKFONT5x7) && !defined(DSP_LCD)
  uint8_t _charWidth(unsigned char c){
  #ifndef DSP_LCD
    return CHARWIDTH * TIME_SIZE;
  #else
    return 1;
  #endif
  }
  uint16_t _textHeight(){
    return CHARHEIGHT * TIME_SIZE;
  }
#endif
uint16_t _textWidth(const char *txt){
  uint16_t w = 0, l=strlen(txt);
  for(uint16_t c=0;c<l;c++) w+=_charWidth(txt[c]);
  return w;
}

/************************
      NUM WIDGET
 ************************/
void NumWidget::init(WidgetConfig wconf, uint16_t buffsize, bool uppercase, uint16_t fgcolor, uint16_t bgcolor) {
  Widget::init(wconf, fgcolor, bgcolor);
  _buffsize = buffsize;
  _text = (char *) malloc(sizeof(char) * _buffsize);
  memset(_text, 0, _buffsize);
  _oldtext = (char *) malloc(sizeof(char) * _buffsize);
  memset(_oldtext, 0, _buffsize);
  _textwidth = _oldtextwidth = _oldleft = 0;
  _uppercase = uppercase;
  _textheight = TIME_SIZE;
}

void NumWidget::setText(const char* txt) {
  strlcpy(_text, txt, _buffsize);
  _getBounds();
  if (strcmp(_oldtext, _text) == 0) return;
  uint16_t realth = _textheight;
#if defined(DSP_OLED) && DSP_MODEL!=DSP_SSD1322
  if(Clock_GFXfontPtr==nullptr) realth = _textheight * 8;
#endif
  if (_active)
  #ifndef CLOCKFONT5x7
    dsp.fillRect(_oldleft == 0 ? _realLeft() : min(_oldleft, _realLeft()),  _config.top-_textheight+1, max(_oldtextwidth, _textwidth), realth, _bgcolor);
  #else
    dsp.fillRect(_oldleft == 0 ? _realLeft() : min(_oldleft, _realLeft()),  _config.top, max(_oldtextwidth, _textwidth), realth, _bgcolor);
  #endif

  _oldtextwidth = _textwidth;
  _oldleft = _realLeft();
  if (_active) _draw();
}

void NumWidget::setText(int val, const char *format){
  char buf[_buffsize];
  snprintf(buf, _buffsize, format, val);
  setText(buf);
}

void NumWidget::_getBounds() {
  _textwidth = _textWidth(_text);
}

void NumWidget::_draw() {
#ifndef DSP_LCD
  if(!_active || TIME_SIZE<2) return;
  // 数字显示使用原有字体（通常为英文数字），如需中文可改用U8g2
  // 此处保持原样，因为数字不需要中文支持
  dsp.setTextSize(Clock_GFXfontPtr==nullptr?TIME_SIZE:1);
  dsp.setFont(Clock_GFXfontPtr);
  dsp.setTextColor(_fgcolor, _bgcolor);
#endif
  if(!_active) return;
  dsp.setCursor(_realLeft(), _config.top);
  dsp.print(_text);
  strlcpy(_oldtext, _text, _buffsize);
  dsp.setFont();
}

/**************************
      PROGRESS WIDGET
 **************************/
void ProgressWidget::_progress() {
  char buf[_width + 1];
  snprintf(buf, _width, "%*s%.*s%*s", _pg <= _barwidth ? 0 : _pg - _barwidth, "", _pg <= _barwidth ? _pg : 5, ".....", _width - _pg, "");
  _pg++; if (_pg >= _width + _barwidth) _pg = 0;
  setText(buf);
}

bool ProgressWidget::_checkDelay(int m, uint32_t &tstamp) {
  if (millis() - tstamp > m) {
    tstamp = millis();
    return true;
  } else {
    return false;
  }
}

void ProgressWidget::loop() {
  if (_checkDelay(_speed, _scrolldelay)) {
    _progress();
  }
}

/**************************
      CLOCK WIDGET
 **************************/
void ClockWidget::init(WidgetConfig wconf, uint16_t fgcolor, uint16_t bgcolor){
  Widget::init(wconf, fgcolor, bgcolor);
  _timeheight = _textHeight();
  _fullclock = TIME_SIZE>35 || DSP_MODEL==DSP_ILI9225;
  if(_fullclock) _superfont = TIME_SIZE / 17;
  else if(TIME_SIZE==19 || TIME_SIZE==2) _superfont=1;
  else _superfont=0;
  _space = (5*_superfont)/2;
  #ifndef HIDE_DATE
  if(_fullclock){
    _dateheight = _superfont<4?1:2;
    _clockheight = _timeheight + _space + CHARHEIGHT * _dateheight;
  } else {
    _clockheight = _timeheight;
  }
  #else
    _clockheight = _timeheight;
  #endif
  _getTimeBounds();
#ifdef PSFBUFFER
  _fb = new psFrameBuffer(dsp.width(), dsp.height());
  _begin();
#endif
}

void ClockWidget::_begin(){
#ifdef PSFBUFFER
  _fb->begin(&dsp, _clockleft, _config.top-_timeheight, _clockwidth, _clockheight+1, config.theme.background);
#endif
}

bool ClockWidget::_getTime(){
  strftime(_timebuffer, sizeof(_timebuffer), "%H:%M", &network.timeinfo);
  bool ret = network.timeinfo.tm_sec==0 || _forceflag!=network.timeinfo.tm_year;
  _forceflag = network.timeinfo.tm_year;
  return ret;
}

uint16_t ClockWidget::_left(){
  if(_fb->ready()) return 0; else return _clockleft;
}
uint16_t ClockWidget::_top(){
  if(_fb->ready()) return _timeheight; else return _config.top;
}

void ClockWidget::_getTimeBounds() {
  _timewidth = _textWidth(_timebuffer);
  uint8_t fs = _superfont>0?_superfont:TIME_SIZE;
  uint16_t rightside = CHARWIDTH * fs * 2;
  if(_fullclock){
    rightside += _space*2+1;
    _clockwidth = _timewidth+rightside;
  } else {
    if(_superfont==0)
      _clockwidth = _timewidth;
    else
      _clockwidth = _timewidth + rightside;
  }
  switch(_config.align){
    case WA_LEFT: _clockleft = _config.left; break;
    case WA_RIGHT: _clockleft = dsp.width()-_clockwidth-_config.left; break;
    default:
      _clockleft = (dsp.width()/2 - _clockwidth/2)+_config.left;
      break;
  }
  char buf[4];
  strftime(buf, 4, "%H", &network.timeinfo);
  _dotsleft=_textWidth(buf);
}

#ifndef DSP_LCD

Adafruit_GFX& ClockWidget::getRealDsp(){
#ifdef PSFBUFFER
  if (_fb && _fb->ready()) return *_fb;
#endif
  return dsp;
}

void ClockWidget::_printClock(bool force){
  auto& gfx = getRealDsp();
  gfx.setTextSize(Clock_GFXfontPtr==nullptr?TIME_SIZE:1);
  gfx.setFont(Clock_GFXfontPtr);
  bool clockInTitle=!config.isScreensaver && _config.top<_timeheight;
  if(force){
    _clearClock();
    _getTimeBounds();
    #ifndef DSP_OLED
    if(CLOCKFONT_MONO) {
      gfx.setTextColor(config.theme.clockbg, config.theme.background);
      gfx.setCursor(_left(), _top());
      gfx.print("88:88");
    }
    #endif
    if(clockInTitle)
      gfx.setTextColor(config.theme.meta, config.theme.metabg);
    else
      gfx.setTextColor(config.theme.clock, config.theme.background);
    gfx.setCursor(_left(), _top());
    gfx.print(_timebuffer);
    if(_fullclock){
      bool fullClockOnScreensaver = (!config.isScreensaver || (_fb->ready() && FULL_SCR_CLOCK));
      _linesleft = _left()+_timewidth+_space;
      if(fullClockOnScreensaver){
        gfx.drawFastVLine(_linesleft, _top()-_timeheight, _timeheight, config.theme.div);
        gfx.drawFastHLine(_linesleft, _top()-(_timeheight)/2, CHARWIDTH * _superfont * 2 + _space, config.theme.div);
        gfx.setFont();
        gfx.setTextSize(_superfont);
        
        // ---------- 星期绘制（中文）----------
        if (_fb && _fb->ready()) {
            // 帧缓冲模式：暂时保留原方法（英文）
            gfx.setCursor(_linesleft+_space+1, _top()-CHARHEIGHT * _superfont);
            gfx.setTextColor(config.theme.clock, config.theme.background);
            gfx.print(LANG::dow[network.timeinfo.tm_wday]);
        } else {
            // 直接显示：使用 U8g2 绘制中文星期
            u8g2Font.setFont(u8g2_font_wqy12_t_gb2312);   // 选用 12px 中文字体
            u8g2Font.setForegroundColor(config.theme.clock);
            u8g2Font.setBackgroundColor(config.theme.background);
            u8g2Font.setFontMode(1);
            // 计算基线坐标：原顶部 + 文本高度 - 2
            int16_t week_y = _top() - CHARHEIGHT * _superfont + CHARHEIGHT * _superfont - 2;
            u8g2Font.drawUTF8(_linesleft+_space+1, week_y, LANG::dow[network.timeinfo.tm_wday]);
        }
        // -------------------------------------
                
        //sprintf(_tmp, "%2d %s %d", network.timeinfo.tm_mday, LANG::mnths[network.timeinfo.tm_mon], network.timeinfo.tm_year+1900);
        sprintf(_tmp, "%04d-%02d-%02d", network.timeinfo.tm_year+1900, network.timeinfo.tm_mon+1, network.timeinfo.tm_mday);
        #ifndef HIDE_DATE
        strlcpy(_datebuf, _tmp, sizeof(_datebuf));
        uint16_t _datewidth = strlen(_datebuf) * CHARWIDTH * _dateheight;
        gfx.setTextSize(_dateheight);
        #if DSP_MODEL==DSP_GC9A01A
        gfx.setCursor((dsp.width()-_datewidth)/2, _top() + _space);
        #else
        gfx.setCursor(_left()+_clockwidth-_datewidth, _top() + _space);
        #endif
        
        // ---------- 日期绘制（中文）----------
        if (_fb && _fb->ready()) {
            gfx.setTextColor(config.theme.clock, config.theme.background);
            gfx.print(_datebuf);
        } else {
            u8g2Font.setFont(u8g2_font_wqy12_t_gb2312);
            u8g2Font.setForegroundColor(config.theme.clock);
            u8g2Font.setBackgroundColor(config.theme.background);
            u8g2Font.setFontMode(1);
            int16_t date_y = _top() + _space + CHARHEIGHT * _dateheight - 2;
            u8g2Font.drawUTF8(_left()+_clockwidth-_datewidth, date_y, _datebuf);
        }
        // -------------------------------------
        
        #endif
      }
    }
  }
  if(_fullclock || _superfont>0){
    gfx.setFont();
    gfx.setTextSize(_superfont);
    if(!_fullclock){
      #ifndef CLOCKFONT5x7
      gfx.setCursor(_left()+_timewidth+_space, _top()-_timeheight+_space);
      #else
      gfx.setCursor(_left()+_timewidth+_space, _top());
      #endif
    }else{
      gfx.setCursor(_linesleft+_space+1, _top()-_timeheight);
    }
    gfx.setTextColor(config.theme.seconds, config.theme.background);
    sprintf(_tmp, "%02d", network.timeinfo.tm_sec);
    gfx.print(_tmp);
  }
  gfx.setTextSize(Clock_GFXfontPtr==nullptr?TIME_SIZE:1);
  gfx.setFont(Clock_GFXfontPtr);
  #ifndef DSP_OLED
  gfx.setTextColor(dots ? config.theme.clock : (CLOCKFONT_MONO?config.theme.clockbg:config.theme.background), config.theme.background);
  #else
  if(clockInTitle)
    gfx.setTextColor(dots ? config.theme.meta:config.theme.metabg, config.theme.metabg);
  else
    gfx.setTextColor(dots ? config.theme.clock:config.theme.background, config.theme.background);
  #endif
  dots=!dots;
  gfx.setCursor(_left()+_dotsleft, _top());
  gfx.print(":");
  gfx.setFont();
  if(_fb->ready()) _fb->display();
}


void ClockWidget::_clearClock(){
#ifdef PSFBUFFER
  if(_fb->ready()) _fb->clear();
  else
#endif
#ifndef CLOCKFONT5x7
  dsp.fillRect(_left(), _top()-_timeheight, _clockwidth, _clockheight+1, config.theme.background);
#else
  dsp.fillRect(_left(), _top(), _clockwidth+1, _clockheight+1, config.theme.background);
#endif
}

void ClockWidget::draw(bool force){
  if(!_active) return;
  _printClock(_getTime() || force);
}

void ClockWidget::_draw(){
  if(!_active) return;
  _printClock(true);
}

void ClockWidget::_reset(){
#ifdef PSFBUFFER
  if(_fb->ready()) {
    _fb->freeBuffer();
    _getTimeBounds();
    _begin();
  }
#endif
}

void ClockWidget::_clear(){
  _clearClock();
}
#else //#ifndef DSP_LCD

void ClockWidget::_printClock(bool force){
  strftime(_timebuffer, sizeof(_timebuffer), "%H:%M", &network.timeinfo);
  if(force){
    dsp.setCursor(dsp.width()-5, 0);
    dsp.print(_timebuffer);
  }
  dsp.setCursor(dsp.width()-5+2, 0);
  dsp.print((network.timeinfo.tm_sec % 2 == 0)?":":" ");
}

void ClockWidget::_clearClock(){}

void ClockWidget::draw(bool force){
  if(!_active) return;
  _printClock(true);
}
void ClockWidget::_draw(){
  if(!_active) return;
  _printClock(true);
}
void ClockWidget::_reset(){}
void ClockWidget::_clear(){}
#endif //#ifndef DSP_LCD

/**************************
      BITRATE WIDGET
 **************************/
void BitrateWidget::init(BitrateConfig bconf, uint16_t fgcolor, uint16_t bgcolor){
  Widget::init(bconf.widget, fgcolor, bgcolor);
  _dimension = bconf.dimension;
  _bitrate = 0;
  _format = BF_UNKNOWN;
  _charSize(bconf.widget.textsize, _charWidth, _textheight);
  memset(_buf, 0, 6);
}

void BitrateWidget::setBitrate(uint16_t bitrate){
  _bitrate = bitrate;
  if(_bitrate>999) _bitrate=999;
  _draw();
}

void BitrateWidget::setFormat(BitrateFormat format){
  _format = format;
  _draw();
}

//TODO move to parent
void BitrateWidget::_charSize(uint8_t textsize, uint8_t& width, uint16_t& height){
#ifndef DSP_LCD
  width = textsize * CHARWIDTH;
  height = textsize * CHARHEIGHT;
#else
  width = 1;
  height = 1;
#endif
}

void BitrateWidget::_draw() {
  _clear();// 清除背景区域
  // 检查 widget 是否激活，是否有有效的格式和比特率
  if (!_active || _format == BF_UNKNOWN || _bitrate == 0) return;

  // 绘制边框框和填充下半部分（创建视觉效果）
  dsp.drawRect(_config.left, _config.top, _dimension, _dimension, _fgcolor);
  dsp.fillRect(_config.left, _config.top + _dimension / 2, _dimension, _dimension / 2, _fgcolor);
  
  // 设置文本属性
  dsp.setFont();
  dsp.setTextSize(_config.textsize);
  dsp.setTextColor(_fgcolor, _bgcolor);
  
  // 格式化并显示比特率数值
  snprintf(_buf, 6, "%d", _bitrate);
  dsp.setCursor(_config.left + _dimension/2 - _charWidth*strlen(_buf)/2 + 1, _config.top + _dimension/4 - _textheight/2+1);
  dsp.print(_buf);
  
  // 切换文本颜色并显示音频格式标识
  dsp.setTextColor(_bgcolor, _fgcolor);
  dsp.setCursor(_config.left + _dimension / 2 - _charWidth * 3 / 2 + 1,
                _config.top + _dimension - _dimension / 4 - _textheight / 2);
  switch (_format) {
    case BF_MP3:  dsp.print("MP3"); break;
    case BF_AAC:  dsp.print("AAC"); break;
    case BF_FLAC: dsp.print("FLC"); break;
    case BF_OGG:  dsp.print("OGG"); break;
    case BF_WAV:  dsp.print("WAV"); break;
    default: break;
  }
}

void BitrateWidget::_clear() {
  dsp.fillRect(_config.left, _config.top, _dimension, _dimension, _bgcolor);
}


//void BitrateWidget::loop() {
//  if (_active && !_locked) _draw();
//}



/**************************
      PLAYLIST WIDGET
 **************************/
void PlayListWidget::init(ScrollWidget* current){
  Widget::init({0, 0, 0, WA_LEFT}, 0, 0);
  _current = current;
  #ifndef DSP_LCD
  _plItemHeight = playlistConf.widget.textsize*(CHARHEIGHT-1)+playlistConf.widget.textsize*4;
  _plTtemsCount = round((float)dsp.height()/_plItemHeight);
  if(_plTtemsCount%2==0) _plTtemsCount++;
  _plCurrentPos = _plTtemsCount/2;
  _plYStart = (dsp.height() / 2 - _plItemHeight / 2) - _plItemHeight * (_plTtemsCount - 1) / 2 + playlistConf.widget.textsize*2;
  #else
  _plTtemsCount = PLMITEMS;
  _plCurrentPos = 1;
  #endif
}

uint8_t PlayListWidget::_fillPlMenu(int from, uint8_t count) {
    int ls = from;
    uint8_t c = 0;
    bool finded = false;

    if (config.playlistLength() == 0) {
        return 0;
    }

    File playlist = config.SDPLFS()->open(REAL_PLAYL, "r");
    if (!playlist) {
        Serial.println("Error: cannot open playlist file");
        return 0;
    }

    File index = config.SDPLFS()->open(REAL_INDEX, "r");
    if (!index) {
        Serial.println("Error: cannot open index file");
        playlist.close();
        return 0;
    }

    char lineBuf[BUFLEN];  // 固定缓冲区，大小由 BUFLEN 决定

    while (true) {
        esp_task_wdt_reset();  // 防止看门狗超时

        if (ls < 1) {
            ls++;
            _printPLitem(c, "");
            c++;
            continue;
        }

        if (!finded) {
            index.seek((ls - 1) * 4, SeekSet);
            uint32_t pos;
            if (index.readBytes((char*)&pos, 4) != 4) {
                Serial.println("Error reading index file");
                break;
            }
            finded = true;
            index.close();
            playlist.seek(pos, SeekSet);
        }

        while (playlist.available()) {
            // 读取一行到 lineBuf
            int len = playlist.readBytesUntil('\n', (uint8_t*)lineBuf, BUFLEN - 1);
            if (len <= 0) break;
            lineBuf[len] = '\0';

            // 查找制表符并截断，只保留电台名称部分
            char* tabPos = strchr(lineBuf, '\t');
            if (tabPos) *tabPos = '\0';

            // 如果需要显示序号，构建带序号的字符串
            if (config.store.numplaylist && strlen(lineBuf) > 0) {
                char numbered[BUFLEN];
                snprintf(numbered, BUFLEN, "%d %s", from + c, lineBuf);
                _printPLitem(c, numbered);
            } else {
                _printPLitem(c, lineBuf);
            }
            c++;
            if (c >= count) break;
        }
        break;
    }

    playlist.close();
    return c;
}

#ifndef DSP_LCD
void PlayListWidget::drawPlaylist(uint16_t currentItem) {
  if (config.playlistLength() == 0) {
    // 清除整个区域并显示提示
    dsp.fillRect(0, 0, dsp.width(), dsp.height(), config.theme.background);
    dsp.setCursor(10, 20); // 根据实际布局调整坐标
    dsp.setTextColor(config.theme.meta, config.theme.background);
    dsp.print("No files found on SD card");
    return;
  }
  uint8_t lastPos = _fillPlMenu(currentItem - _plCurrentPos, _plTtemsCount);
  if (lastPos < _plTtemsCount) {
    dsp.fillRect(0, lastPos * _plItemHeight + _plYStart, dsp.width(), dsp.height()/2, config.theme.background);
  }
}

void PlayListWidget::_printPLitem(uint8_t pos, const char* item){
  // 清除背景
  dsp.fillRect(0, _plYStart + pos * _plItemHeight - 1, dsp.width(), _plItemHeight - 2, config.theme.background);
  if (pos == _plCurrentPos) {
    _current->setText(item);
  } else {
    uint8_t plColor = (abs(pos - _plCurrentPos)-1)>4?4:abs(pos - _plCurrentPos)-1;
    int16_t x = TFT_FRAMEWDT;
    int16_t yTop = _plYStart + pos * _plItemHeight;
    uint16_t textHeight = playlistConf.widget.textsize * CHARHEIGHT;
    dsp.setClipping({0, _plYStart + pos * _plItemHeight - 1, dsp.width(), _plItemHeight - 2});

        // ===== 添加调试 =====
    Serial.printf("Item %d, plColor=%d, color value=0x%04X\n", 
                  pos, plColor, config.theme.playlist[plColor]);
    // ===================
    
    // 使用全局默认字体（需在外部设置）
    u8g2Font.setFont(u8g2_font_wqy12_t_gb2312);  // 使用12px中文字体
    u8g2Font.setForegroundColor(config.theme.playlist[plColor]);
    u8g2Font.setBackgroundColor(config.theme.background);
    u8g2Font.setFontMode(1);
    // U8g2使用基线坐标，需要调整为从顶部绘制
    u8g2Font.drawUTF8(x, yTop + textHeight - 2, item);
    dsp.clearClipping();
  }
}
#else
void PlayListWidget::_printPLitem(uint8_t pos, const char* item){
  if (pos == _plCurrentPos) {
    _current->setText(item);
  } else {
    dsp.setCursor(1, pos);
    char tmp[dsp.width()] = {0};
    strlcpy(tmp, item, dsp.width());
    dsp.print(tmp);
  }
}

void PlayListWidget::drawPlaylist(uint16_t currentItem) {
  dsp.clear();
  _fillPlMenu(currentItem - _plCurrentPos, _plTtemsCount);
  dsp.setCursor(0,1);
  dsp.write(uint8_t(126));
}
#endif

#endif // #if DSP_MODEL!=DSP_DUMMY