/* LED brightness level layer (0-1000 dimming) */

#ifndef LED_LEVEL_H
#define LED_LEVEL_H

#include "drivers/led/led.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct led_level *led_level_t;

/* Brightness range: 0 = off, 1000 = maximum */
#define LED_LEVEL_MIN 0
#define LED_LEVEL_MAX 1000

/* Backend interface for brightness control */
typedef struct {
    /* Set physical brightness (0-1000). Return 0 on success. */
    int (*set_brightness)(void *user_data, uint16_t brightness);
    void *user_data;
} led_level_backend_t;

typedef enum {
    LED_CURVE_LINEAR = 0, /* Linear: y = x */
    LED_CURVE_SINE = 1,   /* Sine: y = (sin(x) + 1) / 2 */
    LED_CURVE_GAMMA = 2   /* Gamma: y = x^2.2 */
} led_level_curve_t;

/* Create LED level instance. Backend may be NULL for delayed binding. */
led_err_t led_level_create(const led_level_backend_t *backend, led_level_t *out_led);

/* Destroy LED level instance. Returns LED_ERR_BUSY if animations active. */
led_err_t led_level_destroy(led_level_t led);

/* Register or re-register backend. */
led_err_t led_level_register_backend(led_level_t led, const led_level_backend_t *backend);

/* Set brightness (0-1000). Returns LED_ERR_INVALID if out of range. */
led_err_t led_level_set(led_level_t led, uint16_t brightness);

/* Get current brightness */
led_err_t led_level_get(led_level_t led, uint16_t *out_brightness);

/* Check if LED has an active animation */
led_err_t led_level_is_active(led_level_t led, bool *out_active);

/* Update all LEDs. Call at 50-100 Hz minimum for smooth visuals. */
led_err_t led_level_update_all(uint32_t time_ms);

/* Apply curve transformation to normalized value (0-1000 input/output) */
uint16_t led_level_curve_apply(uint16_t value, led_level_curve_t curve);

/* Calculate SINE curve for phase (0-255), returns 0-1000 */
uint16_t led_level_curve_sine(uint8_t phase);

/* Calculate GAMMA 2.2 curve, returns 0-1000 */
uint16_t led_level_curve_gamma(uint16_t value);

#ifdef __cplusplus
}
#endif

#endif /* LED_LEVEL_H */
