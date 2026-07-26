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
#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>

/* ── Ring buffer (BSS, zero-init) ─────────────────────── */

static scope_sample_t g_buf[SCOPE_DEPTH];
static volatile bool  g_active = false;
static uint16_t       g_head = 0;
static uint16_t       g_count = 0;
static uint8_t        g_decim = 1;   /* record every N ticks */
static uint8_t        g_decim_cnt = 0;

/* ── Stream mode: SPSC ring + consumer thread ─────────── */
static volatile bool  g_streaming = false;
static uint8_t        g_stream_decim = 50;
static uint8_t        g_stream_cnt = 0;
static const struct device *g_stream_uart;

/* SPSC lock-free ring buffer (producer: motor_ctrl, consumer: stream thread) */
static scope_sample_t g_stream_ring[SCOPE_STREAM_SLOTS];
static volatile uint8_t g_stream_head = 0;  /* written by producer */
static volatile uint8_t g_stream_tail = 0;  /* written by consumer */

/* Consumer thread */
#define STREAM_THREAD_STACK 512
#define STREAM_THREAD_PRIO  K_PRIO_PREEMPT(14)  /* preemptible: won't block motor_ctrl */
K_THREAD_STACK_DEFINE(stream_thread_stack, STREAM_THREAD_STACK);
static struct k_thread stream_thread_data;

static void stream_consumer_thread(void *a, void *b, void *c)
{
    ARG_UNUSED(a); ARG_UNUSED(b); ARG_UNUSED(c);
    uint8_t seq = 0;
    /* Binary frame: [0xA5][seq][9×int16 LE] = 20 bytes */
    uint8_t frame[20];
    frame[0] = 0xA5;  /* sync byte */

    while (1) {
        k_sleep(K_MSEC(1));

        if (g_stream_tail == g_stream_head) {
            continue;
        }

        /* Drain all available samples */
        while (g_stream_tail != g_stream_head) {
            scope_sample_t s = g_stream_ring[g_stream_tail];
            g_stream_tail = (g_stream_tail + 1) % SCOPE_STREAM_SLOTS;

            frame[1] = seq++;
            /* Pack 9×int16 little-endian */
            for (int i = 0; i < SCOPE_CHANNELS; i++) {
                frame[2 + i * 2]     = (uint8_t)(s.ch[i] & 0xFF);
                frame[2 + i * 2 + 1] = (uint8_t)((s.ch[i] >> 8) & 0xFF);
            }
            for (int i = 0; i < 20; i++) {
                uart_poll_out(g_stream_uart, frame[i]);
            }
        }
    }
}

/* ── API ──────────────────────────────────────────────── */

void scope_start(void)
{
    scope_start_decim(1);
}

void scope_start_decim(uint8_t decimation)
{
    g_head = 0;
    g_count = 0;
    g_decim = (decimation < 1) ? 1 : decimation;
    g_decim_cnt = 0;
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
    /* ── Stream mode: quantize + push to SPSC ring (non-blocking) ── */
    if (g_streaming) {
        if (++g_stream_cnt < g_stream_decim) return;
        g_stream_cnt = 0;

        uint8_t next = (g_stream_head + 1) % SCOPE_STREAM_SLOTS;
        if (next == g_stream_tail) return;  /* ring full, drop sample */

        foc_t *foc = foc_isr_get_foc();
        observer_t *obs = foc_isr_get_observer();

        scope_sample_t *s = &g_stream_ring[g_stream_head];
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

        g_stream_head = next;  /* publish after write */
        return;
    }

    /* ── Ring buffer mode (original) ──────────────────── */
    if (!g_active) return;
    if (++g_decim_cnt < g_decim) return;
    g_decim_cnt = 0;

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

/* ══════════════════════════════════════════════════════
 *  Stream mode: SPSC ring + consumer thread
 * ══════════════════════════════════════════════════════ */

static void stream_uart_puts(const char *s)
{
    while (*s) {
        uart_poll_out(g_stream_uart, *s++);
    }
}

void scope_stream_start(uint8_t decimation)
{
    if (!g_stream_uart) {
        g_stream_uart = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
        if (!device_is_ready(g_stream_uart)) return;
    }

    g_stream_decim = (decimation < 1) ? 1 : decimation;
    g_stream_cnt = 0;
    g_stream_head = 0;
    g_stream_tail = 0;
    g_streaming = true;

    /* Emit binary mode header */
    stream_uart_puts("#SCOPE_BIN\n");

    /* Start consumer thread on first use */
    static bool thread_started = false;
    if (!thread_started) {
        k_thread_create(&stream_thread_data, stream_thread_stack,
                        STREAM_THREAD_STACK, stream_consumer_thread,
                        NULL, NULL, NULL,
                        STREAM_THREAD_PRIO, 0, K_NO_WAIT);
        k_thread_name_set(&stream_thread_data, "scope_tx");
        thread_started = true;
    }
}

void scope_stream_stop(void)
{
    g_streaming = false;
    /* Wait for consumer to drain remaining samples */
    k_sleep(K_MSEC(20));
    if (g_stream_uart) {
        stream_uart_puts("#SCOPE_END\n");
    }
}

bool scope_is_streaming(void)
{
    return g_streaming;
}
