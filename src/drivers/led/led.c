#include "drivers/led/led.h"
#include <stddef.h>

led_err_t led_init(led_t *led, int led_id)
{
    if (led == NULL) {
        return LED_ERR_NULL;
    }

    led->board_handle = board_led_get_handle(led_id);
    if (!board_led_is_valid(led->board_handle)) {
        return LED_ERR_INVALID;
    }

    return LED_OK;
}

led_err_t led_set(led_t *led, bool on)
{
    if (led == NULL) {
        return LED_ERR_NULL;
    }

    if (!board_led_is_valid(led->board_handle)) {
        return LED_ERR_INVALID;
    }

    board_led_set_state(led->board_handle, on);
    return LED_OK;
}

led_err_t led_get(led_t *led, bool *out_on)
{
    if (led == NULL || out_on == NULL) {
        return LED_ERR_NULL;
    }

    if (!board_led_is_valid(led->board_handle)) {
        return LED_ERR_INVALID;
    }

    *out_on = board_led_get_state(led->board_handle);
    return LED_OK;
}

led_err_t led_toggle(led_t *led)
{
    if (led == NULL) {
        return LED_ERR_NULL;
    }

    if (!board_led_is_valid(led->board_handle)) {
        return LED_ERR_INVALID;
    }

    board_led_toggle(led->board_handle);
    return LED_OK;
}
