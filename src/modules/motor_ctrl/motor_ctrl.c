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
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(motor_ctrl, LOG_LEVEL_INF);

/* ── Constants (from Workbench) ──────────────────────── */

#define SPEED_LOOP_HZ       1000
#define SPEED_LOOP_PERIOD   K_MSEC(1)

/* Startup sequence parameters (grouped for easy tuning) */
typedef struct {
    uint32_t phase1_duration_ms;   /* Alignment phase duration */
    float    phase1_align_current; /* Alignment current [A] */
    uint32_t phase2_duration_ms;   /* Forced ramp duration */
    float    phase2_final_speed;   /* Final speed for ramp [RPM] */
    float    phase2_current;       /* Ramp current [A] */
    float    obs_min_speed_rpm;    /* Min speed for observer convergence */
    uint32_t consecutive_ok;       /* Consecutive OK count for switchover */
} startup_config_t;

static const startup_config_t g_startup_config = {
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
    pid_set_parameters(pid, 10.66f, 0.0343f, 0.0f, 0.8f, 0.8f);
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
}

static void transition_to_closed_loop(motor_instance_t *m)
{
    m->state = MOTOR_STATE_RUN;
    LOG_INF("Motor %u: observer converged → closed loop", m->motor_id);
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
        m->foc->state.theta_elec += omega_ref * (1.0f / SPEED_LOOP_HZ);
        while (m->foc->state.theta_elec >= 6.283185307f)
            m->foc->state.theta_elec -= 6.283185307f;

        m->foc->state.i_d_ref = 0.0f;
        m->foc->state.i_q_ref = g_startup_config.phase2_current;

        /* Run observer at 1kHz (backward Euler, unconditionally stable) */
        observer_step(m->obs,
                      m->foc->state.v_alpha, m->foc->state.v_beta,
                      m->foc->state.i_alpha, m->foc->state.i_beta);

        float est_speed = foc_rads_to_rpm(
            __builtin_fabsf(m->obs->omega_elec), m->foc->config->pole_pairs);

        if (est_speed > g_startup_config.obs_min_speed_rpm) {
            m->consecutive_ok++;
            if (m->consecutive_ok >= g_startup_config.consecutive_ok) {
                transition_to_closed_loop(m);
                m->foc->state.theta_elec = m->obs->theta_elec;
            }
        } else {
            m->consecutive_ok = 0;
        }

        /* Phase2 timeout: if observer hasn't converged, STOP (safety) */
        if (m->phase_elapsed_ms >= m->phase_duration_ms &&
            m->state != MOTOR_STATE_RUN) {
            LOG_WRN("Motor %u: Phase2 timeout, observer not converged → STOP",
                    m->motor_id);
            foc_isr_set_observer_override(true);
            foc_isr_stop();
            foc_pwm_disable();
            m->foc->state.i_q_ref = 0.0f;
            m->foc->state.i_d_ref = 0.0f;
            m->state = MOTOR_STATE_IDLE;
        }
        break;
    }

    case MOTOR_STATE_RUN: {
        foc_isr_set_observer_override(true);  /* ISR owns theta from observer */

        /* Run observer at 1kHz (backward Euler) */
        observer_step(m->obs,
                      m->foc->state.v_alpha, m->foc->state.v_beta,
                      m->foc->state.i_alpha, m->foc->state.i_beta);

        float speed_meas = foc_rads_to_rpm(
            __builtin_fabsf(m->obs->omega_elec), m->foc->config->pole_pairs);

        float iq_ref = pid_calculate(&m->speed_pid,
                                     m->target_speed_rpm, speed_meas, 0.0f,
                                     1.0f / SPEED_LOOP_HZ);

        /* Iq clamp: prevent excessive current (max 2A) */
        if (iq_ref > 2.0f) iq_ref = 2.0f;
        if (iq_ref < -2.0f) iq_ref = -2.0f;

        m->foc->state.i_d_ref = 0.0f;
        m->foc->state.i_q_ref = iq_ref;
        m->foc->state.theta_elec = m->obs->theta_elec;
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
        .speed_rpm = foc_rads_to_rpm(
            __builtin_fabsf(m->obs->omega_elec), m->foc->config->pole_pairs),
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
