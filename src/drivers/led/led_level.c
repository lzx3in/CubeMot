/* LED brightness level layer implementation */

#include "drivers/led/led_level.h"
#include <string.h>

#ifndef LED_LEVEL_MAX_INSTANCES
#define LED_LEVEL_MAX_INSTANCES 8
#endif

typedef enum {
    LEVEL_MODE_IDLE = 0,
    LEVEL_MODE_ANIMATING = 1,
} level_mode_t;

struct led_level {
    char name[16];
    led_level_backend_t backend;
    bool backend_registered;
    uint16_t current_brightness;
    level_mode_t mode;
    bool in_update; /* Reentrancy guard */
};

static struct led_level level_pool[LED_LEVEL_MAX_INSTANCES];
static bool pool_initialized = false;

/* Sine lookup table: 256 entries, 0-1000 range */
static const uint16_t sine_table[256] = {
    500, 512, 525, 537, 549, 561, 573, 585, 598, 610, 621, 633, 645, 657, 668, 680, 691, 703, 714, 725,  736,  746,
    757, 767, 778, 788, 798, 808, 817, 827, 836, 845, 854, 862, 870, 879, 887, 894, 902, 909, 916, 922,  929,  935,
    941, 947, 952, 957, 962, 966, 971, 975, 978, 982, 985, 988, 990, 993, 995, 996, 998, 999, 999, 1000, 1000, 1000,
    999, 999, 998, 996, 995, 993, 990, 988, 985, 982, 978, 975, 971, 966, 962, 957, 952, 947, 941, 935,  929,  922,
    916, 909, 902, 894, 887, 879, 870, 862, 854, 845, 836, 827, 817, 808, 798, 788, 778, 767, 757, 746,  736,  725,
    714, 703, 691, 680, 668, 657, 645, 633, 621, 610, 598, 585, 573, 561, 549, 537, 525, 512, 500, 488,  475,  463,
    451, 439, 427, 415, 402, 390, 379, 367, 355, 343, 332, 320, 309, 297, 286, 275, 264, 254, 243, 233,  222,  212,
    202, 192, 183, 173, 164, 155, 146, 138, 130, 121, 113, 106, 98,  91,  84,  78,  71,  65,  59,  53,   48,   43,
    38,  34,  29,  25,  22,  18,  15,  12,  10,  7,   5,   4,   2,   1,   1,   0,   0,   0,   1,   1,    2,    4,
    5,   7,   10,  12,  15,  18,  22,  25,  29,  34,  38,  43,  48,  53,  59,  65,  71,  78,  84,  91,   98,   106,
    113, 121, 130, 138, 146, 155, 164, 173, 183, 192, 202, 212, 222, 233, 243, 254, 264, 275, 286, 297,  309,  320,
    332, 343, 355, 367, 379, 390, 402, 415, 427, 439, 451, 463, 475, 488};

/* Gamma 2.2 lookup table: 0-100 input maps to indices 0-100 */
static const uint16_t gamma_table[101] = {
    0,   0,   0,   0,   1,   1,   2,   3,   4,   5,   6,   8,   9,   11,  13,  15,  18,  20,  23,  26,  29,
    32,  36,  39,  43,  47,  52,  56,  61,  66,  71,  76,  82,  87,  93,  99,  106, 112, 119, 126, 133, 141,
    148, 156, 164, 173, 181, 190, 199, 208, 218, 227, 237, 247, 258, 268, 279, 290, 302, 313, 325, 337, 349,
    362, 375, 388, 401, 414, 428, 442, 456, 471, 485, 500, 516, 531, 547, 563, 579, 595, 612, 629, 646, 664,
    681, 699, 718, 736, 755, 774, 793, 813, 832, 852, 873, 893, 914, 935, 957, 978, 1000};

static void init_pool(void)
{
    if (pool_initialized) {
        return;
    }
    memset(level_pool, 0, sizeof(level_pool));
    pool_initialized = true;
}

static bool is_valid_led(led_level_t led)
{
    if (led == NULL) {
        return false;
    }
    for (int i = 0; i < LED_LEVEL_MAX_INSTANCES; i++) {
        if (led == &level_pool[i]) {
            return true;
        }
    }
    return false;
}

static int set_backend_brightness(led_level_t led, uint16_t brightness)
{
    if (!led->backend_registered || led->backend.set_brightness == NULL) {
        return -1;
    }
    return led->backend.set_brightness(led->backend.user_data, brightness);
}

uint16_t led_level_curve_sine(uint8_t phase)
{
    return sine_table[phase];
}

