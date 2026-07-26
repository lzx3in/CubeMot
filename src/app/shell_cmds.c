/**
 * @file shell_cmds.c
 * @brief Zephyr Shell commands for FOC hardware verification
 *
 * Provides interactive debug commands over LPUART1 (ST-Link VCP/USB):
 *   foc start/stop/status  — open-loop FOC control
 *   motor start/stop/status — closed-loop startup sequence
 *   adc offset/diag        — ADC diagnostics
 *
 * These commands call the same capability-layer APIs as serial_cmd,
 * serving as the human-facing interface during development/verification.
 */

#include <zephyr/shell/shell.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <stdlib.h>
#include <stdio.h>  /* snprintf */

#include "drivers/foc/foc_isr.h"
#include "drivers/foc/foc_pwm.h"
#include "drivers/foc/foc_adc.h"
#include "libs/foc/foc_types.h"
#include "libs/foc/foc_core.h"
#include "modules/motor_ctrl/motor_ctrl.h"
#include "common/motor_params.h"
#include "topics/topics.h"
#include "common_time.h"
#include "scope.h"

/* ── msghub handles (lazy init) ─────────────────────── */

static msghub_publisher_t g_motor_cmd_pub = MSGHUB_PUBLISHER_INVALID;
static msghub_subscriber_t g_motor_state_sub = MSGHUB_SUBSCRIBER_INVALID;

static void ensure_msghub_handles(void)
{
    if (g_motor_cmd_pub == MSGHUB_PUBLISHER_INVALID) {
        g_motor_cmd_pub = msghub_create_publisher(MSGHUB_TOPIC(motor_cmd));
    }
    if (g_motor_state_sub == MSGHUB_SUBSCRIBER_INVALID) {
        g_motor_state_sub = msghub_create_subscriber(MSGHUB_TOPIC(motor_state), 0);
    }
}

/* ══════════════════════════════════════════════════════
 *  foc command group
 * ══════════════════════════════════════════════════════ */

static int cmd_foc_start(const struct shell *sh, size_t argc, char **argv)
{
    float id_a = 0.8f; /* default 0.8A */

    if (argc > 1) {
        id_a = (float)atoi(argv[1]) / 1000.0f; /* input in mA */
    }

    foc_t *foc = foc_isr_get_foc();
    foc->state.i_d_ref = id_a;
    foc->state.i_q_ref = 0.0f;

    foc_pwm_enable();
    foc_isr_start();

    shell_print(sh, "FOC started: Id=%.0f mA, Iq=0 mA", (double)(id_a * 1000.0f));
    shell_print(sh, "PWM outputs ENABLED — motor will lock to rotor angle");
    return 0;
}

static int cmd_foc_stop(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    foc_isr_stop();
    foc_pwm_disable();

    shell_print(sh, "FOC stopped, PWM outputs disabled");
    return 0;
}

static int cmd_foc_iq(const struct shell *sh, size_t argc, char **argv)
{
    if (argc < 2) {
        shell_error(sh, "Usage: foc iq <mA>");
        return -EINVAL;
    }

    float iq_a = (float)atoi(argv[1]) / 1000.0f;
    foc_isr_get_foc()->state.i_q_ref = iq_a;

    shell_print(sh, "Iq ref set to %.0f mA", (double)(iq_a * 1000.0f));
    return 0;
}

