#include <Arduino.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "global.h"
#define BOOT_BUTTON_GPIO     GPIO_NUM_0   // ESP32-S3 default BOOT button


void task_monitor_button(void *pvParameters);
