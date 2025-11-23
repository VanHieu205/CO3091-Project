#include "led_blinky.h"
#include "global.h"

static inline void ledDelayMs(uint32_t ms) {
  vTaskDelay(pdMS_TO_TICKS(ms));
}

void led_blinky(void *pvParameters) {
  pinMode(LED_GPIO, OUTPUT);
  pinMode(LED2_GPIO, OUTPUT);

  Serial.println("[LED] Task started");
  
  while (1) {

    // =========================
    //  Đọc giá trị cảm biến
    // =========================
    

    float TEMP  = glob_temperature;
    float HUMI  = glob_humidity;
    float LIGHT = glob_light;
    
    // Nếu button được nhấn → xử lý event
    if (xSemaphoreTake(xBinarySemaphore, 0)) {
        Serial.println("[LED] Button event received");
        // ví dụ đổi chế độ LED1
        led1_mode = LED_ON;
        led2_mode = LED_ON;
        led1_last_manual = millis();
        led2_last_manual = millis();
    }

    bool sensorError = (isnan(TEMP) || isnan(HUMI) || isnan(LIGHT));
    bool extremeAlert = (TEMP >= 38.0f || HUMI >= 95.0f);
    bool highAlert    = ((TEMP >= 33.0f && TEMP < 38.0f) || (HUMI >= 85.0f && HUMI < 95.0f));
    bool lowAlert     = (TEMP < 20.0f || HUMI < 45.0f);
    bool brightEnv    = (!isnan(LIGHT) && LIGHT > 3000.0f);
    bool darkEnv      = (!isnan(LIGHT) && LIGHT <= 3000.0f);

    unsigned long now = millis();

    // ==================================================
    //  1) TỰ ĐỘNG QUAY VỀ AUTO SAU 10 GIÂY KHÔNG BẤM
    // ==================================================
    if (led1_mode != LED_AUTO && (now - led1_last_manual >= LED_TIMEOUT_MS)) {
      led1_mode = LED_AUTO;
      Serial.println("[LED1] Timeout -> AUTO");
    }

    if (led2_mode != LED_AUTO && (now - led2_last_manual >= LED_TIMEOUT_MS)) {
      led2_mode = LED_AUTO;
      Serial.println("[LED2] Timeout -> AUTO");
    }

    // ==================================================
    //  2) LED1 xử lý độc lập
    // ==================================================
    switch (led1_mode)
    {
      case LED_ON:
        digitalWrite(LED_GPIO, HIGH);
        break;

      case LED_OFF:
        digitalWrite(LED_GPIO, LOW);
        break;

      case LED_AUTO:
      default:
        // xử lý theo sensor
        if (sensorError) {
          digitalWrite(LED_GPIO, HIGH); ledDelayMs(100);
          digitalWrite(LED_GPIO, LOW);  ledDelayMs(150);
        }
        else if (extremeAlert) {
          digitalWrite(LED_GPIO, LOW);
        }
        else if (highAlert) {
          digitalWrite(LED_GPIO, HIGH); ledDelayMs(150);
          digitalWrite(LED_GPIO, LOW);  ledDelayMs(150);
        }
        else if (lowAlert) {
          digitalWrite(LED_GPIO, HIGH); ledDelayMs(2000);
          digitalWrite(LED_GPIO, LOW);  ledDelayMs(2000);
        }
        else if (brightEnv) {
          digitalWrite(LED_GPIO, LOW);
        }
        else {
          digitalWrite(LED_GPIO, LOW);
        }
        break;
    }

    // ==================================================
    //  3) LED2 xử lý độc lập
    // ==================================================
    switch (led2_mode)
    {
      case LED_ON:
        digitalWrite(LED2_GPIO, HIGH);
        break;

      case LED_OFF:
        digitalWrite(LED2_GPIO, LOW);
        break;

      case LED_AUTO:
      default:
        if (sensorError) {
          digitalWrite(LED2_GPIO, HIGH); ledDelayMs(100);
          digitalWrite(LED2_GPIO, LOW);  ledDelayMs(150);
        }
        else if (extremeAlert) {
          digitalWrite(LED2_GPIO, HIGH);
        }
        else if (highAlert) {
          digitalWrite(LED2_GPIO, LOW); ledDelayMs(150);
          digitalWrite(LED2_GPIO, HIGH); ledDelayMs(150);
        }
        else if (lowAlert) {
          digitalWrite(LED2_GPIO, LOW);
        }
        else if (brightEnv) {
          digitalWrite(LED2_GPIO, LOW);
        }
        else {
          digitalWrite(LED2_GPIO, LOW);
        }
        break;
    }

    ledDelayMs(100);
  }
}