static int cmd_foc_status(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    foc_t *foc = foc_isr_get_foc();
    foc_state_t *s = &foc->state;
    observer_t *obs = foc_isr_get_observer();

    shell_print(sh, "── FOC Status ──");
    shell_print(sh, "  Running:   %s (count=%u)",
                foc_isr_is_running() ? "YES" : "NO", foc_isr_get_count());
    shell_print(sh, "  PWM:       %s", foc_pwm_is_enabled() ? "ENABLED" : "disabled");
    shell_print(sh, "  Id/Iq ref: %.1f / %.1f mA",
                (double)(s->i_d_ref * 1000.0f), (double)(s->i_q_ref * 1000.0f));
    shell_print(sh, "  Id/Iq act: %.1f / %.1f mA",
                (double)(s->i_d * 1000.0f), (double)(s->i_q * 1000.0f));
    shell_print(sh, "  Vbus:      %.2f V", (double)s->v_bus);
    shell_print(sh, "  Duty A/B/C: %.3f / %.3f / %.3f",
                (double)s->duty_a, (double)s->duty_b, (double)s->duty_c);
    shell_print(sh, "  Theta:     %.3f rad", (double)s->theta_elec);
    shell_print(sh, "  Speed(obs): %.1f RPM (omega=%.1f rad/s)",
                (double)(__builtin_fabsf(obs->omega_elec) * 60.0f / (6.2832f * foc->config->pole_pairs)),
                (double)obs->omega_elec);
    shell_print(sh, "  BEMF:      %.2f V", (double)__builtin_sqrtf(
                obs->e_alpha * obs->e_alpha + obs->e_beta * obs->e_beta));
    shell_print(sh, "  Observer:  %s (consec=%d)",
                observer_is_converged(obs) ? "CONVERGED" : "not converged",
                obs->consecutive_ok);
    return 0;
}

static int cmd_foc_pid(const struct shell *sh, size_t argc, char **argv)
{
    PID_t *pid = foc_get_pid_id();

    if (argc < 3) {
        shell_print(sh, "Current loop PI: Kp=%.4f  Ki=%.4f",
                    (double)pid->kp, (double)pid->ki);
        shell_print(sh, "  (limits: integral=%.1f, output=%.1f)",
                    (double)pid->integral_limit, (double)pid->output_limit);
        shell_print(sh, "Usage: foc pid <kp_milli> <ki_milli>  (e.g. 3300 550 = 3.3, 0.55)");
        return 0;
    }

    float kp = (float)atoi(argv[1]) / 1000.0f;
    float ki = (float)atoi(argv[2]) / 1000.0f;
    foc_set_current_gains(kp, ki);

    shell_print(sh, "Current loop PI set: Kp=%.4f  Ki=%.4f", (double)kp, (double)ki);
    return 0;
}

static int cmd_foc_params(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    const foc_motor_config_t *cfg = &g_motor_params;
    PID_t *pid_i = foc_get_pid_id();
    PID_t *spd = motor_ctrl_get_speed_pid(0);
    observer_t *obs = foc_isr_get_observer();
    const startup_config_t *scfg = motor_ctrl_get_startup_cfg();

    shell_print(sh, "── Motor Parameters ──");
    shell_print(sh, "  Rs=%.2f Ohm  Ls=%.3f mH  PP=%u",
                (double)cfg->rs, (double)(cfg->ls * 1000.0f), cfg->pole_pairs);
    shell_print(sh, "  Max speed: %.0f RPM  Iq_max: %.1f A",
                (double)cfg->max_speed_rpm, (double)cfg->iq_max);
    shell_print(sh, "── Current Loop PI ──");
    shell_print(sh, "  Kp=%.4f  Ki=%.4f", (double)pid_i->kp, (double)pid_i->ki);
    shell_print(sh, "── Speed Loop PI ──");
    if (spd) {
        shell_print(sh, "  Kp=%.5f  Ki=%.5f  (int_lim=%.2f, out_lim=%.2f)",
                    (double)spd->kp, (double)spd->ki,
                    (double)spd->integral_limit, (double)spd->output_limit);
    } else {
        shell_print(sh, "  (motor not initialized)");
    }
    shell_print(sh, "── Observer PLL ──");
    shell_print(sh, "  pll_kp=%.1f  pll_ki=%.1f",
                (double)obs->pll_kp, (double)obs->pll_ki);
    shell_print(sh, "── Startup Sequence ──");
    shell_print(sh, "  align: %u ms @ %.0f mA",
                scfg->phase1_duration_ms, (double)(scfg->phase1_align_current * 1000.0f));
    shell_print(sh, "  ramp:  %u ms -> %.0f RPM @ %.0f mA",
                scfg->phase2_duration_ms, (double)scfg->phase2_final_speed,
                (double)(scfg->phase2_current * 1000.0f));
    shell_print(sh, "── Protection ──");
    shell_print(sh, "  Overcurrent: %.1f A", (double)foc_isr_get_oc_threshold());
    return 0;
}

