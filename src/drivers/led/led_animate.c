/* LED animation layer implementation */

#include "drivers/led/led_animate.h"
#include <string.h>

#ifndef LED_ANIMATE_MAX_INSTANCES
#define LED_ANIMATE_MAX_INSTANCES 8
#endif

typedef struct {
    uint32_t start_time_ms;
    uint32_t last_update_ms;
    uint8_t phase;
    uint8_t repeat_counter;
    led_animate_status_t status;
} animate_runtime_t;

typedef struct led_animation {
    const char *name;

    /* vtable */
    void (*on_start)(void *cfg, animate_runtime_t *runtime, uint32_t start_time_ms, uint8_t phase_offset);
    uint16_t (*on_update)(void *cfg, animate_runtime_t *runtime, uint32_t current_time_ms);
    led_animate_status_t (*get_status)(const animate_runtime_t *runtime, void *cfg);
    uint32_t (*get_cycle_duration_ms)(void *cfg);

    /* Configuration */
    void *cfg;
    uint16_t cfg_size;

    /* Runtime binding */
    led_level_t bound_led;
    animate_runtime_t runtime;

    /* Callback */
    led_animate_done_t done_callback;
    void *done_user_data;
} animation_instance_t;

static animation_instance_t anim_pool[LED_ANIMATE_MAX_INSTANCES];
static bool pool_initialized = false;

extern led_err_t led_level_set_mode(led_level_t led, bool animating);
extern int led_level_set_brightness_internal(led_level_t led, uint16_t brightness);

static void init_pool(void)
{
    if (pool_initialized) {
        return;
    }
    memset(anim_pool, 0, sizeof(anim_pool));
    pool_initialized = true;
}

static bool is_valid_anim(led_animation_t anim)
{
    if (anim == NULL) {
        return false;
    }
    for (int i = 0; i < LED_ANIMATE_MAX_INSTANCES; i++) {
        if (anim == &anim_pool[i]) {
            return anim->cfg != NULL; /* Must be allocated */
        }
    }
    return false;
}

static animation_instance_t *alloc_animation(void)
{
    for (int i = 0; i < LED_ANIMATE_MAX_INSTANCES; i++) {
        if (anim_pool[i].cfg == NULL) {
            return &anim_pool[i];
        }
    }
    return NULL;
}

static void stop_animation_internal(animation_instance_t *anim)
{
    if (anim->bound_led != NULL) {
        led_level_set_mode(anim->bound_led, false);
    }
    anim->runtime.status = LED_ANIMATE_IDLE;
    anim->bound_led = NULL;
}

typedef struct {
    uint32_t period_ms;
    uint16_t min_brightness;
    uint16_t max_brightness;
    led_level_curve_t curve;
    uint8_t repeat_count;
} breathing_cfg_t;

static void breathing_on_start(void *cfg, animate_runtime_t *runtime, uint32_t start_time_ms, uint8_t phase_offset)
{
    breathing_cfg_t *b = (breathing_cfg_t *)cfg;
    runtime->start_time_ms = start_time_ms;
    runtime->phase = phase_offset;
    runtime->repeat_counter = b->repeat_count;
    runtime->status = LED_ANIMATE_RUNNING;
    (void)phase_offset;
}

extern uint16_t led_level_curve_sine(uint8_t phase);
extern uint16_t led_level_curve_gamma(uint16_t value);

static uint16_t breathing_on_update(void *cfg, animate_runtime_t *runtime, uint32_t current_time_ms)
{
    breathing_cfg_t *b = (breathing_cfg_t *)cfg;

    uint32_t elapsed = current_time_ms - runtime->start_time_ms;
    uint32_t period = b->period_ms ? b->period_ms : 1000;

    /* Calculate phase in cycle (0-255) */
    uint32_t phase_32 = ((uint64_t)(elapsed % period) * 256) / period;
    uint8_t phase = (uint8_t)(phase_32 + runtime->phase);

    /* Get curve value */
    uint16_t curve_value;
    switch (b->curve) {
        case LED_CURVE_SINE: curve_value = led_level_curve_sine(phase); break;
        case LED_CURVE_GAMMA: curve_value = led_level_curve_gamma((uint16_t)(phase_32 * 1000 / 256)); break;
        case LED_CURVE_LINEAR:
        default:
            if (phase < 128) {
                curve_value = phase * 1000 / 128;
            } else {
                curve_value = (255 - phase) * 1000 / 127;
            }
            break;
    }

    /* Map to min-max range */
    uint32_t range = b->max_brightness - b->min_brightness;
    return b->min_brightness + (uint16_t)((curve_value * range) / 1000);
}

