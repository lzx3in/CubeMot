#include <stdint.h>

#if defined(ZEPHYR_ENV)
#include <zephyr/kernel.h>
uint32_t common_get_timestamp_ms(void)
{
    return (uint32_t)k_uptime_get();
}

uint32_t common_get_timestamp_ms_isr(void)
{
    return (uint32_t)k_uptime_get();
}
#else
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
#endif
