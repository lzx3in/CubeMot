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



#ifdef __cplusplus
}
#endif
