#include "stm32g4xx_hal.h"

// fault interrupt
void HardFault_Handler(void)
{
    while (1) {
    }
}

// Memory management fault
void MemManage_Handler(void)
{
    while (1) {
    }
}

// Prefetch fault, memory access fault
void BusFault_Handler(void)
{

    while (1) {
    }
}

// Undefined instruction or illegal state
void UsageFault_Handler(void)
{
    while (1) {
    }
}
