/**
 * LED HAL Implementation for STM32G4
 * 
 * This file implements the LED HAL framework interface for STM32G4 MCUs.
 * It directly uses STM32 HAL functions for hardware access.
 */

#include "drivers/framework/led_hal_framework.h"
#include "stm32g4xx_hal.h"
#include "cubemot_config.h"

// ============================================================================
// Board-specific LED configuration (from Kconfig)
// ============================================================================

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

// ============================================================================
// STM32G4 LED HAL Implementation
// ============================================================================

static int stm32g4_led_init(void)
{
    // GPIO initialization is handled by board init
    // This function can add additional setup if needed
    return 0;
}

static void stm32g4_led_deinit(void)
{
    // Optional deinitialization
}

static int stm32g4_led_get_count(void)
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

static void stm32g4_led_set_state(uint8_t led_id, bool state)
{
    if (!led_id_is_valid(led_id)) {
        return;
    }
    GPIO_TypeDef *port = led_get_port(led_id);
    uint16_t pin = led_get_pin(led_id);
    HAL_GPIO_WritePin(port, pin, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void stm32g4_led_toggle(uint8_t led_id)
{
    if (!led_id_is_valid(led_id)) {
        return;
    }
    GPIO_TypeDef *port = led_get_port(led_id);
    uint16_t pin = led_get_pin(led_id);
    HAL_GPIO_TogglePin(port, pin);
}

static bool stm32g4_led_get_state(uint8_t led_id)
{
    if (!led_id_is_valid(led_id)) {
        return false;
    }
    GPIO_TypeDef *port = led_get_port(led_id);
    uint16_t pin = led_get_pin(led_id);
    GPIO_PinState pin_state = HAL_GPIO_ReadPin(port, pin);
    return (pin_state == GPIO_PIN_SET);
}

// ============================================================================
// HAL Operations Structure
// ============================================================================

static const led_hal_ops_t stm32g4_led_ops = {
    .base = {
        .name = "stm32g4_led",
        .init = stm32g4_led_init,
        .deinit = stm32g4_led_deinit,
    },
    .get_count = stm32g4_led_get_count,
    .set_state = stm32g4_led_set_state,
    .toggle = stm32g4_led_toggle,
    .get_state = stm32g4_led_get_state,
};

// ============================================================================
// Registration Function (called by board initialization)
// ============================================================================

/**
 * Initialize and register STM32G4 LED HAL
 * This function is called during board initialization
 */
void stm32g4_led_hal_init(void)
{
    led_hal_register(&stm32g4_led_ops);
}
