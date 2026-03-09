#include "stm32g4xx_hal.h"

extern TIM_HandleTypeDef htim3;

// TIM3 global interrupt
void TIM3_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim3);
}

// Period elapsed callback in non blocking mode
// This function is called  when TIM3 interrupt took place, inside
// HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
// a global variable "uwTick" used as application time base.
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM3)
  {
    HAL_IncTick();
  }
}
