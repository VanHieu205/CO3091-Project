#include "global.h"

#include "led_blinky.h"
#include "neo_blinky.h"
#include "temp_humi_monitor.h"
#include "mainserver.h"
#include "tinyml.h"
#include "monitor_button.h"
#include "coreiot.h"
extern void data_logger_task(void *pvParameters);

void setup()
{
  Serial.begin(115200);
  //xTaskCreate(led_blinky, "Task LED Blink", 4096, NULL, 10, NULL);
  //xTaskCreate(neo_blinky, "Task NEO Blink" ,2048  ,NULL  ,10 , NULL);
  xTaskCreate(temp_humi_monitor, "Task TEMP HUMI Monitor", 4096, NULL, 10, NULL);
  //xTaskCreate(task_monitor_button, "Task Monitor Button", 2048, NULL, 3, NULL);
  //xTaskCreate(main_server_task, "Task Main Server", 8192, NULL, 1, NULL);
  //xTaskCreate(tiny_ml_task, "Tiny ML Task", 4096, NULL, 10, NULL);
  //xTaskCreate(evaluation_task, "Evaluation Task" ,4096 ,NULL  , 10, NULL);
  xTaskCreate(coreiot_task, "Core Iot Task", 4096, NULL, 2, NULL);
}
void loop()
{
}
