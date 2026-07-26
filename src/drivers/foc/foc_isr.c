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
#include <stm32g4xx_ll_adc.h>

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

    /* ── Overcurrent protection ──────────────────────────
     * If |I_alpha| or |I_beta| exceeds 2A, immediately disable PWM.
     * This prevents motor burnout when observer/speed loop diverges. */
    {
        float i_mag = g_motor_foc.state.i_alpha * g_motor_foc.state.i_alpha
                    + g_motor_foc.state.i_beta * g_motor_foc.state.i_beta;
        if (i_mag > 4.0f) {  /* 2A squared = 4.0 */
            foc_pwm_set_duty(0.0f, 0.0f, 0.0f);
            g_isr_running = false;
            LL_TIM_DisableCounter(TIM1);
        }
    }

    /* Observer runs at 1kHz in motor_ctrl thread.
     * ISR extrapolates theta using omega for fresh angle at 30kHz. */
    if (g_observer_override) {
        /* Extrapolate: theta += omega * dt_since_last_observer_update
         * At 30kHz ISR, observer updates every 30 ticks. */
        static uint8_t obs_tick = 0;
        obs_tick++;
        float extrapolate_dt = (float)obs_tick / FOC_PWM_FREQ_HZ;
        float theta_ext = g_motor_observer.theta_elec
                        + g_motor_observer.omega_elec * extrapolate_dt;
        /* Wrap */
        while (theta_ext >= 6.283185307f) theta_ext -= 6.283185307f;
        while (theta_ext < 0.0f) theta_ext += 6.283185307f;
        g_motor_foc.state.theta_elec = theta_ext;
        g_motor_foc.state.omega_elec = g_motor_observer.omega_elec;
        if (obs_tick >= 30) obs_tick = 0;  /* reset every 1ms */
    }

    g_isr_count++;
}

/* ── Init ────────────────────────────────────────────── */

void foc_isr_init(void)
{
    LOG_INF("Initializing FOC ISR");

    /* Initialize FOC with motor parameters */
    foc_init(&g_motor_foc, &g_motor_params);

    /* Set fixed bus voltage (read from ADC2 in current loop at runtime) */
    g_motor_foc.state.v_bus = 24.0f;

    /* Calibrate ADC offsets (motor must be disabled) */
    int16_t ia_off, ib_off, ic_off;
    foc_adc_get_offsets(&ia_off, &ib_off, &ic_off);
    g_motor_foc.state.adc_ia_offset = ia_off;
    g_motor_foc.state.adc_ib_offset = ib_off;
    g_motor_foc.state.adc_ic_offset = ic_off;

    /* Initialize observer — runs at 1kHz in motor_ctrl thread */
    observer_init(&g_motor_observer,
                  g_motor_params.rs,
                  g_motor_params.ls,
                  1.0f / 1000.0f,   /* dt = 1ms */
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
    LOG_INF("  Vbus=%.2f V (initial)", (double)g_motor_foc.state.v_bus);
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
    /* Re-arm injected ADC trigger (JADSTART) before TIM1 starts */
    LL_ADC_INJ_StartConversion(ADC1);
    LL_ADC_INJ_StartConversion(ADC2);
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
