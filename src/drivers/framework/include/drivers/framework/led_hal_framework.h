#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "drivers/framework/hal_framework.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// LED HAL Framework Interface
// ============================================================================

/**
 * LED HAL operations structure
 * Board-specific implementations must provide these functions
 */
typedef struct {
    hal_base_ops_t base;

    /** Get number of available LEDs */
    int (*get_count)(void);

    /** Set LED state */
    void (*set_state)(uint8_t led_id, bool state);

    /** Toggle LED state */
    void (*toggle)(uint8_t led_id);

    /** Get current LED state */
    bool (*get_state)(uint8_t led_id);
} led_hal_ops_t;

// ============================================================================
// LED HAL Framework - Registration API
// ============================================================================

/**
 * Register LED HAL implementation
 * Must be called during board initialization
 *
 * @param ops LED HAL operations structure
 */
void led_hal_register(const led_hal_ops_t *ops);

/**
 * Get registered LED HAL implementation
 *
 * @return Pointer to registered ops, or NULL if not registered
 */
const led_hal_ops_t *led_hal_get(void);

// ============================================================================
// LED HAL Framework - Convenience Macros
// ============================================================================

/**
 * Get number of LEDs (safe macro)
 */
#define LED_COUNT() led_hal_count()

/**
 * Set LED state (safe macro)
 */
#define LED_SET(id, state) led_hal_set((id), (state))

/**
 * Toggle LED state (safe macro)
 */
#define LED_TOGGLE(id) led_hal_toggle((id))

/**
 * Get LED state (safe macro)
 */
#define LED_GET(id) led_hal_get_state((id))

// ============================================================================
// LED HAL Framework - Wrapper Functions
// ============================================================================

/**
 * Get number of LEDs (wrapper with NULL check)
 */
static inline int led_hal_count(void)
{
    const led_hal_ops_t *ops = led_hal_get();
    if (ops && ops->get_count) {
        return ops->get_count();
    }
    return 0;
}

/**
 * Set LED state (wrapper with NULL check)
 */
static inline void led_hal_set(uint8_t led_id, bool state)
{
    const led_hal_ops_t *ops = led_hal_get();
    if (ops && ops->set_state) {
        ops->set_state(led_id, state);
    }
}

/**
 * Toggle LED state (wrapper with NULL check)
 */
static inline void led_hal_toggle(uint8_t led_id)
{
    const led_hal_ops_t *ops = led_hal_get();
    if (ops && ops->toggle) {
        ops->toggle(led_id);
    }
}

/**
 * Get LED state (wrapper with NULL check)
 */
static inline bool led_hal_get_state(uint8_t led_id)
{
    const led_hal_ops_t *ops = led_hal_get();
    if (ops && ops->get_state) {
        return ops->get_state(led_id);
    }
    return false;
}

#ifdef __cplusplus
}
#endif
