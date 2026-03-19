#ifndef _U8G2_FONT_ALIBABAPUHUITIREGULAR22T_H
#define _U8G2_FONT_ALIBABAPUHUITIREGULAR22T_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef U8G2_USE_LARGE_FONTS
#define U8G2_USE_LARGE_FONTS
#endif

#ifndef U8X8_FONT_SECTION

#ifdef __GNUC__
#  define U8X8_SECTION(name) __attribute__ ((section (name)))
#else
#  define U8X8_SECTION(name)
#endif

#if defined(__GNUC__) && defined(__AVR__)
#  define U8X8_FONT_SECTION(name) U8X8_SECTION(".progmem." name)
#endif

#if defined(ESP8266)
#  define U8X8_FONT_SECTION(name) __attribute__((section(".text." name)))
#endif

#ifndef U8X8_FONT_SECTION
#  define U8X8_FONT_SECTION(name)
#endif

#endif

#ifndef U8G2_FONT_SECTION
#define U8G2_FONT_SECTION(name) U8X8_FONT_SECTION(name)
#endif

// 字体数组外部声明（已在 .c 文件中定义）
extern const uint8_t Alibaba_PuHuiTi_Regular22t_22[] U8G2_FONT_SECTION("Alibaba_PuHuiTi_Regular22t_22");

#ifdef __cplusplus
}
#endif

// 为了方便使用，定义字体别名宏
#define u8g2_font_alibabapuhuitiregular22t Alibaba_PuHuiTi_Regular22t_22

#endif
