#include "neo_blinky.h"


void neo_blinky(void *pvParameters){

    Adafruit_NeoPixel strip(LED_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);
    strip.begin();
    
    strip.clear();
    strip.show();
    bool first = 1;
    int f = 1;
    int status = 1;
    int r = 0, g = 0, b = 255;  
    while (1) {
       if (xSemaphoreTake(xBinarySemaphore, 0)) {
            r = map(button_press_count, 1, 6, 0, 255);
            g = map(button_press_count, 1, 6, 255, 0);
            b = 0;
            strip.setPixelColor(0, strip.Color(r, g, b));
            strip.show();
            vTaskDelay(1000);
        }
        switch (status) {
            case 1:  
                g += 15;
                if (g >= 255) {
                    g = 255;
                    status = 2;
                }
                break;
                
            case 2:  
                b -= 15;
                if (b <= 0) {
                    b = 0;
                    status = 3;
                }
                break;

            case 3:  
                r += 15;
                if (r >= 255) {
                    r = 255;
                    status = 4;
                }
                break;

            case 4:  
                g -= 15;
                if (g <= 0) {
                    g = 0;
                    status = 5;
                }
                break;

            case 5:  
                b += 15;
                if (b >= 255) {
                    b = 255;
                    status = 6;
                }
                break;

            case 6:  
                r -= 15;
                if (r <= 0) {
                    r = 0;
                    status = 1;
                    if(first) {
                        f = 5;
                        first = 0;
                    }
                    else {
                        f = 1;
                        first = 1;
                    }
                }
                break;
        }

        strip.setPixelColor(0, strip.Color(r, g, b));
        strip.show();
        vTaskDelay(100 * f);
        
        strip.setPixelColor(0, strip.Color(0, 0, 0));
        strip.show();
        vTaskDelay(100 * f);
        
    }
}
void neo_blinky_realtime(void *pvParameters){
    Adafruit_NeoPixel strip(LED_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);
    strip.begin();
    
    strip.clear();
    strip.show();


    Serial.println("[NEO] Task started - waiting for temperature and humidity data...");
  // Nếu ánh sáng môi trường > 70% → tắt LED (trời sáng)

  while (1) {
        float TEMP = glob_temperature;
        float HUMI = glob_humidity;
        float LIGHT = glob_light;
        if (xSemaphoreTake(xBinarySemaphore, portMAX_DELAY)) {
            strip.setPixelColor(0, strip.Color(100, 0, 0));
            strip.show();
            vTaskDelay(100);
        }
        // --- [0] Trời sáng → Tắt LED ---
        if (LIGHT > 3000) {
            strip.clear();
            strip.show();
            Serial.printf("[NEO] Bright rawLight=%.0f → NEO OFF\n", LIGHT);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        // --- [1] Lỗi sensor ---
        if (TEMP < -50.0f || TEMP > 150.0f || TEMP == -1.0f ||
            HUMI < 0.0f || HUMI > 100.0f || HUMI == -1.0f) {
            // 2 chớp nhanh + 1 chậm (màu đỏ)
            for (int i = 0; i < 2; i++) {
                strip.setPixelColor(0, strip.Color(255, 0, 0)); // đỏ
                strip.show();
                vTaskDelay(100 / portTICK_PERIOD_MS);
                strip.clear();
                strip.show();
                vTaskDelay(150 / portTICK_PERIOD_MS);
            }
            strip.setPixelColor(0, strip.Color(255, 0, 0));
            strip.show();
            vTaskDelay(400 / portTICK_PERIOD_MS);
            strip.clear();
            strip.show();
            vTaskDelay(600 / portTICK_PERIOD_MS);
            continue;
        }

        // --- [2] Cảnh báo cực cao (Neo tím) ---
        if (TEMP > 40.0f || HUMI > 90.0f) {
            strip.setPixelColor(0, strip.Color(255, 0, 255)); // tím
            strip.show();
            Serial.printf("[NEO] ALERT: TEMP=%.1f°C, HUMI=%.1f%% (Over threshold)\n", TEMP, HUMI);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        // --- [3] Nóng (đỏ) ---
        if (TEMP >= 35.0f || HUMI >= 75.0f) {
            for (int i = 0; i < 3; i++) {
                strip.setPixelColor(0, strip.Color(255, 0, 0));
                strip.show();
                vTaskDelay(120 / portTICK_PERIOD_MS);
                strip.clear();
                strip.show();
                vTaskDelay(120 / portTICK_PERIOD_MS);
            }
            vTaskDelay(800 / portTICK_PERIOD_MS);
        }

        // --- [4] Ấm (vàng) ---
        else if ((TEMP >= 28.0f && TEMP < 35.0f) || (HUMI >= 60.0f && HUMI < 75.0f)) {
            strip.setPixelColor(0, strip.Color(255, 255, 0)); // vàng
            strip.show();
            vTaskDelay(300 / portTICK_PERIOD_MS);
            strip.clear();
            strip.show();
            vTaskDelay(300 / portTICK_PERIOD_MS);
        }

        // --- [5] Mát (xanh lá) ---
        else if ((TEMP >= 20.0f && TEMP < 28.0f) || (HUMI >= 40.0f && HUMI < 60.0f)) {
            strip.setPixelColor(0, strip.Color(0, 255, 0)); // xanh lá
            strip.show();
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            strip.clear();
            strip.show();
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }

        // --- [6] Lạnh (xanh dương) ---
        else {
            strip.setPixelColor(0, strip.Color(0, 0, 255)); // xanh dương
            strip.show();
            vTaskDelay(2000 / portTICK_PERIOD_MS);
            strip.clear();
            strip.show();
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }

        // --- [7] Log định kỳ ---
        static uint32_t lastPrint = 0;
        if (millis() - lastPrint > 5000) {
            Serial.printf("[NEO] TEMP=%.1f°C | HUMI=%.1f%% | LIGHT=%.1f\n", TEMP, HUMI, LIGHT);
            lastPrint = millis();
        }
    }
}
