#include "boards/led.h"
#include "boards/board_config.h"
#include "main.h"
#include "stm32g4xx.h"

#define SUPPORTED_LED_COUNT BOARD_LED_COUNT

static GPIO_TypeDef *get_gpio_port_from_index(int port_index)
{
    switch (port_index) {
        case 0: return GPIOA;
        case 1: return GPIOB;
        case 2: return GPIOC;
        case 3: return GPIOD;
        default: return NULL;
    }
}

static uint16_t get_gpio_pin_mask(int pin_number)
{
    if (pin_number < 0 || pin_number > 15) {
        return 0;
    }
    return (uint16_t)(1U << pin_number);
}

static const board_led_config_t board_led_configs[BOARD_LED_COUNT] = {
#if BOARD_HAS_LED1
    [BOARD_LED_1] = {.port_index = BOARD_LED1_PORT, .pin = BOARD_LED1_PIN}
#endif
};

const board_led_config_t *board_led_get_config(board_led_id_t led_id)
{
    if (led_id >= SUPPORTED_LED_COUNT) {
        return NULL;
    }

    return &board_led_configs[led_id];
}

int board_led_is_supported(board_led_id_t led_id)
{
    switch (led_id) {
#if BOARD_HAS_LED1
        case BOARD_LED_1: return 1;
#endif
#if BOARD_HAS_LED2
        case BOARD_LED_2: return 1;
#endif
#if BOARD_HAS_LED3
        case BOARD_LED_3: return 1;
#endif
        default: return 0;
    }
}

void board_led_set_state(const board_led_config_t *config, bool state)
{
    if (config == NULL) {
        return;
    }

    GPIO_TypeDef *port = get_gpio_port_from_index(config->port_index);
    if (port == NULL) {
        return;
    }

    uint16_t pin_mask = get_gpio_pin_mask(config->pin);
    HAL_GPIO_WritePin(port, pin_mask, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void board_led_toggle(const board_led_config_t *config)
{
    if (config == NULL) {
        return;
    }

    GPIO_TypeDef *port = get_gpio_port_from_index(config->port_index);
    if (port == NULL) {
        return;
    }

    uint16_t pin_mask = get_gpio_pin_mask(config->pin);
    HAL_GPIO_TogglePin(port, pin_mask);
}

bool board_led_get_state(const board_led_config_t *config)
{
    if (config == NULL) {
        return false;
    }

    GPIO_TypeDef *port = get_gpio_port_from_index(config->port_index);
    if (port == NULL) {
        return false;
    }

    uint16_t pin_mask = get_gpio_pin_mask(config->pin);
    GPIO_PinState pin_state = HAL_GPIO_ReadPin(port, pin_mask);
    return (pin_state == GPIO_PIN_SET);
}
