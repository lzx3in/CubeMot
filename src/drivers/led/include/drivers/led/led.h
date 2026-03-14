#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "common_device.h"

typedef struct {
    cubemot_device_led id;
    void *ctx;
    int (*init)(void *ctx);
    int (*on)(void *ctx);
    int (*off)(void *ctx);
    int (*toggle)(void *ctx);
    bool (*get_state)(void *ctx);
} driver_led_instance;

static inline bool cubemot_device_led_is_valid(cubemot_device_led id)
{
    return id >= 0 && id < CUBEMOT_DEVICE_LED_COUNT;
}

// Weak default implementation - board layer overrides this
const driver_led_instance *driver_led_get_instance(cubemot_device_led id);

#ifdef __cplusplus
}
#endif
