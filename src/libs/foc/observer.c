/**
 * @file observer.c
 * @brief Sliding Mode Observer + PLL implementation
 *
 * References:
 *   - STM32 MCSDK State Observer + PLL documentation (AN5464)
 *   - "Sensorless PMSM Field-Oriented Control" (ST AN1946)
 *
 * The observer uses a first-order motor model in αβ frame,
 * with sliding-mode correction to track the back-EMF.
 * A PLL extracts the rotor angle and speed from the estimated BEMF.
 */

#include "observer.h"
#include "transform.h"
#include <string.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(observer, LOG_LEVEL_INF);

/* ── Helper: sign function ────────────────────────────── */

static inline float sgn(float x)
{
    if (x > 0.0f) return 1.0f;
    if (x < 0.0f) return -1.0f;
    return 0.0f;
}

/* ── Init ─────────────────────────────────────────────── */

void observer_init(observer_t *obs,
                   float rs, float ls, float dt,
                   float gain1, float gain2,
                   float pll_kp, float pll_ki)
{
    memset(obs, 0, sizeof(*obs));

    obs->rs = rs;
    obs->ls = ls;
    obs->rs_inv_ls = rs / ls;
    obs->inv_ls    = 1.0f / ls;
    obs->dt        = dt;

    obs->gain1  = gain1;
    obs->gain2  = gain2;
    obs->pll_kp = pll_kp;
    obs->pll_ki = pll_ki;

    obs->converged = false;
    obs->consecutive_ok = 0;
}

/* ── Step ─────────────────────────────────────────────── */

void observer_step(observer_t *obs,
                   float v_alpha, float v_beta,
                   float i_alpha, float i_beta)
{
    /* ── BEMF estimation: direct voltage model ─────────────
     * E = V - R*I - L*dI/dt
     * Then low-pass filter to remove switching noise.
     * No integrator, no gain tuning needed. */
    float di_alpha_meas = (i_alpha - obs->i_alpha_prev) / obs->dt;
    float di_beta_meas  = (i_beta  - obs->i_beta_prev)  / obs->dt;
    obs->i_alpha_prev = i_alpha;
    obs->i_beta_prev  = i_beta;

    float e_alpha_raw = v_alpha - obs->rs * i_alpha - obs->ls * di_alpha_meas;
    float e_beta_raw  = v_beta  - obs->rs * i_beta  - obs->ls * di_beta_meas;

    /* First-order LPF: E_hat += alpha * (E_raw - E_hat)
     * At 1kHz, alpha=0.5 → cutoff ≈ 80Hz (passes 58Hz electrical) */
    float lpf_alpha = 0.5f;
    obs->e_alpha += lpf_alpha * (e_alpha_raw - obs->e_alpha);
    obs->e_beta  += lpf_alpha * (e_beta_raw  - obs->e_beta);

    /* ── PLL: Extract angle and speed from BEMF ────────────
     *
     * Phase error:  ε_θ = E_α * cos(θ) + E_β * sin(θ)
     *   (dot product of BEMF vector with rotor direction)
     *
     * When perfectly aligned, ε_θ = 0 (BEMF perpendicular to rotor)
     *   θ_error = E_α * cos(θ_hat) + E_β * sin(θ_hat)  ≈ |E| * sin(θ_hat - θ)
     *
     * PLL loop filter:
     *   ω_hat += pll_ki * θ_error * dt
     *   θ_hat += (ω_hat + pll_kp * θ_error) * dt
     */

    float cos_t, sin_t;
    fast_sincos(obs->theta_elec, &sin_t, &cos_t);

    /* Normalized phase error: project BEMF onto rotor direction,
     * then divide by |BEMF| to get sin(angle_error) in [-1, 1].
     * Negated because BEMF is along q-axis (perpendicular to d-axis).
     * When theta = theta_true, E·[cos,sin] = |E|*sin(theta-theta_true)
     * but our convention needs the negative for correct PLL polarity. */
    float bemf_proj = obs->e_alpha * cos_t + obs->e_beta * sin_t;
    float bemf_amp = __builtin_sqrtf(obs->e_alpha * obs->e_alpha
                                   + obs->e_beta * obs->e_beta);
    float theta_error = (bemf_amp > 0.1f) ? -(bemf_proj / bemf_amp) : 0.0f;

    /* Clamp to [-1, 1] (already normalized, but safety) */
    if (theta_error > 1.0f)  theta_error = 1.0f;
    if (theta_error < -1.0f) theta_error = -1.0f;

    /* PLL integrator */
    obs->pll_integral += obs->pll_ki * theta_error * obs->dt;

    /* PLL integrator anti-windup (max ~700 RPM mechanical for 7pp) */
    float omega_max = 500.0f;
    if (obs->pll_integral >  omega_max) obs->pll_integral =  omega_max;
    if (obs->pll_integral < -omega_max) obs->pll_integral = -omega_max;

    /* Frequency estimate */
    obs->omega_elec = obs->pll_integral + obs->pll_kp * theta_error;

    /* Angle update */
    obs->theta_elec += obs->omega_elec * obs->dt;

    /* Wrap angle to [0, 2π) */
    while (obs->theta_elec >= 2.0f * 3.14159265359f)
        obs->theta_elec -= 2.0f * 3.14159265359f;
    while (obs->theta_elec < 0.0f)
        obs->theta_elec += 2.0f * 3.14159265359f;

    /* NaN/Inf guard: reset observer if any state diverged */
    if (__builtin_isnan(obs->i_alpha_hat) || __builtin_isnan(obs->e_alpha) ||
        __builtin_isnan(obs->omega_elec) || __builtin_isinf(obs->e_alpha)) {
        obs->i_alpha_hat = 0.0f; obs->i_beta_hat = 0.0f;
        obs->e_alpha = 0.0f; obs->e_beta = 0.0f;
        obs->omega_elec = 0.0f; obs->pll_integral = 0.0f;
        obs->theta_elec = 0.0f;
    }

    /* ── Convergence check ────────────────────────────────
     * Consider converged when BEMF amplitude is sufficient and
     * normalized θ_error is small for N consecutive cycles.
     */
    float bemf_min = 0.5f; /* minimum BEMF for reliable angle */

    if (bemf_amp > bemf_min && __builtin_fabsf(theta_error) < 0.3f) {
        obs->consecutive_ok++;
        if (obs->consecutive_ok > 100) {  /* 100ms at 1kHz */
            obs->converged = true;
        }
    } else {
        obs->consecutive_ok = 0;
        obs->converged = false;
    }

    /* speed_rpm_filt is managed by motor_ctrl thread (has pole_pairs).
     * Do NOT update it here — would corrupt with electrical RPM. */
}

/* ── Helpers ──────────────────────────────────────────── */

void observer_force_angle(observer_t *obs, float theta_elec)
{
    obs->theta_elec = theta_elec;
    obs->omega_elec = 0.0f;
    obs->pll_integral = 0.0f;
    obs->converged = false;
    obs->consecutive_ok = 0;
}

void observer_reset_convergence(observer_t *obs)
{
    obs->converged = false;
    obs->consecutive_ok = 0;
}

bool observer_is_converged(const observer_t *obs)
{
    return obs->converged;
}
