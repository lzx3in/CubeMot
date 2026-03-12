/**
 * Button HAL Implementation for STM32G4
 * 
 * This file implements the Button HAL framework interface for STM32G4 MCUs.
 * It directly uses STM32 HAL functions for hardware access.
 */

#include "drivers/framework/button_hal_framework.h"
#include "stm32g4xx_hal.h"

// ============================================================================
// Board-specific Button configuration
// TODO: Move to Kconfig when button support is added
// ============================================================================

// Nucleo G431RB user button: PC13
#define BUTTON0_PORT    GPIOC
#define BUTTON0_PIN     GPIO_PIN_13
#define BUTTON0_IRQn    EXTI15_10_IRQn
#define BUTTON0_IRQ_HANDLER EXTI15_10_IRQHandler

static inline bool button_id_is_valid(int id)
{
    return (id == 0);  // Only one button for now
}

static inline GPIO_TypeDef *button_get_port(int id)
{
    if (id == 0) {
        return BUTTON0_PORT;
    }
    return NULL;
}

static inline uint16_t button_get_pin(int id)
{
    if (id == 0) {
        return BUTTON0_PIN;
    }
    return 0;
}

// ============================================================================
// STM32G4 Button HAL Implementation
// ============================================================================

static int stm32g4_button_init(void)
{
    // GPIO initialization is handled by board init
    // This function can add additional setup if needed
    return 0;
}

static void stm32g4_button_deinit(void)
{
    // Optional deinitialization
}

static int stm32g4_button_get_count(void)
{
    return 1;  // One user button on Nucleo G431RB
}

static bool stm32g4_button_read_state(uint8_t button_id)
{
    if (!button_id_is_valid(button_id)) {
        return false;
    }
    GPIO_TypeDef *port = button_get_port(button_id);
    uint16_t pin = button_get_pin(button_id);
    GPIO_PinState pin_state = HAL_GPIO_ReadPin(port, pin);
    // Button is active low (pressed = 0, released = 1)
    return (pin_state == GPIO_PIN_RESET);
}

static void stm32g4_button_enable_interrupt(uint8_t button_id)
{
    if (!button_id_is_valid(button_id)) {
        return;
    }
    HAL_NVIC_EnableIRQ(BUTTON0_IRQn);
}

static void stm32g4_button_disable_interrupt(uint8_t button_id)
{
    if (!button_id_is_valid(button_id)) {
        return;
    }
    HAL_NVIC_DisableIRQ(BUTTON0_IRQn);
}

// ============================================================================
// HAL Operations Structure
// ============================================================================

static const button_hal_ops_t stm32g4_button_ops = {
    .base = {
        .name = "stm32g4_button",
        .init = stm32g4_button_init,
        .deinit = stm32g4_button_deinit,
    },
    .get_count = stm32g4_button_get_count,
    .read_state = stm32g4_button_read_state,
    .enable_interrupt = stm32g4_button_enable_interrupt,
    .disable_interrupt = stm32g4_button_disable_interrupt,
};

// ============================================================================
// Registration Function (called by board initialization)
// ============================================================================

/**
 * Initialize and register STM32G4 Button HAL
 * This function is called during board initialization
 */
void stm32g4_button_hal_init(void)
{
    button_hal_register(&stm32g4_button_ops);
}
