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
    /* ── Sliding Mode Observer ────────────────────────────
     *
     * Motor model: dI/dt = 1/Ls * (V - R*I - E)
     *
     * Discrete: I_hat[k+1] = I_hat[k] + dt/Ls * (V[k] - R*I_hat[k] - E[k])
     *
     * Correction via sliding mode:
     *   Z_α = G1 * sign(I_hat_α - I_α)   (jumps between ±G1)
     *   Z_β = G1 * sign(I_hat_β - I_β)
     *
     * Back-EMF estimation via low-pass filtering of Z:
     *   E_α[k+1] = E_α[k] + G2 * dt * (Z_α - E_α[k])
     *   E_β[k+1] = E_β[k] + G2 * dt * (Z_β - E_β[k])
     */

    /* Current estimation error */
    float i_alpha_err = obs->i_alpha_hat - i_alpha;
    float i_beta_err  = obs->i_beta_hat - i_beta;

    /* Sliding mode correction term Z = G1 * sign(error) */
    float z_alpha = obs->gain1 * sgn(i_alpha_err);
    float z_beta  = obs->gain1 * sgn(i_beta_err);

    /* Update current estimate: I_hat += dt * (V - R*I_hat - E_hat)/L */
    obs->i_alpha_hat += obs->dt * obs->inv_ls
        * (v_alpha - obs->rs_inv_ls * obs->i_alpha_hat - obs->e_alpha);
    obs->i_beta_hat  += obs->dt * obs->inv_ls
        * (v_beta  - obs->rs_inv_ls * obs->i_beta_hat  - obs->e_beta);

    /* BEMF estimation: E += G2 * dt * (Z - E)   (low-pass filter of Z) */
    obs->e_alpha += obs->gain2 * obs->dt * (z_alpha - obs->e_alpha);
    obs->e_beta  += obs->gain2 * obs->dt * (z_beta  - obs->e_beta);

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
    float theta_error = obs->e_alpha * cos_t + obs->e_beta * sin_t;

    /* Clamp theta error to prevent windup */
    if (theta_error > 100.0f)  theta_error = 100.0f;
    if (theta_error < -100.0f) theta_error = -100.0f;

    /* PLL integrator */
    obs->pll_integral += obs->pll_ki * theta_error * obs->dt;

    /* PLL integrator anti-windup */
    float omega_max = 2000.0f; // ~19000 RPM electrical
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

    /* ── Convergence check ────────────────────────────────
     * Consider converged when BEMF amplitude is stable and
     * θ_error is small for N consecutive cycles.
     */
    float bemf_amp = __builtin_sqrtf(obs->e_alpha * obs->e_alpha + obs->e_beta * obs->e_beta);
    float bemf_min = 0.1f; // minimum BEMF for reliable angle

    if (bemf_amp > bemf_min && __builtin_fabsf(theta_error) < 500.0f) {
        obs->consecutive_ok++;
        if (obs->consecutive_ok > 50) {
            obs->converged = true;
        }
    } else {
        obs->consecutive_ok = 0;
        obs->converged = false;
    }

    /* Filtered speed (low-pass) */
    float alpha = 0.01f;
    obs->speed_rpm_filt = (1.0f - alpha) * obs->speed_rpm_filt
                         + alpha * __builtin_fabsf(obs->omega_elec) * 9.54929658551f;
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
