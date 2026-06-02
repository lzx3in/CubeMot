// Zephyr GPIO button driver for CubeMot
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include "drivers/button.h"

#define SW0_NODE DT_ALIAS(sw0)

static const struct gpio_dt_spec g_button_specs[] = {
    [0] = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0}),
};

static struct gpio_callback g_button_cb_data;
static cubemot_button_callback_t g_user_callback;
static void *g_user_data;

static void button_isr_handler(const struct device *dev,
                               struct gpio_callback *cb,
                               uint32_t pins)
{
    (void)dev;
    (void)cb;
    (void)pins;

    if (g_user_callback) {
        g_user_callback(g_user_data);
    }
}

int cubemot_button_init(uint8_t button_id, cubemot_button_callback_t callback, void *user_data)
{
    if (button_id >= ARRAY_SIZE(g_button_specs) || g_button_specs[button_id].port == NULL) {
        return -1;
    }

    if (!gpio_is_ready_dt(&g_button_specs[button_id])) {
        return -ENODEV;
    }

    int ret = gpio_pin_configure_dt(&g_button_specs[button_id], GPIO_INPUT);
    if (ret < 0) {
        return ret;
    }

    ret = gpio_pin_interrupt_configure_dt(&g_button_specs[button_id], GPIO_INT_EDGE_TO_ACTIVE);
    if (ret < 0) {
        return ret;
    }

    g_user_callback = callback;
    g_user_data = user_data;

    gpio_init_callback(&g_button_cb_data, button_isr_handler,
                       BIT(g_button_specs[button_id].pin));
    gpio_add_callback(g_button_specs[button_id].port, &g_button_cb_data);

    return 0;
}

bool cubemot_button_read(uint8_t button_id)
{
    if (button_id >= ARRAY_SIZE(g_button_specs) || g_button_specs[button_id].port == NULL) {
        return false;
    }
    return (bool)gpio_pin_get_dt(&g_button_specs[button_id]);
}