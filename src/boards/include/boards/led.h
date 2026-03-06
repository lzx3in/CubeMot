#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "cubemot_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_BOARD_HAS_LED1
#define BOARD_LED_1 0
#endif
#if CONFIG_BOARD_HAS_LED2
#define BOARD_LED_2 1
#endif
#if CONFIG_BOARD_HAS_LED3
#define BOARD_LED_3 2
#endif

typedef struct board_led_handle *board_led_t;

board_led_t board_led_get_handle(int led_id);
bool board_led_is_valid(board_led_t led);
int board_led_get_count(void);
void board_led_set_state(board_led_t led, bool state);
void board_led_toggle(board_led_t led);
bool board_led_get_state(board_led_t led);

#ifdef __cplusplus
}
#endif
