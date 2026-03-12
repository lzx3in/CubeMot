#include "blink.h"
#include "drivers/led/led.h"
#include "drivers/button/button_topic.h"
#include "drivers/button/button_msg.h"
#include "FreeRTOS.h"
#include "task.h"

static msghub_subscriber_t s_button_sub;
static TaskHandle_t s_blink_task_handle;
static uint8_t s_blink_freq_index = 0;
static const uint32_t s_blink_periods_ms[3] = {100, 500, 1000};

static void vTaskBlink(void *pvParameters)
{
    (void)pvParameters;

    s_button_sub = msghub_create_subscriber(button_event_topic, 0);
    uint32_t count = 20;

    for (;;) {
        bool updated = false;
        msghub_err_t err = msghub_subscriber_check(s_button_sub, &updated);
        if (err == MSGHUB_OK && updated) {
            button_event_t msg;
            if (msghub_receive(s_button_sub, &msg) == MSGHUB_OK) {
                // Button pressed - switch frequency
                if (msg.pressed) {
                    s_blink_freq_index = (s_blink_freq_index + 1) % 3;
                    count = s_blink_periods_ms[s_blink_freq_index] / 50;
                }
            }
        }

        if (count > 0) {
            count--;
        } else {
            count = s_blink_periods_ms[s_blink_freq_index] / 50;
            led_toggle(0);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void blink_module_init(void)
{
    led_driver_init();
    xTaskCreate(vTaskBlink, "Blink", 512, NULL, 2, &s_blink_task_handle);
}
