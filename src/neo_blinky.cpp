#include "neo_blinky.h"
#define OVERRIDE_TIMEOUT 5000
void neo_blinky(void *pvParameters)
{

    Adafruit_NeoPixel strip(LED_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);
    strip.begin();

    strip.clear();
    strip.show();
    bool first = 1;
    int f = 1;
    int status = 1;
    int r = 0, g = 0, b = 255;
    while (1)
    {
        if (neoOverride) 
        {
            if (millis() - lastNeoUpdate < OVERRIDE_TIMEOUT) 
            {
                strip.setPixelColor(0, strip.Color(r_glob, g_glob, b_glob));
                strip.show();
                vTaskDelay(50);
                continue; 
            } 
            else 
            {
                neoOverride = false; 
            }
        }
        if (xSemaphoreTake(xBinarySemaphore, 0))
        {
            r = map(button_press_count, 1, 6, 0, 255);
            g = map(button_press_count, 1, 6, 255, 0);
            b = 0;
            strip.setPixelColor(0, strip.Color(r, g, b));
            strip.show();
            vTaskDelay(1000);
            continue;
        }
        switch (status)
        {
        case 1:
            g += 15;
            if (g >= 255)
            {
                g = 255;
                status = 2;
            }
            break;

        case 2:
            b -= 15;
            if (b <= 0)
            {
                b = 0;
                status = 3;
            }
            break;

        case 3:
            r += 15;
            if (r >= 255)
            {
                r = 255;
                status = 4;
            }
            break;

        case 4:
            g -= 15;
            if (g <= 0)
            {
                g = 0;
                status = 5;
            }
            break;

        case 5:
            b += 15;
            if (b >= 255)
            {
                b = 255;
                status = 6;
            }
            break;

        case 6:
            r -= 15;
            if (r <= 0)
            {
                r = 0;
                status = 1;
                if (first)
                {
                    f = 5;
                    first = 0;
                }
                else
                {
                    f = 1;
                    first = 1;
                }
            }
            break;
        }

        strip.setPixelColor(0, strip.Color(r, g, b));
        strip.show();
        vTaskDelay(100 * f);

        strip.setPixelColor(0, strip.Color(0, 0, 0));
        strip.show();
        vTaskDelay(100 * f);
    }
}
