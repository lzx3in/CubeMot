/**
 * STM32G4 Chip-level GPIO Implementation
 * 
 * This file implements the chip_gpio_* interface for STM32G4 MCUs.
 * It provides thin wrappers around STM32 HAL GPIO functions.
 */

#include "chips/gpio.h"
#include "stm32g4xx_hal.h"

// ============================================================================
// Port Handle Implementation
// ============================================================================

typedef struct GPIO_TypeDef chip_gpio_port;

/**
 * @brief Get GPIO port handle from port index
 */
chip_gpio_port_t *chip_gpio_get_port(chip_gpio_port_index_t port_index)
{
    switch (port_index) {
        case CHIP_GPIO_PORT_A: return (chip_gpio_port_t *)GPIOA;
        case CHIP_GPIO_PORT_B: return (chip_gpio_port_t *)GPIOB;
        case CHIP_GPIO_PORT_C: return (chip_gpio_port_t *)GPIOC;
        case CHIP_GPIO_PORT_D: return (chip_gpio_port_t *)GPIOD;
        case CHIP_GPIO_PORT_E: return (chip_gpio_port_t *)GPIOE;
        case CHIP_GPIO_PORT_F: return (chip_gpio_port_t *)GPIOF;
        case CHIP_GPIO_PORT_G: return (chip_gpio_port_t *)GPIOG;
        default: return NULL;
    }
}

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Convert chip_gpio_mode_t to STM32 HAL mode
 */
static uint32_t gpio_mode_to_hal(chip_gpio_mode_t mode)
{
    switch (mode) {
        case CHIP_GPIO_MODE_INPUT:
            return GPIO_MODE_INPUT;
        case CHIP_GPIO_MODE_OUTPUT_PP:
            return GPIO_MODE_OUTPUT_PP;
        case CHIP_GPIO_MODE_OUTPUT_OD:
            return GPIO_MODE_OUTPUT_OD;
        case CHIP_GPIO_MODE_AF_PP:
            return GPIO_MODE_AF_PP;
        case CHIP_GPIO_MODE_AF_OD:
            return GPIO_MODE_AF_OD;
        case CHIP_GPIO_MODE_ANALOG:
            return GPIO_MODE_ANALOG;
        case CHIP_GPIO_MODE_IT_RISING:
            return GPIO_MODE_IT_RISING;
        case CHIP_GPIO_MODE_IT_FALLING:
            return GPIO_MODE_IT_FALLING;
        case CHIP_GPIO_MODE_IT_RISING_FALLING:
            return GPIO_MODE_IT_RISING_FALLING;
        case CHIP_GPIO_MODE_EVT_RISING:
            return GPIO_MODE_EVT_RISING;
        case CHIP_GPIO_MODE_EVT_FALLING:
            return GPIO_MODE_EVT_FALLING;
        case CHIP_GPIO_MODE_EVT_RISING_FALLING:
            return GPIO_MODE_EVT_RISING_FALLING;
        default:
            return GPIO_MODE_INPUT;
    }
}

/**
 * @brief Convert chip_gpio_pull_t to STM32 HAL pull
 */
static uint32_t gpio_pull_to_hal(chip_gpio_pull_t pull)
{
    switch (pull) {
        case CHIP_GPIO_NOPULL:
            return GPIO_NOPULL;
        case CHIP_GPIO_PULLUP:
            return GPIO_PULLUP;
        case CHIP_GPIO_PULLDOWN:
            return GPIO_PULLDOWN;
        default:
            return GPIO_NOPULL;
    }
}

/**
 * @brief Convert chip_gpio_speed_t to STM32 HAL speed
 */
static uint32_t gpio_speed_to_hal(chip_gpio_speed_t speed)
{
    switch (speed) {
        case CHIP_GPIO_SPEED_LOW:
            return GPIO_SPEED_FREQ_LOW;
        case CHIP_GPIO_SPEED_MEDIUM:
            return GPIO_SPEED_FREQ_MEDIUM;
        case CHIP_GPIO_SPEED_HIGH:
            return GPIO_SPEED_FREQ_HIGH;
        case CHIP_GPIO_SPEED_VERY_HIGH:
            return GPIO_SPEED_FREQ_VERY_HIGH;
        default:
            return GPIO_SPEED_FREQ_LOW;
    }
}

/**
 * @brief Convert chip_gpio_state_t to STM32 HAL pin state
 */
