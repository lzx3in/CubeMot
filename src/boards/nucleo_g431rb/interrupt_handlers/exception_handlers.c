#include "stm32g4xx_hal.h"

// Non maskable interrupt
void NMI_Handler(void)
{
    while (1) {
    }
}

// Debug monitor
void DebugMon_Handler(void) {}