static int cmd_foc_limit(const struct shell *sh, size_t argc, char **argv)
{
    if (argc < 2) {
        shell_print(sh, "Overcurrent threshold: %.1f A",
                    (double)foc_isr_get_oc_threshold());
        shell_print(sh, "Usage: foc limit <amps_x10>  (e.g. 20 = 2.0A)");
        return 0;
    }

    float amps = (float)atoi(argv[1]) / 10.0f;
    if (amps < 0.1f || amps > 10.0f) {
        shell_error(sh, "Range: 0.1A ~ 10.0A");
        return -EINVAL;
    }
    foc_isr_set_oc_threshold(amps);
    shell_print(sh, "Overcurrent threshold set: %.1f A", (double)amps);
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_foc,
    SHELL_CMD_ARG(start, NULL,
        "Start FOC open-loop (Id lock)\n"
        "Usage: foc start [id_mA]  (default 800 mA)",
        cmd_foc_start, 1, 1),
    SHELL_CMD_ARG(stop, NULL, "Stop FOC and disable PWM",
        cmd_foc_stop, 1, 0),
    SHELL_CMD_ARG(iq, NULL,
        "Set Iq reference\n"
        "Usage: foc iq <mA>",
        cmd_foc_iq, 2, 0),
    SHELL_CMD_ARG(status, NULL, "Show FOC state",
        cmd_foc_status, 1, 0),
    SHELL_CMD_ARG(pid, NULL,
        "Show/set current loop PI gains\n"
        "Usage: foc pid [kp_milli] [ki_milli]",
        cmd_foc_pid, 1, 2),
    SHELL_CMD_ARG(params, NULL, "Show all motor params and gains",
        cmd_foc_params, 1, 0),
    SHELL_CMD_ARG(limit, NULL,
        "Show/set overcurrent threshold\n"
        "Usage: foc limit [amps_x10]",
        cmd_foc_limit, 1, 1),
    SHELL_SUBCMD_SET_END
);

/* ══════════════════════════════════════════════════════
 *  motor command group
 * ══════════════════════════════════════════════════════ */

static int cmd_motor_start(const struct shell *sh, size_t argc, char **argv)
{
    float rpm = 500.0f;

    if (argc > 1) {
        rpm = (float)atoi(argv[1]);
    }

    ensure_msghub_handles();

    motor_cmd_t cmd = {
        .motor_id = 0,
        .cmd = MOTOR_CMD_START,
        .target_speed_rpm = rpm,
        .ramp_time_ms = 0,
    };
    msghub_publish(g_motor_cmd_pub, &cmd);

    shell_print(sh, "Motor START published: target=%.0f RPM", (double)rpm);
    shell_print(sh, "Use 'motor status' to watch state transitions");
    return 0;
}

static int cmd_motor_stop(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    ensure_msghub_handles();

    motor_cmd_t cmd = {
        .motor_id = 0,
        .cmd = MOTOR_CMD_STOP,
    };
    msghub_publish(g_motor_cmd_pub, &cmd);

    shell_print(sh, "Motor STOP published");
    return 0;
}

static int cmd_motor_status(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    ensure_msghub_handles();

    static const char *state_names[] = {
        "IDLE", "ALIGN", "START", "RUN", "FAULT", "STOP"
    };

    motor_state_t mstate;
    bool updated = false;
    msghub_subscriber_check(g_motor_state_sub, &updated);
    if (updated) {
        msghub_receive(g_motor_state_sub, &mstate);
    }

    const char *sname = (mstate.state < 6) ? state_names[mstate.state] : "???";

    shell_print(sh, "── Motor Status ──");
    shell_print(sh, "  State:   %s (%u)", sname, mstate.state);
    shell_print(sh, "  Speed:   %.1f RPM", (double)mstate.speed_rpm);
    shell_print(sh, "  Id/Iq:   %.1f / %.1f mA",
                (double)(mstate.i_d * 1000.0f), (double)(mstate.i_q * 1000.0f));
    shell_print(sh, "  Vbus:    %.2f V", (double)mstate.v_bus);
    shell_print(sh, "  Faults:  0x%04X", mstate.faults);
    return 0;
}

