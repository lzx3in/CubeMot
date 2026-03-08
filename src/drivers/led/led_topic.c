#include "drivers/led/led_topic.h"
#include "drivers/led/led_msg.h"

// LED State Broadcast Topic (Published by Driver, subscribable by any task)
MSGHUB_TOPIC_DEFINE(led_state, led_state_t, 4);
msghub_topic_t led_state_topic = MSGHUB_TOPIC(led_state);

// LED Command Receive Topic (Published by any task, subscribed by Driver)
MSGHUB_TOPIC_DEFINE(led_command, led_cmd_t, 4);
msghub_topic_t led_command_topic = MSGHUB_TOPIC(led_command);
