#include "app.h"

#include "modules/blink/blink.h"
#include "modules/led_controller/led_controller.h"
#include "modules/button_detector/button_detector.h"
#include "modules/commander/commander.h"
#include "modules/vehicle/vehicle.h"
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

volatile app_state_t g_app_state = APP_STATE_UNINITIALIZED;

static const app_config_t k_default_config = {
    .auto_start = true, .startup_delay_ms = 1000, .enable_safety = false, .enable_debug = false};

static int init_modules(void)
{
#if CONFIG_MODULE_LED_CONTROLLER_ENABLE
    led_controller_init();
#endif

#if CONFIG_MODULE_BUTTON_DETECTOR_ENABLE
    button_detector_init();
#endif

#if CONFIG_MODULE_BLINK_ENABLE
    blink_module_init();
#endif

#if CONFIG_MODULE_COMMANDER_ENABLE
    commander_init();
#endif

#if CONFIG_MODULE_VEHICLE_ENABLE
    vehicle_init(NULL);  // Use default config
#endif

    return 0;
}

/* ── Thread stacks ───────────────────────────────────── */

#if CONFIG_MODULE_COMMANDER_ENABLE
#define COMMANDER_STACK_SIZE 2048
K_THREAD_STACK_DEFINE(commander_stack, COMMANDER_STACK_SIZE);
static struct k_thread commander_thread_data;
#endif

#if CONFIG_MODULE_VEHICLE_ENABLE
#define VEHICLE_STACK_SIZE 2048
K_THREAD_STACK_DEFINE(vehicle_stack, VEHICLE_STACK_SIZE);
static struct k_thread vehicle_thread_data;
#endif

static void start_threads(void)
{
#if CONFIG_MODULE_COMMANDER_ENABLE
    k_thread_create(&commander_thread_data, commander_stack,
                    COMMANDER_STACK_SIZE,
                    (k_thread_entry_t)commander_thread,
                    NULL, NULL, NULL,
                    K_PRIO_COOP(7), 0, K_NO_WAIT);
    k_thread_name_set(&commander_thread_data, "commander");
#endif

#if CONFIG_MODULE_VEHICLE_ENABLE
    k_thread_create(&vehicle_thread_data, vehicle_stack,
                    VEHICLE_STACK_SIZE,
                    (k_thread_entry_t)vehicle_thread,
                    NULL, NULL, NULL,
                    K_PRIO_COOP(7), 0, K_NO_WAIT);
    k_thread_name_set(&vehicle_thread_data, "vehicle");
#endif
}

int app_init(void)
{
    g_app_state = APP_STATE_INITIALIZING;

    int ret = init_modules();
    if (ret != 0) {
        g_app_state = APP_STATE_ERROR;
        return ret;
    }

    start_threads();
    
    g_app_state = APP_STATE_RUNNING;
    return 0;
}

app_state_t app_get_state(void)
{
    return g_app_state;
}

const app_config_t *app_get_default_config(void)
{
    return &k_default_config;
}
