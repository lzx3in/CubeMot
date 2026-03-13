/**
 * Button HAL Framework Implementation
 *
 * Provides registration and lookup for Button HAL implementations.
 */

#include <stddef.h>
#include "drivers/framework/button_hal_framework.h"
#include "common_error.h"

static const button_hal_ops_t *g_button_ops = NULL;

cubemot_err_t button_hal_register(const button_hal_ops_t *ops)
{
    if (ops == NULL) {
        return CUBEMOT_DRIVER_BUTTON_ERR_INVALID;
    }

    // Prevent duplicate registration
    if (g_button_ops != NULL) {
        // In debug mode, could log a warning here
        return CUBEMOT_DRIVER_BUTTON_ERR;
    }

    g_button_ops = ops;

    // Auto-initialize if init function is provided
    if (ops->base.init) {
        int ret = ops->base.init();
        if (ret != 0) {
            g_button_ops = NULL; // Rollback registration on init failure
            return CUBEMOT_DRIVER_BUTTON_ERR_HAL;
        }
    }

    return CUBEMOT_DRIVER_BUTTON_OK;
}

const button_hal_ops_t *button_hal_get(void)
{
    return g_button_ops;
}
