#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    CUBEMOT_DEVICE_LED_0 = 0,
    CUBEMOT_DEVICE_LED_1 = 1,
    CUBEMOT_DEVICE_LED_2 = 2,
    CUBEMOT_DEVICE_LED_COUNT,
} cubemot_device_led;

typedef enum {
    CUBEMOT_DEVICE_BUTTON_0 = 0,
    CUBEMOT_DEVICE_BUTTON_1 = 1,
    CUBEMOT_DEVICE_BUTTON_2 = 2,
    CUBEMOT_DEVICE_BUTTON_COUNT,
} cubemot_device_button;

#ifdef __cplusplus
}
#endif
