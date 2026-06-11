#pragma once

/**
 * @file foc_core.h
 * @brief FOC core: current control loop (HF task, runs at PWM rate)
 *
 * Called from PWM ISR context. All operations are float-based
 * (STM32G4 has hardware FPU), execution ~5-8µs at 170MHz.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "foc_types.h"
#include "pid.h"
#include "transform.h"
#include "svpwm.h"

/* ── PI Controller (Current Loop) ───────────────────── */

/**
 * @brief  Initialize FOC instance with motor parameters
 * @param  foc     FOC instance
 * @param  config  Motor configuration (Flash-resident)
 * @param  pid_id  D-axis PI controller instance
 * @param  pid_iq  Q-axis PI controller instance
 */
void foc_init(foc_t *foc, const foc_motor_config_t *config);

/**
 * @brief  Run one FOC current control iteration
 *
 * Steps:
 *   1. Read ADC currents → Ia, Ib, Ic
 *   2. Clarke transform → Iα, Iβ
 *   3. Park transform  → Id, Iq
 *   4. Id PI controller → Vd
 *   5. Iq PI controller → Vq (output → foc->state.duty_*)
 *   6. Inverse Park     → Vα, Vβ
 *   7. SVPWM → duty cycles
 *
 * Must be called from PWM ISR (or its bottom half).
 * Returns immediately; call foc_apply_duty() to write PWM registers.
 */
void foc_current_loop(foc_t *foc);

/**
 * @brief  Write duty cycles to PWM registers
 *
 * Separated from foc_current_loop to allow ISR bottom-half deferral.
 */
void foc_apply_duty(foc_t *foc);

#ifdef __cplusplus
}
#endif
