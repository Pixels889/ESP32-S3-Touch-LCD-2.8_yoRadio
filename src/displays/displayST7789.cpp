#include "../core/options.h"
#if DSP_MODEL==DSP_ST7789 || DSP_MODEL==DSP_ST7789_240 || DSP_MODEL==DSP_ST7789_76 || DSP_MODEL==DSP_ST7789_170
#include "dspcore.h"
#include "../core/config.h"

/*
#if DSP_HSPI
DspCore::DspCore(): Adafruit_ST7789(&SPI2, TFT_CS, TFT_DC, TFT_RST) {}
#else
DspCore::DspCore(): Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST) {}
#endif
*/

#if DSP_HSPI
DspCore::DspCore(): Adafruit_ST7789(&SPI2, TFT_CS, TFT_DC, TFT_RST) {
    // 如果使用硬件 SPI，需要在这里设置 SPI2 引脚
    SPI2.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
}
#else
// 软件 SPI 构造函数，传入所有自定义引脚
DspCore::DspCore(): Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST) {
    // 软件 SPI 不需要设置引脚，但可以设置 SPI 模式
   // setSPIMode(SPI_MODE3);  // 尝试 SPI_MODE3
}
#endif

void DspCore::initDisplay() {
  if(DSP_MODEL==DSP_ST7789_76){
    init(76,284);
  }else if(DSP_MODEL==DSP_ST7789_170){
    init(170,320);
  }else{
    init(240,(DSP_MODEL==DSP_ST7789)?320:240);
  }
  invert();
  cp437(true);
  flip();
  setTextWrap(false);
  setTextSize(1);
  fillScreen(0x0000);
}

void DspCore::clearDsp(bool black){ fillScreen(black?0:config.theme.background); }
void DspCore::flip(){
#if DSP_MODEL==DSP_ST7789 || DSP_MODEL==DSP_ST7789_76 || DSP_MODEL==DSP_ST7789_170
  setRotation(config.store.flipscreen?1:3);
#endif
#if DSP_MODEL==DSP_ST7789_240
  if(ROTATE_90){
    setRotation(config.store.flipscreen?3:1);
  }else{
    setRotation(config.store.flipscreen?2:0);
  }
#endif
}
void DspCore::invert(){ invertDisplay(
#if DSP_MODEL==DSP_ST7789_170
!
#endif
config.store.invertdisplay); }
void DspCore::sleep(void){ enableSleep(true); delay(150); enableDisplay(false); delay(150); }
void DspCore::wake(void){ enableDisplay(true); delay(150); enableSleep(false); delay(150); }

#endif
