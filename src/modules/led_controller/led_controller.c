#include "modules/led_controller/led_controller.h"
#include <zephyr/drivers/led.h>
#include "topics/topics.h"
#include "common_device.h"
#include "common_time.h"
#include "msghub/msghub.h"
#include <zephyr/kernel.h>

/* Board DTS: leds { compatible = "gpio-leds"; ... } */
static const struct device *const led_dev = DEVICE_DT_GET(DT_NODELABEL(leds));

K_SEM_DEFINE(g_led_cmd_sem, 0, 1);

static struct {
    msghub_publisher_t state_pub[CUBEMOT_DEVICE_LED_COUNT];
    msghub_subscriber_t cmd_sub;
    bool current_state[CUBEMOT_DEVICE_LED_COUNT];
    volatile bool cmd_pending;
    led_cmd_t pending_cmd;
} g_led;

static void publish_led_state(cubemot_device_led led_id, bool state)
{
    if (!cubemot_device_led_is_valid(led_id)) {
        return;
    }
    led_state_t msg = {.led_id = (uint8_t)led_id, .state = state, .timestamp = common_get_timestamp_ms()};
    msghub_publish(g_led.state_pub[led_id], &msg);
}

static void on_led_command(msghub_subscriber_t sub, void *context)
{
    (void)context;
    msghub_receive(sub, &g_led.pending_cmd);
    g_led.cmd_pending = true;
    k_sem_give(&g_led_cmd_sem);
}

static void led_ctrl_thread_fn(void *arg1, void *arg2, void *arg3)
{
    (void)arg1;
    (void)arg2;
    (void)arg3;

    for (;;) {
        k_sem_take(&g_led_cmd_sem, K_FOREVER);

        if (g_led.cmd_pending) {
            g_led.cmd_pending = false;
            const led_cmd_t cmd = g_led.pending_cmd;

            if (cubemot_device_led_is_valid((cubemot_device_led)cmd.led_id)) {
                if (cmd.state) {
                    led_on(led_dev, cmd.led_id);
                } else {
                    led_off(led_dev, cmd.led_id);
                }
                g_led.current_state[cmd.led_id] = cmd.state;
                publish_led_state((cubemot_device_led)cmd.led_id, cmd.state);
            }
        }
    }
}

K_THREAD_DEFINE(led_ctrl_thread, 256, led_ctrl_thread_fn, NULL, NULL, NULL, 2, 0, 0);

int led_controller_init(void)
{
    for (int i = 0; i < CUBEMOT_DEVICE_LED_COUNT; i++) {
        int instance = i;
        g_led.state_pub[i] = msghub_create_publisher_multi(MSGHUB_TOPIC(led_state), &instance);
        if (g_led.state_pub[i] == MSGHUB_PUBLISHER_INVALID) {
            for (int j = 0; j < i; j++) {
                msghub_destroy_publisher(g_led.state_pub[j]);
            }
            return -1;
        }
    }

    g_led.cmd_sub = msghub_create_subscriber(MSGHUB_TOPIC(led_command), 0);
    if (g_led.cmd_sub == MSGHUB_SUBSCRIBER_INVALID) {
        for (int i = 0; i < CUBEMOT_DEVICE_LED_COUNT; i++) {
            msghub_destroy_publisher(g_led.state_pub[i]);
        }
        return -2;
    }

    msghub_subscriber_set_callback(g_led.cmd_sub, on_led_command, NULL);

    for (int i = 0; i < CUBEMOT_DEVICE_LED_COUNT; i++) {
        g_led.current_state[i] = false;
    }
    g_led.cmd_pending = false;

    return 0;
}