static int cmd_motor_watch(const struct shell *sh, size_t argc, char **argv)
{
    int interval_ms = 500;
    int count = 10;

    if (argc > 1) interval_ms = atoi(argv[1]);
    if (argc > 2) count = atoi(argv[2]);
    if (interval_ms < 50) interval_ms = 50;

    ensure_msghub_handles();

    static const char *state_names[] = {
        "IDLE", "ALIGN", "START", "RUN", "FAULT", "STOP"
    };

    shell_print(sh, "Watching motor (%d ms interval, %d samples, Ctrl+C to stop):",
                interval_ms, count);
    shell_print(sh, "%-8s %8s %8s %8s %6s",
                "STATE", "RPM", "Id(mA)", "Iq(mA)", "Vbus");

    for (int i = 0; i < count; i++) {
        k_msleep(interval_ms);

        motor_state_t mstate;
        bool updated = false;
        msghub_subscriber_check(g_motor_state_sub, &updated);
        if (updated) {
            msghub_receive(g_motor_state_sub, &mstate);
        }

        const char *sname = (mstate.state < 6) ? state_names[mstate.state] : "???";
        shell_print(sh, "%-8s %8.1f %8.1f %8.1f %6.1f",
                    sname,
                    (double)mstate.speed_rpm,
                    (double)(mstate.i_d * 1000.0f),
                    (double)(mstate.i_q * 1000.0f),
                    (double)mstate.v_bus);
    }
    return 0;
}

static int cmd_motor_pid(const struct shell *sh, size_t argc, char **argv)
{
    PID_t *pid = motor_ctrl_get_speed_pid(0);
    if (!pid) {
        shell_error(sh, "Motor 0 not initialized");
        return -ENODEV;
    }

    if (argc < 3) {
        shell_print(sh, "Speed loop PI: Kp=%.5f  Ki=%.5f",
                    (double)pid->kp, (double)pid->ki);
        shell_print(sh, "  (limits: integral=%.2f, output=%.2f)",
                    (double)pid->integral_limit, (double)pid->output_limit);
        shell_print(sh, "Usage: motor pid <kp_milli> <ki_milli>  (e.g. 2 1 = 0.002, 0.001)");
        return 0;
    }

    float kp = (float)atoi(argv[1]) / 1000.0f;
    float ki = (float)atoi(argv[2]) / 1000.0f;
    motor_ctrl_set_speed_gains(0, kp, ki);

    shell_print(sh, "Speed loop PI set: Kp=%.5f  Ki=%.5f", (double)kp, (double)ki);
    return 0;
}

