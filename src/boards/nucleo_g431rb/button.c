#include "drivers/button/button.h"
#include "cubemot_config.h"
#include "chips/gpio.h"
#include "common_device.h"

struct button_hw_config {
    chip_gpio_port_index_t port;
    chip_gpio_pin_t pin;
};

static const struct button_hw_config g_button_hw_config[CUBEMOT_DEVICE_BUTTON_COUNT] = {
#if CONFIG_HAS_DRIVER_BUTTON_0
    [CUBEMOT_DEVICE_BUTTON_0] = {
        .port = (chip_gpio_port_index_t)CONFIG_BOARD_BUTTON0_PORT,
        .pin = (chip_gpio_pin_t)CONFIG_BOARD_BUTTON0_PIN,
    },
#endif
#if CONFIG_HAS_DRIVER_BUTTON_1
    [CUBEMOT_DEVICE_BUTTON_1] = {
        .port = (chip_gpio_port_index_t)CONFIG_BOARD_BUTTON1_PORT,
        .pin = (chip_gpio_pin_t)CONFIG_BOARD_BUTTON1_PIN,
    },
#endif
#if CONFIG_HAS_DRIVER_BUTTON_2
    [CUBEMOT_DEVICE_BUTTON_2] = {
        .port = (chip_gpio_port_index_t)CONFIG_BOARD_BUTTON2_PORT,
        .pin = (chip_gpio_pin_t)CONFIG_BOARD_BUTTON2_PIN,
    },
#endif
};

static int button_init(void *ctx)
{
    const struct button_hw_config *cfg = (const struct button_hw_config *)ctx;
    if (!cfg) return -1;
    
    chip_gpio_port_t *port = chip_gpio_get_port(cfg->port);
    chip_gpio_init_t init = {
        .pin = cfg->pin,
        .mode = CHIP_GPIO_MODE_IT_FALLING,
        .pull = CHIP_GPIO_NOPULL,
        .speed = CHIP_GPIO_SPEED_LOW,
        .alternate = 0
    };
    chip_gpio_init(port, &init);
    return 0;
}

static bool button_read(void *ctx)
{
    const struct button_hw_config *cfg = (const struct button_hw_config *)ctx;
    if (!cfg) return false;
    
    chip_gpio_port_t *port = chip_gpio_get_port(cfg->port);
    chip_gpio_state_t state = chip_gpio_read(port, cfg->pin);
    // Button pressed = GPIO low (active low)
    return state == CHIP_GPIO_RESET;
}

static int button_enable_irq(void *ctx)
{
    const struct button_hw_config *cfg = (const struct button_hw_config *)ctx;
    if (!cfg) return -1;
    
    // Use generic chip_gpio API instead of STM32-specific HAL
    chip_gpio_exti_enable_irq(cfg->pin, 8);  // Priority 8
    return 0;
}

static int button_disable_irq(void *ctx)
{
    const struct button_hw_config *cfg = (const struct button_hw_config *)ctx;
    if (!cfg) return -1;
    
    chip_gpio_exti_disable_irq(cfg->pin);
    return 0;
}

static driver_button_instance g_button_instances[] = {
#if CONFIG_HAS_DRIVER_BUTTON_0
    [CUBEMOT_DEVICE_BUTTON_0] = {
        .id = CUBEMOT_DEVICE_BUTTON_0,
        .ctx = (void *)&g_button_hw_config[CUBEMOT_DEVICE_BUTTON_0],
        .init = button_init,
        .read = button_read,
        .enable_irq = button_enable_irq,
        .disable_irq = button_disable_irq,
    },
#endif
#if CONFIG_HAS_DRIVER_BUTTON_1
    [CUBEMOT_DEVICE_BUTTON_1] = {
        .id = CUBEMOT_DEVICE_BUTTON_1,
        .ctx = (void *)&g_button_hw_config[CUBEMOT_DEVICE_BUTTON_1],
        .init = button_init,
        .read = button_read,
        .enable_irq = button_enable_irq,
        .disable_irq = button_disable_irq,
    },
#endif
#if CONFIG_HAS_DRIVER_BUTTON_2
    [CUBEMOT_DEVICE_BUTTON_2] = {
        .id = CUBEMOT_DEVICE_BUTTON_2,
        .ctx = (void *)&g_button_hw_config[CUBEMOT_DEVICE_BUTTON_2],
        .init = button_init,
        .read = button_read,
        .enable_irq = button_enable_irq,
        .disable_irq = button_disable_irq,
    },
#endif
};

driver_button_instance *driver_button_get_instance(cubemot_device_button id)
{
    if (!cubemot_device_button_is_valid(id)) {
        return NULL;
    }

#if CONFIG_HAS_DRIVER_BUTTON_0
    if (id == CUBEMOT_DEVICE_BUTTON_0) {
        return &g_button_instances[CUBEMOT_DEVICE_BUTTON_0];
    }
#endif
#if CONFIG_HAS_DRIVER_BUTTON_1
    if (id == CUBEMOT_DEVICE_BUTTON_1) {
        return &g_button_instances[CUBEMOT_DEVICE_BUTTON_1];
    }
#endif
#if CONFIG_HAS_DRIVER_BUTTON_2
    if (id == CUBEMOT_DEVICE_BUTTON_2) {
        return &g_button_instances[CUBEMOT_DEVICE_BUTTON_2];
    }
#endif
    
    return NULL;
}
