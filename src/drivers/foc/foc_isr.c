/**
 * @file foc_isr.c
 * @brief FOC 30kHz ISR — TIM1 update interrupt
 *
 * Real-time heart of motor control. Runs at PWM frequency:
 *   ADC read → Clarke → Park → PI → iPark → SVPWM → PWM duty
 *   + Observer step (angle estimation)
 */

#include "foc_isr.h"
#include "foc_core.h"
#include "foc_adc.h"
#include "foc_pwm.h"
#include "observer.h"
#include "common/motor_params.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stm32g4xx_ll_tim.h>

LOG_MODULE_REGISTER(foc_isr, LOG_LEVEL_INF);

/* ── Global FOC state (ISR-owned) ────────────────────── */

static foc_t g_motor_foc;
static observer_t g_motor_observer;
static volatile bool g_isr_running = false;
static volatile uint32_t g_isr_count = 0;
static volatile bool g_observer_override = true;  /* ISR updates theta from observer */

/* ── Public accessors ────────────────────────────────── */

foc_t *foc_isr_get_foc(void)
{
    return &g_motor_foc;
}

observer_t *foc_isr_get_observer(void)
{
    return &g_motor_observer;
}

uint32_t foc_isr_get_count(void)
{
    return g_isr_count;
}

bool foc_isr_is_running(void)
{
    return g_isr_running;
}

/* ── TIM1 Update ISR ─────────────────────────────────── */

static void foc_tim1_isr(void *arg)
{
    (void)arg;

    /* Clear update interrupt flag */
    LL_TIM_ClearFlag_UPDATE(TIM1);

    if (!g_isr_running) {
        return;
    }

    /* Run FOC current loop:
     * ADC read → Clarke → Park → PI(id/iq) → iPark → SVPWM → PWM duty
     */
    foc_current_loop(&g_motor_foc);

    /* Run sliding mode observer (angle estimation) */
    observer_step(&g_motor_observer,
                  g_motor_foc.state.v_alpha,
                  g_motor_foc.state.v_beta,
                  g_motor_foc.state.i_alpha,
                  g_motor_foc.state.i_beta);

    /* Update FOC angle from observer (unless motor_ctrl overrides theta) */
    if (g_observer_override) {
        g_motor_foc.state.theta_elec = g_motor_observer.theta_elec;
        g_motor_foc.state.omega_elec = g_motor_observer.omega_elec;
    }

    g_isr_count++;
}

/* ── Init ────────────────────────────────────────────── */

void foc_isr_init(void)
{
    LOG_INF("Initializing FOC ISR");

    /* Initialize FOC with motor parameters */
    foc_init(&g_motor_foc, &g_motor_params);

    /* Read initial bus voltage from ADC2 regular channel */
    float vbus_init = foc_adc_read_vbus_blocking();
    g_motor_foc.state.v_bus = vbus_init;

    /* Calibrate ADC offsets (motor must be disabled) */
    int16_t ia_off, ib_off, ic_off;
    foc_adc_get_offsets(&ia_off, &ib_off, &ic_off);
    g_motor_foc.state.adc_ia_offset = ia_off;
    g_motor_foc.state.adc_ib_offset = ib_off;
    g_motor_foc.state.adc_ic_offset = ic_off;

    /* Initialize observer with Workbench gains */
    observer_init(&g_motor_observer,
                  g_motor_params.rs,
                  g_motor_params.ls,
                  1.0f / FOC_PWM_FREQ_HZ,
                  g_motor_params.observer_gain1,
                  g_motor_params.observer_gain2,
                  g_motor_params.pll_kp,
                  g_motor_params.pll_ki);

    /* Configure and enable TIM1 update interrupt */
    IRQ_CONNECT(TIM1_UP_TIM16_IRQn, 0, foc_tim1_isr, NULL, 0);
    irq_enable(TIM1_UP_TIM16_IRQn);
    LL_TIM_EnableIT_UPDATE(TIM1);

    g_isr_running = false;
    g_isr_count = 0;

    LOG_INF("FOC ISR ready (30kHz, TIM1_UP_IRQn, priority 0)");
    LOG_INF("  ADC offsets: Ia=%d Ib=%d Ic=%d", ia_off, ib_off, ic_off);
    LOG_INF("  Vbus=%.2f V", (double)vbus_init);
}

/* ── Start/Stop ──────────────────────────────────────── */

void foc_isr_start(void)
{
    if (g_isr_running) {
        LOG_WRN("FOC ISR already running");
        return;
    }
    g_isr_running = true;
    g_isr_count = 0;
    /* Start TIM1 counter to generate TRGO for ADC trigger */
    LL_TIM_EnableCounter(TIM1);
    LOG_INF("FOC ISR started (TIM1 counting)");
}

void foc_isr_stop(void)
{
    if (!g_isr_running) {
        return;
    }
    g_isr_running = false;
    foc_pwm_set_duty(0.0f, 0.0f, 0.0f);
    /* Stop TIM1 counter */
    LL_TIM_DisableCounter(TIM1);
    LOG_INF("FOC ISR stopped");
}

void foc_isr_set_vbus(float vbus)
{
    g_motor_foc.state.v_bus = vbus;
}

void foc_isr_set_observer_override(bool override)
{
    g_observer_override = override;
}

bool foc_isr_get_observer_override(void)
{
    return g_observer_override;
}
