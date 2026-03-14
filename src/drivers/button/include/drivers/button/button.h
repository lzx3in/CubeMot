#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "common_device.h"

typedef struct {
    cubemot_device_button id;
    void *ctx;
    int (*init)(void *ctx);
    bool (*read)(void *ctx);
    int (*enable_irq)(void *ctx);
    int (*disable_irq)(void *ctx);
    void (*irq_handler)(void *ctx);
} driver_button_instance;

static inline bool cubemot_device_button_is_valid(cubemot_device_button id)
{
    return id >= CUBEMOT_DEVICE_BUTTON_0 && id < CUBEMOT_DEVICE_BUTTON_COUNT;
}

// Weak default implementation - board layer overrides this
driver_button_instance *driver_button_get_instance(cubemot_device_button id);

#ifdef __cplusplus
}
#endif
