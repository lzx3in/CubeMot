#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "drivers/framework/hal_framework.h"
#include "common_error.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Button HAL Framework Interface
// ============================================================================

/**
 * Button HAL operations structure
 * Board-specific implementations must provide these functions
 */
typedef struct {
    hal_base_ops_t base;

    /** Get number of available buttons */
    int (*get_count)(void);

    /** Read button state (true = pressed, false = released) */
    bool (*read_state)(uint8_t button_id);

    /** Enable button interrupt */
    void (*enable_interrupt)(uint8_t button_id);

    /** Disable button interrupt */
    void (*disable_interrupt)(uint8_t button_id);
} button_hal_ops_t;

// ============================================================================
// Button HAL Framework - Registration API
// ============================================================================

/**
 * Register Button HAL implementation
 * Must be called during board initialization
 *
 * @param ops Button HAL operations structure
 * @return CUBEMOT_DRIVER_BUTTON_OK on success, negative error code on failure
 */
cubemot_err_t button_hal_register(const button_hal_ops_t *ops);

/**
 * Get registered Button HAL implementation
 *
 * @return Pointer to registered ops, or NULL if not registered
 */
const button_hal_ops_t *button_hal_get(void);

// ============================================================================
// Button HAL Framework - Convenience Macros
// ============================================================================

/**
 * Get number of buttons (safe macro)
 */
#define BUTTON_COUNT() button_hal_count()

/**
 * Read button state (safe macro)
 */
#define BUTTON_READ(id) button_hal_read((id))

/**
 * Enable button interrupt (safe macro)
 */
#define BUTTON_ENABLE_IRQ(id) button_hal_enable_irq((id))

/**
 * Disable button interrupt (safe macro)
 */
#define BUTTON_DISABLE_IRQ(id) button_hal_disable_irq((id))

// ============================================================================
// Button HAL Framework - Wrapper Functions
// ============================================================================

/**
 * Get number of buttons (wrapper with NULL check)
 */
static inline int button_hal_count(void)
{
    const button_hal_ops_t *ops = button_hal_get();
    if (ops && ops->get_count) {
        return ops->get_count();
    }
    return 0;
}

/**
 * Read button state (wrapper with NULL check)
 */
static inline bool button_hal_read(uint8_t button_id)
{
    const button_hal_ops_t *ops = button_hal_get();
    if (ops && ops->read_state) {
        return ops->read_state(button_id);
    }
    return false;
}

/**
 * Enable button interrupt (wrapper with NULL check)
 */
static inline void button_hal_enable_irq(uint8_t button_id)
{
    const button_hal_ops_t *ops = button_hal_get();
    if (ops && ops->enable_interrupt) {
        ops->enable_interrupt(button_id);
    }
}

/**
 * Disable button interrupt (wrapper with NULL check)
 */
static inline void button_hal_disable_irq(uint8_t button_id)
{
    const button_hal_ops_t *ops = button_hal_get();
    if (ops && ops->disable_interrupt) {
        ops->disable_interrupt(button_id);
    }
}

#ifdef __cplusplus
}
#endif
