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

    /* Observer parameters (from Workbench) */
    .observer_gain1     = -22528.0f / 16384.0f,   /* Normalized G1 = -1.375 */
    .observer_gain2     = 31586.0f / 4096.0f,     /* Normalized G2 = 7.712 */
    .pll_kp             = 195.0f / 16384.0f,      /* Normalized KP = 0.0119 */
    .pll_ki             = 5.0f / 65535.0f,        /* Normalized KI = 7.63e-5 */
    .observer_min_speed_rpm = 524,
};
