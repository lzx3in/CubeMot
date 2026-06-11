/**
 * @file foc_core.c
 * @brief FOC current control loop implementation
 *
 * Runs at PWM frequency (30kHz). Performs:
 *   ADC read → Clarke → Park → PI(D/Q) → iPark → SVPWM → duty update
 */

#include "foc_core.h"
#include "foc_pwm.h"
#include "foc_adc.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(foc_core, LOG_LEVEL_INF);

/* ── Default PI gains for current loop ──────────────────
 * From Workbench: Kp=3378, Ki=2252, Div=1024/4096
 * Normalized: Kp = 3378/1024 ≈ 3.30, Ki = 2252/4096 ≈ 0.55
 */

static void foc_pid_init_current(PID_t *pid, float kp, float ki)
{
    pid_init(pid, PID_MODE_DERIVATIV_CALC, 1.0f / 30000.0f);
    pid_set_parameters(pid, kp, ki, 0.0f, 10.0f, 10.0f);
}

/* ── Global PID controllers ───────────────────────────── */

static PID_t g_pid_id;   // D-axis (flux) current controller
static PID_t g_pid_iq;   // Q-axis (torque) current controller

/* ── Init ─────────────────────────────────────────────── */

void foc_init(foc_t *foc, const foc_motor_config_t *config)
{
    foc->config = config;
    memset(&foc->state, 0, sizeof(foc->state));

    /* Initialize PI controllers with Workbench gains */
    float kp = 3378.0f / 1024.0f;  // 3.299
    float ki = 2252.0f / 4096.0f;  // 0.550
    foc_pid_init_current(&g_pid_id, kp, ki);
    foc_pid_init_current(&g_pid_iq, kp, ki);

    foc->initialized = true;
    LOG_INF("FOC initialized: %u pole pairs, Rs=%.2fΩ, Ls=%.3fmH",
            config->pole_pairs, (double)config->rs,
            (double)(config->ls * 1000.0f));
}

/* ── Current loop ─────────────────────────────────────── */

void foc_current_loop(foc_t *foc)
{
    if (!foc->initialized) return;

    foc_state_t *s = &foc->state;
    foc_adc_raw_t raw;

    /* 1. Read ADC currents */
    foc_adc_read_raw(&raw);
    s->adc_ia = raw.ia;
    s->adc_ib = raw.ib;
    s->adc_ic = raw.ic;

    /* 2. Convert to Amperes */
    float ia = foc_adc_to_current(raw.ia, s->adc_ia_offset);
    float ib = foc_adc_to_current(raw.ib, s->adc_ib_offset);
    /* ic = foc_adc_to_current(raw.ic, ...) — available for 3-phase diagnostics */

    /* 3. Clarke: abc → αβ */
    float i_alpha = ia;
    float i_beta  = (ia + 2.0f * ib) * 0.57735026919f; // 1/√3

    s->i_alpha = i_alpha;
    s->i_beta  = i_beta;

    /* 4. Park: αβ → dq */
    float sin_t, cos_t;
    fast_sincos(s->theta_elec, &sin_t, &cos_t);
    float i_d =  i_alpha * cos_t + i_beta * sin_t;
    float i_q = -i_alpha * sin_t + i_beta * cos_t;

    s->i_d = i_d;
    s->i_q = i_q;

    /* 5. PI controllers */
    float dt = 1.0f / 30000.0f;
    s->v_d = pid_calculate(&g_pid_id, s->i_d_ref, i_d, 0.0f, dt);
    s->v_q = pid_calculate(&g_pid_iq, s->i_q_ref, i_q, 0.0f, dt);

    /* 6. Inverse Park: dq → αβ */
    s->v_alpha = s->v_d * cos_t - s->v_q * sin_t;
    s->v_beta  = s->v_d * sin_t + s->v_q * cos_t;

    /* 7. SVPWM → duty cycles */
    svpwm_duty_t duty;
    svpwm_minmax(s->v_alpha, s->v_beta, s->v_bus, &duty);
    svpwm_clamp(&duty);

    s->duty_a = duty.duty_a;
    s->duty_b = duty.duty_b;
    s->duty_c = duty.duty_c;

    /* 8. Write PWM registers */
    foc_pwm_set_duty(duty.duty_a, duty.duty_b, duty.duty_c);
}

void foc_apply_duty(foc_t *foc)
{
    (void)foc;
    /* Duty already written in foc_current_loop */
}
