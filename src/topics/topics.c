#include "msghub/msghub.h"
#include "topics/topics.h"

// ============================================================================
// Button Topics
// ============================================================================

MSGHUB_TOPIC_DEFINE(button_event, button_event_t, 4);

// ============================================================================
// LED Topics
// ============================================================================

MSGHUB_TOPIC_DEFINE(led_state, led_state_t, 4);
MSGHUB_TOPIC_DEFINE(led_command, led_cmd_t, 4);

MSGHUB_TOPIC_DEFINE(motor_cmd, motor_cmd_t, 4);
MSGHUB_TOPIC_DEFINE(motor_state, motor_state_t, 4);

// ============================================================================
// Commander Topics
// ============================================================================

MSGHUB_TOPIC_DEFINE(commander_status, commander_status_t, 4);
MSGHUB_TOPIC_DEFINE(commander_cmd, commander_cmd_t, 4);

// ============================================================================
// Vehicle Motion Topics
// ============================================================================

MSGHUB_TOPIC_DEFINE(cmd_vel, cmd_vel_t, 4);
MSGHUB_TOPIC_DEFINE(vehicle_state, vehicle_state_t, 4);

// ============================================================================
// Servo Topics
// ============================================================================

MSGHUB_TOPIC_DEFINE(servo_cmd, servo_cmd_t, 4);
MSGHUB_TOPIC_DEFINE(servo_state, servo_state_t, 4);
