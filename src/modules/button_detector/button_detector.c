#include "modules/button_detector/button_detector.h"
#include <zephyr/input/input.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>
#include "common_time.h"
#include "msghub/msghub.h"
#include "topics/topics.h"
#include "common_device.h"

/* Board DTS: gpio_keys { compatible = "gpio-keys"; ... } */
static const struct device *const keys_dev = DEVICE_DT_GET(DT_NODELABEL(gpio_keys));

static struct {
    bool initialized;
    msghub_publisher_t event_pub;
} g_btn;

static void input_callback(struct input_event *evt, void *user_data)
{
    (void)user_data;

    if (!g_btn.initialized) {
        return;
    }

    if (evt->type != INPUT_EV_KEY || evt->code != INPUT_KEY_0) {
        return;
    }

    /* Only report press events (value == 1) */
    if (evt->value != 1) {
        return;
    }

    uint32_t timestamp = common_get_timestamp_ms();

    button_event_t msg = {.button_id = CUBEMOT_DEVICE_BUTTON_0, .pressed = true, .timestamp = timestamp};
    msghub_publish(g_btn.event_pub, &msg);
}

INPUT_CALLBACK_DEFINE(keys_dev, input_callback, NULL);

int button_detector_init(void)
{
    if (g_btn.initialized) {
        return -1;
    }

    if (!device_is_ready(keys_dev)) {
        return -ENODEV;
    }

    g_btn.event_pub = msghub_create_publisher(MSGHUB_TOPIC(button_event));
    if (g_btn.event_pub == MSGHUB_PUBLISHER_INVALID) {
        return -3;
    }

    g_btn.initialized = true;
    return 0;
}