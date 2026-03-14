#include "topics/topics.h"
#include "drivers/led/led.h"
#include "common_device.h"
#include "FreeRTOS.h"
#include "task.h"
#include "common_time.h"

static struct {
    msghub_publisher_t state_pub[CUBEMOT_DEVICE_LED_COUNT];
    msghub_subscriber_t cmd_sub;
    bool current_state[CUBEMOT_DEVICE_LED_COUNT];
    volatile bool cmd_pending;
    led_cmd_t pending_cmd;
    TaskHandle_t task_handle;
} g_led_controller;

static void publish_led_state(cubemot_device_led led_id, bool state)
{
    if (!cubemot_device_led_is_valid(led_id)) {
        return;
    }
    led_state_t msg = {.led_id = (uint8_t)led_id, .state = state, .timestamp = common_get_timestamp_ms()};
    msghub_publish(g_led_controller.state_pub[led_id], &msg);
}

static void on_led_command(msghub_subscriber_t sub, void *context)
{
    (void)context;

    led_cmd_t cmd;
    msghub_receive(sub, &cmd);

    g_led_controller.pending_cmd = cmd;
    g_led_controller.cmd_pending = true;

    xTaskNotifyFromISR(g_led_controller.task_handle, 1, eSetBits, NULL);
}

static void vTaskLED_Controller(void *pvParameters)
{
    (void)pvParameters;

    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (g_led_controller.cmd_pending) {
            g_led_controller.cmd_pending = false;
            const led_cmd_t cmd = g_led_controller.pending_cmd;

            if (cubemot_device_led_is_valid((cubemot_device_led)cmd.led_id)) {
                const driver_led_instance *led = driver_led_get_instance((cubemot_device_led)cmd.led_id);
                if (led) {
                    if (cmd.state) {
                        led->on(led->ctx);
                    } else {
                        led->off(led->ctx);
                    }
                    g_led_controller.current_state[cmd.led_id] = cmd.state;
                    publish_led_state((cubemot_device_led)cmd.led_id, cmd.state);
                }
            }
        }
    }
}

int led_controller_init(void)
{
    // Create independent state publisher for each LED
    for (int i = 0; i < CUBEMOT_DEVICE_LED_COUNT; i++) {
        int instance = i;
        g_led_controller.state_pub[i] = msghub_create_publisher_multi(MSGHUB_TOPIC(led_state), &instance);
        if (g_led_controller.state_pub[i] == MSGHUB_PUBLISHER_INVALID) {
            for (int j = 0; j < i; j++) {
                msghub_destroy_publisher(g_led_controller.state_pub[j]);
            }
            return -1;
        }
    }

    g_led_controller.cmd_sub = msghub_create_subscriber(MSGHUB_TOPIC(led_command), 0);
    if (g_led_controller.cmd_sub == MSGHUB_SUBSCRIBER_INVALID) {
        for (int i = 0; i < CUBEMOT_DEVICE_LED_COUNT; i++) {
            msghub_destroy_publisher(g_led_controller.state_pub[i]);
        }
        return -2;
    }

    msghub_subscriber_set_callback(g_led_controller.cmd_sub, on_led_command, NULL);

    for (int i = 0; i < CUBEMOT_DEVICE_LED_COUNT; i++) {
        g_led_controller.current_state[i] = false;
    }
    g_led_controller.cmd_pending = false;

    xTaskCreate(vTaskLED_Controller, "LED_Ctrl", 256, NULL, 2, &g_led_controller.task_handle);

    return 0;
}
