#pragma once

/**
 * @file svpwm.h
 * @brief Space Vector PWM modulation
 *
 * Implements 7-segment (center-aligned) SVPWM.
 * Outputs duty cycles 0.0–1.0 for three PWM channels.
 *
 * Algorithm: inject common-mode 3rd harmonic for 15.5% voltage boost
 *            vs sine-PWM, equivalent to SVM without explicit sector selection.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <math.h>

/* ── SVPWM result ──────────────────────────────────── */

typedef struct {
    float duty_a;   // Phase A duty [0, 1]
    float duty_b;   // Phase B duty [0, 1]
    float duty_c;   // Phase C duty [0, 1]
} svpwm_duty_t;

/* ── Min-max injection SVPWM ───────────────────────── */

/**
 * @brief  SVPWM via common-mode injection (computationally light)
 *
 *         1. Compute V_offset = -(Vmax + Vmin) / 2
 *         2. Add offset to all three phases
 *         3. Scale to [0, 1]
 *
 * @param  v_alpha   α-axis voltage reference
 * @param  v_beta    β-axis voltage reference
 * @param  v_bus     Bus voltage [V]
 * @param  out       Output duty cycles
 */
static inline void svpwm_minmax(float v_alpha, float v_beta, float v_bus, svpwm_duty_t *out)
{
    /* Inverse Clarke: αβ → abc phase voltages */
    float va = v_alpha;
    float vb = -0.5f * v_alpha + 0.86602540378f * v_beta;
    float vc = -0.5f * v_alpha - 0.86602540378f * v_beta;

    /* Common-mode injection for SVPWM */
    float v_max = va;
    if (vb > v_max) v_max = vb;
    if (vc > v_max) v_max = vc;

    float v_min = va;
    if (vb < v_min) v_min = vb;
    if (vc < v_min) v_min = vc;

    float v_offset = -(v_max + v_min) * 0.5f;

    /* Add offset and normalize to duty [0, 1] */
    float v_norm = 2.0f / v_bus; // peak phase voltage → bus voltage normalization
    out->duty_a = (va + v_offset) * v_norm * 0.5f + 0.5f;
    out->duty_b = (vb + v_offset) * v_norm * 0.5f + 0.5f;
    out->duty_c = (vc + v_offset) * v_norm * 0.5f + 0.5f;
}

/**
 * @brief  Clamp duties to [0, 1] range
 */
static inline void svpwm_clamp(svpwm_duty_t *duty)
{
    if (duty->duty_a < 0.0f) duty->duty_a = 0.0f;
    if (duty->duty_a > 1.0f) duty->duty_a = 1.0f;
    if (duty->duty_b < 0.0f) duty->duty_b = 0.0f;
    if (duty->duty_b > 1.0f) duty->duty_b = 1.0f;
    if (duty->duty_c < 0.0f) duty->duty_c = 0.0f;
    if (duty->duty_c > 1.0f) duty->duty_c = 1.0f;
}

#ifdef __cplusplus
}
#endif