static led_animate_status_t breathing_get_status(const animate_runtime_t *runtime, void *cfg)
{
    (void)cfg;
    (void)runtime;
    return LED_ANIMATE_RUNNING; /* Breathing continues until stopped */
}

static uint32_t breathing_get_duration_ms(void *cfg)
{
    breathing_cfg_t *b = (breathing_cfg_t *)cfg;
    return b->period_ms;
}

typedef struct {
    uint32_t on_time_ms;
    uint32_t off_time_ms;
    uint16_t on_brightness;
    uint8_t repeat_count;
} blink_cfg_t;

static void blink_on_start(void *cfg, animate_runtime_t *runtime, uint32_t start_time_ms, uint8_t phase_offset)
{
    blink_cfg_t *b = (blink_cfg_t *)cfg;
    runtime->start_time_ms = start_time_ms;
    runtime->phase = 0;
    runtime->repeat_counter = b->repeat_count;
    runtime->status = LED_ANIMATE_RUNNING;
    (void)phase_offset;
}

static uint16_t blink_on_update(void *cfg, animate_runtime_t *runtime, uint32_t current_time_ms)
{
    blink_cfg_t *b = (blink_cfg_t *)cfg;
    uint32_t elapsed = current_time_ms - runtime->start_time_ms;
    uint32_t cycle_time = b->on_time_ms + b->off_time_ms;

    if (cycle_time == 0) {
        return 0;
    }

    uint32_t pos = elapsed % cycle_time;
    return (pos < b->on_time_ms) ? b->on_brightness : 0;
}

static led_animate_status_t blink_get_status(const animate_runtime_t *runtime, void *cfg)
{
    (void)cfg;
    (void)runtime;
    return LED_ANIMATE_RUNNING;
}

static uint32_t blink_get_duration_ms(void *cfg)
{
    blink_cfg_t *b = (blink_cfg_t *)cfg;
    return b->on_time_ms + b->off_time_ms;
}

typedef struct {
    uint16_t from_brightness;
    uint16_t to_brightness;
    uint32_t duration_ms;
    led_level_curve_t curve;
} fade_cfg_t;

static void fade_on_start(void *cfg, animate_runtime_t *runtime, uint32_t start_time_ms, uint8_t phase_offset)
{
    (void)cfg;
    (void)phase_offset;
    runtime->start_time_ms = start_time_ms;
    runtime->status = LED_ANIMATE_RUNNING;
}

static uint16_t fade_on_update(void *cfg, animate_runtime_t *runtime, uint32_t current_time_ms)
{
    fade_cfg_t *f = (fade_cfg_t *)cfg;

    if (f->duration_ms == 0) {
        return f->to_brightness;
    }

    uint32_t elapsed = current_time_ms - runtime->start_time_ms;

    if (elapsed >= f->duration_ms) {
        return f->to_brightness;
    }

    /* Calculate progress (0-1000) */
    uint32_t progress = (elapsed * 1000) / f->duration_ms;

    /* Apply curve */
    uint16_t curved_progress;
    switch (f->curve) {
        case LED_CURVE_GAMMA: curved_progress = led_level_curve_gamma((uint16_t)progress); break;
        case LED_CURVE_SINE: curved_progress = led_level_curve_sine((uint8_t)(progress * 255 / 2000 + 64)); break;
        case LED_CURVE_LINEAR:
        default: curved_progress = (uint16_t)progress; break;
    }

    /* Interpolate */
    int32_t range = (int32_t)f->to_brightness - (int32_t)f->from_brightness;
    int32_t delta = (range * (int32_t)curved_progress) / 1000;

    return (uint16_t)(f->from_brightness + delta);
}

static led_animate_status_t fade_get_status(const animate_runtime_t *runtime, void *cfg)
{
    fade_cfg_t *f = (fade_cfg_t *)cfg;
    uint32_t elapsed = runtime->last_update_ms - runtime->start_time_ms;

    if (elapsed >= f->duration_ms) {
        return LED_ANIMATE_DONE;
    }
    return LED_ANIMATE_RUNNING;
}

static uint32_t fade_get_duration_ms(void *cfg)
{
    fade_cfg_t *f = (fade_cfg_t *)cfg;
    return f->duration_ms;
}

typedef struct {
    uint16_t brightness;
} constant_cfg_t;

static uint16_t constant_on_update(void *cfg, animate_runtime_t *runtime, uint32_t current_time_ms)
{
    (void)runtime;
    (void)current_time_ms;
    return ((constant_cfg_t *)cfg)->brightness;
}

