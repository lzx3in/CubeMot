#include "boards/delay.h"
#include "stm32g4xx_hal.h"

void board_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}

uint32_t board_get_tick(void)
{
    return HAL_GetTick();
}
