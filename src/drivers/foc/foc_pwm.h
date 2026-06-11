/**
 * @file foc_pwm.h
 * @brief FOC PWM driver for STM32G4 TIM1
 *
 * Configures TIM1 for 3-phase center-aligned PWM with:
 * - Complementary outputs (CH1/CH2/CH3 + CH1N/CH2N/CH3N)
 * - Dead-time insertion
 * - TRGO on OC4REF for ADC triggering
 * - Break input (BKIN2) for emergency stop
 *
 * Zephyr integration: uses device tree for clock enable + pinmux,
 * direct register access for advanced TIM1 features.
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/kernel.h>

/* ── Configuration ──────────────────────────────────── */

#define FOC_PWM_FREQ_HZ         30000
#define FOC_PWM_DEADTIME_NS     550
#define FOC_PWM_TIMER_CLK_HZ    170000000

/* PWM period in timer ticks (center-aligned, so half-period) */
#define FOC_PWM_HALF_PERIOD     (FOC_PWM_TIMER_CLK_HZ / (FOC_PWM_FREQ_HZ * 2))

/* ── API ────────────────────────────────────────────── */

/**
 * @brief  Initialize TIM1 for FOC PWM output
 * @return 0 on success, negative on error
 */
int foc_pwm_init(void);

/**
 * @brief  Set duty cycles for all three phases
 * @param  duty_a  Phase A duty [0.0, 1.0]
 * @param  duty_b  Phase B duty [0.0, 1.0]
 * @param  duty_c  Phase C duty [0.0, 1.0]
 *
 * Callable from ISR context (direct register write).
 * Must be called with interrupts disabled or from PWM ISR.
 */
void foc_pwm_set_duty(float duty_a, float duty_b, float duty_c);

/**
 * @brief  Enable PWM outputs (start the motor)
 */
void foc_pwm_enable(void);

/**
 * @brief  Disable PWM outputs (stop the motor, all outputs idle)
 */
void foc_pwm_disable(void);

/**
 * @brief  Check if PWM outputs are enabled
 */
bool foc_pwm_is_enabled(void);

/**
 * @brief  Get the PWM period value (used for ADC timing calculations)
 */
uint32_t foc_pwm_get_period(void);

#ifdef __cplusplus
}
#endif
