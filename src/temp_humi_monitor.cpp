#include "temp_humi_monitor.h"
#include "global.h"
#include <WiFi.h>
#include <Wire.h>

LiquidCrystal_I2C lcd(33, 16, 2);   // 33 thập phân = 0x21
DHT20 dht20;
static bool lcd_hold = false;
byte barChar[5][8] = {
  // 0 = trống
  {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
  // 1..3 = các mức phần lẻ của thanh
  {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x1F}, 
  {0x0,0x0,0x0,0x0,0x0,0x0,0x1F,0x1F},
  {0x0,0x0,0x0,0x0,0x1F,0x1F,0x1F,0x1F},
  // 4 = khối đầy
  {0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F}
};

void lcdInitCustomChars() {
  for (int i = 0; i < 5; ++i) lcd.createChar(i, barChar[i]);
}

// In đúng 16 ký tự trên một dòng
void lcdPrintLine(LiquidCrystal_I2C &lcdRef, uint8_t row, const char *text) {
  char buf[17];
  strncpy(buf, text, 16);
  buf[16] = '\0';
  int len = strlen(buf);
  for (int i = len; i < 16; ++i) buf[i] = ' ';
  buf[16] = '\0';
  lcdRef.setCursor(0, row);
  lcdRef.print(buf);
}

// Vẽ thanh bar, tối đa maxChars ký tự
void drawBar(LiquidCrystal_I2C &lcdRef, uint8_t row, uint8_t col, float value, float maxValue, uint8_t maxChars=12) {
  // Nếu giá trị không hợp lệ thì xóa vùng hiển thị
  if (!isfinite(value) || !isfinite(maxValue) || maxValue <= 0.0f) {
    lcdRef.setCursor(col, row);
    for (uint8_t i = 0; i < maxChars; ++i) lcdRef.print(' ');
    return;
  }

  float ratio = constrain(value / maxValue, 0.0f, 1.0f);
  float scaled = ratio * maxChars;
  int full = (int)floor(scaled);
  float frac = scaled - full;
  int partialIdx = (int)round(frac * 4.0f); // 0..4

  //giới hạn giá trị
  if (full < 0) full = 0;
  if (full > maxChars) full = maxChars;
  if (partialIdx < 0) partialIdx = 0;
  if (partialIdx > 4) partialIdx = 4;

  lcdRef.setCursor(col, row);

  for (int i = 0; i < full && i < maxChars; ++i) lcdRef.write(byte(4));

  if (full < maxChars) {
    if (partialIdx == 0) {
      // không có phần lẻ, in khoảng trắng
      lcdRef.print(' ');
    } else {
      // partialIdx map tới custom char 1..4
      // partialIdx != 4 khi full == maxChars
      lcdRef.write(byte(partialIdx));
    }
    // đệm phần còn lại bằng khoảng trắng
    for (int j = full + 1; j < maxChars; ++j) lcdRef.print(' ');
  }
}

// Thời gian mỗi trang
const unsigned long PAGE_MS = 5000; // 5s/trang
const uint8_t PAGE_COUNT = 4;
static uint8_t currentPage = 0;
static unsigned long lastPageMs = 0;

//updateLCDPages dùng biến global
void updateLCDPages() {
        
  unsigned long now = millis();
  // Nếu không đang giữ màn hình và đã tới thời gian chuyển trang thì tăng trang
  if (!lcd_hold && (now - lastPageMs >= PAGE_MS)) {
        currentPage = (currentPage + 1) % PAGE_COUNT;
        lastPageMs = now;
  }

  // Đọc các biến global
  float temperature = glob_temperature;
  float humidity = glob_humidity;
  float light = glob_light;
  float ai = last_inference;
  bool wifiOk = isWifiConnected;

  bool sensorError = (temperature == -1.0f || humidity == -1.0f);

  // Cập nhật logic
  String stateLabel;
        if (sensorError) {
          stateLabel = "SENSOR ERROR";
        } 
        else if (temperature >= 38 && humidity >= 95) {
          stateLabel = "EXTREME HOT&WET!";
        } 
        else if ((temperature >= 33 && temperature < 38) || 
                (humidity >= 85 && humidity < 95)) {
          stateLabel = "VERY HOT/HUMID!";
        } 
        else if (temperature < 20 && humidity < 45) {
          stateLabel = "COLD && DRY!";
        } 
        else if (temperature >= 38 && humidity <45) {
          stateLabel = "EXTREME HOT&DRY!";
        } 
        else if (temperature < 20 && humidity >= 95) {
          stateLabel = "COLD & WET!";
        }
        else if (temperature < 20) {
          stateLabel = "COLD!";
        } 
        else if (temperature >=33) {
          stateLabel = "VERY HOT!";
        } 
        else if (temperature >= 38) {
          stateLabel = "EXTREME HOT!";
        } 
        else if (humidity >= 95) {
          stateLabel = "WET!";
        } 
        else if (humidity >= 85 && humidity < 95) {
          stateLabel = "HUMID!";
        } 
        else if (humidity < 45) {
          stateLabel = "DRY!";
        } 
        else {
          stateLabel = "NORMAL";
        }
        
  // chuẩn bị buffer in ra LCD
  char line[17];

  switch (currentPage) {
    case 0: {
      // Dòng 1: T:xx.x H:yy. Build rồi pad đủ 16 ký tự.
      if (sensorError) {
        snprintf(line, sizeof(line), "T:ERR H:ERR");
      } else {
        // dùng định dạng cố định để hiển thị ổn định
        snprintf(line, sizeof(line), "T:%4.1f H:%3.0f", temperature, humidity);
      }
      lcdPrintLine(lcd, 0, line);

      // Dòng 2: stateLabel
      strncpy(line, stateLabel.c_str(), 16);
      line[16] = '\0';
      lcdPrintLine(lcd, 1, line);
      break;
    }

    case 1: { // Trang ánh sáng
      snprintf(line, sizeof(line), "L:%4d", (int)light);
      lcdPrintLine(lcd, 0, line);
      drawBar(lcd, 1, 0, light, 5000.0f, 12);
      lcd.setCursor(12, 1);
      lcd.print("    ");
      break;
    }

    case 2: { // Trang AI
      if (!isfinite(ai)) ai = 0.0f;
      snprintf(line, sizeof(line), "AI:%.3f", ai);
      lcdPrintLine(lcd, 0, line);
      if (ai > 0.5f) {
        lcdPrintLine(lcd, 1, "AI:ABNORMAL     ");
      } else {
        lcdPrintLine(lcd, 1, "AI:NORMAL       ");
      }
      break;
    }

    case 3: { // Trang WiFi
      if (wifiOk && WiFi.status() == WL_CONNECTED) {
        String ip = WiFi.localIP().toString();
        String l1 = "STA: " + ip;
        strncpy(line, l1.c_str(), 16); line[16] = '\0';
        lcdPrintLine(lcd, 0, line);

        char rbuf[17];
        long rssi = WiFi.RSSI();
        snprintf(rbuf, sizeof(rbuf), "RSSI:%ld", rssi);
        lcdPrintLine(lcd, 1, rbuf);
      } else {
        lcdPrintLine(lcd, 0, "WiFi:DISCONNECT ");
        lcdPrintLine(lcd, 1, "Check Network   ");
      }
      break;
    }
  }
}

// task chính 
void temp_humi_monitor(void *pvParameters) {

    // khởi tạo I2C và cảm biến
    Wire.begin(11, 12);     // chân I2C của YOLO UNO
    Wire.setClock(100000);  // chạy bus ở 100kHz
    Serial.begin(115200);
    dht20.begin();

    // Khởi tạo LCD
    lcd.begin();
    lcdInitCustomChars();   // tạo các ký tự tùy chỉnh 1 lần
    lcd.backlight();
    lcd.clear();
    lcdPrintLine(lcd, 0, "Starting    ");
    lcdPrintLine(lcd, 1, "Initializing ");
    vTaskDelay(pdMS_TO_TICKS(500));
    
    //bộ đếm thời gian cho cập nhật cảm biến, serial, và autopage LCD
    unsigned long lastUpdateMs = 0; // thay cho lastSerialMs và lastLCDUpdate
    const unsigned long UPDATE_INTERVAL = 5000UL; // đọc cảm biến, in, autopage mỗi 5 giây

    while (1) {
        
        //Xử lý sự kiện nút nhấn
        // giữ timeout 0 
        if (xBinarySemaphore != NULL) {
             // non-blocking: chỉ thao tác nếu có event
             if (xSemaphoreTake(xBinarySemaphore, 0)) {
                 
                 Serial.printf("[LCD] Button event consumed in temp_humi_monitor: %u\n", button_press_count);

                 // Cập nhật trang hoặc bật/tắt chế độ giữ dựa trên số lần nhấn
                 if (button_press_count == 1) {
                     // chuyển sang trang tiếp theo
                     currentPage = (currentPage + 1) % PAGE_COUNT;
                     lastPageMs = millis(); // reset bộ đếm autopage
                 } else if (button_press_count == 2) {
                     // quay về trang trước
                     if (currentPage == 0) currentPage = PAGE_COUNT - 1;
                     else currentPage--;
                     lastPageMs = millis(); // reset bộ đếm autopage
                 } else if (button_press_count >= 3) {
                     // nhấn >=3 lần để bật/tắt chế độ giữ trang
                     lcd_hold = !lcd_hold;
                     Serial.printf("[LCD] Hold toggled -> %s\n", lcd_hold ? "ON" : "OFF");
                     lastPageMs = millis(); // reset bộ đếm autopage
                 }
                 // Sau khi có sự kiện nút, vẽ lại LCD
                 updateLCDPages();
             }
         }

        //2. Cập nhật dữ liệu cảm biến, global, và in serial (mỗi 5s)
        unsigned long now = millis();
        if (now - lastUpdateMs >= UPDATE_INTERVAL) {
            lastUpdateMs = now;
            
            //đọc cảm biến 
            dht20.read();
            float temperature = dht20.getTemperature();
            float humidity    = dht20.getHumidity();
            float light = analogRead(LDR_PIN); // ADC 0..4095

            bool sensorError = false;
            if (temperature == -1.0f || humidity == -1.0f) { // so sánhvới -1.0f
                 Serial.println("Failed to read from DHT20!");
                 sensorError = true;
            }

            if (sensorError) {
                temperature = -1.0f;
                humidity    = -1.0f;
            }

            // Cập nhật biến toàn cục
            glob_temperature = temperature;
            glob_humidity    = humidity;
            glob_light = light;

            //Xác định state (dùng cùng logic với updateLCDPages())
            String stateLabel;
            if (sensorError) {
                stateLabel = "SENSOR ERROR";
            } 
            else if (temperature >= 38 && humidity >= 95) {
                stateLabel = "EXTREME HOT&WET!";
            } else if ((temperature >= 33 && temperature < 38) || (humidity >= 85 && humidity < 95)) {
                stateLabel = "VERY HOT/HUMID!";
            } else if (temperature < 20 && humidity < 45) {
                stateLabel = "COLD && DRY!";
            } else if (temperature >= 38 && humidity <45) {
                stateLabel = "EXTREME HOT&DRY!";
            } else if (temperature < 20 && humidity >= 95) {
                stateLabel = "COLD & WET!";
            } else if (temperature < 20) {
                stateLabel = "COLD!";
            } else if (temperature >=33) {
                stateLabel = "VERY HOT!";
            } else if (temperature >= 38) {
                stateLabel = "EXTREME HOT!";
            } else if (humidity >= 95) {
                stateLabel = "WET!";
            } else if (humidity >= 85 && humidity < 95) {
                stateLabel = "HUMID!";
            } else if (humidity < 45) {
                stateLabel = "DRY!";
            } else {
                stateLabel = "NORMAL";
            }
            

            // --- In ra Serial (mỗi 5s) ---
            Serial.printf("T:%.1f, H:%.0f%%, Light:%d, State:%s\n", 
                          temperature, humidity, (int)light, stateLabel.c_str());
        }
        
        // --- 3. Cập nhật hiển thị LCD ---
        // LCD cập nhật thường xuyên để phản hồi nút nhấn nhanh,
        // còn logic tự chuyển trang được xử lý trong updateLCDPages() dựa vào lastPageMs.
        updateLCDPages();

        vTaskDelay(pdMS_TO_TICKS(100)); // nhường CPU 100ms
    }
}
