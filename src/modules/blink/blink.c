#include "modules/blink/blink.h"
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

    msghub_publisher_t led_cmd_pub = msghub_create_publisher(MSGHUB_TOPIC(led_command));
    led_cmd_t led_cmd = {.led_id = CUBEMOT_DEVICE_LED_0, .state = false};
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
            led_cmd.state = !led_cmd.state;
            msghub_publish(led_cmd_pub, &led_cmd);
        }
        k_msleep(50);
    }
}

K_THREAD_DEFINE(blink_thread, 512, blink_thread_fn, NULL, NULL, NULL, 2, 0, 0);

void blink_module_init(void)
{
    s_button_sub = msghub_create_subscriber(MSGHUB_TOPIC(button_event), 0);
}
