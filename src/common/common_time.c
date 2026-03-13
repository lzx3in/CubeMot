#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
uint32_t common_get_timestamp_ms(void)
{
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

uint32_t common_get_timestamp_ms_isr(void)
{
    return xTaskGetTickCountFromISR() * portTICK_PERIOD_MS;
}
