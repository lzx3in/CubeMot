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
