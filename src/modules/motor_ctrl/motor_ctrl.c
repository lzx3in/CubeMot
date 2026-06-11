/**
 * @file motor_ctrl.c
 * @brief Motor control module: speed loop + startup sequence
 *
 * Speed loop (1kHz thread):
 *   1. Read observer speed estimate
 *   2. Speed PID → Iq_ref
 *   3. Update FOC I_q_ref, I_d_ref = 0
 *
 * Startup sequence:
 *   Phase 1 (1000ms): Alignment — I_d=0.8A, I_q=0, θ=0
 *   Phase 2 (1164ms): Forced ramp — θ forced, Iq=0.8A, speed ramp 0→582RPM
 *   Transition:       Observer convergence check
 *   Closed loop:      Speed PID + observer angle for Park transforms
 */

#include "modules/motor_ctrl/motor_ctrl.h"
#include "foc_core.h"
#include "observer.h"
#include "foc_pwm.h"
#include "foc_adc.h"
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

#define PHASE1_DURATION_MS  1000
#define PHASE1_ALIGN_I      0.8f
#define PHASE2_DURATION_MS  1164
#define PHASE2_FINAL_SPEED  582.0f   // RPM
#define PHASE2_I            0.8f
#define TRANSITION_DURATION 25       // ms

#define OBS_MIN_SPEED_RPM   524.0f
#define NB_CONSECUTIVE_OK   2

/* ── Global state ────────────────────────────────────── */

static uint8_t g_motor_id;
static foc_t g_foc;
static observer_t g_obs;
static PID_t g_speed_pid;

static motor_run_state_t g_state = MOTOR_STATE_IDLE;
static motor_cmd_t g_pending_cmd;

static uint32_t g_phase_elapsed_ms;
static uint32_t g_phase_duration_ms;
static uint32_t g_consecutive_ok;

static msghub_publisher_t g_state_pub;
static msghub_subscriber_t g_cmd_sub;
static bool g_cmd_pending = false;
static bool g_enabled = false;

/* ── Speed PID gains (from Workbench) ──────────────────
 * Kp=2730/256=10.66, Ki=562/16384=0.0343, Kd=0
 * Saturation: ±0.8A (Iq_max)
 */

static void speed_pid_init(PID_t *pid)
{
    pid_init(pid, PID_MODE_DERIVATIV_CALC_NO_SP, 1.0f / SPEED_LOOP_HZ);
    pid_set_parameters(pid, 10.66f, 0.0343f, 0.0f, 0.8f, 0.8f);
}

/* ── Init ─────────────────────────────────────────────── */

int motor_ctrl_init(uint8_t motor_id, const foc_motor_config_t *config)
{
    g_motor_id = motor_id;

    /* Initialize FOC core */
    foc_init(&g_foc, config);

    /* Initialize observer (from workbench gains) */
    float dt = 1.0f / 30000.0f;
    float gain1 = -22528.0f / 16384.0f;  // Normalized G1 = -1.375
    float gain2 = 31586.0f / 4096.0f;    // Normalized G2 = 7.71
    float pll_kp = 195.0f / 16384.0f;    // Normalized KP = 0.0119
    float pll_ki = 5.0f / 65535.0f;      // Normalized KI = 7.6e-5

    observer_init(&g_obs,
                  config->rs, config->ls, dt,
                  gain1, gain2, pll_kp, pll_ki);

    /* Initialize speed PID */
    speed_pid_init(&g_speed_pid);

    /* Subscribe to motor_cmd topic */
    g_cmd_sub = msghub_create_subscriber(MSGHUB_TOPIC(motor_cmd), 0);
    g_state_pub = msghub_create_publisher(MSGHUB_TOPIC(motor_state));

    LOG_INF("Motor %u initialized", motor_id);
    return 0;
}

/* ── Startup phase management ────────────────────────── */

static void start_phase1(void)
{
    g_state = MOTOR_STATE_ALIGN;
    g_phase_elapsed_ms = 0;
    g_phase_duration_ms = PHASE1_DURATION_MS;
    g_consecutive_ok = 0;

    /* Force θ=0, align rotor with I_d current */
    g_foc.state.theta_elec = 0.0f;
    g_foc.state.i_d_ref = PHASE1_ALIGN_I;
    g_foc.state.i_q_ref = 0.0f;
}

static void start_phase2(void)
{
    g_state = MOTOR_STATE_START;
    g_phase_elapsed_ms = 0;
    g_phase_duration_ms = PHASE2_DURATION_MS;

    /* Enable observer (it tracks from Phase 2 onwards as per workbench) */
    observer_reset_convergence(&g_obs);
    g_obs.theta_elec = 0.0f;
}

static void transition_to_closed_loop(void)
{
    g_state = MOTOR_STATE_RUN;
    LOG_INF("Observer converged → closed loop");
}

/* ── Speed loop + state machine (1kHz) ───────────────── */

