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
