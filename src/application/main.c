#include "drivers/led/led.h"
#include "drivers/led/led_msg.h"
#include "boards/init.h"
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

int main(void)
{
    board_init();

    led_driver_init();

    xTaskCreate(vTaskBlink, "Blink", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    // Should never reach here
    for (;;) {
    }
}