led_err_t led_animate_breathing_create(const led_animate_breathing_cfg_t *cfg, led_animation_t *out_anim)
{
    if (cfg == NULL || out_anim == NULL) {
        return LED_ERR_NULL;
    }
    if (cfg->period_ms == 0 || cfg->min_brightness > cfg->max_brightness || cfg->max_brightness > LED_LEVEL_MAX) {
        return LED_ERR_INVALID;
    }

    init_pool();

    animation_instance_t *anim = alloc_animation();
    if (anim == NULL) {
        return LED_ERR_NOMEM;
    }

    breathing_cfg_t *b = (breathing_cfg_t *)anim->cfg;
    if (b == NULL) {
        static breathing_cfg_t pool[LED_ANIMATE_MAX_INSTANCES];
        b = &pool[anim - anim_pool];
    }

    /* Store config */
    memcpy(b, cfg, sizeof(*cfg));
    anim->cfg = b;
    anim->cfg_size = sizeof(*b);

    /* Set vtable */
    anim->name = "breathing";
    anim->on_start = breathing_on_start;
    anim->on_update = breathing_on_update;
    anim->get_status = breathing_get_status;
    anim->get_cycle_duration_ms = breathing_get_duration_ms;

    anim->bound_led = NULL;
    anim->done_callback = NULL;

    *out_anim = anim;
    return LED_OK;
}

led_err_t led_animate_blink_create(const led_animate_blink_cfg_t *cfg, led_animation_t *out_anim)
{
    if (cfg == NULL || out_anim == NULL) {
        return LED_ERR_NULL;
    }

    init_pool();

    animation_instance_t *anim = alloc_animation();
    if (anim == NULL) {
        return LED_ERR_NOMEM;
    }

    static blink_cfg_t pool[LED_ANIMATE_MAX_INSTANCES];
    blink_cfg_t *b = &pool[anim - anim_pool];

    memcpy(b, cfg, sizeof(*cfg));
    anim->cfg = b;
    anim->cfg_size = sizeof(*b);

    anim->name = "blink";
    anim->on_start = blink_on_start;
    anim->on_update = blink_on_update;
    anim->get_status = blink_get_status;
    anim->get_cycle_duration_ms = blink_get_duration_ms;

    anim->bound_led = NULL;
    anim->done_callback = NULL;

    *out_anim = anim;
    return LED_OK;
}

led_err_t led_animate_fade_create(const led_animate_fade_cfg_t *cfg, led_animation_t *out_anim)
{
    if (cfg == NULL || out_anim == NULL) {
        return LED_ERR_NULL;
    }
    if (cfg->from_brightness > LED_LEVEL_MAX || cfg->to_brightness > LED_LEVEL_MAX) {
        return LED_ERR_INVALID;
    }

    init_pool();

    animation_instance_t *anim = alloc_animation();
    if (anim == NULL) {
        return LED_ERR_NOMEM;
    }

    static fade_cfg_t pool[LED_ANIMATE_MAX_INSTANCES];
    fade_cfg_t *f = &pool[anim - anim_pool];

    memcpy(f, cfg, sizeof(*cfg));
    anim->cfg = f;
    anim->cfg_size = sizeof(*f);

    anim->name = "fade";
    anim->on_start = fade_on_start;
    anim->on_update = fade_on_update;
    anim->get_status = fade_get_status;
    anim->get_cycle_duration_ms = fade_get_duration_ms;

    anim->bound_led = NULL;
    anim->done_callback = NULL;

    *out_anim = anim;
    return LED_OK;
}

led_err_t led_animate_constant_create(uint16_t brightness, led_animation_t *out_anim)
{
    if (out_anim == NULL) {
        return LED_ERR_NULL;
    }
    if (brightness > LED_LEVEL_MAX) {
        return LED_ERR_INVALID;
    }

    init_pool();

    animation_instance_t *anim = alloc_animation();
    if (anim == NULL) {
        return LED_ERR_NOMEM;
    }

    static constant_cfg_t pool[LED_ANIMATE_MAX_INSTANCES];
    constant_cfg_t *c = &pool[anim - anim_pool];

    c->brightness = brightness;
    anim->cfg = c;
    anim->cfg_size = sizeof(*c);

    anim->name = "constant";
    anim->on_start = NULL;
    anim->on_update = constant_on_update;
    anim->get_status = NULL;
    anim->get_cycle_duration_ms = NULL;

    anim->bound_led = NULL;
    anim->done_callback = NULL;

    *out_anim = anim;
    return LED_OK;
}

