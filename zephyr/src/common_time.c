#include "common_time.h"
#include <zephyr/kernel.h>
uint32_t common_get_timestamp_ms(void)
{
    return (uint32_t)k_uptime_get();
}
uint32_t common_get_timestamp_ms_isr(void)
{
    return (uint32_t)k_uptime_get();
}
