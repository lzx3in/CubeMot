#pragma once

/**
 * @file observer.h
 * @brief Sliding Mode Observer + PLL for sensorless FOC
 *
 * Estimates rotor electrical angle and speed from
 * measured currents and applied voltages.
 *
 * Algorithm:
 *   1. Motor model in αβ: Iαβ_dot = 1/Ls * (Vαβ - Rs·Iαβ - Eαβ)
 *   2. Sliding mode: ε = sign(I_hat - I_meas)
 *   3. BEMF estimation: Eαβ = G1 * ε (sliding mode output, filtered)
 *   4. PLL: lock onto estimated BEMF to extract θ and ω
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

/* ── Observer state ──────────────────────────────────── */

typedef struct {
    /* Current estimates (αβ frame) */
    float i_alpha_hat;
    float i_beta_hat;

    /* BEMF estimates */
    float e_alpha;
    float e_beta;

    /* PLL state */
    float theta_elec;     // Electrical angle [rad]
    float omega_elec;     // Electrical angular speed [rad/s]
    float pll_integral;   // PLL integrator

    /* Gains (from config) */
    float gain1;          // Sliding mode gain G1
    float gain2;          // BEMF filter coefficient (higher = less filtering)
    float pll_kp;
    float pll_ki;

    /* Motor constants */
    float rs;             // Stator resistance [Ohm]
    float ls;             // Stator inductance [H]
    float rs_inv_ls;      // Rs / Ls
    float inv_ls;         // 1 / Ls

    /* Previous measured currents (for dI/dt) */
    float i_alpha_prev;
    float i_beta_prev;

    /* Convergence check */
    bool  converged;
    uint16_t consecutive_ok;
    float  speed_rpm_filt; // Filtered speed estimate

    /* Timestamp */
    float dt;             // Sample time [s]
} observer_t;

/* ── API ──────────────────────────────────────────────── */

/**
 * @brief  Initialize observer with motor parameters
 * @param  obs           Observer instance
 * @param  rs            Stator resistance [Ω]
 * @param  ls            Stator inductance [H]
 * @param  dt            Sample time [s] (1/FOC_freq)
 * @param  gain1         Sliding mode gain (e.g., -22528 for SDK)
 * @param  gain2         BEMF filter gain (e.g., 31586 for SDK)
 * @param  pll_kp        PLL proportional gain
 * @param  pll_ki        PLL integral gain
 */
void observer_init(observer_t *obs,
                   float rs, float ls, float dt,
                   float gain1, float gain2,
                   float pll_kp, float pll_ki);

/**
 * @brief  Run one observer iteration
 *
 * Called at FOC rate (30kHz), or can be decimated.
 *
 * @param  obs       Observer instance
 * @param  v_alpha   Applied α-axis voltage [V]
 * @param  v_beta    Applied β-axis voltage [V]
 * @param  i_alpha   Measured α-axis current [A]
 * @param  i_beta    Measured β-axis current [A]
 *
 * Updates obs->theta_elec, obs->omega_elec, obs->converged
 */
void observer_step(observer_t *obs,
                   float v_alpha, float v_beta,
                   float i_alpha, float i_beta);

/**
 * @brief  Force angle to a known value (used during forced startup)
 */
void observer_force_angle(observer_t *obs, float theta_elec);

/**
 * @brief  Reset convergence state (used when restarting startup sequence)
 */
void observer_reset_convergence(observer_t *obs);

/**
 * @brief  Check if observer has converged (BEMF tracking stable)
 */
bool observer_is_converged(const observer_t *obs);

#ifdef __cplusplus
}
#endif
