#include "app.h"
#include "FreeRTOS.h"
#include "task.h"

extern volatile app_state_t g_app_state;

static void app_main_task(void *pvParameters)
{
    (void)pvParameters;

    if (app_init() != 0) {
        g_app_state = APP_STATE_ERROR;
        for (;;) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    if (app_start() != 0) {
        g_app_state = APP_STATE_ERROR;
        for (;;) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void app_launch(void)
{
    xTaskCreate(app_main_task, "AppMain", 512, NULL, 2, NULL);

    vTaskStartScheduler();
}
