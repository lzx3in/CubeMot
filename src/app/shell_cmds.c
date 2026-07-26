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
#include <stdlib.h>

#include "drivers/foc/foc_isr.h"
#include "drivers/foc/foc_pwm.h"
#include "drivers/foc/foc_adc.h"
#include "libs/foc/foc_types.h"
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

static int cmd_motor_log(const struct shell *sh, size_t argc, char **argv)
{
    extern volatile bool g_motor_log_enabled;
    if (argc > 1 && (argv[1][0] == '1' || argv[1][0] == 'o')) {
        g_motor_log_enabled = true;
        shell_print(sh, "Motor log ON (CSV @ 10Hz: t,state,RPM,Id,Iq,omega,BEMF,theta_foc,theta_obs)");
    } else {
        g_motor_log_enabled = false;
        shell_print(sh, "Motor log OFF");
    }
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
    SHELL_CMD_ARG(log, NULL,
        "Toggle real-time CSV log (10Hz)\n"
        "Usage: motor log [on|off]",
        cmd_motor_log, 1, 1),
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

SHELL_STATIC_SUBCMD_SET_CREATE(sub_adc,
    SHELL_CMD_ARG(offset, NULL, "Show ADC offsets and live values",
        cmd_adc_offset, 1, 0),
    SHELL_CMD_ARG(diag, NULL, "Run software-trigger ADC diagnostic",
        cmd_adc_diag, 1, 0),
    SHELL_SUBCMD_SET_END
);

/* ══════════════════════════════════════════════════════
 *  scope command group (high-res ring buffer diagnostics)
 * ══════════════════════════════════════════════════════ */

static int cmd_scope_start(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    scope_start();
    shell_print(sh, "Scope STARTED (128 samples @ 1kHz = 128ms window)");
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

    scope_stop();  /* freeze before reading */

    uint16_t count = scope_get_count();
    const scope_sample_t *buf = scope_get_buf();

    shell_print(sh, "th_foc,th_obs,omega,id,iq,iq_ref,rpm,bemf,state");

    for (uint16_t i = 0; i < count; i++) {
        const scope_sample_t *s = &buf[i];
        shell_print(sh, "%d,%d,%d,%d,%d,%d,%d,%d,%d",
                    s->ch[SC_THETA_FOC], s->ch[SC_THETA_OBS],
                    s->ch[SC_OMEGA], s->ch[SC_ID_MA],
                    s->ch[SC_IQ_MA], s->ch[SC_IQ_REF_MA],
                    s->ch[SC_RPM], s->ch[SC_BEMF_CV],
                    s->ch[SC_STATE]);
        if ((i & 15) == 15) k_msleep(1);  /* yield every 16 lines */
    }

    shell_print(sh, "# %u samples dumped", count);
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
    SHELL_CMD(start, NULL, "Start capture (clears buffer)", cmd_scope_start),
    SHELL_CMD(stop, NULL, "Stop capture", cmd_scope_stop),
    SHELL_CMD(dump, NULL, "Dump buffer as CSV", cmd_scope_dump),
    SHELL_CMD(status, NULL, "Show scope status", cmd_scope_status),
    SHELL_SUBCMD_SET_END
);

/* ══════════════════════════════════════════════════════
 *  Root command registration
 * ══════════════════════════════════════════════════════ */

SHELL_CMD_REGISTER(foc, &sub_foc, "FOC open-loop control", NULL);
SHELL_CMD_REGISTER(motor, &sub_motor, "Motor closed-loop control", NULL);
SHELL_CMD_REGISTER(adc, &sub_adc, "ADC diagnostics", NULL);
SHELL_CMD_REGISTER(scope, &sub_scope, "High-res scope (1kHz ring buf)", NULL);
