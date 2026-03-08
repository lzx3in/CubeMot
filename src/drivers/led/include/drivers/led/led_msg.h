#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t led_id;     // LED ID: 0, 1, 2...
    bool state;         // true = ON, false = OFF
    uint32_t timestamp; // System tick time (ms)
} led_state_t;

typedef struct {
    uint8_t led_id; // LED ID
    bool state;     // Target state: true=ON, false=OFF
} led_cmd_t;

#ifdef __cplusplus
}
#endif
