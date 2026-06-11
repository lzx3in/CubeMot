#pragma once

/**
 * @file servo_ctrl.h
 * @brief Servo control module — angle control via msghub
 *
 * Subscribes to servo_cmd topic, publishes servo_state at 10Hz.
 * Provides smooth angle interpolation for steering.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief  Initialize servo control module
 *
 * Initializes servo driver and subscribes to servo_cmd topic.
 *
 * @return 0 on success
 */
int servo_ctrl_init(void);

/**
 * @brief  Servo control thread entry
 *
 * Runs at 100Hz, processes commands and publishes state.
 *
 * @param  arg1  Unused
 * @param  arg2  Unused
 * @param  arg3  Unused
 */
void servo_ctrl_thread(void *arg1, void *arg2, void *arg3);

#ifdef __cplusplus
}
#endif
