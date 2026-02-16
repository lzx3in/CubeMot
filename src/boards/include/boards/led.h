#ifndef BOARD_LED_H
#define BOARD_LED_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BOARD_LED_NONE = -1,
    BOARD_LED_1 = 0,
    BOARD_LED_2,
    BOARD_LED_3,
    BOARD_LED_COUNT
} board_led_id_t;

struct board_led_config_t {
    uint8_t port_index;
    uint16_t pin;
};

typedef struct board_led_config_t board_led_config_t;

const board_led_config_t *board_led_get_config(board_led_id_t led_id);
int board_led_is_supported(board_led_id_t led_id);

void board_led_set_state(const board_led_config_t *config, bool state);
void board_led_toggle(const board_led_config_t *config);
bool board_led_get_state(const board_led_config_t *config);

#ifdef __cplusplus
}
#endif

#endif
