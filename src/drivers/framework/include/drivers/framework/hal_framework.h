#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// HAL Framework - Base Types
// ============================================================================

/**
 * Base HAL operations structure
 * All HAL implementations must provide these
 */
typedef struct {
    const char *name;     /**< Implementation name (e.g., "stm32g4_led") */
    int (*init)(void);    /**< Initialize hardware */
    void (*deinit)(void); /**< Deinitialize hardware */
} hal_base_ops_t;

// ============================================================================
// HAL Framework - Registration API
// ============================================================================

/**
 * Initialize HAL framework
 * Called once at system startup
 */
void hal_framework_init(void);

/**
 * Check if HAL framework is initialized
 */
bool hal_framework_is_initialized(void);

#ifdef __cplusplus
}
#endif