uint16_t led_level_curve_gamma(uint16_t value)
{
    if (value >= 1000) {
        return 1000;
    }
    uint16_t idx = value / 10;
    uint16_t frac = value % 10;

    if (idx >= 100) {
        return 1000;
    }

    uint16_t v0 = gamma_table[idx];
    uint16_t v1 = gamma_table[idx + 1];
    return v0 + ((v1 - v0) * frac) / 10;
}

uint16_t led_level_curve_apply(uint16_t value, led_level_curve_t curve)
{
    if (value > LED_LEVEL_MAX) {
        value = LED_LEVEL_MAX;
    }

    switch (curve) {
        case LED_CURVE_LINEAR: return value;
        case LED_CURVE_SINE: return led_level_curve_sine((uint8_t)(value * 255 / 1000));
        case LED_CURVE_GAMMA: return led_level_curve_gamma(value);
        default: return value;
    }
}

led_err_t led_level_create(const char *name, const led_level_backend_t *backend, led_level_t *out_led)
{
    if (out_led == NULL) {
        return LED_ERR_NULL;
    }

    init_pool();

    for (int i = 0; i < LED_LEVEL_MAX_INSTANCES; i++) {
        if (!level_pool[i].backend_registered) {
            led_level_t led = &level_pool[i];
            memset(led, 0, sizeof(*led));

            if (name != NULL) {
                strncpy(led->name, name, sizeof(led->name) - 1);
                led->name[sizeof(led->name) - 1] = '\0';
            }

            led->mode = LEVEL_MODE_IDLE;
            led->current_brightness = 0;

            if (backend != NULL) {
                led_level_register_backend(led, backend);
            }

            *out_led = led;
            return LED_OK;
        }
    }

    return LED_ERR_NOMEM;
}

led_err_t led_level_destroy(led_level_t led)
{
    if (!is_valid_led(led)) {
        return LED_ERR_NOT_FOUND;
    }
    if (led->mode == LEVEL_MODE_ANIMATING) {
        return LED_ERR_BUSY;
    }

    led->backend_registered = false;
    led->name[0] = '\0';

    return LED_OK;
}

led_err_t led_level_register_backend(led_level_t led, const led_level_backend_t *backend)
{
    if (!is_valid_led(led)) {
        return LED_ERR_NOT_FOUND;
    }
    if (backend == NULL) {
        return LED_ERR_NULL;
    }
    if (backend->set_brightness == NULL) {
        return LED_ERR_INVALID;
    }

    led->backend = *backend;
    led->backend_registered = true;

    /* Initialize to off */
    set_backend_brightness(led, 0);
    led->current_brightness = 0;

    return LED_OK;
}

led_err_t led_level_set(led_level_t led, uint16_t brightness)
{
    if (!is_valid_led(led)) {
        return LED_ERR_NOT_FOUND;
    }
    if (!led->backend_registered) {
        return LED_ERR_INVALID;
    }
    if (brightness > LED_LEVEL_MAX) {
        return LED_ERR_INVALID;
    }

    /* Stop any animation */
    led->mode = LEVEL_MODE_IDLE;

    int result = set_backend_brightness(led, brightness);
    if (result != 0) {
        return LED_ERR_BACKEND;
    }

    led->current_brightness = brightness;
    return LED_OK;
}

led_err_t led_level_get(led_level_t led, uint16_t *out_brightness)
{
    if (!is_valid_led(led)) {
        return LED_ERR_NOT_FOUND;
    }
    if (out_brightness == NULL) {
        return LED_ERR_NULL;
    }

    *out_brightness = led->current_brightness;
    return LED_OK;
}

led_err_t led_level_is_active(led_level_t led, bool *out_active)
{
    if (!is_valid_led(led)) {
        return LED_ERR_NOT_FOUND;
    }
    if (out_active == NULL) {
        return LED_ERR_NULL;
    }

    *out_active = (led->mode == LEVEL_MODE_ANIMATING);
    return LED_OK;
}

led_err_t led_level_set_mode(led_level_t led, bool animating);

led_err_t led_level_set_mode(led_level_t led, bool animating)
{
    if (!is_valid_led(led)) {
        return LED_ERR_NOT_FOUND;
    }
    led->mode = animating ? LEVEL_MODE_ANIMATING : LEVEL_MODE_IDLE;
    return LED_OK;
}

int led_level_set_brightness_internal(led_level_t led, uint16_t brightness)
{
    if (!is_valid_led(led) || !led->backend_registered) {
        return -1;
    }
    int result = set_backend_brightness(led, brightness);
    if (result == 0) {
        led->current_brightness = brightness;
    }
    return result;
}

led_err_t led_level_update_all(uint32_t time_ms)
{
    /* Animation layer will iterate and update */
    (void)time_ms;
    return LED_OK;
}