static GPIO_PinState gpio_state_to_hal(chip_gpio_state_t state)
{
    return (state == CHIP_GPIO_SET) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

/**
 * @brief Convert STM32 HAL pin state to chip_gpio_state_t
 */
static chip_gpio_state_t gpio_state_from_hal(GPIO_PinState state)
{
    return (state == GPIO_PIN_SET) ? CHIP_GPIO_SET : CHIP_GPIO_RESET;
}

/**
 * @brief Convert logical pin index to STM32 HAL pin bitmask
 */
static uint16_t gpio_pin_to_hal(chip_gpio_pin_t pin)
{
    return (uint16_t)(1U << (uint32_t)pin);
}

// ============================================================================
// GPIO Core Functions Implementation
// ============================================================================

void chip_gpio_init(chip_gpio_port_t *port, const chip_gpio_init_t *init)
{
    if (port == NULL || port == NULL || init == NULL) {
        return;
    }

    GPIO_InitTypeDef hal_init = {
        .Pin = gpio_pin_to_hal(init->pin),
        .Mode = gpio_mode_to_hal(init->mode),
        .Pull = gpio_pull_to_hal(init->pull),
        .Speed = gpio_speed_to_hal(init->speed),
        .Alternate = init->alternate
    };

    HAL_GPIO_Init((GPIO_TypeDef *)port, &hal_init);
}

void chip_gpio_deinit(chip_gpio_port_t *port, chip_gpio_pin_t pin)
{
    if (port == NULL || port == NULL) {
        return;
    }

    HAL_GPIO_DeInit((GPIO_TypeDef *)port, gpio_pin_to_hal(pin));
}

chip_gpio_state_t chip_gpio_read(chip_gpio_port_t *port, chip_gpio_pin_t pin)
{
    if (port == NULL || port == NULL) {
        return CHIP_GPIO_RESET;
    }

    GPIO_PinState state = HAL_GPIO_ReadPin((GPIO_TypeDef *)port, gpio_pin_to_hal(pin));
    return gpio_state_from_hal(state);
}

void chip_gpio_write(chip_gpio_port_t *port, chip_gpio_pin_t pin, chip_gpio_state_t state)
{
    if (port == NULL || port == NULL) {
        return;
    }

    HAL_GPIO_WritePin((GPIO_TypeDef *)port, gpio_pin_to_hal(pin), gpio_state_to_hal(state));
}

void chip_gpio_toggle(chip_gpio_port_t *port, chip_gpio_pin_t pin)
{
    if (port == NULL || port == NULL) {
        return;
    }

    HAL_GPIO_TogglePin((GPIO_TypeDef *)port, gpio_pin_to_hal(pin));
}

// ============================================================================
// GPIO EXTI Functions Implementation
// ============================================================================

void chip_gpio_exti_clear_it(chip_gpio_pin_t pin)
{
    __HAL_GPIO_EXTI_CLEAR_IT(gpio_pin_to_hal(pin));
}

void chip_gpio_exti_irq_handler(chip_gpio_pin_t pin)
{
    uint16_t hal_pin = gpio_pin_to_hal(pin);
    if (__HAL_GPIO_EXTI_GET_IT(hal_pin) != 0x00u)
    {
        __HAL_GPIO_EXTI_CLEAR_IT(hal_pin);
        chip_gpio_exti_callback(pin);
    }
}

__attribute__((weak)) void chip_gpio_exti_callback(chip_gpio_pin_t pin)
{
}

// ============================================================================
// GPIO EXTI IRQ Functions Implementation
// ============================================================================

void chip_gpio_exti_enable_irq(chip_gpio_pin_t pin, uint32_t priority)
{
    uint16_t hal_pin = gpio_pin_to_hal(pin);
    IRQn_Type irqn;

    if (hal_pin >= (1U << 15)) {
        irqn = EXTI15_10_IRQn;
    } else if (hal_pin >= (1U << 10)) {
        irqn = EXTI15_10_IRQn;
    } else {
        return;
    }
    HAL_NVIC_SetPriority(irqn, priority, 0);
    HAL_NVIC_EnableIRQ(irqn);
}

void chip_gpio_exti_disable_irq(chip_gpio_pin_t pin)
{
    uint16_t hal_pin = gpio_pin_to_hal(pin);
    IRQn_Type irqn;

    if (hal_pin >= (1U << 10)) {
        irqn = EXTI15_10_IRQn;
    } else if (hal_pin >= (1U << 5)) {
        irqn = EXTI9_5_IRQn;
    } else {
        return;
    }

    HAL_NVIC_DisableIRQ(irqn);
}
