#include "coreiot.h"
#include "led_blinky.h"
#define BROKER       "app.coreiot.io"
#define MQTT_PORT    1883
#define ACCESS_TOKEN "IzfworlqD9zcA9X6U8zV"



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
    mqttClient.setCallback(callback);
    while (!mqttClient.connected())
    {
        Serial.println("[MQTT] Connecting to CoreIoT...");
        if (mqttClient.connect("ESP32_ClientID", ACCESS_TOKEN, NULL)) // Login bằng TOKEN
        {
            Serial.println("[MQTT] Connected to CoreIoT MQTT");
            mqttClient.subscribe("v1/devices/me/rpc/request/+");
            Serial.println("Subscribed to v1/devices/me/rpc/request/+");
        }
        else
        {
            Serial.printf("[MQTT] Failed (%d)\n", mqttClient.state());
            delay(3000);
        }
    }
}



void callback(char* topic, byte* payload, unsigned int length) {
  pinMode(LED_GPIO, OUTPUT);
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.println("] ");

  // Allocate a temporary buffer for the message
  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';
  Serial.print("Payload: ");
  Serial.println(message);

  // Parse JSON
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, message);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  const char* method = doc["method"];
  if (strcmp(method, "POWER") == 0) {
    // Check params type (could be boolean, int, or string according to your RPC)
    // Example: {"method": "setValueLED", "params": "ON"}
    const char* params = doc["params"];

    if (strcmp(params, "ON") == 0) {
      Serial.println("Device turned ON.");
      digitalWrite(LED_GPIO,HIGH);

    } else {   
      Serial.println("Device turned OFF.");
      
      digitalWrite(LED_GPIO,LOW);
    }
  } else {
    Serial.print("Unknown method: ");
    Serial.println(method);
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