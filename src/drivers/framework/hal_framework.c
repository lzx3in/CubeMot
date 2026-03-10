/**
 * HAL Framework Implementation
 *
 * Provides registration and lookup mechanisms for HAL implementations.
 */

#include "drivers/framework/hal_framework.h"

static bool s_framework_initialized = false;

void hal_framework_init(void)
{
    s_framework_initialized = true;
}

bool hal_framework_is_initialized(void)
{
    return s_framework_initialized;
}
