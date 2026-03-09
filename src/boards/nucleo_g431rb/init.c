#include "stm32g4xx_hal.h"

extern void SystemClock_Config(void);
extern void MX_GPIO_Init(void);

void board_init(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
}
