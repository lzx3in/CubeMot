/**
 * @file vehicle.c
 * @brief Vehicle kinematics: differential drive + odometry
 *
 * V2 Implementation:
 *   - Differential drive (2 motors: left + right)
 *   - Simple odometry integration (dead reckoning)
 *   - No steering servo yet (that's V3 Ackermann)
 *
 * V3 Future:
 *   - Ackermann steering geometry
 *   - 4 motors + 2 servos
 *   - IMU fusion for better odometry
 */

#include "modules/vehicle/vehicle.h"
#include "modules/commander/commander.h"
#include "topics/topics.h"
#include "common_time.h"
#include "common_error.h"
#include "transform.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(vehicle, LOG_LEVEL_INF);

/* ── Configuration ───────────────────────────────────── */

#define VEHICLE_HZ          100
#define VEHICLE_PERIOD      K_MSEC(10)
#define STATE_PUB_HZ        50
#define STATE_PUB_DIV       (VEHICLE_HZ / STATE_PUB_HZ)

/* ── Default vehicle config (V2: differential drive) ── */

static const vehicle_config_t g_default_config = {
    .wheelbase_m = 0.30f,      // 30cm
    .track_width_m = 0.25f,    // 25cm
    .wheel_radius_m = 0.05f,   // 5cm radius (10cm wheels)
    .max_speed_ms = 1.0f,      // 1 m/s max
    .max_angular_rads = 3.14f, // ~180 deg/s max
    .max_steer_deg = 45.0f,    // Not used in V2
};

/* ── State ───────────────────────────────────────────── */

static vehicle_config_t g_config;

// Odometry state
static float g_odom_x = 0.0f;      // Position X [m]
static float g_odom_y = 0.0f;      // Position Y [m]
static float g_odom_yaw = 0.0f;    // Heading [rad]
static float g_odom_vx = 0.0f;     // Current linear velocity [m/s]
static float g_odom_wz = 0.0f;     // Current angular velocity [rad/s]

/* ── msghub ──────────────────────────────────────────── */

static msghub_subscriber_t g_cmd_vel_sub;
static msghub_publisher_t g_vehicle_state_pub;
static msghub_publisher_t g_motor_cmd_pub;

/* ── Kinematics ──────────────────────────────────────── */

/**
 * @brief  Differential drive inverse kinematics
 *
 * cmd_vel (linear_x, angular_z) → (v_left, v_right) [m/s]
 */
static void diff_drive_inverse_kinematics(
    float linear_x, float angular_z,
    float *v_left, float *v_right)
{
    float L = g_config.track_width_m;
    
    // Differential drive: v = (v_r + v_l)/2, ω = (v_r - v_l)/L
    *v_left = linear_x - angular_z * L / 2.0f;
    *v_right = linear_x + angular_z * L / 2.0f;
}

/**
 * @brief  Convert wheel speed [m/s] → motor RPM
 */
static float ms_to_rpm(float v_ms)
{
    float circumference = 2.0f * 3.14159f * g_config.wheel_radius_m;
    float rps = v_ms / circumference;  // Revolutions per second
    return rps * 60.0f;                // RPM
}

/**
 * @brief  Clamp value to range
 */
static float clampf(float v, float min, float max)
{
    if (v < min) return min;
    if (v > max) return max;
    return v;
}

/**
 * @brief  Normalize angle to [-π, π]
 */
static float normalize_angle(float angle)
{
    const float two_pi = 2.0f * 3.14159f;
    while (angle > 3.14159f) angle -= two_pi;
    while (angle < -3.14159f) angle += two_pi;
    return angle;
}

/* ── Odometry integration ────────────────────────────── */

static void update_odometry(float dt, float v_left, float v_right)
{
    float L = g_config.track_width_m;
    
    // Forward kinematics: wheel speeds → (v, ω)
    float v = (v_right + v_left) / 2.0f;
    float w = (v_right - v_left) / L;
    
    // Integrate position (simple Euler)
    float sin_yaw, cos_yaw;
    fast_sincos(g_odom_yaw, &sin_yaw, &cos_yaw);
    
    g_odom_x += v * cos_yaw * dt;
    g_odom_y += v * sin_yaw * dt;
    g_odom_yaw = normalize_angle(g_odom_yaw + w * dt);
    
    // Store current velocities for state publishing
    g_odom_vx = v;
    g_odom_wz = w;
}

/* ── cmd_vel processing ──────────────────────────────── */

