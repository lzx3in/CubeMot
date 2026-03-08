#include "drivers/led/led.h"
#include "drivers/led/led_msg.h"
#include "boards/led.h"
#include "FreeRTOS.h"
#include "task.h"

// ============================================================================
// Driver Private State
// ============================================================================

#define LED_DRIVER_MAX_LEDS 3

static struct {
    bool initialized;
    TaskHandle_t task_handle;
    msghub_publisher_t state_pub[LED_DRIVER_MAX_LEDS];
    msghub_subscriber_t cmd_sub;
    bool current_state[LED_DRIVER_MAX_LEDS];
} g_led_driver;

// ============================================================================
// Internal Functions
// ============================================================================

// Get system timestamp
static uint32_t get_timestamp_ms(void)
{
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

// Publish LED state (use corresponding LED's publisher instance)
static void publish_state(uint8_t led_id, bool state)
{
    if (led_id >= LED_DRIVER_MAX_LEDS) {
        return;
    }
    led_state_t msg = {.led_id = led_id, .state = state, .timestamp = get_timestamp_ms()};
    msghub_publish(g_led_driver.state_pub[led_id], &msg);
}

// ============================================================================
// LED Driver Task (Process Commands + Publish State)
// ============================================================================

static void vTaskLED_Driver(void *pvParameters)
{
    (void)pvParameters;

    for (;;) {
        led_cmd_t cmd;
        bool updated = false;

        // Check for new commands (non-blocking)
        msghub_subscriber_check(g_led_driver.cmd_sub, &updated);
        if (updated) {
            msghub_receive(g_led_driver.cmd_sub, &cmd);

            // Validate LED ID
            if (cmd.led_id >= 3) {
                continue;
            }

            // Execute hardware operation
            board_led_t handle = board_led_get_handle(cmd.led_id);
            if (board_led_is_valid(handle)) {
                board_led_set_state(handle, cmd.state);
                g_led_driver.current_state[cmd.led_id] = cmd.state;
                publish_state(cmd.led_id, cmd.state);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // 10ms poll interval
    }
}

// ============================================================================
// Public API
// ============================================================================

int led_driver_init(void)
{
    if (g_led_driver.initialized) {
        return -1; // Already initialized
    }

    // Create independent state publisher for each LED (use different instance)
    for (int i = 0; i < LED_DRIVER_MAX_LEDS; i++) {
        int instance = i;
        g_led_driver.state_pub[i] = msghub_create_publisher_multi(led_state_topic, &instance);
        if (g_led_driver.state_pub[i] == MSGHUB_PUBLISHER_INVALID) {
            // Cleanup already created publishers
            for (int j = 0; j < i; j++) {
                msghub_destroy_publisher(g_led_driver.state_pub[j]);
            }
            return -2;
        }
    }

    // Create command subscriber (instance 0 = primary instance)
    g_led_driver.cmd_sub = msghub_create_subscriber(led_command_topic, 0);
    if (g_led_driver.cmd_sub == MSGHUB_SUBSCRIBER_INVALID) {
        for (int i = 0; i < LED_DRIVER_MAX_LEDS; i++) {
            msghub_destroy_publisher(g_led_driver.state_pub[i]);
        }
        return -3;
    }

    // Initialize state cache
    for (int i = 0; i < LED_DRIVER_MAX_LEDS; i++) {
        g_led_driver.current_state[i] = false;
    }

    // Create driver task
    xTaskCreate(vTaskLED_Driver, "LED_Drv", 256, NULL, 2, &g_led_driver.task_handle);

    g_led_driver.initialized = true;
    return 0;
}

void led_set(uint8_t led_id, bool on)
{
    if (!g_led_driver.initialized) {
        return;
    }

    led_cmd_t cmd = {.led_id = led_id, .state = on};

    // Publish command to topic (create temporary publisher)
    msghub_publisher_t pub = msghub_create_publisher(led_command_topic);
    if (pub != MSGHUB_PUBLISHER_INVALID) {
        msghub_publish(pub, &cmd);
        msghub_destroy_publisher(pub);
    }
}

void led_toggle(uint8_t led_id)
{
    if (!g_led_driver.initialized || led_id >= 3) {
        return;
    }

    // Toggle cached state and publish
    bool new_state = !g_led_driver.current_state[led_id];
    led_set(led_id, new_state);
}

msghub_subscriber_t led_get_state_subscriber(uint8_t instance)
{
    return msghub_create_subscriber(led_state_topic, instance);
}
