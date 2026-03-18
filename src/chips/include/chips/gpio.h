#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// Chip-level GPIO Abstraction
// ============================================================================
/**
 * @file gpio.h
 * @brief Chip-level GPIO abstraction layer
 *
 * This layer provides thin wrappers around chip-specific GPIO operations.
 * Implementations are provided by specific chip directories (e.g., chips/stm32g4/).
 */

// ============================================================================
// Type Definitions
// ============================================================================

/**
 * @brief GPIO pin state
 */
typedef enum {
    CHIP_GPIO_RESET = 0,
    CHIP_GPIO_SET
} chip_gpio_state_t;

/**
 * @brief GPIO mode
 */
typedef enum {
    CHIP_GPIO_MODE_INPUT,             /*!< Input Floating Mode */
    CHIP_GPIO_MODE_OUTPUT_PP,         /*!< Output Push Pull Mode */
    CHIP_GPIO_MODE_OUTPUT_OD,         /*!< Output Open Drain Mode */
    CHIP_GPIO_MODE_AF_PP,             /*!< Alternate Function Push Pull Mode */
    CHIP_GPIO_MODE_AF_OD,             /*!< Alternate Function Open Drain Mode */
    CHIP_GPIO_MODE_ANALOG,            /*!< Analog Mode */
    CHIP_GPIO_MODE_IT_RISING,         /*!< External Interrupt with Rising edge */
    CHIP_GPIO_MODE_IT_FALLING,        /*!< External Interrupt with Falling edge */
    CHIP_GPIO_MODE_IT_RISING_FALLING, /*!< External Interrupt with Rising/Falling edge */
    CHIP_GPIO_MODE_EVT_RISING,        /*!< External Event with Rising edge */
    CHIP_GPIO_MODE_EVT_FALLING,       /*!< External Event with Falling edge */
    CHIP_GPIO_MODE_EVT_RISING_FALLING /*!< External Event with Rising/Falling edge */
} chip_gpio_mode_t;

/**
 * @brief GPIO speed
 */
typedef enum {
    CHIP_GPIO_SPEED_LOW,      /*!< Up to 5 MHz */
    CHIP_GPIO_SPEED_MEDIUM,   /*!< 5 MHz to 25 MHz */
    CHIP_GPIO_SPEED_HIGH,     /*!< 25 MHz to 50 MHz */
    CHIP_GPIO_SPEED_VERY_HIGH /*!< 50 MHz to 120 MHz */
} chip_gpio_speed_t;

/**
 * @brief GPIO pull
 */
typedef enum {
    CHIP_GPIO_NOPULL,  /*!< No Pull-up or Pull-down */
    CHIP_GPIO_PULLUP,  /*!< Pull-up activation */
    CHIP_GPIO_PULLDOWN /*!< Pull-down activation */
} chip_gpio_pull_t;

/**
 * @brief GPIO port index (bitmask values for bitwise operations)
 */
typedef enum {
    CHIP_GPIO_PORT_A = 0x0000U,
    CHIP_GPIO_PORT_B = 0x0001U,
    CHIP_GPIO_PORT_C = 0x0002U,
    CHIP_GPIO_PORT_D = 0x0004U,
    CHIP_GPIO_PORT_E = 0x0008U,
    CHIP_GPIO_PORT_F = 0x0010U,
    CHIP_GPIO_PORT_G = 0x0020U
} chip_gpio_port_index_t;

/**
 * @brief GPIO pin numbers (logical pin index)
 */
typedef enum {
    CHIP_GPIO_PIN_0 = 0,
    CHIP_GPIO_PIN_1,
    CHIP_GPIO_PIN_2,
    CHIP_GPIO_PIN_3,
    CHIP_GPIO_PIN_4,
    CHIP_GPIO_PIN_5,
    CHIP_GPIO_PIN_6,
    CHIP_GPIO_PIN_7,
    CHIP_GPIO_PIN_8,
    CHIP_GPIO_PIN_9,
    CHIP_GPIO_PIN_10,
    CHIP_GPIO_PIN_11,
    CHIP_GPIO_PIN_12,
    CHIP_GPIO_PIN_13,
    CHIP_GPIO_PIN_14,
    CHIP_GPIO_PIN_15,
    CHIP_GPIO_PIN_MAX
} chip_gpio_pin_t;

