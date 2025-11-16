#ifndef __TEMP_HUMI_MONITOR__
#define __TEMP_HUMI_MONITOR__
#include <Arduino.h>
#include "LiquidCrystal_I2C.h"
#include "DHT20.h"
#include "global.h"
#include <Wire.h>

#define LDR_PIN 4 // GPIO pin connected to LDR
void temp_humi_monitor(void *pvParameters);
void printToLCD(const String &line1, const String &line2);

#endif