#include "main.h"
#include "drivers/led/led.h"
#include "boards/led.h"
#include "boards/board_config.h"

void SystemClock_Config(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

#if BOARD_HAS_LED1
    const board_led_config_t *led1_hw_config = board_led_get_config(BOARD_LED_1);

    led_t led1;
    led_init(&led1, led1_hw_config);

    while (1) {
        led_toggle(&led1);
        HAL_Delay(500);
    }
#else
    while (1) {
    }
#endif
}
