#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

enum LedState {
    LED_AUTO = 0,   // chế độ cũ: chạy theo cảm biến
    LED_OFF,        // tắt cả 2 LED (ưu tiên)
    LED_ON,         // bật LED1 cố định (hoặc LED2 nếu bạn muốn)
    LED_BLINK       // nháy theo cảm biến (pattern của bạn)
};

// trạng thái do server điều khiển
extern int led1_mode;
extern int led2_mode;
extern unsigned long led1_last_manual;
extern unsigned long led2_last_manual;
extern const unsigned long LED_TIMEOUT_MS;

extern float glob_temperature;
extern float glob_humidity;
extern float glob_light;
extern float last_inference;
extern String ssid;
extern String password;
extern String wifi_ssid;
extern String wifi_password;
extern boolean isWifiConnected;
extern SemaphoreHandle_t xBinarySemaphoreInternet;

extern SemaphoreHandle_t xBinarySemaphore;
extern volatile uint8_t button_press_count;


#endif
