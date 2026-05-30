#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "common_device.h"

static inline bool cubemot_device_led_is_valid(cubemot_device_led id)
{
    return id >= 0 && id < CUBEMOT_DEVICE_LED_COUNT;
}

int led_init(void);
int led_on(uint8_t led_id);
int led_off(uint8_t led_id);
int led_toggle(uint8_t led_id);
