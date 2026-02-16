#ifndef DRIVERS_LED_H
#define DRIVERS_LED_H

#include <stdbool.h>

struct board_led_config_t;

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
    const struct board_led_config_t *hw_config;
} led_t;

led_error_t led_init(led_t *led, const struct board_led_config_t *hw_config);
led_error_t led_set_state(led_t *led, led_state_t state);
led_error_t led_toggle(led_t *led);
led_error_t led_get_state(led_t *led, led_state_t *state);

#endif
