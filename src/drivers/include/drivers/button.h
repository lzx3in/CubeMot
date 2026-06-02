#ifndef CUBEMOT_DRIVER_BUTTON_H
#define CUBEMOT_DRIVER_BUTTON_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Button callback type */
typedef void (*cubemot_button_callback_t)(void *user_data);

/**
 * @brief Initialize button GPIO with interrupt
 * @param button_id Button index
 * @param callback ISR callback (may be called from interrupt context)
 * @param user_data User data passed to callback
 * @return 0 on success, negative errno on failure
 */
int cubemot_button_init(uint8_t button_id, cubemot_button_callback_t callback, void *user_data);

/**
 * @brief Read button state
 * @param button_id Button index
 * @return true if pressed, false if released
 */
bool cubemot_button_read(uint8_t button_id);

#ifdef __cplusplus
}
#endif

#endif /* CUBEMOT_DRIVER_BUTTON_H */