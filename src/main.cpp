#include "Arduino.h"
#include "core/options.h"
#include "core/config.h"
#include "pluginsManager/pluginsManager.h"
#include "core/telnet.h"
#include "core/player.h"
#include "core/display.h"
#include "core/network.h"
#include "core/netserver.h"
#include "core/controls.h"
//#include "core/mqtt.h"
#include "core/optionschecker.h"
#include "core/timekeeper.h"
#include "core/audiohandlers.h"
#include "core/PWR_Key.h"
#include "esp_task_wdt.h"

#ifdef USE_NEXTION
#include "displays/nextion.h"
#endif

#if USE_OTA
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
#include <NetworkUdp.h>
#else
#include <WiFiUdp.h>
#endif
#include <ArduinoOTA.h>
#endif

#if DSP_HSPI || TS_HSPI || VS_HSPI
SPIClass  SPI2(HSPI);
#endif

extern __attribute__((weak)) void yoradio_on_setup();

#if USE_OTA
void setupOTA(){
  if(strlen(config.store.mdnsname)>0)
    ArduinoOTA.setHostname(config.store.mdnsname);
#ifdef OTA_PASS
  ArduinoOTA.setPassword(OTA_PASS);
#endif
  ArduinoOTA
    .onStart([]() {
      player.sendCommand({PR_STOP, 0});
      display.putRequest(NEWMODE, UPDATING);
      telnet.printf("Start OTA updating %s\n", ArduinoOTA.getCommand() == U_FLASH?"firmware":"filesystem");
    })
    .onEnd([]() {
      telnet.printf("\nEnd OTA update, Rebooting...\n");
      ESP.restart();
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      telnet.printf("Progress OTA: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      telnet.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) {
        telnet.printf("Auth Failed\n");
      } else if (error == OTA_BEGIN_ERROR) {
        telnet.printf("Begin Failed\n");
      } else if (error == OTA_CONNECT_ERROR) {
        telnet.printf("Connect Failed\n");
      } else if (error == OTA_RECEIVE_ERROR) {
        telnet.printf("Receive Failed\n");
      } else if (error == OTA_END_ERROR) {
        telnet.printf("End Failed\n");
      }
    });
  ArduinoOTA.begin();
}
#endif

void setup() {
  disableCore0WDT();  // 禁用核心0的看门狗
  disableCore1WDT();  // 禁用核心1的看门狗
  PWR_Init();

    Serial.begin(115200);
    delay(1000);
    
    Wire.begin(11, 10);   // SDA=11, SCL=10
    Wire.setClock(100000); // 可选：降低速度提高稳定性
//
  if(REAL_LEDBUILTIN!=255) pinMode(REAL_LEDBUILTIN, OUTPUT);
  if (yoradio_on_setup) yoradio_on_setup();
  pm.on_setup();
  //esp_task_wdt_reset();  // 喂狗
   config.init(); 
  //esp_task_wdt_reset();  // 喂狗
  display.init();
  //esp_task_wdt_reset();  // 喂狗
  //Serial.println("[DISPLAY] Initializing ST7789...");  // 新增
  player.init();
  //esp_task_wdt_reset();  // 喂狗
  network.begin();
  //esp_task_wdt_reset();  // 喂狗
  if (network.status != CONNECTED && network.status!=SDREADY) {
    netserver.begin();
    initControls();
    display.putRequest(DSP_START);
    while(!display.ready())
    //esp_task_wdt_reset();  // 喂狗
    delay(10);
    return;
  }
  if(SDC_CS!=255) {
    display.putRequest(WAITFORSD, 0);
    Serial.print("##[BOOT]#\tSD search\t");
  }
  config.initPlaylistMode();
  netserver.begin();
  telnet.begin();
  initControls();
  display.putRequest(DSP_START);
  while(!display.ready()) delay(10);
  #if USE_OTA
    setupOTA();
  #endif
  if (config.getMode()==PM_SDCARD) player.initHeaders(config.station.url);
  player.lockOutput=false;
  if (config.store.smartstart == 1) {
    player.sendCommand({PR_PLAY, config.lastStation()});
  }
  pm.on_end_setup();
  enableCore0WDT();
  enableCore1WDT();
}

void loop() {
  esp_task_wdt_reset();  // 每次循环都喂狗
  PWR_Loop();

  timekeeper.loop1();
  telnet.loop();
  if (network.status == CONNECTED || network.status==SDREADY) {
    player.loop();
#if USE_OTA
    ArduinoOTA.handle();
#endif
  }
  loopControls();
  #ifdef NETSERVER_LOOP1
  netserver.loop();
  #endif

}


