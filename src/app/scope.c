/**
 * @file scope.c
 * @brief High-resolution diagnostic scope implementation
 *
 * Ring buffer captures 9 FOC channels at 1kHz from motor_ctrl thread.
 * No ISR involvement — reads shared state (single-float atomic on CM4).
 */

#include "scope.h"
#include "drivers/foc/foc_isr.h"
#include "libs/foc/foc_types.h"
#include "libs/foc/observer.h"
#include <string.h>

/* ── Ring buffer (BSS, zero-init) ─────────────────────── */

static scope_sample_t g_buf[SCOPE_DEPTH];
static volatile bool  g_active = false;
static uint16_t       g_head = 0;
static uint16_t       g_count = 0;

/* ── API ──────────────────────────────────────────────── */

void scope_start(void)
{
    g_head = 0;
    g_count = 0;
    memset(g_buf, 0, sizeof(g_buf));
    g_active = true;
}

void scope_stop(void)
{
    g_active = false;
}

bool scope_is_active(void)
{
    return g_active;
}

void scope_record(int8_t motor_state)
{
    if (!g_active) return;

    foc_t *foc = foc_isr_get_foc();
    observer_t *obs = foc_isr_get_observer();

    scope_sample_t *s = &g_buf[g_head];

    /* Quantize: physical → int16 with scaling */
    s->ch[SC_THETA_FOC] = (int16_t)(foc->state.theta_elec * 1000.0f);
    s->ch[SC_THETA_OBS] = (int16_t)(obs->theta_elec * 1000.0f);
    s->ch[SC_OMEGA]     = (int16_t)(obs->omega_elec * 10.0f);
    s->ch[SC_ID_MA]     = (int16_t)(foc->state.i_d * 1000.0f);
    s->ch[SC_IQ_MA]     = (int16_t)(foc->state.i_q * 1000.0f);
    s->ch[SC_IQ_REF_MA] = (int16_t)(foc->state.i_q_ref * 1000.0f);
    s->ch[SC_RPM]       = (int16_t)(obs->speed_rpm_filt);
    float bemf = __builtin_sqrtf(obs->e_alpha * obs->e_alpha
                               + obs->e_beta * obs->e_beta);
    s->ch[SC_BEMF_CV]   = (int16_t)(bemf * 100.0f);
    s->ch[SC_STATE]     = motor_state;

    g_head = (g_head + 1) % SCOPE_DEPTH;
    if (g_count < SCOPE_DEPTH) g_count++;
}

uint16_t scope_get_count(void)
{
    return g_count;
}

const scope_sample_t *scope_get_buf(void)
{
    return g_buf;
}
