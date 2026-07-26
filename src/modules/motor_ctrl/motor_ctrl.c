/**
 * @file motor_ctrl.c
 * @brief Motor control module: speed loop + startup sequence (multi-instance)
 *
 * Architecture:
 *   - Supports up to MAX_MOTORS instances
 *   - Each motor has independent FOC core, observer, speed PID
 *   - Speed loop runs at 1kHz for all motors in one thread
 *   - Startup sequence: Alignment → Forced Ramp → Switchover → Closed Loop
 *   - Publishes motor_state via msghub at 100Hz per motor
 */

#include "modules/motor_ctrl/motor_ctrl.h"
#include "foc_core.h"
#include "observer.h"
#include "foc_pwm.h"
#include "foc_adc.h"
#include "foc_isr.h"
#include "pid.h"
#include "topics/topics.h"
#include "common_time.h"
#include "common_device.h"
#include "common_error.h"
#include "common/motor_params.h"
#include "scope.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>
LOG_MODULE_REGISTER(motor_ctrl, LOG_LEVEL_INF);

/* ── Constants (from Workbench) ──────────────────────── */

#define SPEED_LOOP_HZ       1000
#define SPEED_LOOP_PERIOD   K_MSEC(1)

/* PLL tracking gains (15Hz BW): switched in at RUN transition */
#define PLL_TRACK_KP        132.0f
#define PLL_TRACK_KI        8874.0f

/* Startup sequence parameters (grouped for easy tuning) */
static startup_config_t g_startup_config = {
    .phase1_duration_ms  = 1000,
    .phase1_align_current = 0.8f,
    .phase2_duration_ms  = 4000,   /* 4s ramp for observer convergence */
    .phase2_final_speed  = 582.0f,
    .phase2_current      = 0.8f,
    .obs_min_speed_rpm   = 200.0f, /* relaxed: was 524 */
    .consecutive_ok      = 10,     /* tolerate ADC glitches: was 2 */
};

/* ── Motor instance structure ────────────────────────── */

typedef struct {
    bool initialized;
    uint8_t motor_id;
    foc_t *foc;             /* Shared with foc_isr (30kHz ISR context) */
    observer_t *obs;        /* Shared with foc_isr */
    PID_t speed_pid;
    motor_run_state_t state;

    // Startup sequence state
    uint32_t phase_elapsed_ms;
    uint32_t phase_duration_ms;
    uint32_t consecutive_ok;
    float target_speed_rpm;

    // Pending command
    motor_cmd_t pending_cmd;
    bool cmd_pending;

    // msghub
    msghub_publisher_t state_pub;
} motor_instance_t;

static motor_instance_t g_motors[MAX_MOTORS];
static uint8_t g_motor_count = 0;
static msghub_subscriber_t g_cmd_sub;
static bool g_hw_initialized = false;

/* ── Speed PID gains (from Workbench) ────────────────── */

static void speed_pid_init(PID_t *pid)
{
    pid_init(pid, PID_MODE_DERIVATIV_CALC_NO_SP, 1.0f / SPEED_LOOP_HZ);
    /* Speed PID (1kHz loop):
     * Kp=0.001 → 500 RPM error = 0.5A (gentle proportional)
     * Ki=0.002 → integral converges in ~2s at 250 RPM error
     * integral_limit=1000 (RPM·s), output anti-windup is effective clamp
     * output_limit=0.8A (observer dI/dt removed, noise floor lowered) */
    pid_set_parameters(pid, 0.001f, 0.002f, 0.0f, 1000.0f, 0.8f);
}

/* ── Init ─────────────────────────────────────────────── */

int motor_ctrl_init(uint8_t motor_id, const foc_motor_config_t *config,
                    foc_t *foc, observer_t *obs)
{
    if (motor_id >= MAX_MOTORS) {
        LOG_ERR("motor_id %u >= MAX_MOTORS %u", motor_id, MAX_MOTORS);
        return -1;
    }

    motor_instance_t *m = &g_motors[motor_id];
    if (m->initialized) {
        LOG_WRN("Motor %u already initialized", motor_id);
        return -1;
    }

    m->motor_id = motor_id;

    /* Use shared FOC and observer from ISR */
    m->foc = foc;
    m->obs = obs;

    /* Initialize speed PID */
    speed_pid_init(&m->speed_pid);

    /* Create publisher for this motor */
    m->state_pub = msghub_create_publisher(MSGHUB_TOPIC(motor_state));

    m->state = MOTOR_STATE_IDLE;
    m->initialized = true;
    g_motor_count++;

    LOG_INF("Motor %u initialized (%u total, shared FOC)", motor_id, g_motor_count);
    return 0;
}

/* ── Startup phase management (per-instance) ─────────── */

