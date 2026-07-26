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
#include "pid.h"

/* ── Configuration ───────────────────────────────────── */

#define MAX_MOTORS 2  // V2: 1 motor, V3: 2 motors

/* ── Startup sequence parameters ─────────────────────── */

typedef struct {
    uint32_t phase1_duration_ms;   /* Alignment phase duration */
    float    phase1_align_current; /* Alignment current [A] */
    uint32_t phase2_duration_ms;   /* Forced ramp duration */
    float    phase2_final_speed;   /* Final speed for ramp [RPM] */
    float    phase2_current;       /* Ramp current [A] */
    float    obs_min_speed_rpm;    /* Min speed for observer convergence */
    uint32_t consecutive_ok;       /* Consecutive OK count for switchover */
} startup_config_t;

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
 *
 * @param  arg1  Unused
 * @param  arg2  Unused
 * @param  arg3  Unused
 */
void motor_ctrl_thread(void *arg1, void *arg2, void *arg3);

/* ── Runtime tuning accessors ──────────────────────── */

/**
 * @brief  Get speed PID controller for a motor
 * @param  motor_id  Motor index
 * @return Pointer to PID_t, or NULL if invalid
 */
PID_t *motor_ctrl_get_speed_pid(uint8_t motor_id);

/**
 * @brief  Set speed loop PI gains for a motor
 * @param  motor_id  Motor index
 * @param  kp        Proportional gain
 * @param  ki        Integral gain
 */
void motor_ctrl_set_speed_gains(uint8_t motor_id, float kp, float ki);

/**
 * @brief  Get startup configuration (read-only view)
 */
const startup_config_t *motor_ctrl_get_startup_cfg(void);

/**
 * @brief  Set a startup parameter by key
 * @param  key    One of: align_ms, align_ma, ramp_ms, ramp_rpm, ramp_ma
 * @param  value  Integer value (mA for currents, ms for times, RPM for speed)
 * @return 0 on success, -1 if key unknown
 */
int motor_ctrl_set_startup_param(const char *key, int value);

#ifdef __cplusplus
}
#endif
