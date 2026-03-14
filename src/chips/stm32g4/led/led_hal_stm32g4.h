#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize and register STM32G4 LED HAL
 * Called during board initialization
 */
void stm32g4_led_hal_init(void);

#ifdef __cplusplus
}
#endif
