#pragma once

/**
 * @file foc_types.h
 * @brief FOC data types and common structures
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

/* ── Motor configuration (from Workbench) ────────────── */

typedef struct {
    /* Electrical parameters */
    float   rs;             // Stator resistance [Ω]
    float   ls;             // Stator inductance [H] (Ld≈Lq for SPMSM)
    float   ld_lq_ratio;    // Ld/Lq ratio (1.0 for SPMSM)
    uint8_t pole_pairs;     // Number of pole pairs
    float   voltage_const;  // Voltage constant [Vrms/kRPM]
    float   rated_flux;     // Rated flux linkage [Wb]

    /* Ratings */
    float   max_speed_rpm;  // Maximum mechanical speed [RPM]
    float   nominal_current;// Nominal current [A]
    float   iq_max;         // Maximum Q-axis current [A]
    float   id_demag;       // Demagnetization current [A]

    /* Observer parameters */
    float   observer_gain1; // State observer gain 1
    float   observer_gain2; // State observer gain 2
    float   pll_kp;         // PLL proportional gain
    float   pll_ki;         // PLL integral gain
    uint16_t observer_min_speed_rpm; // Minimum speed for observer convergence
} foc_motor_config_t;

/* ── FOC runtime state ───────────────────────────────── */

typedef struct {
    /* Clarke/Park results */
    float i_alpha;          // α-axis current [A]
    float i_beta;           // β-axis current [A]
    float i_d;              // D-axis current [A]
    float i_q;              // Q-axis current [A]

    /* Voltage outputs */
    float v_d;              // D-axis voltage [A] — from PI output
    float v_q;              // Q-axis voltage [A] — from PI output
    float v_alpha;          // α-axis voltage [A]
    float v_beta;           // β-axis voltage [A]

    /* Angle and speed */
    float theta_elec;       // Electrical angle [rad]
    float omega_elec;       // Electrical angular speed [rad/s]
    float speed_rpm;        // Mechanical speed [RPM]

    /* References */
    float i_d_ref;          // D-axis current reference [A]
    float i_q_ref;          // Q-axis current reference [A]

    /* ADC raw (latest reading) */
    int16_t adc_ia;
    int16_t adc_ib;
    int16_t adc_ic;
    int16_t adc_vbus;

    /* ADC offsets */
    int16_t adc_ia_offset;
    int16_t adc_ib_offset;
    int16_t adc_ic_offset;

    /* Bus voltage [V] */
    float v_bus;

    /* Duty cycles (latest output) */
    float duty_a;
    float duty_b;
    float duty_c;

    /* Fault flags */
    bool fault_overcurrent;
    bool fault_overvoltage;
    bool fault_undervoltage;
    bool fault_overtemp;
    bool fault_startup_failed;
} foc_state_t;

/* ── FOC instance ─────────────────────────────────────── */

typedef struct {
    const foc_motor_config_t *config;  // Motor parameters (Flash)
    foc_state_t              state;    // Runtime state
    bool                     initialized;
} foc_t;

/* ── ADC → Physical Unit Conversion ──────────────────── */

/* ADC constants (from hardware reference) */
#define FOC_ADC_VREF        3.3f       // ADC reference voltage [V]
#define FOC_ADC_RESOLUTION  4096.0f    // 12-bit, full scale
#define FOC_ADC_ALIGN_LEFT  1          // Left-aligned (workbench convention)

/**
 * @brief  Convert ADC raw value to current [A]
 *
 * Ia = (ADC_raw / 4096) * Vref / (Rshunt * Gain)
 *    = ADC_raw * 3.3 / (4096 * 0.33 * 1.53)
 *    = ADC_raw * 0.001597
 */
static inline float foc_adc_to_current(int16_t raw, int16_t offset)
{
    static const float scale = FOC_ADC_VREF
        / (FOC_ADC_RESOLUTION * 0.33f * 1.53f);
    return (float)(raw - offset) * scale;
}

/**
 * @brief  Convert ADC raw value to bus voltage [V]
 *
 * Vbus = (ADC_raw / 4096) * Vref / PARTITIONING_FACTOR
 *      = ADC_raw * 3.3 / (4096 * 0.0625)
 *      = ADC_raw * 0.01289
 */
static inline float foc_adc_to_vbus(int16_t raw)
{
    static const float scale = FOC_ADC_VREF
        / (FOC_ADC_RESOLUTION * 0.0625f);
    return (float)raw * scale;
}

/* ── Speed Unit Conversion ────────────────────────────── */

static inline float foc_rpm_to_rads(float rpm, uint8_t pole_pairs)
{
    return rpm * 2.0f * 3.14159265359f * (float)pole_pairs / 60.0f;
}

static inline float foc_rads_to_rpm(float rads, uint8_t pole_pairs)
{
    return rads * 60.0f / (2.0f * 3.14159265359f * (float)pole_pairs);
}

#ifdef __cplusplus
}
#endif
