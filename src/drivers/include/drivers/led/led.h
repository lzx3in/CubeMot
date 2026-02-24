#ifndef DRIVERS_LED_H
#define DRIVERS_LED_H

#include <stdbool.h>
#include "boards/led.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LED_SUCCESS = 0,
    LED_ERROR_INVALID_PARAM,
    LED_ERROR_NOT_INITIALIZED
} led_error_t;

typedef enum {
    LED_OFF = 0,
    LED_ON
} led_state_t;

typedef struct {
    board_led_t handle;
} led_t;

led_error_t led_init(led_t *led, int led_id);
led_error_t led_set_state(led_t *led, led_state_t state);
led_error_t led_toggle(led_t *led);
led_error_t led_get_state(led_t *led, led_state_t *state);

#ifdef __cplusplus
}
#endif

#endif
