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
#include <stddef.h>

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
/* Fast sin/cos LUT: 256-entry linear interpolated, ~0.1 deg accuracy */
static const float sin_table[257] = {
    0.000000f, 0.024541f, 0.049068f, 0.073565f, 0.098017f, 0.122411f, 0.146730f, 0.170962f,
    0.195090f, 0.219101f, 0.242980f, 0.266713f, 0.290285f, 0.313682f, 0.336890f, 0.359895f,
    0.382683f, 0.405241f, 0.427555f, 0.449611f, 0.471397f, 0.492898f, 0.514103f, 0.534998f,
    0.555570f, 0.575808f, 0.595699f, 0.615232f, 0.634393f, 0.653173f, 0.671559f, 0.689541f,
    0.707107f, 0.724247f, 0.740951f, 0.757209f, 0.773010f, 0.788346f, 0.803208f, 0.817585f,
    0.831470f, 0.844854f, 0.857729f, 0.870087f, 0.881921f, 0.893224f, 0.903989f, 0.914210f,
    0.923880f, 0.932993f, 0.941544f, 0.949528f, 0.956940f, 0.963776f, 0.970031f, 0.975702f,
    0.980785f, 0.985278f, 0.989177f, 0.992480f, 0.995185f, 0.997290f, 0.998795f, 0.999699f,
    1.000000f, 0.999699f, 0.998795f, 0.997290f, 0.995185f, 0.992480f, 0.989177f, 0.985278f,
    0.980785f, 0.975702f, 0.970031f, 0.963776f, 0.956940f, 0.949528f, 0.941544f, 0.932993f,
    0.923880f, 0.914210f, 0.903989f, 0.893224f, 0.881921f, 0.870087f, 0.857729f, 0.844854f,
    0.831470f, 0.817585f, 0.803208f, 0.788346f, 0.773010f, 0.757209f, 0.740951f, 0.724247f,
    0.707107f, 0.689541f, 0.671559f, 0.653173f, 0.634393f, 0.615232f, 0.595699f, 0.575808f,
    0.555570f, 0.534998f, 0.514103f, 0.492898f, 0.471397f, 0.449611f, 0.427555f, 0.405241f,
    0.382683f, 0.359895f, 0.336890f, 0.313682f, 0.290285f, 0.266713f, 0.242980f, 0.219101f,
    0.195090f, 0.170962f, 0.146730f, 0.122411f, 0.098017f, 0.073565f, 0.049068f, 0.024541f,
    0.000000f,-0.024541f,-0.049068f,-0.073565f,-0.098017f,-0.122411f,-0.146730f,-0.170962f,
   -0.195090f,-0.219101f,-0.242980f,-0.266713f,-0.290285f,-0.313682f,-0.336890f,-0.359895f,
   -0.382683f,-0.405241f,-0.427555f,-0.449611f,-0.471397f,-0.492898f,-0.514103f,-0.534998f,
   -0.555570f,-0.575808f,-0.595699f,-0.615232f,-0.634393f,-0.653173f,-0.671559f,-0.689541f,
   -0.707107f,-0.724247f,-0.740951f,-0.757209f,-0.773010f,-0.788346f,-0.803208f,-0.817585f,
   -0.831470f,-0.844854f,-0.857729f,-0.870087f,-0.881921f,-0.893224f,-0.903989f,-0.914210f,
   -0.923880f,-0.932993f,-0.941544f,-0.949528f,-0.956940f,-0.963776f,-0.970031f,-0.975702f,
   -0.980785f,-0.985278f,-0.989177f,-0.992480f,-0.995185f,-0.997290f,-0.998795f,-0.999699f,
   -1.000000f,-0.999699f,-0.998795f,-0.997290f,-0.995185f,-0.992480f,-0.989177f,-0.985278f,
   -0.980785f,-0.975702f,-0.970031f,-0.963776f,-0.956940f,-0.949528f,-0.941544f,-0.932993f,
   -0.923880f,-0.914210f,-0.903989f,-0.893224f,-0.881921f,-0.870087f,-0.857729f,-0.844854f,
   -0.831470f,-0.817585f,-0.803208f,-0.788346f,-0.773010f,-0.757209f,-0.740951f,-0.724247f,
   -0.707107f,-0.689541f,-0.671559f,-0.653173f,-0.634393f,-0.615232f,-0.595699f,-0.575808f,
   -0.555570f,-0.534998f,-0.514103f,-0.492898f,-0.471397f,-0.449611f,-0.427555f,-0.405241f,
   -0.382683f,-0.359895f,-0.336890f,-0.313682f,-0.290285f,-0.266713f,-0.242980f,-0.219101f,
   -0.195090f,-0.170962f,-0.146730f,-0.122411f,-0.098017f,-0.073565f,-0.049068f,-0.024541f,
};

static inline void fast_sincos(float theta, float *sin_out, float *cos_out)
{
    /* NaN/Inf guard: prevents dead-loop in while() below */
    if (__builtin_isnan(theta) || __builtin_isinf(theta)) {
        *sin_out = 0.0f;
        *cos_out = 1.0f;
        return;
    }
    const float two_pi = 6.283185307f;
    while (theta >= two_pi) theta -= two_pi;
    while (theta < 0.0f)     theta += two_pi;
    float idx_f = theta * 40.74366543f;
    int idx = (int)idx_f;
    float frac = idx_f - (float)idx;
    if (idx < 0) idx = 0;
    if (idx > 255) idx = 255;
    float s0 = sin_table[idx], s1 = sin_table[idx + 1];
    float c0 = sin_table[(idx + 64) & 255], c1 = sin_table[((idx + 64) & 255) + 1];
    *sin_out = s0 + frac * (s1 - s0);
    *cos_out = c0 + frac * (c1 - c0);
}

#ifdef __cplusplus
}
#endif
