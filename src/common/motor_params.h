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
     * PLL bandwidth: 5Hz → pll_kp=31, pll_ki=100 */
    .observer_gain1     = 5628.0f,    /* L1: current correction gain */
    .observer_gain2     = 104.0f,     /* L2: BEMF integrator gain */
    .pll_kp             = 31.0f,      /* PLL proportional */
    .pll_ki             = 100.0f,     /* PLL integral */
    .observer_min_speed_rpm = 50,
};
