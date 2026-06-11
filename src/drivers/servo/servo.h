#pragma once

/**
 * @file servo.h
 * @brief Servo PWM driver — 50Hz PWM with 500-2500μs pulse width
 *
 * Uses TIM3 Channel 1 (PA6) for servo control.
 * Supports up to 2 servos on TIM3 CH1/CH2.
 *
 * V2: 1 servo (steering)
 * V3: 2 servos (front/rear steering)
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* ── Configuration ───────────────────────────────────── */

#define MAX_SERVOS 2

/* ── API ─────────────────────────────────────────────── */

/**
 * @brief  Initialize servo PWM driver
 *
 * Configures TIM3 for 50Hz PWM on CH1/CH2.
 *
 * @return 0 on success
 */
int servo_init(void);

/**
 * @brief  Set servo angle
 *
 * @param  servo_id   Servo index (0 or 1)
 * @param  angle_deg  Target angle in degrees [-90, 90]
 *                    0° = center (1500μs), ±90° = full travel
 * @return 0 on success, -1 if invalid
 */
int servo_set_angle(uint8_t servo_id, float angle_deg);

/**
 * @brief  Set servo pulse width directly
 *
 * @param  servo_id   Servo index (0 or 1)
 * @param  pulse_us   Pulse width in microseconds [500, 2500]
 * @return 0 on success, -1 if out of range
 */
int servo_set_pulse_us(uint8_t servo_id, uint16_t pulse_us);

/**
 * @brief  Get current servo angle
 *
 * @param  servo_id   Servo index
 * @return Current angle in degrees, or 0 if invalid
 */
float servo_get_angle(uint8_t servo_id);

#ifdef __cplusplus
}
#endif
