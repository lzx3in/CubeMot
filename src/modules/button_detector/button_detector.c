#include "modules/button_detector/button_detector.h"
#include "drivers/button/button.h"
#include "common_time.h"
#include "msghub/msghub.h"
#include "topics/topics.h"

static struct {
    bool initialized;
    msghub_publisher_t event_pub;
    driver_button_instance *instance;
} g_button_detector;

static void button_detector_isr_callback(void *ctx)
{
    (void)ctx;
    if (!g_button_detector.initialized) {
        return;
    }

    uint32_t timestamp = common_get_timestamp_ms_isr();

    button_event_t msg = {.button_id = CUBEMOT_DEVICE_BUTTON_0, .pressed = true, .timestamp = timestamp};
    msghub_publish_from_isr(g_button_detector.event_pub, &msg);
}

int button_detector_init(void)
{
    if (g_button_detector.initialized) {
        return -1;
    }

    driver_button_instance *inst = driver_button_get_instance(CUBEMOT_DEVICE_BUTTON_0);
    if (!inst) {
        return -2;
    }

    g_button_detector.instance = inst;

    if (inst->init) {
        inst->init(inst->ctx);
    }

    inst->irq_handler = button_detector_isr_callback;

    g_button_detector.event_pub = msghub_create_publisher(MSGHUB_TOPIC(button_event));
    if (g_button_detector.event_pub == MSGHUB_PUBLISHER_INVALID) {
        return -3;
    }

    if (inst->enable_irq) {
        inst->enable_irq(inst->ctx);
    }

    g_button_detector.initialized = true;
    return 0;
}

#ifdef BUILD_TESTING
void button_detector_deinit(void)
{
    if (g_button_detector.event_pub != MSGHUB_PUBLISHER_INVALID) {
        msghub_destroy_publisher(g_button_detector.event_pub);
        g_button_detector.event_pub = MSGHUB_PUBLISHER_INVALID;
    }
    g_button_detector.instance = NULL;
    g_button_detector.initialized = false;
}
#endif