static int cmd_motor_cfg(const struct shell *sh, size_t argc, char **argv)
{
    const startup_config_t *cfg = motor_ctrl_get_startup_cfg();

    if (argc < 2) {
        shell_print(sh, "── Startup Config ──");
        shell_print(sh, "  align_ms  = %u", cfg->phase1_duration_ms);
        shell_print(sh, "  align_ma  = %.0f", (double)(cfg->phase1_align_current * 1000.0f));
        shell_print(sh, "  ramp_ms   = %u", cfg->phase2_duration_ms);
        shell_print(sh, "  ramp_rpm  = %.0f", (double)cfg->phase2_final_speed);
        shell_print(sh, "  ramp_ma   = %.0f", (double)(cfg->phase2_current * 1000.0f));
        shell_print(sh, "Usage: motor cfg <key> <value>");
        return 0;
    }

    if (argc < 3) {
        shell_error(sh, "Usage: motor cfg <key> <value>");
        return -EINVAL;
    }

    int value = atoi(argv[2]);
    if (motor_ctrl_set_startup_param(argv[1], value) != 0) {
        shell_error(sh, "Unknown key '%s'", argv[1]);
        shell_print(sh, "Valid keys: align_ms, align_ma, ramp_ms, ramp_rpm, ramp_ma");
        return -EINVAL;
    }

    shell_print(sh, "%s = %d", argv[1], value);
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_motor,
    SHELL_CMD_ARG(start, NULL,
        "Start motor startup sequence\n"
        "Usage: motor start [rpm]  (default 500)",
        cmd_motor_start, 1, 1),
    SHELL_CMD_ARG(stop, NULL, "Stop motor",
        cmd_motor_stop, 1, 0),
    SHELL_CMD_ARG(status, NULL, "Show motor state (single read)",
        cmd_motor_status, 1, 0),
    SHELL_CMD_ARG(watch, NULL,
        "Watch motor state periodically\n"
        "Usage: motor watch [interval_ms] [count]  (default 500ms x10)",
        cmd_motor_watch, 1, 2),
    SHELL_CMD_ARG(pid, NULL,
        "Show/set speed loop PI gains\n"
        "Usage: motor pid [kp_milli] [ki_milli]",
        cmd_motor_pid, 1, 2),
    SHELL_CMD_ARG(cfg, NULL,
        "Show/set startup parameters\n"
        "Usage: motor cfg [key] [value]",
        cmd_motor_cfg, 1, 2),
    SHELL_SUBCMD_SET_END
);

/* ══════════════════════════════════════════════════════
 *  adc command group
 * ══════════════════════════════════════════════════════ */

static int cmd_adc_offset(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    foc_t *foc = foc_isr_get_foc();
    foc_state_t *s = &foc->state;

    shell_print(sh, "── ADC Offsets & Live ──");
    shell_print(sh, "  Offset Ia: %d", s->adc_ia_offset);
    shell_print(sh, "  Offset Ib: %d", s->adc_ib_offset);
    shell_print(sh, "  Offset Ic: %d", s->adc_ic_offset);
    shell_print(sh, "  Live   Ia: %d (delta=%d)", s->adc_ia, s->adc_ia - s->adc_ia_offset);
    shell_print(sh, "  Live   Ib: %d (delta=%d)", s->adc_ib, s->adc_ib - s->adc_ib_offset);
    shell_print(sh, "  Live   Ic: %d", s->adc_ic);
    shell_print(sh, "  Vbus:      %.2f V (raw=%d)", (double)s->v_bus, s->adc_vbus);
    return 0;
}

static int cmd_adc_diag(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(sh, "Running ADC software-trigger diagnostic...");

    extern void foc_adc_sw_trigger_test(void);
    extern void foc_adc_get_sw_diag(int16_t *ia, int16_t *ib, int16_t *ic,
                                    int16_t *vbus_raw, float *vbus_v, bool *valid);

    foc_adc_sw_trigger_test();

    int16_t ia, ib, ic, vbus_raw;
    float vbus_v;
    bool valid;
    foc_adc_get_sw_diag(&ia, &ib, &ic, &vbus_raw, &vbus_v, &valid);

    if (!valid) {
        shell_error(sh, "ADC diagnostic FAILED (timeout)");
        return -EIO;
    }

    shell_print(sh, "── ADC SW Trigger Result ──");
    shell_print(sh, "  Ia:    %d", ia);
    shell_print(sh, "  Ib:    %d", ib);
    shell_print(sh, "  Ic:    %d", ic);
    shell_print(sh, "  Vbus:  raw=%d  →  %.2f V", vbus_raw, (double)vbus_v);
    return 0;
}

