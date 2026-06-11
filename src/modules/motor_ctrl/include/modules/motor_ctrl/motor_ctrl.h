#pragma once

/**
 * @file motor_ctrl.h
 * @brief Per-motor control module: speed loop, startup sequence, fault handling
 *
 * Architecture:
 *   - FOC core + observer run at 30kHz (PWM ISR, not managed here)
 *   - Speed loop runs at 1kHz in Zephyr thread
 *   - Startup sequence: Alignment → Forced Ramp → Switchover → Closed Loop
 *   - Publishes motor_state via msghub at 100Hz
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "foc_types.h"

/* ── Motor instance ──────────────────────────────────── */

typedef enum {
    MOTOR_STATE_IDLE = 0,
    MOTOR_STATE_ALIGN,
    MOTOR_STATE_START,
    MOTOR_STATE_RUN,
    MOTOR_STATE_STOP,
    MOTOR_STATE_FAULT
} motor_run_state_t;

/* ── API ─────────────────────────────────────────────── */

/**
 * @brief  Initialize motor control module
 *
 * Must be called before any other motor_ctrl functions.
 * Initializes FOC core, observer, speed PID, and subscribes
 * to motor_cmd topic.
 *
 * @param  motor_id     Motor index (0-based)
 * @param  config       Motor parameters (Flash-resident)
 * @param  pole_pairs   Number of pole pairs
 * @return 0 on success
 */
int motor_ctrl_init(uint8_t motor_id, const foc_motor_config_t *config);

/**
 * @brief  Enter the motor control thread
 *
 * Never returns. Runs the 1kHz speed loop + startup sequence.
 * Called from a Zephyr thread context.
 */
void motor_ctrl_thread(void);

#ifdef __cplusplus
}
#endif
