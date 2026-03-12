#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "msghub/msghub.h"
#include "drivers/button/button_topic.h"
#include "cubemot_error.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Button Driver API
// ============================================================================

/**
 * Initialize Button driver
 * Creates internal driver task and msghub topics
 *
 * @return CUBEMOT_DRIVER_BUTTON_OK on success, negative error code on failure
 */
cubemot_err_t button_driver_init(void);

/**
 * Get button state subscriber handle
 *
 * @param instance Subscriber instance ID (0 = primary instance)
 * @return Subscriber handle, MSGHUB_SUBSCRIBER_INVALID on failure
 */
msghub_subscriber_t button_get_state_subscriber(uint8_t instance);

// ============================================================================
// ISR Callback (called from HAL GPIO EXTI callback)
// ============================================================================

/**
 * Button driver ISR callback
 * Called from HAL_GPIO_EXTI_Callback when button interrupt occurs
 *
 * @param button_id Button ID that triggered the interrupt
 */
void button_driver_isr_callback(uint8_t button_id);

// ============================================================================
// Test Support (BUILD_TESTING only)
// ============================================================================

#ifdef BUILD_TESTING
/**
 * Deinitialize Button driver (test only)
 * Resets driver state for next test
 */
void button_driver_deinit(void);
#endif

#ifdef __cplusplus
}
#endif
