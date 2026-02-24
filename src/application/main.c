#include "main.h"
#include "drivers/led/led.h"
#include "boards/led.h"
#include "cubemot_config.h"

void SystemClock_Config(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

#if CONFIG_BOARD_HAS_LED1
    led_t led1;
    if (led_init(&led1, BOARD_LED_1) == LED_SUCCESS) {
        while (1) {
            led_toggle(&led1);
            HAL_Delay(500);
        }
    }
#else
    while (1) {
    }
#endif
}
