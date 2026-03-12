#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t button_id;  // Button ID: 0, 1, 2...
    bool pressed;       // true = pressed, false = released
    uint32_t timestamp; // System tick time (ms)
} button_event_t;

#ifdef __cplusplus
}
#endif
