#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "msghub/msghub.h"

typedef struct {
    uint8_t button_id;
    bool pressed;
    uint32_t timestamp;
} button_event_t;

MSGHUB_TOPIC_DECLARE(button_event);

#ifdef __cplusplus
}
#endif