static void start_phase1(motor_instance_t *m)
{
    m->state = MOTOR_STATE_ALIGN;
    m->phase_elapsed_ms = 0;
    m->phase_duration_ms = g_startup_config.phase1_duration_ms;
    m->consecutive_ok = 0;

    m->foc->state.theta_elec = 0.0f;
    m->foc->state.i_d_ref = g_startup_config.phase1_align_current;
    m->foc->state.i_q_ref = 0.0f;
}

static void start_phase2(motor_instance_t *m)
{
    m->state = MOTOR_STATE_START;
    m->phase_elapsed_ms = 0;
    m->phase_duration_ms = g_startup_config.phase2_duration_ms;

    /* Sync observer to current forced angle (not zero!) */
    observer_force_angle(m->obs, m->foc->state.theta_elec);
    m->obs->i_alpha_hat = 0.0f;
    m->obs->i_beta_hat = 0.0f;
    m->obs->e_alpha = 0.0f;
    m->obs->e_beta = 0.0f;

    /* Restore acquisition PLL gains (5Hz) for reliable lock-in */
    m->obs->pll_kp = g_motor_params.pll_kp;
    m->obs->pll_ki = g_motor_params.pll_ki;
}

static void transition_to_closed_loop(motor_instance_t *m)
{
    m->state = MOTOR_STATE_RUN;

    /* Switch PLL to high-bandwidth tracking gains (15Hz) */
    m->obs->pll_kp = PLL_TRACK_KP;
    m->obs->pll_ki = PLL_TRACK_KI;

    LOG_INF("Motor %u: observer converged → closed loop (PLL 15Hz)", m->motor_id);
}

/* ── Process command for one motor ───────────────────── */

static void process_command(motor_instance_t *m, const motor_cmd_t *cmd)
{
    switch (cmd->cmd) {
    case MOTOR_CMD_START:
        if (m->state == MOTOR_STATE_IDLE) {
            m->foc->state.i_q_ref = 0.0f;
            m->foc->state.i_d_ref = 0.0f;
            m->target_speed_rpm = cmd->target_speed_rpm;
            foc_isr_set_observer_override(false);  /* motor_ctrl controls theta */
            foc_isr_start();
            foc_pwm_enable();
            start_phase1(m);
            LOG_INF("Motor %u START: target=%d RPM",
                    m->motor_id, (int)cmd->target_speed_rpm);
        } else {
            LOG_WRN("Motor %u: START ignored (state=%d, not IDLE)",
                    m->motor_id, m->state);
        }
        break;
    case MOTOR_CMD_SET_SPEED:
        if (m->state == MOTOR_STATE_RUN) {
            float old = m->target_speed_rpm;
            m->target_speed_rpm = cmd->target_speed_rpm;
            LOG_INF("Motor %u speed: %.0f → %.0f RPM",
                    m->motor_id, (double)old, (double)cmd->target_speed_rpm);
        } else {
            LOG_WRN("Motor %u: SET_SPEED ignored (state=%d, not RUN)",
                    m->motor_id, m->state);
        }
        break;
    case MOTOR_CMD_STOP:
        if (m->state != MOTOR_STATE_IDLE) {
            foc_isr_set_observer_override(true);  /* ISR resumes theta control */
            foc_isr_stop();
            foc_pwm_disable();
            m->foc->state.i_q_ref = 0.0f;
            m->foc->state.i_d_ref = 0.0f;
            pid_reset_integral(&m->speed_pid);
            m->obs->speed_rpm_filt = 0.0f;  /* reset for next start */
            m->state = MOTOR_STATE_IDLE;
            LOG_INF("Motor %u STOP", m->motor_id);
        }
        break;
    case MOTOR_CMD_EMERGENCY:
        foc_isr_set_observer_override(true);
        foc_isr_stop();
        foc_pwm_disable();
        m->foc->state.i_q_ref = 0.0f;
        m->foc->state.i_d_ref = 0.0f;
        m->state = MOTOR_STATE_IDLE;
        LOG_ERR("Motor %u emergency stop!", m->motor_id);
        break;
    }
}

/* ── Run one speed loop iteration for a motor ────────── */

