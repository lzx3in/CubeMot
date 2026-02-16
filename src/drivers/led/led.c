#include "drivers/led/led.h"
#include "boards/led.h"

led_error_t led_init(led_t *led, const struct board_led_config_t *hw_config)
{
    if (led == NULL || hw_config == NULL) {
        return LED_ERROR_INVALID_PARAM;
    }

    led->hw_config = hw_config;
    return LED_SUCCESS;
}

led_error_t led_set_state(led_t *led, led_state_t state)
{
    if (led == NULL) {
        return LED_ERROR_INVALID_PARAM;
    }

    if (led->hw_config == NULL) {
        return LED_ERROR_NOT_INITIALIZED;
    }

    board_led_set_state(led->hw_config, (state == LED_ON) ? true : false);

    return LED_SUCCESS;
}

led_error_t led_toggle(led_t *led)
{
    if (led == NULL) {
        return LED_ERROR_INVALID_PARAM;
    }

    if (led->hw_config == NULL) {
        return LED_ERROR_NOT_INITIALIZED;
    }

    board_led_toggle(led->hw_config);

    return LED_SUCCESS;
}

led_error_t led_get_state(led_t *led, led_state_t *state)
{
    if (led == NULL || state == NULL) {
        return LED_ERROR_INVALID_PARAM;
    }

    if (led->hw_config == NULL) {
        return LED_ERROR_NOT_INITIALIZED;
    }

    bool hw_state = board_led_get_state(led->hw_config);
    *state = hw_state ? LED_ON : LED_OFF;

    return LED_SUCCESS;
}
