#pragma once
#include <stdint.h>
typedef void (*button_callback_t)(void *ctx);
int button_init(uint8_t button_id, button_callback_t callback);
