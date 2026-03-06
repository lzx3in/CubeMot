#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "boards/led.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LED_OK = 0,             /**< Success */
    LED_ERR_NULL = -1,      /**< Null pointer parameter */
    LED_ERR_INVALID = -2,   /**< Invalid parameter or ID */
    LED_ERR_NOMEM = -3,     /**< No memory / pool exhausted */
    LED_ERR_NOT_FOUND = -4, /**< Handle not found */
    LED_ERR_BUSY = -5,      /**< Resource busy */
    LED_ERR_BACKEND = -6,   /**< Backend operation failed */
} led_err_t;

typedef struct {
    board_led_t board_handle;
} led_t;

led_err_t led_init(led_t *led, int led_id);
led_err_t led_set(led_t *led, bool on);
led_err_t led_get(led_t *led, bool *out_on);
led_err_t led_toggle(led_t *led);

#ifdef __cplusplus
}
#endif
