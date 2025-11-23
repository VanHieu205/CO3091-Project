#include "temp_humi_monitor.h"
#include "global.h"
#include <WiFi.h>
#include <Wire.h>

LiquidCrystal_I2C lcd(33, 16, 2);   // 33 decimal = 0x21
DHT20 dht20;
static bool lcd_hold = false;
// ------------------- custom chars for bar (create once) -------------------
byte barChar[5][8] = {
  // 0 = empty (we will avoid writing byte(0) as a glyph to prevent garbage)
  {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
  // 1..3 = partial levels
  {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x1F}, 
  {0x0,0x0,0x0,0x0,0x0,0x0,0x1F,0x1F},
  {0x0,0x0,0x0,0x0,0x1F,0x1F,0x1F,0x1F},
  // 4 = full block
  {0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F}
};

void lcdInitCustomChars() {
  // createChar indices 0..4, but we'll only write bytes 1..4 as blocks/partials
  for (int i = 0; i < 5; ++i) lcd.createChar(i, barChar[i]);
}

// ------------------- helper: print exactly 16 chars on a row -------------------
void lcdPrintLine(LiquidCrystal_I2C &lcdRef, uint8_t row, const char *text) {
  char buf[17];
  // copy up to 16 chars
  strncpy(buf, text, 16);
  buf[16] = '\0';
  int len = strlen(buf);
  // pad with spaces if shorter
  for (int i = len; i < 16; ++i) buf[i] = ' ';
  buf[16] = '\0';
  lcdRef.setCursor(0, row);
  lcdRef.print(buf);
}

// ------------------- improved drawBar: writes exactly maxChars characters -------------------
void drawBar(LiquidCrystal_I2C &lcdRef, uint8_t row, uint8_t col, float value, float maxValue, uint8_t maxChars=12) {
  // If invalid, clear the region
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

  // safety
  if (full < 0) full = 0;
  if (full > maxChars) full = maxChars;
  if (partialIdx < 0) partialIdx = 0;
  if (partialIdx > 4) partialIdx = 4;

  lcdRef.setCursor(col, row);

  // Write full blocks (we use custom char index 4 for full)
  for (int i = 0; i < full && i < maxChars; ++i) lcdRef.write(byte(4));

  // If space remains, write one partial or space, then pad with spaces to reach maxChars
  if (full < maxChars) {
    if (partialIdx == 0) {
      // no partial => write space
      lcdRef.print(' ');
    } else {
      // partialIdx maps to custom char 1..4 (we created them as 1..4)
      // but ensure partialIdx!=4 simultaneously with full==maxChars handled above
      lcdRef.write(byte(partialIdx));
    }
    // pad remainder
    for (int j = full + 1; j < maxChars; ++j) lcdRef.print(' ');
  }
}

// PAGE timing: match vTaskDelay(5000) in task
const unsigned long PAGE_MS = 5000; // 5s/page
const uint8_t PAGE_COUNT = 4;
static uint8_t currentPage = 0;
static unsigned long lastPageMs = 0;

// ------------------- updateLCDPages uses globals -------------------
void updateLCDPages() {
          // --- consume button event (non-blocking) ---
        if (xBinarySemaphore != NULL) {
          // non-blocking: chỉ xử lý nếu có event (giống neo_blinky)
          if (xSemaphoreTake(xBinarySemaphore, 0)) {
            // reset stored count (optional; monitor_button may overwrite next time)
            button_press_count = 0;
            Serial.printf("[LCD] Button event consumed in temp_humi_monitor: %u\n", button_press_count);

            // xử lý button_press_count:
            if (button_press_count == 1) {
              // next page
              currentPage = (currentPage + 1) % PAGE_COUNT;
              lastPageMs = millis();
            } else if (button_press_count == 2) {
              // prev page
              if (currentPage == 0) currentPage = PAGE_COUNT - 1;
              else currentPage--;
              lastPageMs = millis();
            } else if (button_press_count >= 3) {
              // toggle hold
              lcd_hold = !lcd_hold;
              Serial.printf("[LCD] Hold toggled -> %s\n", lcd_hold ? "ON" : "OFF");
              lastPageMs = millis();
            }
          }
        }
        
  unsigned long now = millis();
  if (now - lastPageMs >= PAGE_MS) {
    currentPage = (currentPage + 1) % PAGE_COUNT;
    lastPageMs = now;
  }

  // read globals once (atomic-ish)
  float temperature = glob_temperature;
  float humidity = glob_humidity;
  float light = glob_light;
  float ai = last_inference;
  bool wifiOk = isWifiConnected;

  bool sensorError = (temperature == -1.0f || humidity == -1.0f);

  // --- PHẦN SỬA ĐỔI: Cập nhật logic giống hệt trong main loop ---
  String stateLabel;
  if (sensorError) {
    stateLabel = "SENSOR ERROR";
  } 
  else if (temperature >= 38 || humidity >= 95) {
    stateLabel = "EXTREME HOT/WET!";
  } 
  else if ((temperature >= 33 && temperature < 38) || 
           (humidity >= 85 && humidity < 95)) {
    stateLabel = "VERY HOT/HUMID!";
  } 
  else if (temperature < 20 || humidity < 45) {
    stateLabel = "COLD/DRY!";
  } 
  else {
    stateLabel = "NORMAL";
  }
  // --------------------------------------------------------------

  // prepare buffers
  char line[17];

  switch (currentPage) {
    case 0: {
      // Line 1: T:xx.x H:yy  (no units). Build then pad to 16 exactly.
      if (sensorError) {
        snprintf(line, sizeof(line), "T:ERR H:ERR");
      } else {
        // Use fixed width formatting for stability
        snprintf(line, sizeof(line), "T:%4.1f H:%3.0f", temperature, humidity);
      }
      lcdPrintLine(lcd, 0, line);

      // Line 2: stateLabel (truncate/pad)
      strncpy(line, stateLabel.c_str(), 16);
      line[16] = '\0';
      lcdPrintLine(lcd, 1, line);
      break;
    }

    case 1: { // Light Page (Giữ nguyên)
      snprintf(line, sizeof(line), "L:%4d", (int)light);
      lcdPrintLine(lcd, 0, line);
      drawBar(lcd, 1, 0, light, 5000.0f, 12);
      lcd.setCursor(12, 1);
      lcd.print("    ");
      break;
    }

    case 2: { // AI Page (Giữ nguyên)
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

    case 3: { // WiFi Page (Giữ nguyên)
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

// ------------------- main task -------------------
void temp_humi_monitor(void *pvParameters) {

    // initialize I2C and sensor
    Wire.begin(11, 12);     // YOLO UNO I2C pins (verify pins for your board)
    Wire.setClock(100000);  // stable 100kHz
    Serial.begin(115200);
    dht20.begin();

    // LCD init
    lcd.begin();
    lcdInitCustomChars();   // create custom chars once
    lcd.backlight();
    lcd.clear();
    lcdPrintLine(lcd, 0, "Starting...      ");
    lcdPrintLine(lcd, 1, "Initializing...  ");
    vTaskDelay(pdMS_TO_TICKS(500));

    while (1) {

        // Read DHT20
        dht20.read();
        float temperature = dht20.getTemperature();
        float humidity    = dht20.getHumidity();
        float light = analogRead(LDR_PIN); // raw ADC 0..4095

        bool sensorError = false;
        if (temperature == -1 || humidity == -1) {
            Serial.println("Failed to read from DHT20!");
            sensorError = true;
        }

        if (sensorError) {
            temperature = -1;
            humidity    = -1;
        }

        // Update global variables (keeps compatibility with other tasks)
        glob_temperature = temperature;
        glob_humidity    = humidity;
        glob_light = light;

        // --- Determine state (UNCHANGED logic you provided) ---
        String stateLabel;
        if (sensorError) {
            stateLabel = "SENSOR ERROR";
        }
        else if (temperature >= 38 || humidity >= 95) {
            stateLabel = "EXTREME HOT/WET!";
        }
        else if ((temperature >= 33 && temperature < 38) ||
                 (humidity >= 85 && humidity < 95)) {
            stateLabel = "VERY HOT/HUMID!";
        }
        else if (temperature < 24 || humidity < 45) {
            stateLabel = "COLD/DRY!";
        }
        else {
            stateLabel = "NORMAL";
        }



        // --- Print to Serial (unchanged) ---
        Serial.print("Humidity: ");
        Serial.print(humidity);
        Serial.print("%  Temperature: ");
        Serial.print(temperature);
        Serial.print("  Light: ");
        Serial.print((int)light);
        Serial.print("   State=");
        Serial.println(stateLabel);

        // --- Print to LCD: use the page-update helper ---
        updateLCDPages();

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
