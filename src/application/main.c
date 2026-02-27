#include "drivers/led/led.h"
#include "boards/led.h"
#include "boards/delay.h"
#include "boards/init.h"
#include "cubemot_config.h"

int main(void)
{
    board_init();

#if CONFIG_BOARD_HAS_LED1
    led_t led1;
    if (led_init(&led1, BOARD_LED_1) == LED_OK) {
        for (;;) {
            led_toggle(&led1);
            board_delay_ms(500);
        }
    }
#else
    for (;;) {
    }
#endif
}
