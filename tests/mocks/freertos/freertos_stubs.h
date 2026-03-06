#pragma once

#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define pdMS_TO_TICKS(xTimeInMs) (xTimeInMs)
#define portTICK_PERIOD_MS 1

typedef uint32_t TickType_t;

// vTaskDelay stub using nanosleep
static inline void vTaskDelay(TickType_t xTicksToDelay)
{
    struct timespec ts;
    ts.tv_sec = xTicksToDelay / 1000;
    ts.tv_nsec = (xTicksToDelay % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

#ifdef __cplusplus
}
#endif