static int cmd_adc_cal(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    if (foc_pwm_is_enabled()) {
        shell_error(sh, "PWM is ENABLED — stop FOC first (foc stop)");
        return -EBUSY;
    }

    shell_print(sh, "Recalibrating ADC offsets (motor must be idle)...");

    int16_t ia_off, ib_off, ic_off;
    foc_adc_get_offsets(&ia_off, &ib_off, &ic_off);

    foc_t *foc = foc_isr_get_foc();
    foc->state.adc_ia_offset = ia_off;
    foc->state.adc_ib_offset = ib_off;
    foc->state.adc_ic_offset = ic_off;

    shell_print(sh, "ADC offsets recalibrated:");
    shell_print(sh, "  Ia: %d  Ib: %d  Ic: %d", ia_off, ib_off, ic_off);
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_adc,
    SHELL_CMD_ARG(offset, NULL, "Show ADC offsets and live values",
        cmd_adc_offset, 1, 0),
    SHELL_CMD_ARG(diag, NULL, "Run software-trigger ADC diagnostic",
        cmd_adc_diag, 1, 0),
    SHELL_CMD_ARG(cal, NULL, "Recalibrate ADC offsets (PWM must be off)",
        cmd_adc_cal, 1, 0),
    SHELL_SUBCMD_SET_END
);

/* ══════════════════════════════════════════════════════
 *  scope command group (high-res ring buffer diagnostics)
 * ══════════════════════════════════════════════════════ */

/* ── Scope dump: direct UART output (bypass log/shell queue) ── */
static const struct device *scope_console_uart;

static void scope_uart_puts(const char *s)
{
    while (*s) {
        uart_poll_out(scope_console_uart, *s++);
    }
}

static int cmd_scope_start(const struct shell *sh, size_t argc, char **argv)
{
    uint8_t decim = 1;
    if (argc > 1) decim = (uint8_t)atoi(argv[1]);
    if (decim < 1) decim = 1;
    scope_start_decim(decim);
    shell_print(sh, "Scope STARTED (decim=%u, %ums window)",
                decim, SCOPE_DEPTH * decim);
    return 0;
}

static int cmd_scope_stop(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    scope_stop();
    shell_print(sh, "Scope STOPPED (%u samples captured)", scope_get_count());
    return 0;
}

static int cmd_scope_dump(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    /* Lazy-init console UART device */
    if (!scope_console_uart) {
        scope_console_uart = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
        if (!device_is_ready(scope_console_uart)) {
            shell_error(sh, "Console UART not ready");
            return -ENODEV;
        }
    }

    scope_stop();  /* freeze before reading */

    uint16_t count = scope_get_count();
    const scope_sample_t *buf = scope_get_buf();
    char line[64];  /* 9×int16 max "-32768,"×9 + \n = 63 chars */

    /* Header — direct UART, no shell/log */
    scope_uart_puts("th_foc,th_obs,omega,id,iq,iq_ref,rpm,bemf,state\n");

    for (uint16_t i = 0; i < count; i++) {
        const scope_sample_t *s = &buf[i];
        int len = snprintf(line, sizeof(line),
                           "%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
                           s->ch[SC_THETA_FOC], s->ch[SC_THETA_OBS],
                           s->ch[SC_OMEGA], s->ch[SC_ID_MA],
                           s->ch[SC_IQ_MA], s->ch[SC_IQ_REF_MA],
                           s->ch[SC_RPM], s->ch[SC_BEMF_CV],
                           s->ch[SC_STATE]);
        for (int j = 0; j < len; j++) {
            uart_poll_out(scope_console_uart, line[j]);
        }
        if ((i & 15) == 15) {
            k_msleep(1);  /* yield every 16 lines */
        }
    }

    snprintf(line, sizeof(line), "# %u samples\n", count);
    scope_uart_puts(line);

    shell_print(sh, "Scope dump complete: %u samples", count);
    return 0;
}

