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

#ifdef __cplusplus
}
#endif
