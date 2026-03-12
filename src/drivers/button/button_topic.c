#include "drivers/button/button_topic.h"
#include "drivers/button/button_msg.h"

// Button Event Topic (Published by ISR, subscribed by modules)
MSGHUB_TOPIC_DEFINE(button_event, button_event_t, 4);
msghub_topic_t button_event_topic = MSGHUB_TOPIC(button_event);
