/**
 * LED HAL Framework Implementation
 *
 * Provides registration and lookup for LED HAL implementations.
 */

#include <stddef.h>
#include "drivers/framework/led_hal_framework.h"

static const led_hal_ops_t *g_led_ops = NULL;

void led_hal_register(const led_hal_ops_t *ops)
{
    if (ops == NULL) {
        return;
    }

    // Prevent duplicate registration
    if (g_led_ops != NULL) {
        // In debug mode, could log a warning here
        return;
    }

    g_led_ops = ops;

    // Auto-initialize if init function is provided
    if (ops->base.init) {
        ops->base.init();
    }
}

const led_hal_ops_t *led_hal_get(void)
{
    return g_led_ops;
}
