#include "boards/led.h"
#include "cubemot_config.h"
#include "stm32g4xx_hal.h"

static inline GPIO_TypeDef *gpio_from_index(int index)
{
    switch (index) {
        case 0: return GPIOA;
        case 1: return GPIOB;
        case 2: return GPIOC;
        case 3: return GPIOD;
        case 4: return GPIOE;
        default: return NULL;
    }
}

static inline uint16_t pin_from_number(int num)
{
    return (num < 0 || num > 15) ? 0 : (uint16_t)(1U << num);
}

static inline GPIO_TypeDef *led_get_port(int id)
{
    switch (id) {
#if CONFIG_BOARD_HAS_LED1
        case 0: return gpio_from_index(CONFIG_BOARD_LED1_PORT);
#endif
#if CONFIG_BOARD_HAS_LED2
        case 1: return gpio_from_index(CONFIG_BOARD_LED2_PORT);
#endif
#if CONFIG_BOARD_HAS_LED3
        case 2: return gpio_from_index(CONFIG_BOARD_LED3_PORT);
#endif
        default: return NULL;
    }
}

static inline uint16_t led_get_pin(int id)
{
    switch (id) {
#if CONFIG_BOARD_HAS_LED1
        case 0: return pin_from_number(CONFIG_BOARD_LED1_PIN);
#endif
#if CONFIG_BOARD_HAS_LED2
        case 1: return pin_from_number(CONFIG_BOARD_LED2_PIN);
#endif
#if CONFIG_BOARD_HAS_LED3
        case 2: return pin_from_number(CONFIG_BOARD_LED3_PIN);
#endif
        default: return 0;
    }
}

static inline bool led_id_is_valid(int id)
{
    return led_get_port(id) != NULL;
}

static inline int led_get_count(void)
{
    int count = 0;
#if CONFIG_BOARD_HAS_LED1
    count++;
#endif
#if CONFIG_BOARD_HAS_LED2
    count++;
#endif
#if CONFIG_BOARD_HAS_LED3
    count++;
#endif
    return count;
}

struct board_led_handle {
    int id;
};

static struct board_led_handle led_handles[3];

board_led_t board_led_get_handle(int led_id)
{
    if (!led_id_is_valid(led_id)) {
        return NULL;
    }
    led_handles[led_id].id = led_id;
    return &led_handles[led_id];
}

bool board_led_is_valid(board_led_t led)
{
    return (led != NULL && led_id_is_valid(led->id));
}

int board_led_get_count(void)
{
    return led_get_count();
}

void board_led_set_state(board_led_t led, bool state)
{
    if (!board_led_is_valid(led)) {
        return;
    }
    GPIO_TypeDef *port = led_get_port(led->id);
    uint16_t pin = led_get_pin(led->id);
    HAL_GPIO_WritePin(port, pin, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void board_led_toggle(board_led_t led)
{
    if (!board_led_is_valid(led)) {
        return;
    }
    GPIO_TypeDef *port = led_get_port(led->id);
    uint16_t pin = led_get_pin(led->id);
    HAL_GPIO_TogglePin(port, pin);
}

bool board_led_get_state(board_led_t led)
{
    if (!board_led_is_valid(led)) {
        return false;
    }
    GPIO_TypeDef *port = led_get_port(led->id);
    uint16_t pin = led_get_pin(led->id);
    GPIO_PinState pin_state = HAL_GPIO_ReadPin(port, pin);
    return (pin_state == GPIO_PIN_SET);
}