/**
 * @brief GPIO port handle (chip-specific)
 * @note Opaque type - actual type defined by chip implementation
 */
typedef struct chip_gpio_port chip_gpio_port_t;

/**
 * @brief GPIO initialization structure
 */
typedef struct {
    chip_gpio_pin_t pin;     /*!< Specifies the GPIO pins to be configured */
    chip_gpio_mode_t mode;   /*!< Specifies the operating mode */
    chip_gpio_pull_t pull;   /*!< Specifies Pull-up or Pull-Down activation */
    chip_gpio_speed_t speed; /*!< Specifies the speed for the selected pins */
    uint32_t alternate;      /*!< Peripheral alternate function (chip-specific) */
} chip_gpio_init_t;

// ============================================================================
// GPIO Core Functions
// ============================================================================

/**
 * @brief Initialize GPIO pin(s)
 * @param port GPIO port handle
 * @param init Initialization structure
 */
void chip_gpio_init(chip_gpio_port_t *port, const chip_gpio_init_t *init);

/**
 * @brief Deinitialize GPIO pin(s)
 * @param port GPIO port handle
 * @param pin Pin(s) to deinitialize
 */
void chip_gpio_deinit(chip_gpio_port_t *port, chip_gpio_pin_t pin);

/**
 * @brief Read GPIO pin state
 * @param port GPIO port handle
 * @param pin Pin to read
 * @return Pin state (CHIP_GPIO_SET or CHIP_GPIO_RESET)
 */
chip_gpio_state_t chip_gpio_read(chip_gpio_port_t *port, chip_gpio_pin_t pin);

/**
 * @brief Write GPIO pin state
 * @param port GPIO port handle
 * @param pin Pin to write
 * @param state State to set (CHIP_GPIO_SET or CHIP_GPIO_RESET)
 */
void chip_gpio_write(chip_gpio_port_t *port, chip_gpio_pin_t pin, chip_gpio_state_t state);

/**
 * @brief Toggle GPIO pin state
 * @param port GPIO port handle
 * @param pin Pin to toggle
 */
void chip_gpio_toggle(chip_gpio_port_t *port, chip_gpio_pin_t pin);

// ============================================================================
// GPIO EXTI Functions
// ============================================================================

/**
 * @brief Clear EXTI interrupt flag
 * @param pin Pin that triggered the EXTI
 */
void chip_gpio_exti_clear_it(chip_gpio_pin_t pin);

/**
 * @brief Handle EXTI interrupt
 * @param pin Pin that triggered the EXTI
 */
void chip_gpio_exti_irq_handler(chip_gpio_pin_t pin);

/**
 * @brief EXTI callback (weak implementation, can be overridden)
 * @param pin Pin that triggered the EXTI
 */
void chip_gpio_exti_callback(chip_gpio_pin_t pin);

/**
 * @brief Enable EXTI interrupt for a GPIO pin
 * @param pin Pin to enable interrupt for
 * @param priority Interrupt priority (0 = highest)
 */
void chip_gpio_exti_enable_irq(chip_gpio_pin_t pin, uint32_t priority);

/**
 * @brief Disable EXTI interrupt for a GPIO pin
 * @param pin Pin to disable interrupt for
 */
void chip_gpio_exti_disable_irq(chip_gpio_pin_t pin);

// ============================================================================
// Port Access (chip-specific)
// ============================================================================

/**
 * @brief Get GPIO port handle from port index
 * @param port_index Port index (0=A, 1=B, 2=C, etc.)
 * @return Port handle, or NULL if invalid
 * @note This is chip-specific and may not be available on all platforms
 */
chip_gpio_port_t *chip_gpio_get_port(chip_gpio_port_index_t port_index);

#ifdef __cplusplus
}
#endif
