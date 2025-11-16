#include "temp_humi_monitor.h"

DHT20 dht20;
LiquidCrystal_I2C lcd(0x27, 16, 2);   // Nếu không hiện → đổi thành 0x3F

void printToLCD(const String &line1, const String &line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

void temp_humi_monitor(void *pvParameters) {
    Serial.begin(115200);

    // I2C mặc định theo board (nếu dùng Wokwi/ESP32-S3: thay Wire.begin(41,42))
    Wire.begin();   // hoặc Wire.begin(SDA, SCL) nếu board yêu cầu

    // LCD init (thư viện của bạn dùng lcd.begin() không tham số)
    lcd.begin(16,2);
    lcd.backlight();
    lcd.clear();
    lcd.print("Starting...");

    // DHT20 init
    dht20.begin();
    vTaskDelay(pdMS_TO_TICKS(500));

    while (1) {

        // --- SENSOR READING ---
        dht20.read();
        float temperature = dht20.getTemperature();
        float humidity = dht20.getHumidity();

        bool sensorError = false;
        if (isnan(temperature) || isnan(humidity)) sensorError = true;

        // --- UPDATE GLOBALS ---
        if (!sensorError) {
            glob_temperature = temperature;
            glob_humidity = humidity;
        } else {
            glob_temperature = -1;
            glob_humidity = -1;
        }

        // --- DETERMINE STATE ---
        String stateLabel;

        if (sensorError) {
            stateLabel = "SENSOR ERROR";
        }
        else if (temperature > 40 || humidity > 90) {
            stateLabel = "CRITICAL!";
        }
        else if ((temperature >= 35 && temperature < 40) ||
                 (humidity >= 80 && humidity < 90)) {
            stateLabel = "WARNING";
        }
        else if (temperature < 20 || humidity < 40) {
            stateLabel = "COLD/DRY";
        }
        else {
            stateLabel = "NORMAL";
        }

        // --- PRINT UART ---
        if (sensorError) {
            Serial.println("[SENSOR] Temp:ERR  Humi:ERR");
        } else {
            Serial.printf("[SENSOR] Temp:%.1fC  Humi:%.1f%%  => %s\n",
                          temperature, humidity, stateLabel.c_str());
        }

        // --- LCD OUTPUT ---
        char line1[17];
        char line2[17];

        if (sensorError) {
            snprintf(line1, sizeof(line1), "Temp: ERR Humi:ERR");
            snprintf(line2, sizeof(line2), "Sensor ERROR");
        } else {
            snprintf(line1, sizeof(line1), "T:%.1fC H:%.1f%%", temperature, humidity);
            snprintf(line2, sizeof(line2), "%-16s", stateLabel.c_str());
        }

        printToLCD(String(line1), String(line2));

        // --- BACKLIGHT FLASH FOR WARNING/CRITICAL ---
        if (!sensorError && (stateLabel == "WARNING" || stateLabel == "CRITICAL!")) {
            lcd.noBacklight();
            vTaskDelay(pdMS_TO_TICKS(150));
            lcd.backlight();
        }

        // Wait 5s
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