led_err_t led_animate_destroy(led_animation_t anim)
{
    if (!is_valid_anim(anim)) {
        return LED_ERR_NOT_FOUND;
    }

    stop_animation_internal(anim);
    anim->cfg = NULL; /* Mark as free */

    return LED_OK;
}

led_err_t led_animate_get_name(led_animation_t anim, const char **out_name)
{
    if (!is_valid_anim(anim)) {
        return LED_ERR_NOT_FOUND;
    }
    if (out_name == NULL) {
        return LED_ERR_NULL;
    }

    *out_name = anim->name ? anim->name : "unknown";
    return LED_OK;
}

led_err_t led_animate_get_status(led_animation_t anim, led_animate_status_t *out_status)
{
    if (!is_valid_anim(anim)) {
        return LED_ERR_NOT_FOUND;
    }
    if (out_status == NULL) {
        return LED_ERR_NULL;
    }

    *out_status = anim->runtime.status;
    return LED_OK;
}

led_err_t led_animate_play_with_callback(led_level_t led, led_animation_t anim, uint8_t phase_offset,
                                         led_animate_done_t on_done, void *user_data)
{
    if (led == NULL || anim == NULL) {
        return LED_ERR_NULL;
    }
    if (!is_valid_anim(anim)) {
        return LED_ERR_NOT_FOUND;
    }

    /* Stop any existing animation on this LED */
    for (int i = 0; i < LED_ANIMATE_MAX_INSTANCES; i++) {
        if (anim_pool[i].bound_led == led) {
            stop_animation_internal(&anim_pool[i]);
        }
    }

    /* Stop any existing LED binding for this animation */
    if (anim->bound_led != NULL && anim->bound_led != led) {
        led_level_set_mode(anim->bound_led, false);
    }

    /* Bind and start */
    anim->bound_led = led;
    anim->done_callback = on_done;
    anim->done_user_data = user_data;

    memset(&anim->runtime, 0, sizeof(anim->runtime));
    anim->runtime.status = LED_ANIMATE_RUNNING;

    if (anim->on_start != NULL) {
        anim->on_start(anim->cfg, &anim->runtime, 0, phase_offset);
    }

    led_level_set_mode(led, true);

    return LED_OK;
}

led_err_t led_animate_play(led_level_t led, led_animation_t anim)
{
    return led_animate_play_with_callback(led, anim, 0, NULL, NULL);
}

led_err_t led_animate_play_with_phase(led_level_t led, led_animation_t anim, uint8_t phase_offset)
{
    return led_animate_play_with_callback(led, anim, phase_offset, NULL, NULL);
}

led_err_t led_animate_stop(led_level_t led)
{
    if (led == NULL) {
        return LED_ERR_NULL;
    }

    for (int i = 0; i < LED_ANIMATE_MAX_INSTANCES; i++) {
        if (anim_pool[i].bound_led == led) {
            stop_animation_internal(&anim_pool[i]);
            return LED_OK;
        }
    }

    return LED_ERR_NOT_FOUND;
}

led_err_t led_animate_is_active(led_level_t led, bool *out_active)
{
    if (led == NULL || out_active == NULL) {
        return LED_ERR_NULL;
    }

    for (int i = 0; i < LED_ANIMATE_MAX_INSTANCES; i++) {
        if (anim_pool[i].bound_led == led && anim_pool[i].runtime.status == LED_ANIMATE_RUNNING) {
            *out_active = true;
            return LED_OK;
        }
    }

    *out_active = false;
    return LED_OK;
}

void led_animate_update_all(uint32_t time_ms)
{
    for (int i = 0; i < LED_ANIMATE_MAX_INSTANCES; i++) {
        animation_instance_t *anim = &anim_pool[i];

        if (anim->cfg == NULL || anim->bound_led == NULL) {
            continue;
        }
        if (anim->runtime.status != LED_ANIMATE_RUNNING) {
            continue;
        }

        /* Update brightness */
        uint16_t brightness = anim->on_update(anim->cfg, &anim->runtime, time_ms);

        if (brightness > LED_LEVEL_MAX) {
            brightness = LED_LEVEL_MAX;
        }

        led_level_set_brightness_internal(anim->bound_led, brightness);
        anim->runtime.last_update_ms = time_ms;

        /* Check completion */
        if (anim->get_status != NULL) {
            led_animate_status_t status = anim->get_status(&anim->runtime, anim->cfg);
            if (status == LED_ANIMATE_DONE) {
                stop_animation_internal(anim);
                if (anim->done_callback != NULL) {
                    anim->done_callback(anim->bound_led, anim->done_user_data);
                }
            }
        }
    }
}
