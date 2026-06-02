// Zephyr GPIO LED driver for CubeMot
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include "drivers/led.h"

#define LED0_NODE DT_ALIAS(led0)

static const struct gpio_dt_spec g_led_specs[] = {
    [0] = GPIO_DT_SPEC_GET_OR(LED0_NODE, gpios, {0}),
};

static const uint8_t g_led_count = ARRAY_SIZE(g_led_specs);

int cubemot_led_init(void)
{
    for (int i = 0; i < g_led_count; i++) {
        if (g_led_specs[i].port == NULL) {
            continue;
        }
        if (!gpio_is_ready_dt(&g_led_specs[i])) {
            return -1;
        }
        int ret = gpio_pin_configure_dt(&g_led_specs[i], GPIO_OUTPUT_INACTIVE);
        if (ret < 0) {
            return ret;
        }
    }
    return 0;
}

int cubemot_led_on(uint8_t led_id)
{
    if (led_id >= g_led_count || g_led_specs[led_id].port == NULL) {
        return -1;
    }
    return gpio_pin_set_dt(&g_led_specs[led_id], 1);
}

int cubemot_led_off(uint8_t led_id)
{
    if (led_id >= g_led_count || g_led_specs[led_id].port == NULL) {
        return -1;
    }
    return gpio_pin_set_dt(&g_led_specs[led_id], 0);
}

int cubemot_led_toggle(uint8_t led_id)
{
    if (led_id >= g_led_count || g_led_specs[led_id].port == NULL) {
        return -1;
    }
    return gpio_pin_toggle_dt(&g_led_specs[led_id]);
}