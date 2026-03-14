#pragma once

#include "msghub/msghub.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t button_id;  // Button ID: 0, 1, 2...
    bool pressed;       // true = pressed, false = released
    uint32_t timestamp; // System tick time (ms)
} button_event_t;

MSGHUB_TOPIC_DECLARE(button_event);

#ifdef __cplusplus
}
#endif
