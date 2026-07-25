/**
 * @file foc_adc.h
 * @brief FOC current sensing ADC driver for STM32G4
 *
 * Uses ADC1+ADC2 in dual injected mode, triggered by TIM1 TRGO.
 * Three-shunt topology with independent ADC resources:
 *   Phase U (Ia): ADC1 CH2  (PA1)
 *   Phase V (Ib): ADC1/ADC2 CH14 shared (PB11)
 *   Phase W (Ic): ADC2 CH4  (PA7)
 *   Vbus:         ADC2 CH11 (PC5) — regular conversion
 *
 * All conversions are 12-bit, left-aligned.
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/kernel.h>

/* ── ADC raw readings ───────────────────────────────── */

typedef struct {
    int16_t ia;   /* Phase U current, ADC1_IN2 */
    int16_t ib;   /* Phase V current, ADC1_IN14 (shared) */
    int16_t ic;   /* Phase W current, ADC2_IN4 */
    int16_t vbus; /* Bus voltage, ADC2_IN11 */
} foc_adc_raw_t;

/* ── API ────────────────────────────────────────────── */

/**
 * @brief  Initialize ADC1+ADC2 for FOC current sensing
 * @return 0 on success, negative on error
 */
int foc_adc_init(void);

/**
 * @brief  Read latest injected conversion results
 *
 * Called from PWM ISR (or its bottom half).
 * Results are from the previous PWM cycle's ADC trigger.
 *
 * @param  out   Pointer to result structure
 */
void foc_adc_read_raw(foc_adc_raw_t *out);

/**
 * @brief  Start regular conversion for Vbus (triggered by timer or software)
 */
void foc_adc_start_vbus(void);

/**
 * @brief  Check if regular conversion is complete
 */
bool foc_adc_vbus_ready(void);

/**
 * @brief  Blocking read of bus voltage [V] from ADC2 regular channel
 *
 * Starts a software-triggered conversion, waits for completion,
 * and returns the converted voltage. Used at init time before ISR runs.
 *
 * @return Bus voltage in Volts
 */
float foc_adc_read_vbus_blocking(void);

/**
 * @brief  Calibrate ADC offsets
 *
 * Reads ADC values with all phases disabled (0 current) to
 * determine zero-current offset for each channel.
 *
 * @param  ia_offset  Phase U offset
 * @param  ib_offset  Phase V offset
 * @param  ic_offset  Phase W offset
 */
void foc_adc_get_offsets(int16_t *ia_offset, int16_t *ib_offset, int16_t *ic_offset);

#ifdef __cplusplus
}
#endif
