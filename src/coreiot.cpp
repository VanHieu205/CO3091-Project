#include "coreiot.h"

#define BROKER       "app.coreiot.io"
#define MQTT_PORT    1883
#define ACCESS_TOKEN "5CLjpFU5u4nrSlAI41lE"


// ========================== MQTT Client ==========================
WiFiClient espClient;
PubSubClient mqttClient(espClient);


void connectWiFi()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());

    Serial.printf("[WiFi] Connecting to %s ...\n", wifi_ssid.c_str());
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.printf("\n[WiFi] Connected! IP = %s\n", WiFi.localIP().toString().c_str());
}


// ============= Connect MQTT =============
void connectMQTT()
{
    mqttClient.setServer(BROKER, MQTT_PORT);

    while (!mqttClient.connected())
    {
        Serial.println("[MQTT] Connecting to CoreIoT...");
        if (mqttClient.connect("ESP32_ClientID", ACCESS_TOKEN, NULL)) // Login bằng TOKEN
        {
            Serial.println("[MQTT] Connected to CoreIoT MQTT");
        }
        else
        {
            Serial.printf("[MQTT] Failed (%d)\n", mqttClient.state());
            delay(3000);
        }
    }
}


// ====================== Task gửi dữ liệu cảm biến ======================
void coreiot_task(void *pv)
{
    connectWiFi();
    connectMQTT();

    // Tọa độ HCMUT
    float _lat = 10.772175109674038;
    float _long = 106.65789107082472;

    while (1)
    {
        if (!mqttClient.connected()) connectMQTT();
        mqttClient.loop();

        // lấy dữ liệu cảm biến từ biến global
        float T = glob_temperature;
        float H = glob_humidity;
        float L = glob_light;

        String payload = "{";
        payload += "\"temperature\":" + String(T,1) + ",";
        payload += "\"humidity\":"    + String(H,1) + ",";
        payload += "\"light\":"       + String(L,0) + ",";
        payload += "\"lat\":"         + String(_lat,5) + ",";
        payload += "\"long\":"        + String(_long,5);
        payload += "}";

        mqttClient.publish("v1/devices/me/telemetry", payload.c_str());

        Serial.printf("[MQTT] Sent → %s\n", payload.c_str());

        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
}