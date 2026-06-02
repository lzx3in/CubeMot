#include "modules/blink/blink.h"
#include "drivers/led.h"
#include "topics/topics.h"
#include "msghub/msghub.h"
#include "common_device.h"
#include <zephyr/kernel.h>

static msghub_subscriber_t s_button_sub;
static uint8_t s_blink_freq_index = 0;
static const uint32_t s_blink_periods_ms[3] = {100, 500, 1000};

static void blink_thread_fn(void *arg1, void *arg2, void *arg3)
{
    (void)arg1;
    (void)arg2;
    (void)arg3;

    uint32_t count = s_blink_periods_ms[s_blink_freq_index] / 50;

    for (;;) {
        bool updated = false;
        msghub_err_t err = msghub_subscriber_check(s_button_sub, &updated);
        if (err == MSGHUB_OK && updated) {
            button_event_t msg;
            if (msghub_receive(s_button_sub, &msg) == MSGHUB_OK) {
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
            cubemot_led_toggle(CUBEMOT_DEVICE_LED_0);
        }
        k_msleep(50);
    }
}

K_THREAD_DEFINE(blink_thread, 512, blink_thread_fn, NULL, NULL, NULL, 2, 0, 0);

void blink_module_init(void)
{
    cubemot_led_init();
    s_button_sub = msghub_create_subscriber(MSGHUB_TOPIC(button_event), 0);
}