static void run_speed_loop(motor_instance_t *m)
{
    switch (m->state) {
    case MOTOR_STATE_IDLE:
        break;

    case MOTOR_STATE_ALIGN:
        foc_isr_set_observer_override(false);  /* motor_ctrl owns theta */
        m->phase_elapsed_ms += 1;
        if (m->phase_elapsed_ms >= m->phase_duration_ms) {
            start_phase2(m);
        }
        break;

    case MOTOR_STATE_START: {
        foc_isr_set_observer_override(false);  /* motor_ctrl owns theta */
        m->phase_elapsed_ms += 1;

        float progress = (float)m->phase_elapsed_ms / (float)m->phase_duration_ms;
        if (progress > 1.0f) progress = 1.0f;
        float ramp_speed = progress * g_startup_config.phase2_final_speed;

        float omega_ref = foc_rpm_to_rads(ramp_speed, m->foc->config->pole_pairs);

        /* Open-loop theta ramp (drives the motor) */
        m->foc->state.theta_elec += omega_ref * (1.0f / SPEED_LOOP_HZ);
        while (m->foc->state.theta_elec >= 6.283185307f)
            m->foc->state.theta_elec -= 6.283185307f;

        m->foc->state.i_d_ref = 0.0f;
        m->foc->state.i_q_ref = g_startup_config.phase2_current;

        /* Run observer at 1kHz (voltage model BEMF + PLL) */
        observer_step(m->obs,
                      m->foc->state.v_alpha, m->foc->state.v_beta,
                      m->foc->state.i_alpha, m->foc->state.i_beta);

        if (ramp_speed < 200.0f) {
            /* Phase A: force PLL theta to open-loop (give it direction) */
            m->obs->theta_elec = m->foc->state.theta_elec;
            m->obs->pll_integral = omega_ref;
            m->obs->omega_elec = omega_ref;
            m->obs->e_alpha = 0.0f;
            m->obs->e_beta = 0.0f;
            m->consecutive_ok = 0;
        } else {
            /* Phase B: release PLL — let it free-run.
             * Check angle error between PLL and open-loop. */
            float angle_err = m->obs->theta_elec - m->foc->state.theta_elec;
            /* Normalize to [-pi, pi] */
            while (angle_err > 3.14159f) angle_err -= 6.28318f;
            while (angle_err < -3.14159f) angle_err += 6.28318f;

            if (__builtin_fabsf(angle_err) < 0.5f) {  /* < ~30 degrees */
                m->consecutive_ok++;
            } else {
                m->consecutive_ok = 0;
            }

            if (m->consecutive_ok >= 100) {  /* 100ms locked */
                transition_to_closed_loop(m);
                /* No theta jump needed — PLL already aligned */
            }
        }

        /* Phase2 timeout safety */
        if (m->phase_elapsed_ms >= m->phase_duration_ms &&
            m->state != MOTOR_STATE_RUN) {
            LOG_WRN("Motor %u: Phase2 timeout → STOP", m->motor_id);
            foc_isr_stop();
            foc_pwm_disable();
            m->foc->state.i_q_ref = 0.0f;
            m->foc->state.i_d_ref = 0.0f;
            m->state = MOTOR_STATE_IDLE;
        }
        break;
    }

    case MOTOR_STATE_RUN: {
        /* True closed-loop FOC: observer theta drives Park transform */
        foc_isr_set_observer_override(true);  /* ISR uses observer theta */

        /* Pre-filter currents for observer (remove 30kHz aliasing)
         * ISR writes instantaneous i_alpha/i_beta at 30kHz (with PWM ripple).
         * Observer at 1kHz would alias ripple into BEMF band.
         * LPF alpha=0.2 → ~32Hz cutoff, attenuates aliased components. */
        static float i_alpha_filt = 0.0f;
        static float i_beta_filt  = 0.0f;
        i_alpha_filt += 0.2f * (m->foc->state.i_alpha - i_alpha_filt);
        i_beta_filt  += 0.2f * (m->foc->state.i_beta  - i_beta_filt);

        /* Run observer at 1kHz with filtered currents */
        observer_step(m->obs,
                      m->foc->state.v_alpha, m->foc->state.v_beta,
                      i_alpha_filt, i_beta_filt);

        /* Speed estimate (filtered, alpha=0.03 → ~5Hz BW @ 1kHz)
         * Rejects 30kHz switching ripple aliased to 1kHz */
        float speed_raw = foc_rads_to_rpm(
            __builtin_fabsf(m->obs->omega_elec), m->foc->config->pole_pairs);
        m->obs->speed_rpm_filt = 0.97f * m->obs->speed_rpm_filt
                               + 0.03f * speed_raw;
        float speed_meas = m->obs->speed_rpm_filt;

        /* Speed PID → Iq reference */
        float iq_ref = pid_calculate(&m->speed_pid,
                                     m->target_speed_rpm, speed_meas, 0.0f,
                                     1.0f / SPEED_LOOP_HZ);
        if (iq_ref > 2.0f) iq_ref = 2.0f;
        if (iq_ref < -2.0f) iq_ref = -2.0f;

        /* Slew rate limit: max 0.05A per 1ms step (50A/s)
         * Prevents current step that destabilizes observer.
         * 0→0.8A takes 16ms — fast enough for speed loop, gentle for observer. */
        float iq_prev = m->foc->state.i_q_ref;
        float iq_delta = iq_ref - iq_prev;
        const float slew_max = 0.05f;
        if (iq_delta > slew_max)  iq_delta = slew_max;
        if (iq_delta < -slew_max) iq_delta = -slew_max;
        iq_ref = iq_prev + iq_delta;

        m->foc->state.i_d_ref = 0.0f;
        m->foc->state.i_q_ref = iq_ref;
        /* theta_elec is owned by ISR (observer) — don't touch it here */
        break;
    }

    default:
        break;
    }
}

