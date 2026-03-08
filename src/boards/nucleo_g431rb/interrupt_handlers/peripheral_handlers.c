#include "stm32g4xx_hal.h"

extern TIM_HandleTypeDef htim3;

// TIM3 global interrupt
void TIM3_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim3);
}
