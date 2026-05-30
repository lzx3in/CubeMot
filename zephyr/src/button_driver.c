#include "button_driver.h"
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>

#define BUTTON0_NODE DT_NODELABEL(button0)

static const struct gpio_dt_spec g_button_specs[] = {
    [0] = GPIO_DT_SPEC_GET_OR(BUTTON0_NODE, gpios, {0}),
};

static struct gpio_callback g_button_callbacks[1];
static button_callback_t g_user_callbacks[1];

static void button_isr_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    (void)dev;
    (void)pins;
    for (int i = 0; i < 1; i++) {
        if (cb == &g_button_callbacks[i] && g_user_callbacks[i]) {
            g_user_callbacks[i](NULL);
        }
    }
}

int button_init(uint8_t button_id, button_callback_t callback)
{
    if (button_id >= 1 || g_button_specs[button_id].port == NULL)
        return -1;
    int ret = gpio_pin_configure_dt(&g_button_specs[button_id], GPIO_INPUT);
    if (ret < 0)
        return ret;
    ret = gpio_pin_interrupt_configure_dt(&g_button_specs[button_id], GPIO_INT_EDGE_TO_ACTIVE);
    if (ret < 0)
        return ret;
    g_user_callbacks[button_id] = callback;
    gpio_init_callback(&g_button_callbacks[button_id], button_isr_handler, BIT(g_button_specs[button_id].pin));
    gpio_add_callback(g_button_specs[button_id].port, &g_button_callbacks[button_id]);
    return 0;
}
