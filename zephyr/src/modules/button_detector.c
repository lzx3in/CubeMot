#include "button_detector.h"
#include "button_driver.h"
#include "common_time.h"
#include "msghub/msghub.h"
#include "topics/topics.h"
#include "common_device.h"

static struct {
    bool initialized;
    msghub_publisher_t event_pub;
} g_btn;

static void button_isr_callback(void *ctx)
{
    (void)ctx;
    if (!g_btn.initialized)
        return;
    button_event_t msg = {
        .button_id = CUBEMOT_DEVICE_BUTTON_0, .pressed = true, .timestamp = common_get_timestamp_ms_isr()};
    msghub_publish_from_isr(g_btn.event_pub, &msg);
}

int button_detector_init(void)
{
    if (g_btn.initialized)
        return -1;
    g_btn.event_pub = msghub_create_publisher(MSGHUB_TOPIC(button_event));
    if (g_btn.event_pub == MSGHUB_PUBLISHER_INVALID)
        return -3;
    int ret = button_init(CUBEMOT_DEVICE_BUTTON_0, button_isr_callback);
    if (ret < 0) {
        msghub_destroy_publisher(g_btn.event_pub);
        g_btn.event_pub = MSGHUB_PUBLISHER_INVALID;
        return ret;
    }
    g_btn.initialized = true;
    return 0;
}
