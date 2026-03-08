#pragma once

#include <stdint.h>
#include "msghub/msghub.h"
#include "drivers/led/led_topic.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// LED Driver API
// ============================================================================

/**
 * Initialize LED driver
 * Creates internal driver task and msghub topics
 *
 * @return 0 on success, negative on failure
 */
int led_driver_init(void);

/**
 * Set LED state
 *
 * @param led_id LED ID (0, 1, 2...)
 * @param on true = ON, false = OFF
 */
void led_set(uint8_t led_id, bool on);

/**
 * Toggle LED state
 *
 * @param led_id LED ID (0, 1, 2...)
 */
void led_toggle(uint8_t led_id);

/**
 * Get LED state subscriber handle
 *
 * @param instance Subscriber instance ID (0 = primary instance)
 * @return Subscriber handle, MSGHUB_SUBSCRIBER_INVALID on failure
 */
msghub_subscriber_t led_get_state_subscriber(uint8_t instance);

#ifdef __cplusplus
}
#endif
