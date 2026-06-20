#include "app.h"

#if CONFIG_MODULE_BLINK_ENABLE
#include "modules/blink/blink.h"
#endif
#if CONFIG_MODULE_LED_CONTROLLER_ENABLE
#include "modules/led_controller/led_controller.h"
#endif
#if CONFIG_MODULE_BUTTON_DETECTOR_ENABLE
#include "modules/button_detector/button_detector.h"
#endif
#include "modules/commander/commander.h"
#include "modules/vehicle/vehicle.h"
#if CONFIG_MODULE_MOTOR_CTRL_ENABLE
#include "modules/motor_ctrl/motor_ctrl.h"
#include "common/motor_params.h"
#endif
#include "comm/serial_cmd.h"
#include "drivers/foc/foc_pwm.h"
#include "drivers/foc/foc_adc.h"
#include "drivers/foc/foc_isr.h"
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

    /* Initialize FOC hardware (PWM, ADC, ISR) */
    foc_pwm_init();
    foc_adc_init();
    foc_isr_init();  /* ISR initialized but NOT started yet */

    /* Initialize motor control instances (V1: 1 motor, shared FOC with ISR) */
#if CONFIG_MODULE_MOTOR_CTRL_ENABLE
    motor_ctrl_init(0, &g_motor_params, foc_isr_get_foc(), foc_isr_get_observer());
#endif

#if CONFIG_MODULE_COMMANDER_ENABLE
    commander_init();
#endif

#if CONFIG_MODULE_VEHICLE_ENABLE
    vehicle_init(NULL);  // Use default config
#endif

#if CONFIG_MODULE_SERIAL_CMD_ENABLE
    serial_cmd_init();
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

#if CONFIG_MODULE_SERIAL_CMD_ENABLE
#define SERIAL_RX_STACK_SIZE 2048
#define SERIAL_TX_STACK_SIZE 2048
K_THREAD_STACK_DEFINE(serial_rx_stack, SERIAL_RX_STACK_SIZE);
K_THREAD_STACK_DEFINE(serial_tx_stack, SERIAL_TX_STACK_SIZE);
static struct k_thread serial_rx_thread_data;
static struct k_thread serial_tx_thread_data;
#endif

#if CONFIG_MODULE_MOTOR_CTRL_ENABLE
#define MOTOR_CTRL_STACK_SIZE 2048
K_THREAD_STACK_DEFINE(motor_ctrl_stack, MOTOR_CTRL_STACK_SIZE);
static struct k_thread motor_ctrl_thread_data;
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

#if CONFIG_MODULE_MOTOR_CTRL_ENABLE
    k_thread_create(&motor_ctrl_thread_data, motor_ctrl_stack,
                    MOTOR_CTRL_STACK_SIZE,
                    (k_thread_entry_t)motor_ctrl_thread,
                    NULL, NULL, NULL,
                    K_PRIO_COOP(5), 0, K_NO_WAIT);
    k_thread_name_set(&motor_ctrl_thread_data, "motor_ctrl");
#endif

#if CONFIG_MODULE_VEHICLE_ENABLE
    k_thread_create(&vehicle_thread_data, vehicle_stack,
                    VEHICLE_STACK_SIZE,
                    (k_thread_entry_t)vehicle_thread,
                    NULL, NULL, NULL,
                    K_PRIO_COOP(7), 0, K_NO_WAIT);
    k_thread_name_set(&vehicle_thread_data, "vehicle");
#endif

#if CONFIG_MODULE_SERIAL_CMD_ENABLE
    k_thread_create(&serial_rx_thread_data, serial_rx_stack,
                    SERIAL_RX_STACK_SIZE,
                    (k_thread_entry_t)serial_cmd_rx_thread,
                    NULL, NULL, NULL,
                    SERIAL_RX_THREAD_PRIORITY, 0, K_NO_WAIT);
    k_thread_name_set(&serial_rx_thread_data, "serial_rx");
    
    k_thread_create(&serial_tx_thread_data, serial_tx_stack,
                    SERIAL_TX_STACK_SIZE,
                    (k_thread_entry_t)serial_cmd_tx_thread,
                    NULL, NULL, NULL,
                    SERIAL_TX_THREAD_PRIORITY, 0, K_NO_WAIT);
    k_thread_name_set(&serial_tx_thread_data, "serial_tx");
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
