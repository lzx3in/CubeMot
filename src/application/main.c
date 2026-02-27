#include "drivers/led/led.h"
#include "boards/led.h"
#include "boards/delay.h"
#include "boards/init.h"
#include "cubemot_config.h"
#include "FreeRTOS.h"
#include "task.h"

static void vTaskLED(void *pvParameters)
{
    (void)pvParameters;

#if CONFIG_BOARD_HAS_LED1
    led_t led1;
    if (led_init(&led1, BOARD_LED_1) == LED_OK) {
        for (;;) {
            led_toggle(&led1);
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
#else
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
#endif
}

int main(void)
{
    board_init();

    xTaskCreate(vTaskLED, "LED", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    // Should never reach here
    for (;;) {
    }
}