void motor_ctrl_thread(void)
{
    /* Wait for PWM + ADC initialization to complete */
    k_sleep(K_MSEC(100));

    /* Initialize PWM + ADC */
    foc_pwm_init();
    foc_adc_init();

    /* Calibrate ADC offsets */
    int16_t ia_off, ib_off, ic_off;
    foc_adc_get_offsets(&ia_off, &ib_off, &ic_off);
    g_foc.state.adc_ia_offset = ia_off;
    g_foc.state.adc_ib_offset = ib_off;
    g_foc.state.adc_ic_offset = ic_off;

    LOG_INF("Motor control thread started");

    while (1) {
        /* Check for pending commands */
        bool updated = false;
        msghub_subscriber_check(g_cmd_sub, &updated);
        if (updated) {
            motor_cmd_t cmd;
            msghub_receive(g_cmd_sub, &cmd);
            if (cmd.motor_id == g_motor_id) {
                g_pending_cmd = cmd;
                g_cmd_pending = true;
            }
        }

        /* Process commands */
        if (g_cmd_pending) {
            g_cmd_pending = false;
            switch (g_pending_cmd.cmd) {
            case MOTOR_CMD_START:
                if (g_state == MOTOR_STATE_IDLE) {
                    g_foc.state.i_q_ref = 0.0f;
                    g_foc.state.i_d_ref = 0.0f;
                    foc_pwm_enable();
                    start_phase1();
                    LOG_INF("Motor START: target=%d RPM",
                            (int)g_pending_cmd.target_speed_rpm);
                }
                break;
            case MOTOR_CMD_STOP:
                if (g_state != MOTOR_STATE_IDLE) {
                    foc_pwm_disable();
                    g_foc.state.i_q_ref = 0.0f;
                    g_foc.state.i_d_ref = 0.0f;
                    pid_reset_integral(&g_speed_pid);
                    g_state = MOTOR_STATE_IDLE;
                    LOG_INF("Motor STOP");
                }
                break;
            case MOTOR_CMD_EMERGENCY:
                foc_pwm_disable();
                g_foc.state.i_q_ref = 0.0f;
                g_foc.state.i_d_ref = 0.0f;
                g_state = MOTOR_STATE_IDLE;
                LOG_ERR("Emergency stop!");
                break;
            }
        }

        /* ── State Machine ──────────────────────────────── */
        switch (g_state) {
        case MOTOR_STATE_IDLE:
            /* Nothing to do */
            break;

        case MOTOR_STATE_ALIGN:
            g_phase_elapsed_ms += 1;
            if (g_phase_elapsed_ms >= g_phase_duration_ms) {
                start_phase2();
            }
            break;

        case MOTOR_STATE_START: {
            g_phase_elapsed_ms += 1;

            /* Forced ramp: linear speed increase from 0 to PHASE2_FINAL_SPEED */
            float progress = (float)g_phase_elapsed_ms / (float)g_phase_duration_ms;
            if (progress > 1.0f) progress = 1.0f;
            float ramp_speed = progress * PHASE2_FINAL_SPEED;

            /* Force electrical angle: θ += ω_elec * dt */
            float omega_ref = foc_rpm_to_rads(ramp_speed, g_foc.config->pole_pairs);
            g_foc.state.theta_elec += omega_ref * (1.0f / SPEED_LOOP_HZ);
            while (g_foc.state.theta_elec >= 6.283185307f)
                g_foc.state.theta_elec -= 6.283185307f;

            g_foc.state.i_d_ref = 0.0f;
            g_foc.state.i_q_ref = PHASE2_I;

            /* Run observer in parallel (check convergence) */
            observer_step(&g_obs,
                          g_foc.state.v_alpha, g_foc.state.v_beta,
                          g_foc.state.i_alpha, g_foc.state.i_beta);

            /* Check if observer has converged */
            float est_speed = foc_rads_to_rpm(
                __builtin_fabsf(g_obs.omega_elec), g_foc.config->pole_pairs);

            if (est_speed > OBS_MIN_SPEED_RPM) {
                g_consecutive_ok++;
                if (g_consecutive_ok >= NB_CONSECUTIVE_OK) {
                    transition_to_closed_loop();
                    /* Switch angle to observer estimate */
                    g_foc.state.theta_elec = g_obs.theta_elec;
                }
            } else {
                g_consecutive_ok = 0;
            }
            break;
        }

        case MOTOR_STATE_RUN: {
            /* ── Speed loop PID ─────────────────────────── */
            float speed_ref = g_pending_cmd.target_speed_rpm;
            float speed_meas = foc_rads_to_rpm(
                __builtin_fabsf(g_obs.omega_elec), g_foc.config->pole_pairs);

            /* Speed PID: output = Iq reference */
            float iq_ref = pid_calculate(&g_speed_pid,
                                         speed_ref, speed_meas, 0.0f,
                                         1.0f / SPEED_LOOP_HZ);

            g_foc.state.i_d_ref = 0.0f;
            g_foc.state.i_q_ref = iq_ref;

            /* Observer provides the angle */
            g_foc.state.theta_elec = g_obs.theta_elec;

            break;
        }

        default:
            break;
        }

        /* ── Publish motor state @ 100Hz ────────────────── */
        static uint32_t pub_counter = 0;
        if (++pub_counter >= 10) {
            pub_counter = 0;
            motor_state_t state = {
                .motor_id = g_motor_id,
                .state = (uint8_t)g_state,
                .speed_rpm = foc_rads_to_rpm(
                    __builtin_fabsf(g_obs.omega_elec), g_foc.config->pole_pairs),
                .i_d = g_foc.state.i_d,
                .i_q = g_foc.state.i_q,
                .v_bus = g_foc.state.v_bus,
                .faults = 0,
                .timestamp = common_get_timestamp_ms(),
            };
            msghub_publish(g_state_pub, &state);
        }

        k_sleep(SPEED_LOOP_PERIOD);
    }
}

/* ── Kconfig ──────────────────────────────────────────── */
// File: modules/motor_ctrl/Kconfig
//
// config MODULE_MOTOR_CTRL_ENABLE
//     bool "Motor Control Module"
//     default y
//     help
//       Enables the motor control module with FOC + sensorless observer.