static void process_cmd_vel(const cmd_vel_t *vel)
{
    // Only process commands when Commander is in ACTIVE state
    if (commander_get_state() != CMD_STATE_ACTIVE) {
        return;
    }
    
    // Clamp inputs
    float linear = clampf(vel->linear_x,
                          -g_config.max_speed_ms,
                          g_config.max_speed_ms);
    float angular = clampf(vel->angular_z,
                           -g_config.max_angular_rads,
                           g_config.max_angular_rads);
    
    // Inverse kinematics
    float v_left, v_right;
    diff_drive_inverse_kinematics(linear, angular, &v_left, &v_right);
    
    // Convert to RPM
    float rpm_left = ms_to_rpm(v_left);
    float rpm_right = ms_to_rpm(v_right);
    
    // Clamp to motor limits
    const float MAX_RPM = 1500.0f;
    rpm_left = clampf(rpm_left, -MAX_RPM, MAX_RPM);
    rpm_right = clampf(rpm_right, -MAX_RPM, MAX_RPM);
    
    // Send motor commands
    // Motor 0 = left, Motor 1 = right
    motor_cmd_t cmd_left = {
        .motor_id = 0,
        .cmd = (rpm_left != 0.0f) ? MOTOR_CMD_START : MOTOR_CMD_STOP,
        .target_speed_rpm = rpm_left,
        .ramp_time_ms = 0.0f,
    };
    motor_cmd_t cmd_right = {
        .motor_id = 1,
        .cmd = (rpm_right != 0.0f) ? MOTOR_CMD_START : MOTOR_CMD_STOP,
        .target_speed_rpm = rpm_right,
        .ramp_time_ms = 0.0f,
    };
    
    msghub_publish(g_motor_cmd_pub, &cmd_left);
    msghub_publish(g_motor_cmd_pub, &cmd_right);
    
    // Update odometry with actual commanded speeds
    float dt = 1.0f / VEHICLE_HZ;
    update_odometry(dt, v_left, v_right);
}

/* ── Init ────────────────────────────────────────────── */

int vehicle_init(const vehicle_config_t *config)
{
    if (config == NULL) {
        g_config = g_default_config;
    } else {
        g_config = *config;
    }
    
    g_cmd_vel_sub = msghub_create_subscriber(MSGHUB_TOPIC(cmd_vel), 0);
    g_vehicle_state_pub = msghub_create_publisher(MSGHUB_TOPIC(vehicle_state));
    g_motor_cmd_pub = msghub_create_publisher(MSGHUB_TOPIC(motor_cmd));
    
    // Reset odometry
    g_odom_x = 0.0f;
    g_odom_y = 0.0f;
    g_odom_yaw = 0.0f;
    g_odom_vx = 0.0f;
    g_odom_wz = 0.0f;
    
    LOG_INF("Vehicle init: %dcm wheelbase, %dcm track, %dcm wheels",
            (int)(g_config.wheelbase_m * 100.0f),
            (int)(g_config.track_width_m * 100.0f),
            (int)(g_config.wheel_radius_m * 100.0f));
    return 0;
}

/* ── Main thread ─────────────────────────────────────── */

void vehicle_thread(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    /* Startup delay: allow msghub topics and downstream modules to initialize */
    k_sleep(K_MSEC(100));
    LOG_INF("Vehicle thread started");
    
    uint32_t pub_counter = 0;
    
    while (1) {
        /* ── Check cmd_vel ───────────────────────────── */
        cmd_vel_t vel;
        bool updated = false;
        msghub_subscriber_update(g_cmd_vel_sub, &vel, &updated);
        if (updated) {
            process_cmd_vel(&vel);
        }
        
        /* ── Publish vehicle_state @ 50Hz ────────────── */
        if (++pub_counter >= STATE_PUB_DIV) {
            pub_counter = 0;
            
            vehicle_state_t state = {
                .x = g_odom_x,
                .y = g_odom_y,
                .yaw = g_odom_yaw,
                .linear_x = g_odom_vx,
                .angular_z = g_odom_wz,
                .timestamp = common_get_timestamp_ms(),
            };
            msghub_publish(g_vehicle_state_pub, &state);
        }
        
        k_sleep(VEHICLE_PERIOD);
    }
}

/* ── Reset odometry ──────────────────────────────────── */

void vehicle_reset_odometry(void)
{
    g_odom_x = 0.0f;
    g_odom_y = 0.0f;
    g_odom_yaw = 0.0f;
    LOG_INF("Odometry reset to (0, 0, 0)");
}
