#include"monitor_button.h"

void task_monitor_button(void *pvParameters)
{
    pinMode(BOOT_BUTTON_GPIO, INPUT_PULLUP);
   
    const TickType_t debounceTicks = pdMS_TO_TICKS(50);
    const TickType_t measureWindow = pdMS_TO_TICKS(1000); // 1 giây
    bool counting = false;
    // Create binary semaphore
    xBinarySemaphore = xSemaphoreCreateCounting(3, 0);

    while (1) {
        // Check if button is pressed (active-low)
        if (gpio_get_level(BOOT_BUTTON_GPIO) == 0) {
            // Debounce delay
            vTaskDelay(pdMS_TO_TICKS(50));
            if (gpio_get_level(BOOT_BUTTON_GPIO) == 0) {
                // Give the semaphore to signal the LED task
                
                // Wait until button is released
                if (!counting) {
                    counting = true;
                    button_press_count = 0;

                    TickType_t start = xTaskGetTickCount();
                    TickType_t end = start + measureWindow;

                    while (xTaskGetTickCount() < end) {
                        if (gpio_get_level(BOOT_BUTTON_GPIO) == 0) {
                            // Đếm 1 lần nhấn
                            button_press_count++;
                            // Chờ nút thả ra
                            while (gpio_get_level(BOOT_BUTTON_GPIO) == 0) {
                                vTaskDelay(pdMS_TO_TICKS(10));
                            }
                        }
                        vTaskDelay(pdMS_TO_TICKS(10));
                    }

                    counting = false;
                    // Sau 1s đo, button_press_count có giá trị 1->20
                    if (button_press_count > 20) button_press_count = 20;

                    Serial.printf("[BUTTON] Press count = %d\n", button_press_count);
                }
                xSemaphoreGive(xBinarySemaphore);
                xSemaphoreGive(xBinarySemaphore);
                xSemaphoreGive(xBinarySemaphore);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