static int cmd_scope_status(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    shell_print(sh, "Scope: %s, %u/%u samples",
                scope_is_active() ? "ACTIVE" : "idle",
                scope_get_count(), SCOPE_DEPTH);
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_scope,
    SHELL_CMD_ARG(start, NULL, "Start capture [decimation] (default=1)",
                  cmd_scope_start, 1, 1),
    SHELL_CMD(stop, NULL, "Stop capture", cmd_scope_stop),
    SHELL_CMD(dump, NULL, "Dump buffer as CSV", cmd_scope_dump),
    SHELL_CMD(status, NULL, "Show scope status", cmd_scope_status),
    SHELL_SUBCMD_SET_END
);

/* ══════════════════════════════════════════════════════
 *  obs command group (observer tuning & diagnostics)
 * ══════════════════════════════════════════════════════ */

static int cmd_obs_pll(const struct shell *sh, size_t argc, char **argv)
{
    observer_t *obs = foc_isr_get_observer();

    if (argc < 3) {
        shell_print(sh, "Observer PLL: Kp=%.1f  Ki=%.1f",
                    (double)obs->pll_kp, (double)obs->pll_ki);
        shell_print(sh, "Usage: obs pll <kp> <ki>  (integers)");
        return 0;
    }

    obs->pll_kp = (float)atoi(argv[1]);
    obs->pll_ki = (float)atoi(argv[2]);

    shell_print(sh, "Observer PLL set: Kp=%.1f  Ki=%.1f",
                (double)obs->pll_kp, (double)obs->pll_ki);
    return 0;
}

static int cmd_obs_status(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    observer_t *obs = foc_isr_get_observer();
    foc_t *foc = foc_isr_get_foc();

    float cos_t, sin_t;
    fast_sincos(obs->theta_elec, &sin_t, &cos_t);
    float bemf_proj = obs->e_alpha * cos_t + obs->e_beta * sin_t;
    float bemf_amp = __builtin_sqrtf(obs->e_alpha * obs->e_alpha
                                   + obs->e_beta * obs->e_beta);
    float theta_error = (bemf_amp > 0.1f) ? -(bemf_proj / bemf_amp) : 0.0f;

    shell_print(sh, "── Observer Status ──");
    shell_print(sh, "  theta_elec:  %.4f rad", (double)obs->theta_elec);
    shell_print(sh, "  omega_elec:  %.2f rad/s", (double)obs->omega_elec);
    shell_print(sh, "  speed_filt:  %.1f RPM", (double)obs->speed_rpm_filt);
    shell_print(sh, "  BEMF a/b:    %.3f / %.3f V",
                (double)obs->e_alpha, (double)obs->e_beta);
    shell_print(sh, "  BEMF amp:    %.3f V", (double)bemf_amp);
    shell_print(sh, "  theta_error: %.4f (norm)", (double)theta_error);
    shell_print(sh, "  PLL integral:%.3f", (double)obs->pll_integral);
    shell_print(sh, "  Converged:   %s (consec=%u)",
                obs->converged ? "YES" : "NO", obs->consecutive_ok);
    shell_print(sh, "  FOC theta:   %.4f rad (delta=%.4f)",
                (double)foc->state.theta_elec,
                (double)(foc->state.theta_elec - obs->theta_elec));
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_obs,
    SHELL_CMD_ARG(pll, NULL,
        "Show/set observer PLL gains\n"
        "Usage: obs pll [kp] [ki]",
        cmd_obs_pll, 1, 2),
    SHELL_CMD_ARG(status, NULL, "Show observer internal state",
        cmd_obs_status, 1, 0),
    SHELL_SUBCMD_SET_END
);

/* ══════════════════════════════════════════════════════
 *  Root command registration
 * ══════════════════════════════════════════════════════ */

SHELL_CMD_REGISTER(foc, &sub_foc, "FOC open-loop control", NULL);
SHELL_CMD_REGISTER(motor, &sub_motor, "Motor closed-loop control", NULL);
SHELL_CMD_REGISTER(adc, &sub_adc, "ADC diagnostics", NULL);
SHELL_CMD_REGISTER(scope, &sub_scope, "High-res scope (1kHz ring buf)", NULL);
SHELL_CMD_REGISTER(obs, &sub_obs, "Observer tuning & diagnostics", NULL);
