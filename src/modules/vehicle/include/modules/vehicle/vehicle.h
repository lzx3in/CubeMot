#pragma once

/**
 * @file vehicle.h
 * @brief Vehicle kinematics: Ackermann steering + odometry
 *
 * Architecture:
 *   - Receives cmd_vel from commander
 *   - Computes wheel speeds + steering angles (Ackermann geometry)
 *   - Integrates odometry (position + heading)
 *   - Publishes vehicle_state at 50Hz
 *
 * V2: Differential drive (2 wheels)
 * V3: Full Ackermann (4 wheels + 2 servos)
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* ── Vehicle configuration ───────────────────────────── */

typedef struct {
    float wheelbase_m;      // Distance between front/rear axles [m]
    float track_width_m;    // Distance between left/right wheels [m]
    float wheel_radius_m;   // Wheel radius [m]
    float max_speed_ms;     // Max linear speed [m/s]
    float max_angular_rads; // Max angular speed [rad/s]
    float max_steer_deg;    // Max steering angle [deg]
} vehicle_config_t;

/* ── API ─────────────────────────────────────────────── */

/**
 * @brief  Initialize vehicle module
 * @param  config  Vehicle geometry parameters
 * @return 0 on success
 */
int vehicle_init(const vehicle_config_t *config);

/**
 * @brief  Vehicle thread entry point
 *
 * Never returns. Runs at 100Hz:
 *   1. Subscribe to cmd_vel
 *   2. Compute wheel commands (Ackermann mixer)
 *   3. Integrate odometry
 *   4. Publish vehicle_state + motor_cmd + servo_cmd
 *
 * @param  arg1  Unused
 * @param  arg2  Unused
 * @param  arg3  Unused
 */
void vehicle_thread(void *arg1, void *arg2, void *arg3);

/**
 * @brief  Reset odometry to (0, 0, 0)
 */
void vehicle_reset_odometry(void);

#ifdef __cplusplus
}
#endif
