#ifndef CUBEMOT_DRIVER_LED_H
#define CUBEMOT_DRIVER_LED_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize LED GPIO
 *
 * @return 0 on success, negative errno on failure
 */
int cubemot_led_init(void);

/**
 * @brief Turn LED on
 * @param led_id LED index
 * @return 0 on success, negative errno on failure
 */
int cubemot_led_on(uint8_t led_id);

/**
 * @brief Turn LED off
 * @param led_id LED index
 * @return 0 on success, negative errno on failure
 */
int cubemot_led_off(uint8_t led_id);

/**
 * @brief Toggle LED
 * @param led_id LED index
 * @return 0 on success, negative errno on failure
 */
int cubemot_led_toggle(uint8_t led_id);

#ifdef __cplusplus
}
#endif

#endif /* CUBEMOT_DRIVER_LED_H */