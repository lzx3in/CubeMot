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
 *   - Supports multiple motor instances (V2: 1 motor, V3: 2+ motors)
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "foc_types.h"
#include "foc_core.h"
#include "observer.h"

/* ── Configuration ───────────────────────────────────── */

#define MAX_MOTORS 2  // V2: 1 motor, V3: 2 motors

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
 * @brief  Initialize motor control instance with shared FOC/observer
 *
 * The foc_t and observer_t are owned by foc_isr (30kHz ISR context).
 * motor_ctrl uses them for speed loop and state publishing.
 *
 * @param  motor_id     Motor index (0-based)
 * @param  config       Motor parameters (Flash-resident)
 * @param  foc          Shared FOC object (from foc_isr)
 * @param  obs          Shared observer object (from foc_isr)
 * @return 0 on success
 */
int motor_ctrl_init(uint8_t motor_id, const foc_motor_config_t *config,
                    foc_t *foc, observer_t *obs);

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
