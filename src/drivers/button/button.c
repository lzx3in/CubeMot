#include "drivers/button/button.h"
#include "drivers/button/button_msg.h"
#include "drivers/framework/button_hal_framework.h"
#include "cubemot_error.h"
#include "FreeRTOS.h"
#include "task.h"

#define BUTTON_DRIVER_MAX_BUTTONS 4

static struct {
    bool initialized;
    msghub_publisher_t event_pub[BUTTON_DRIVER_MAX_BUTTONS];
} g_button_driver;

static uint32_t get_timestamp_ms_isr(void)
{
    return xTaskGetTickCountFromISR() * portTICK_PERIOD_MS;
}

void button_driver_isr_callback(uint8_t button_id)
{
    if (!g_button_driver.initialized) {
        return;
    }

    // Read current state from HAL
    bool state = button_hal_read(button_id);
    uint32_t timestamp = get_timestamp_ms_isr();

    // Publish event directly from ISR
    button_event_t msg = {.button_id = button_id, .pressed = state, .timestamp = timestamp};
    msghub_publish_from_isr(g_button_driver.event_pub[button_id], &msg);
}

cubemot_err_t button_driver_init(void)
{
    if (g_button_driver.initialized) {
        return CUBEMOT_DRIVER_BUTTON_ERR; // Already initialized
    }

    for (int i = 0; i < BUTTON_DRIVER_MAX_BUTTONS; i++) {
        int instance = i;
        g_button_driver.event_pub[i] = msghub_create_publisher_multi(button_event_topic, &instance);
        if (g_button_driver.event_pub[i] == MSGHUB_PUBLISHER_INVALID) {
            // Cleanup previously created publishers
            for (int j = 0; j < i; j++) {
                msghub_destroy_publisher(g_button_driver.event_pub[j]);
                g_button_driver.event_pub[j] = MSGHUB_PUBLISHER_INVALID;
            }
            return CUBEMOT_DRIVER_BUTTON_ERR_NO_MEM;
        }
    }

    g_button_driver.initialized = true;
    return CUBEMOT_DRIVER_BUTTON_OK;
}

msghub_subscriber_t button_get_state_subscriber(uint8_t instance)
{
    return msghub_create_subscriber(button_event_topic, instance);
}

#ifdef BUILD_TESTING
// Test support: deinitialize driver
void button_driver_deinit(void)
{
    for (int i = 0; i < BUTTON_DRIVER_MAX_BUTTONS; i++) {
        if (g_button_driver.event_pub[i] != MSGHUB_PUBLISHER_INVALID) {
            msghub_destroy_publisher(g_button_driver.event_pub[i]);
            g_button_driver.event_pub[i] = MSGHUB_SUBSCRIBER_INVALID;
        }
    }

    g_button_driver.initialized = false;
}
#endif