/* ── Publish motor state (per-instance) ──────────────── */

static void publish_state(motor_instance_t *m)
{
    motor_state_t state = {
        .motor_id = m->motor_id,
        .state = (uint8_t)m->state,
        .speed_rpm = m->obs->speed_rpm_filt,
        .i_d = m->foc->state.i_d,
        .i_q = m->foc->state.i_q,
        .v_bus = m->foc->state.v_bus,
        .faults = 0,
        .timestamp = common_get_timestamp_ms(),
    };
    msghub_publish(m->state_pub, &state);
}

/* ── Main thread (runs all motors) ───────────────────── */

void motor_ctrl_thread(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    /* Startup delay: allow FOC hardware (PWM, ADC, ISR) to stabilize */
    k_sleep(K_MSEC(100));

    /* Create subscriber (shared by all motors).
     * Hardware (PWM, ADC, ISR) is already initialized by app_init.
     * ADC offsets are set in motor_ctrl_init() via foc_isr. */
    if (!g_hw_initialized) {
        g_cmd_sub = msghub_create_subscriber(MSGHUB_TOPIC(motor_cmd), 0);
        g_hw_initialized = true;
    }

    LOG_INF("Motor control thread started (%u motors)", g_motor_count);

    uint32_t pub_counter = 0;

    while (1) {
        /* Check for commands */
        bool updated = false;
        msghub_subscriber_check(g_cmd_sub, &updated);
        if (updated) {
            motor_cmd_t cmd;
            msghub_receive(g_cmd_sub, &cmd);

            /* Dispatch to target motor */
            if (cmd.motor_id < MAX_MOTORS && g_motors[cmd.motor_id].initialized) {
                process_command(&g_motors[cmd.motor_id], &cmd);
            } else {
                LOG_WRN("Unknown motor_id %u in command", cmd.motor_id);
            }
        }

        /* Run speed loop for all motors */
        for (uint8_t i = 0; i < MAX_MOTORS; i++) {
            if (g_motors[i].initialized) {
                run_speed_loop(&g_motors[i]);
            }
        }

        /* High-res scope capture @ 1kHz (ring buffer OR stream) */
        if (scope_is_active() || scope_is_streaming()) {
            scope_record((int8_t)g_motors[0].state);
        }

        /* Publish states @ 100Hz */
        if (++pub_counter >= 10) {
            pub_counter = 0;
            for (uint8_t i = 0; i < MAX_MOTORS; i++) {
                if (g_motors[i].initialized) {
                    publish_state(&g_motors[i]);
                }
            }
        }

        k_sleep(SPEED_LOOP_PERIOD);
    }
}

/* ── Runtime tuning accessors ─────────────────────── */

PID_t *motor_ctrl_get_speed_pid(uint8_t motor_id)
{
    if (motor_id >= MAX_MOTORS || !g_motors[motor_id].initialized) {
        return NULL;
    }
    return &g_motors[motor_id].speed_pid;
}

void motor_ctrl_set_speed_gains(uint8_t motor_id, float kp, float ki)
{
    if (motor_id >= MAX_MOTORS || !g_motors[motor_id].initialized) {
        return;
    }
    PID_t *pid = &g_motors[motor_id].speed_pid;
    pid_set_parameters(pid, kp, ki, pid->kd,
                       pid->integral_limit, pid->output_limit);
}

const startup_config_t *motor_ctrl_get_startup_cfg(void)
{
    return &g_startup_config;
}

int motor_ctrl_set_startup_param(const char *key, int value)
{
    if (strcmp(key, "align_ms") == 0) {
        g_startup_config.phase1_duration_ms = (uint32_t)value;
    } else if (strcmp(key, "align_ma") == 0) {
        g_startup_config.phase1_align_current = (float)value / 1000.0f;
    } else if (strcmp(key, "ramp_ms") == 0) {
        g_startup_config.phase2_duration_ms = (uint32_t)value;
    } else if (strcmp(key, "ramp_rpm") == 0) {
        g_startup_config.phase2_final_speed = (float)value;
    } else if (strcmp(key, "ramp_ma") == 0) {
        g_startup_config.phase2_current = (float)value / 1000.0f;
    } else {
        return -1;
    }
    return 0;
}
