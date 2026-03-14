#include "drivers/led/led.h"
#include "cubemot_config.h"
#include "chips/gpio.h"
#include "common_device.h"

struct led_hw_config {
    chip_gpio_port_index_t port;
    chip_gpio_pin_t pin;
};

static const struct led_hw_config g_led_hw_config[CUBEMOT_DEVICE_LED_COUNT] = {
#if CONFIG_HAS_DRIVER_LED_0
    [CUBEMOT_DEVICE_LED_0] = {
        .port = (chip_gpio_port_index_t)CONFIG_BOARD_LED0_PORT,
        .pin = (chip_gpio_pin_t)CONFIG_BOARD_LED0_PIN
    },
#endif
#if CONFIG_HAS_DRIVER_LED_1
    [CUBEMOT_DEVICE_LED_1] = {
        .port = (chip_gpio_port_index_t)CONFIG_BOARD_LED1_PORT,
        .pin = (chip_gpio_pin_t)CONFIG_BOARD_LED1_PIN
    },
#endif
#if CONFIG_HAS_DRIVER_LED_2
    [CUBEMOT_DEVICE_LED_2] = {
        .port = (chip_gpio_port_index_t)CONFIG_BOARD_LED2_PORT,
        .pin = (chip_gpio_pin_t)CONFIG_BOARD_LED2_PIN
    },
#endif
};

static int led_init(void *ctx)
{
    const struct led_hw_config *cfg = (const struct led_hw_config *)ctx;
    if (!cfg) return -1;
    
    chip_gpio_port_t *port = chip_gpio_get_port(cfg->port);
    chip_gpio_init_t init = {
        .pin = cfg->pin,
        .mode = CHIP_GPIO_MODE_OUTPUT_PP,
        .pull = CHIP_GPIO_NOPULL,
        .speed = CHIP_GPIO_SPEED_LOW,
        .alternate = 0
    };
    chip_gpio_init(port, &init);
    chip_gpio_write(port, cfg->pin, CHIP_GPIO_RESET);
    return 0;
}

static int led_on(void *ctx)
{
    const struct led_hw_config *cfg = (const struct led_hw_config *)ctx;
    if (!cfg) return -1;
    
    chip_gpio_port_t *port = chip_gpio_get_port(cfg->port);
    chip_gpio_write(port, cfg->pin, CHIP_GPIO_SET);
    return 0;
}

static int led_off(void *ctx)
{
    const struct led_hw_config *cfg = (const struct led_hw_config *)ctx;
    if (!cfg) return -1;
    
    chip_gpio_port_t *port = chip_gpio_get_port(cfg->port);
    chip_gpio_write(port, cfg->pin, CHIP_GPIO_RESET);
    return 0;
}

static int led_toggle(void *ctx)
{
    const struct led_hw_config *cfg = (const struct led_hw_config *)ctx;
    if (!cfg) return -1;
    
    chip_gpio_port_t *port = chip_gpio_get_port(cfg->port);
    chip_gpio_toggle(port, cfg->pin);
    return 0;
}

static bool led_get_state(void *ctx)
{
    const struct led_hw_config *cfg = (const struct led_hw_config *)ctx;
    if (!cfg) return false;
    
    chip_gpio_port_t *port = chip_gpio_get_port(cfg->port);
    chip_gpio_state_t state = chip_gpio_read(port, cfg->pin);
    return state == CHIP_GPIO_SET;
}

static const driver_led_instance g_led_instances[] = {
#if CONFIG_HAS_DRIVER_LED_0
    [CUBEMOT_DEVICE_LED_0] = {
        .id = CUBEMOT_DEVICE_LED_0,
        .ctx = (void *)&g_led_hw_config[CUBEMOT_DEVICE_LED_0],
        .init = led_init,
        .on = led_on,
        .off = led_off,
        .toggle = led_toggle,
        .get_state = led_get_state,
    },
#endif
#if CONFIG_HAS_DRIVER_LED_1
    [CUBEMOT_DEVICE_LED_1] = {
        .id = CUBEMOT_DEVICE_LED_1,
        .ctx = (void *)&g_led_hw_config[CUBEMOT_DEVICE_LED_1],
        .init = led_init,
        .on = led_on,
        .off = led_off,
        .toggle = led_toggle,
        .get_state = led_get_state,
    },
#endif
#if CONFIG_HAS_DRIVER_LED_2
    [CUBEMOT_DEVICE_LED_2] = {
        .id = CUBEMOT_DEVICE_LED_2,
        .ctx = (void *)&g_led_hw_config[CUBEMOT_DEVICE_LED_2],
        .init = led_init,
        .on = led_on,
        .off = led_off,
        .toggle = led_toggle,
        .get_state = led_get_state,
    },
#endif
};

const driver_led_instance *driver_led_get_instance(cubemot_device_led id)
{
    if (!cubemot_device_led_is_valid(id)) {
        return NULL;
    }

#if CONFIG_HAS_DRIVER_LED_0
    if (id == CUBEMOT_DEVICE_LED_0) {
        return &g_led_instances[CUBEMOT_DEVICE_LED_0];
    }
#endif
#if CONFIG_HAS_DRIVER_LED_1
    if (id == CUBEMOT_DEVICE_LED_1) {
        return &g_led_instances[CUBEMOT_DEVICE_LED_1];
    }
#endif
#if CONFIG_HAS_DRIVER_LED_2
    if (id == CUBEMOT_DEVICE_LED_2) {
        return &g_led_instances[CUBEMOT_DEVICE_LED_2];
    }
#endif
    
    return NULL;
}
