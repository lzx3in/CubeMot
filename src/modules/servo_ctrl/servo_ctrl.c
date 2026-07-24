/**
 * @file servo_ctrl.c
 * @brief Servo control module implementation
 *
 * Subscribes to servo_cmd topic and controls servo angles.
 * Publishes servo_state at 10Hz.
 * Provides smooth angle interpolation (100Hz update rate).
 */

#include "modules/servo_ctrl/servo_ctrl.h"
#include <zephyr/drivers/pwm.h>
#include "topics/topics.h"
#include "msghub/msghub.h"
#include "common_time.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <math.h>

LOG_MODULE_REGISTER(servo_ctrl, LOG_LEVEL_INF);

/* ── Configuration ───────────────────────────────────── */

#define SERVO_CTRL_THREAD_STACK_SIZE 1024
#define SERVO_CTRL_THREAD_PRIORITY 5
#define SERVO_CTRL_UPDATE_HZ 100
#define SERVO_STATE_PUBLISH_HZ 10

#define MAX_SERVOS 2
#define SERVO_PULSE_MIN_US 500
#define SERVO_PULSE_MAX_US 2500
#define SERVO_PULSE_CENTER_US 1500
#define SERVO_ANGLE_MAX 90.0f

/* ── PWM device (from devicetree) ────────────────────── */

static const struct pwm_dt_spec servo_pwm[MAX_SERVOS] = {
    PWM_DT_SPEC_GET_BY_NAME(DT_NODELABEL(servo), steer_front),
    PWM_DT_SPEC_GET_BY_NAME(DT_NODELABEL(servo), steer_rear),
};

/* ── State ───────────────────────────────────────────── */

static msghub_subscriber_t g_servo_cmd_sub;
static msghub_publisher_t g_servo_state_pub;

static float g_target_angles[MAX_SERVOS] = {0.0f, 0.0f};
static float g_current_angles[MAX_SERVOS] = {0.0f, 0.0f};

/* ── Servo helpers ───────────────────────────────────── */

static int servo_set_pulse_us(uint8_t id, uint16_t pulse_us)
{
    if (id >= MAX_SERVOS) {
        return -EINVAL;
    }
    if (pulse_us < SERVO_PULSE_MIN_US) pulse_us = SERVO_PULSE_MIN_US;
    if (pulse_us > SERVO_PULSE_MAX_US) pulse_us = SERVO_PULSE_MAX_US;

    return pwm_set_dt(&servo_pwm[id], PWM_MSEC(20), PWM_USEC(pulse_us));
}

static int servo_set_angle(uint8_t id, float angle_deg)
{
    if (id >= MAX_SERVOS) {
        return -EINVAL;
    }
    if (angle_deg < -SERVO_ANGLE_MAX) angle_deg = -SERVO_ANGLE_MAX;
    if (angle_deg > SERVO_ANGLE_MAX) angle_deg = SERVO_ANGLE_MAX;

    /* -90° → 500μs, 0° → 1500μs, +90° → 2500μs */
    float pulse_us = SERVO_PULSE_CENTER_US +
                     (angle_deg / SERVO_ANGLE_MAX) *
                     (SERVO_PULSE_MAX_US - SERVO_PULSE_CENTER_US);

    return servo_set_pulse_us(id, (uint16_t)pulse_us);
}

/* ── Thread ──────────────────────────────────────────── */

K_THREAD_STACK_DEFINE(servo_ctrl_stack, 1024);
static struct k_thread servo_ctrl_thread_data;

void servo_ctrl_thread(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    LOG_INF("Servo control thread started");

    uint32_t state_publish_counter = 0;
    int64_t last_update_time = k_uptime_get();

    while (1) {
        int64_t current_time = k_uptime_get();
        float dt_ms = (float)(current_time - last_update_time);
        last_update_time = current_time;

        // Process incoming commands
        bool updated = false;
        msghub_subscriber_check(g_servo_cmd_sub, &updated);
        if (updated) {
            servo_cmd_t cmd;
            msghub_receive(g_servo_cmd_sub, &cmd);
            if (cmd.servo_id < MAX_SERVOS) {
                g_target_angles[cmd.servo_id] = cmd.angle_deg;
                LOG_INF("Servo %u target: %d deg", cmd.servo_id, (int)cmd.angle_deg);
            }
        }

        // Simple linear interpolation (avoid expf which is not in minimal libc)
        // Time constant ~50ms for smooth steering
        float alpha = dt_ms / 50.0f;
        if (alpha > 1.0f) alpha = 1.0f;
        
        for (uint8_t i = 0; i < MAX_SERVOS; i++) {
            g_current_angles[i] += alpha * (g_target_angles[i] - g_current_angles[i]);
            servo_set_angle(i, g_current_angles[i]);
        }

        // Publish state at 10Hz
        state_publish_counter++;
        if (state_publish_counter >= (SERVO_CTRL_UPDATE_HZ / SERVO_STATE_PUBLISH_HZ)) {
            state_publish_counter = 0;

            for (uint8_t i = 0; i < MAX_SERVOS; i++) {
                servo_state_t state = {
                    .servo_id = i,
                    .angle_deg = g_current_angles[i],
                    .timestamp = common_get_timestamp_ms(),
                };
                msghub_publish(g_servo_state_pub, &state);
            }
        }

        k_msleep(1000 / SERVO_CTRL_UPDATE_HZ);
    }
}

/* ── Public API ──────────────────────────────────────── */

int servo_ctrl_init(void)
{
    LOG_INF("Initializing servo control module");

    /* Verify PWM devices are ready */
    for (int i = 0; i < MAX_SERVOS; i++) {
        if (!pwm_is_ready_dt(&servo_pwm[i])) {
            LOG_ERR("Servo %d PWM device not ready", i);
            return -ENODEV;
        }
        /* Center position */
        servo_set_pulse_us(i, SERVO_PULSE_CENTER_US);
    }

    // Create msghub subscriber and publisher
    g_servo_cmd_sub = msghub_create_subscriber(MSGHUB_TOPIC(servo_cmd), 0);
    g_servo_state_pub = msghub_create_publisher(MSGHUB_TOPIC(servo_state));

    // Start thread
    k_thread_create(&servo_ctrl_thread_data, servo_ctrl_stack,
                    K_THREAD_STACK_SIZEOF(servo_ctrl_stack),
                    servo_ctrl_thread, NULL, NULL, NULL,
                    K_PRIO_COOP(7), 0, K_NO_WAIT);
    k_thread_name_set(&servo_ctrl_thread_data, "servo_ctrl");

    LOG_INF("Servo control module initialized");
    return 0;
}
