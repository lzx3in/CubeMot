#include "blink.h"
#include "drivers/led/led.h"
#include "FreeRTOS.h"
#include "task.h"

static void vTaskBlink(void *pvParameters)
{
    (void)pvParameters;

    for (;;) {
        led_toggle(0);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void blink_module_init(void)
{
    led_driver_init();
    xTaskCreate(vTaskBlink, "Blink", 256, NULL, 1, NULL);
}
