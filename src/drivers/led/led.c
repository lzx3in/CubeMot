#include "drivers/led/led.h"
#include "boards/led.h"
#include <stddef.h>

led_error_t led_init(led_t *led, int led_id)
{
    if (led == NULL) {
        return LED_ERROR_INVALID_PARAM;
    }

    led->handle = board_led_get_handle(led_id);
    if (!board_led_is_valid(led->handle)) {
        return LED_ERROR_NOT_INITIALIZED;
    }

    return LED_SUCCESS;
}

led_error_t led_set_state(led_t *led, led_state_t state)
{
    if (led == NULL) {
        return LED_ERROR_INVALID_PARAM;
    }

    if (!board_led_is_valid(led->handle)) {
        return LED_ERROR_NOT_INITIALIZED;
    }

    board_led_set_state(led->handle, (state == LED_ON));
    return LED_SUCCESS;
}

led_error_t led_toggle(led_t *led)
{
    if (led == NULL) {
        return LED_ERROR_INVALID_PARAM;
    }

    if (!board_led_is_valid(led->handle)) {
        return LED_ERROR_NOT_INITIALIZED;
    }

    board_led_toggle(led->handle);
    return LED_SUCCESS;
}

led_error_t led_get_state(led_t *led, led_state_t *state)
{
    if (led == NULL || state == NULL) {
        return LED_ERROR_INVALID_PARAM;
    }

    if (!board_led_is_valid(led->handle)) {
        return LED_ERROR_NOT_INITIALIZED;
    }

    bool hw_state = board_led_get_state(led->handle);
    *state = hw_state ? LED_ON : LED_OFF;

    return LED_SUCCESS;
}
