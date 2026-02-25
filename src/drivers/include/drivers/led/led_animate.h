/* LED animation layer (effects and sequences) */

#ifndef LED_ANIMATE_H
#define LED_ANIMATE_H

#include "drivers/led/led_level.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct led_animation *led_animation_t;

typedef enum {
    LED_ANIMATE_IDLE = 0,    /* Not started or stopped */
    LED_ANIMATE_RUNNING = 1, /* Active and updating */
    LED_ANIMATE_DONE = 2,    /* Completed */
} led_animate_status_t;

typedef void (*led_animate_done_t)(led_level_t led, void *user_data);

typedef struct {
    uint32_t period_ms;      /* Full cycle duration */
    uint16_t min_brightness; /* Dark end (typically 10-100) */
    uint16_t max_brightness; /* Bright end (typically 800-1000) */
    led_level_curve_t curve; /* Curve type for transition */
    uint8_t repeat_count;    /* 0 = infinite */
} led_animate_breathing_cfg_t;

led_err_t led_animate_breathing_create(const led_animate_breathing_cfg_t *cfg, led_animation_t *out_anim);

typedef struct {
    uint32_t on_time_ms;    /* Duration LED stays on */
    uint32_t off_time_ms;   /* Duration LED stays off */
    uint16_t on_brightness; /* Brightness during on phase (0-1000) */
    uint8_t repeat_count;   /* 0 = infinite */
} led_animate_blink_cfg_t;

led_err_t led_animate_blink_create(const led_animate_blink_cfg_t *cfg, led_animation_t *out_anim);

typedef struct {
    uint16_t from_brightness;
    uint16_t to_brightness;
    uint32_t duration_ms;
    led_level_curve_t curve;
} led_animate_fade_cfg_t;

led_err_t led_animate_fade_create(const led_animate_fade_cfg_t *cfg, led_animation_t *out_anim);

led_err_t led_animate_constant_create(uint16_t brightness, led_animation_t *out_anim);

led_err_t led_animate_destroy(led_animation_t anim);
led_err_t led_animate_get_name(led_animation_t anim, const char **out_name);
led_err_t led_animate_get_status(led_animation_t anim, led_animate_status_t *out_status);

led_err_t led_animate_play(led_level_t led, led_animation_t anim);
led_err_t led_animate_play_with_phase(led_level_t led, led_animation_t anim, uint8_t phase_offset);
led_err_t led_animate_play_with_callback(led_level_t led, led_animation_t anim, uint8_t phase_offset,
                                         led_animate_done_t on_done, void *user_data);
led_err_t led_animate_stop(led_level_t led);
led_err_t led_animate_is_active(led_level_t led, bool *out_active);

#ifdef __cplusplus
}
#endif

#endif /* LED_ANIMATE_H */
