/**
 * @file foc_isr.h
 * @brief FOC 30kHz ISR interface
 *
 * Provides the real-time current control loop that runs at PWM frequency.
 */

#pragma once

#include "foc_core.h"
#include "observer.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief  Initialize FOC ISR
 *
 * Sets up TIM1 update interrupt and initializes FOC/observer instances.
 * Must be called after foc_pwm_init() and foc_adc_init().
 */
void foc_isr_init(void);

/**
 * @brief  Start FOC ISR
 *
 * Enables the 30kHz current control loop.
 */
void foc_isr_start(void);

/**
 * @brief  Stop FOC ISR
 *
 * Disables the current control loop and sets duty to zero.
 */
void foc_isr_stop(void);

/**
 * @brief  Get FOC instance
 * @return Pointer to global FOC instance
 */
foc_t *foc_isr_get_foc(void);

/**
 * @brief  Get observer instance
 * @return Pointer to global observer instance
 */
observer_t *foc_isr_get_observer(void);

/**
 * @brief  Get ISR execution count
 * @return Number of ISR invocations since start
 */
uint32_t foc_isr_get_count(void);

/**
 * @brief  Check if ISR is running
 * @return true if ISR is active
 */
bool foc_isr_is_running(void);

/**
 * @brief  Set bus voltage manually
 * @param  vbus  Bus voltage in Volts
 */
void foc_isr_set_vbus(float vbus);

/**
 * @brief  Enable/disable observer theta override
 *
 * When disabled (false), the ISR does NOT write observer's theta_elec
 * into the FOC state. Used by motor_ctrl during startup (ALIGN/START)
 * when motor_ctrl controls theta_elec directly.
 *
 * @param  override  true = ISR updates theta from observer (default)
 */
void foc_isr_set_observer_override(bool override);

/**
 * @brief  Check if observer theta override is active
 * @return true if ISR is NOT updating theta from observer
 */
bool foc_isr_get_observer_override(void);
