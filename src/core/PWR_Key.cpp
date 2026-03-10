#include "options.h"
#include "PWR_Key.h"
#include "config.h"
#include "display.h"
#include "touchscreen.h"
#include "player.h"

static uint32_t pressStartTime = 0;
static bool buttonPressed = false;
static bool shutdownTriggered = false;
static uint32_t lastDebounceTime = 0;
static bool lastButtonState = HIGH;
static bool buttonState = HIGH;
static bool shutdownPending = false;

void PWR_Init(void) {
    pinMode(PWR_KEY_Input_PIN, INPUT_PULLUP);
    pinMode(PWR_Control_PIN, OUTPUT);
    digitalWrite(PWR_Control_PIN, LOW);
    vTaskDelay(100);
    digitalWrite(PWR_Control_PIN, HIGH);
    
    lastButtonState = digitalRead(PWR_KEY_Input_PIN);
    buttonState = lastButtonState;
    shutdownPending = false;
}

void Shutdown(void) {
    digitalWrite(PWR_Control_PIN, LOW);
}

void PWR_Loop(void) {
    bool reading = digitalRead(PWR_KEY_Input_PIN);
    
    // 防抖处理
    if (reading != lastButtonState) {
        lastDebounceTime = millis();
    }
    
    if ((millis() - lastDebounceTime) > DEBOUNCE_TIME) {
        buttonState = reading;
    }
    
    lastButtonState = reading;
    bool pressed = (buttonState == LOW); // 按下为低
    
    if (pressed) {
        if (!buttonPressed) {
            // 首次按下
            buttonPressed = true;
            pressStartTime = millis();
            shutdownTriggered = false;
            shutdownPending = false;
        } else {
            // 持续按下，检测长按事件
            uint32_t holdTime = millis() - pressStartTime;
            
            if (!shutdownPending && !shutdownTriggered && holdTime >= SHUTDOWN_TIME) {
                shutdownPending = true;
                
                // 发送关机提示消息
                display.putRequest(SHUTDOWN_PROMPT, 0);
                
                Serial.println("Long press - release to shutdown");
            }
        }
    } else {
        if (buttonPressed) {
            uint32_t holdTime = millis() - pressStartTime;
            
            if (shutdownPending) {
                // 长按后松手，执行关机，不执行任何短按操作
                shutdownPending = false;
                shutdownTriggered = true;
                Shutdown();
            } 
            // 只有非关机状态且短于关机时间才处理短按
            else if (!shutdownTriggered && holdTime < SHUTDOWN_TIME) {
                // 短按处理
                if (holdTime < SHORT_PRESS_TIME) {
                    displayMode_e currentMode = display.mode();
                    
                    if (currentMode == SCREENBLANK) {
                        display.putRequest(NEWMODE, PLAYER);
                        Serial.println("Power key: wake up");
                    } else if (currentMode == PLAYER) {
                        if (config.getMode() == PM_WEB) {
                            config.changeMode(PM_SDCARD);
                            Serial.println("Power key: switch to SD mode");
                        } else {
                            config.changeMode(PM_WEB);
                            Serial.println("Power key: switch to Web mode");
                        }
                        if (player.isRunning()) {
                            player.sendCommand({PR_PLAY, config.lastStation()});
                        }
                    }
                }
            }
            buttonPressed = false;
        }
    }
}