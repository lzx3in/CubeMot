#pragma once

/**
 * @file transform.h
 * @brief FOC coordinate transforms: Clarke, Park, and inverses
 *
 * Reference frames:
 *   abc   — 3-phase stationary (motor phase currents/voltages)
 *   αβ    — 2-phase stationary (Clarke output)
 *   dq    — 2-phase rotating (Park output, rotor-aligned)
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <math.h>

/* ── Vector types ──────────────────────────────────── */

typedef struct {
    float a, b, c;
} abc_t;

typedef struct {
    float alpha, beta;
} alphabeta_t;

typedef struct {
    float d, q;
} dq_t;

/* ── Clarke Transform: abc → αβ ────────────────────── */

/**
 * @brief  Full Clarke (3-phase → 2-phase, amplitude-invariant)
 *         Iα = Ia
 *         Iβ = (Ia + 2*Ib) / √3
 */
static inline alphabeta_t clarke_transform(float a, float b, float c)
{
    (void)c; // a + b + c = 0 for balanced system
    alphabeta_t out;
    out.alpha = a;
    out.beta  = (a + 2.0f * b) * 0.57735026919f; // 1/√3
    return out;
}

/* ── Inverse Clarke: αβ → abc ──────────────────────── */

/**
 * @brief  Inverse Clarke (2-phase → 3-phase)
 *         Va = Vα
 *         Vb = (-Vα + √3·Vβ) / 2
 *         Vc = (-Vα - √3·Vβ) / 2
 */
static inline abc_t iclarke_transform(float alpha, float beta)
{
    abc_t out;
    out.a = alpha;
    out.b = -0.5f * alpha + 0.86602540378f * beta;  // √3/2
    out.c = -0.5f * alpha - 0.86602540378f * beta;
    return out;
}

/* ── Park Transform: αβ → dq ───────────────────────── */

/**
 * @brief  Park transform (stationary → rotating)
 *         Id =  Iα·cos(θ) + Iβ·sin(θ)
 *         Iq = -Iα·sin(θ) + Iβ·cos(θ)
 * @param  theta  Electrical angle [rad]
 */
static inline dq_t park_transform(float alpha, float beta, float sin_theta, float cos_theta)
{
    dq_t out;
    out.d =  alpha * cos_theta + beta * sin_theta;
    out.q = -alpha * sin_theta + beta * cos_theta;
    return out;
}

/* ── Inverse Park: dq → αβ ─────────────────────────── */

/**
 * @brief  Inverse Park (rotating → stationary)
 *         Vα = Vd·cos(θ) - Vq·sin(θ)
 *         Vβ = Vd·sin(θ) + Vq·cos(θ)
 */
static inline alphabeta_t ipark_transform(float d, float q, float sin_theta, float cos_theta)
{
    alphabeta_t out;
    out.alpha = d * cos_theta - q * sin_theta;
    out.beta  = d * sin_theta + q * cos_theta;
    return out;
}

/* ── Cordic-like sin/cos (fast approximation) ──────── */

/**
 * @brief  Fast sin/cos using STM32G4 CORDIC accelerator or math.h
 *         Wraps arm_sin_cos_f32 or standard sinf/cosf
 */
static inline void fast_sincos(float theta, float *sin_out, float *cos_out)
{
    *sin_out = sinf(theta);
    *cos_out = cosf(theta);
}

#ifdef __cplusplus
}
#endif
