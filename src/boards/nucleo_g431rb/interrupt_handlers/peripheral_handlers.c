#include "stm32g4xx_hal.h"
#include "chips/gpio.h"
#include "drivers/button/button.h"
#include "common_device.h"

extern TIM_HandleTypeDef htim3;

void TIM3_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim3);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM3)
  {
    HAL_IncTick();
  }
}

void EXTI15_10_IRQHandler(void)
{
    chip_gpio_exti_irq_handler(CHIP_GPIO_PIN_13);
}

void chip_gpio_exti_callback(chip_gpio_pin_t pin)
{
    if (pin == CHIP_GPIO_PIN_13) {
        const driver_button_instance *inst = driver_button_get_instance(CUBEMOT_DEVICE_BUTTON_0);
        if (inst && inst->irq_handler) {
            inst->irq_handler(inst->ctx);
        }
    }
}
