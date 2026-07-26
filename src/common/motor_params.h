/**
 * @file motor_params.h
 * @brief Motor parameters for GBM2804-100T gimbal motor
 *
 * Values from ST Motor Control Workbench configuration.
 */

#pragma once

#include "foc_types.h"

/**
 * @brief  Motor parameters for GBM2804-100T
 */
static const foc_motor_config_t g_motor_params = {
    /* Electrical parameters */
    .rs             = 5.29f,        /* Stator resistance [Ohm] */
    .ls             = 0.001058f,    /* Stator inductance [H] = 1.058mH */
    .ld_lq_ratio    = 1.0f,        /* Non-salient pole */
    .pole_pairs     = 7,            /* 7 pole pairs (14 poles) */
    .voltage_const  = 0.0f,         /* Not used for now */
    .rated_flux     = 0.0f,         /* Not used for now */

    /* Ratings */
    .max_speed_rpm      = 3000.0f,
    .nominal_current    = 2.0f,
    .iq_max             = 4.0f,
    .id_demag           = 0.0f,

    /* Observer parameters (floating-point Luenberger, pole placement)
     * Motor: Rs=5.29, Ls=1.058mH, Rs/Ls=5000
     * Observer bandwidth: 50Hz → omega_obs=314 rad/s
     * L1 = 2*omega_obs + Rs/Ls = 5628
     * L2 = Ls * omega_obs^2 = 104
     * PLL dual-stage:
     *   Acquisition (START): 5Hz → wn=31, zeta=0.7, kp=44, ki=986
     *   Tracking (RUN):     15Hz → wn=94, zeta=0.7, kp=132, ki=8874
     * Init uses acquisition gains; motor_ctrl switches at RUN transition. */
    .observer_gain1     = 5628.0f,    /* unused in voltage model */
    .observer_gain2     = 0.1f,       /* unused in voltage model */
    .pll_kp             = 44.0f,      /* PLL acquisition (5Hz BW) */
    .pll_ki             = 986.0f,     /* PLL acquisition (5Hz BW) */
    .observer_min_speed_rpm = 50,
};
