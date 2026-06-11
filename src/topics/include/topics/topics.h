#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "msghub/msghub.h"

// ============================================================================
// Button Topics
// ============================================================================

typedef struct {
    uint8_t button_id;
    bool pressed;
    uint32_t timestamp;
} button_event_t;

MSGHUB_TOPIC_DECLARE(button_event);

// ============================================================================
// LED Topics
// ============================================================================

typedef struct {
    uint8_t led_id;
    bool state;
    uint32_t timestamp;
} led_state_t;

typedef struct {
    uint8_t led_id;
    bool state;
} led_cmd_t;

MSGHUB_TOPIC_DECLARE(led_state);
MSGHUB_TOPIC_DECLARE(led_command);

// ============================================================================
// Motor Control Topics
// ============================================================================

typedef enum {
    MOTOR_CMD_STOP = 0,
    MOTOR_CMD_START,
    MOTOR_CMD_EMERGENCY
} motor_cmd_type_t;

typedef struct {
    uint8_t motor_id;      // Motor index (0-based)
    motor_cmd_type_t cmd;  // Command type
    float target_speed_rpm;// Target speed [RPM] for START
    float ramp_time_ms;    // Ramp duration [ms], 0 = instant
} motor_cmd_t;

typedef struct {
    uint8_t motor_id;
    uint8_t state;         // 0=IDLE,1=ALIGN,2=START,3=RUN,4=FAULT,5=STOP
    float speed_rpm;       // Current mechanical speed [RPM]
    float i_d;             // D-axis current [A]
    float i_q;             // Q-axis current [A]
    float v_bus;           // Bus voltage [V]
    uint16_t faults;       // Fault bitmask
    uint32_t timestamp;
} motor_state_t;

MSGHUB_TOPIC_DECLARE(motor_cmd);
MSGHUB_TOPIC_DECLARE(motor_state);

// ============================================================================
// Commander Topics
// ============================================================================

typedef enum {
    COMMANDER_STATE_INIT = 0,
    COMMANDER_STATE_STANDBY,
    COMMANDER_STATE_ARMED,
    COMMANDER_STATE_ACTIVE,
    COMMANDER_STATE_FAULT
} commander_state_t;

typedef struct {
    commander_state_t state;
    uint16_t fault_code;
    uint32_t timestamp;
} commander_status_t;

MSGHUB_TOPIC_DECLARE(commander_status);

typedef enum {
    CMD_OP_NOP = 0,
    CMD_OP_ARM,
    CMD_OP_DISARM,
    CMD_OP_ESTOP,
    CMD_OP_RESET_FAULT,
} cmd_op_t;

typedef struct {
    cmd_op_t op;
    uint32_t timestamp;
} commander_cmd_t;

MSGHUB_TOPIC_DECLARE(commander_cmd);

// ============================================================================
// Vehicle Motion Topics
// ============================================================================

typedef struct {
    float linear_x;     // Forward velocity [m/s]
    float angular_z;    // Yaw rate [rad/s]
    uint32_t timestamp;
} cmd_vel_t;

typedef struct {
    float x;            // Position X [m]
    float y;            // Position Y [m]
    float yaw;          // Heading [rad]
    float linear_x;     // Forward velocity [m/s]
    float angular_z;    // Yaw rate [rad/s]
    uint32_t timestamp;
} vehicle_state_t;

MSGHUB_TOPIC_DECLARE(cmd_vel);
MSGHUB_TOPIC_DECLARE(vehicle_state);

// ============================================================================
// Servo Topics
// ============================================================================

typedef struct {
    uint8_t servo_id;
    float angle_deg;    // Target angle [-45, 45]
    uint32_t timestamp;
} servo_cmd_t;

typedef struct {
    uint8_t servo_id;
    float angle_deg;    // Current angle
    uint32_t timestamp;
} servo_state_t;

MSGHUB_TOPIC_DECLARE(servo_cmd);
MSGHUB_TOPIC_DECLARE(servo_state);

#ifdef __cplusplus
}
#endif
