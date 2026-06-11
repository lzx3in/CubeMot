#pragma once

/**
 * @file filter.h
 * @brief Digital filters for motor control
 *
 * - 1st-order low-pass filter: y[n] = α·x[n] + (1-α)·y[n-1]
 * - Moving average filter with circular buffer
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>

/* ── 1st-order Low-Pass Filter ─────────────────────── */

typedef struct {
    float alpha;    // smoothing factor [0, 1]
    float y_prev;   // last output
} lpf_t;

/**
 * @brief  Initialize low-pass filter
 * @param  lpf      Filter instance
 * @param  cutoff_hz Cutoff frequency [Hz]
 * @param  sample_hz Sampling frequency [Hz]
 */
static inline void lpf_init(lpf_t *lpf, float cutoff_hz, float sample_hz)
{
    float dt = 1.0f / sample_hz;
    float tau = 1.0f / (2.0f * 3.14159265359f * cutoff_hz);
    lpf->alpha = dt / (tau + dt);
    lpf->y_prev = 0.0f;
}

/**
 * @brief  Set alpha directly (useful when filter needs re-tuning at runtime)
 */
static inline void lpf_set_alpha(lpf_t *lpf, float alpha)
{
    lpf->alpha = alpha;
}

/**
 * @brief  Process one sample
 */
static inline float lpf_update(lpf_t *lpf, float x)
{
    float y = lpf->alpha * x + (1.0f - lpf->alpha) * lpf->y_prev;
    lpf->y_prev = y;
    return y;
}

/**
 * @brief  Reset filter state
 */
static inline void lpf_reset(lpf_t *lpf)
{
    lpf->y_prev = 0.0f;
}

/* ── Moving Average Filter ─────────────────────────── */

#define MAF_WINDOW_MAX 32

typedef struct {
    float   buffer[MAF_WINDOW_MAX];
    uint8_t window_size;
    uint8_t index;
    float   sum;
    uint8_t filled; // whether buffer has been fully filled at least once
} maf_t;

/**
 * @brief  Initialize moving average filter
 * @param  maf     Filter instance
 * @param  window  Window size (≤ MAF_WINDOW_MAX)
 */
static inline void maf_init(maf_t *maf, uint8_t window)
{
    if (window > MAF_WINDOW_MAX) window = MAF_WINDOW_MAX;
    memset(maf->buffer, 0, sizeof(maf->buffer));
    maf->window_size = window;
    maf->index = 0;
    maf->sum = 0.0f;
    maf->filled = 0;
}

/**
 * @brief  Process one sample, return average
 */
static inline float maf_update(maf_t *maf, float x)
{
    /* subtract oldest value in this slot */
    maf->sum -= maf->buffer[maf->index];
    /* store new value */
    maf->buffer[maf->index] = x;
    maf->sum += x;
    /* advance index */
    maf->index++;
    if (maf->index >= maf->window_size) {
        maf->index = 0;
        maf->filled = 1;
    }
    /* return average */
    if (maf->filled) {
        return maf->sum / (float)maf->window_size;
    } else {
        return maf->sum / (float)maf->index; // partial window
    }
}

#ifdef __cplusplus
}
#endif
