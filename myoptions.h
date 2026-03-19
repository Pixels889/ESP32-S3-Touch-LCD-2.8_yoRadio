// My youtube channel: https://www.youtube.com/@LeventeDaradici/videos
// My pastebin page: https://pastebin.com/u/LeventeDaradici
// ёRadio on Github: https://github.com/e2002/yoradio
// ...
#ifndef myoptions_h
#define myoptions_h

#define L10N_LANGUAGE     EN 
#define HOSTNAME          "Yo-Radio"


// 熄屏时间（秒）
#ifndef SCREENSAVER_TIMEOUT
#define SCREENSAVER_TIMEOUT   180  // 3分钟 = 180秒
#endif

// ========== 显示屏 ST7789（完全按您的连接）==========

#define DSP_MODEL           DSP_ST7789
#define DSP_HSPI            1   // 使用软件还是硬件spi
//#define DSP_HSPI            true
#define LED_INVERT          1          // 若背光不亮可改为 false
#define TFT_CS              42
#define TFT_DC              41
#define TFT_RST             39
#define TFT_MOSI            45            // ✅ 您的 MOSI
#define TFT_SCLK            40            // ✅ 您的 SCLK
#define BRIGHTNESS_PIN      5


// ========== I2S 音频 - PCM5101==========
#define I2S_BCLK            48
#define I2S_LRC             38
#define I2S_DOUT            47
#define VS1053_CS           255


#define MUTE_PIN          255               /*  MUTE Pin */
//#define MUTE_VAL          HIGH              /*  Write this to MUTE_PIN when player is stopped */
#define PLAYER_FORCE_MONO false             /*  mono option on boot - false stereo, true mono  */
#define I2S_INTERNAL      false              /* 使用外部 I2S DAC (PCM5101)  */

/*        SDCARD                  */
// ========== SD 卡配置（SPI 模式）==========
// ========== SD 卡配置（SPI 模式）==========
#define SDC_CS              21              // 片选引脚 (SD_D3)
#define SD_SCLK             14              // 时钟引脚 (SD_SCK)
#define SD_MOSI             17              // 主机输出从机输入 (SD_CMD)
#define SD_MISO             16              // 主机输入从机输出 (SD_D0)
#define SD_HSPI             false           // 使用默认 SPI（VSPI），引脚将在代码中手动设置

//#ifndef SDC_CS
//  #define SDC_CS        255  // SDCARD CS pin
//#endif
//#ifndef SD_HSPI
 // #define SD_HSPI       false  // use HSPI for SD (miso=12, mosi=13, clk=14) instead of VSPI (by default)
//#endif
//#if SDC_CS!=255
//  #define USE_SD
//#endif


// 或者，如果项目要求必须定义，可以设为 -1 或 255 表示未使用

// ========== 以下外设全部禁用 ==========
//#define BME_SCL           41
//#define BME_SDA           42
//#define BMP280_ADDRESS    0x76
//#define SEALEVELPRESSURE_HPA 1013.25

//#define VS1053_CS           255
//#define VS1053_DCS          255
//#define VS1053_DREQ         255
//#define VS1053_RST          255

// 启用触摸功能
#define TS_MODEL_UNDEFINED  0
#define TS_MODEL_XPT2046    1
#define TS_MODEL_GT911      2
#define TS_MODEL_CST328     3

// 选择 CST328
#define TS_MODEL TS_MODEL_CST328
//#define TS_MODEL TS_MODEL_UNDEFINED

// CST328 硬件引脚（根据你的连接）
#define TS_SDA  1
#define TS_SCL  3
#define TS_RST  2
#define TS_INT  4



//        RTC                     
// 启用 PCF85063 RTC
#define RTC_MODULE PCF85063

// 定义 I2C 引脚
#define RTC_SDA    11
#define RTC_SCL    10
#define RTC_INT    9   // 可选，用于中断


//麦克风
// --- INMP441 I2S 引脚配置 ---
// 根据你的连接定义：
// INMP441 SD  ->  ESP32-S3 GPIO 50
// INMP441 L/R ->  ESP32-S3 GPIO 49 (接GND选左声道)
// INMP441 SCK ->  ESP32-S3 GPIO 18
// INMP441 WS  ->  ESP32-S3 GPIO 15
// ========== INMP441 麦克风配置 ==========
//#define MIC_I2S_PORT        I2S_NUM_1           // 使用 I2S 控制器 1
//#define MIC_BCLK            18                  // SCK 引脚
//#define MIC_WS              15                  // WS 引脚
//#define MIC_SD              50                  // SD 引脚
//#define MIC_LR              49                  // L/R 引脚（控制声道）

// 声道选择：0 = 左声道（L/R 接低电平），1 = 右声道（L/R 接高电平）
//define MIC_CHANNEL_SELECT  0   // 请根据实际硬件连接修改：0-左，1-右

// --- I2S 驱动配置参数 (示例值，可根据项目需要调整) ---
//#define I2S_PORT I2S_NUM_1          // 使用 I2S 控制器 1
//#define SAMPLE_RATE 16000            // 采样率 (Hz)，语音常用16kHz
//#define I2S_BITS_PER_SAMPLE I2S_BITS_PER_SAMPLE_32BIT  // INMP441 输出24位数据，通常用32位读取 [citation:1][citation:8]




// 启用 QMI8658 陀螺仪（你的开发板自带）
//#define USE_QMI8658


/*
//        RTC                     
#define RTC_MODULE_UNDEFINED    0
#define DS3231                  1
#define DS1307                  2
#define PCF85063                3

#ifndef RTC_MODULE
  #define RTC_MODULE            RTC_MODULE_UNDEFINED  //  DS3231 or DS1307  //
#endif
#ifndef RTC_SDA
  #define RTC_SDA               255
#endif
#ifndef RTC_SCL
  #define RTC_SCL               255
#endif
*/

//#define IR_PIN            27
//#define LIGHT_SENSOR      34
//#define AUTOBACKLIGHT_MAX 1024
//#define LED_BUILTIN       17
//#define BATTERY_OFF       true

// ========== 通用配置 ==========
#define BOOTDELAY           5000
//#define PLAYER_FORCE_MONO false

#endif