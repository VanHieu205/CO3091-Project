#include "global.h"

float glob_temperature = 25 ;
float glob_humidity = 25;
float glob_light = 0;
float last_inference = 0.0f;

unsigned long led1_last_manual = 0;
unsigned long led2_last_manual = 0;
unsigned long bootMillis = 0;
unsigned long const LED_TIMEOUT_MS = 10000;
int led1_mode = LED_AUTO;
int led2_mode = LED_AUTO;
int r_glob = 0;
int g_glob = 0;
int b_glob = 0;
unsigned long lastNeoUpdate = 0;  
bool neoOverride = false;

String ssid = "ESP32-YOUR NETWORK HERE!!!";
String password = "12345678";
String wifi_ssid = "GoBi Lau";
String wifi_password = "gobicamon";
boolean isWifiConnected = false;
SemaphoreHandle_t xBinarySemaphoreInternet = xSemaphoreCreateBinary();
SemaphoreHandle_t xBinarySemaphore = xSemaphoreCreateBinary();
volatile uint8_t button_press_count = 0;
