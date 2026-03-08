#pragma once

#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t TickType_t;
typedef void *TaskHandle_t;

#define pdMS_TO_TICKS(xTimeInMs) ((xTimeInMs) / portTICK_PERIOD_MS)
#define portTICK_PERIOD_MS 1

// vTaskDelay stub using nanosleep
static inline void vTaskDelay(TickType_t xTicksToDelay)
{
    struct timespec ts;
    ts.tv_sec = xTicksToDelay / 1000;
    ts.tv_nsec = (xTicksToDelay % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

// xTaskGetTickCount stub - returns milliseconds since epoch
static inline TickType_t xTaskGetTickCount(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (TickType_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

// xTaskCreate stub - no-op for testing
static inline int32_t xTaskCreate(void *pvTaskCode, const char *const pcName, uint32_t usStackDepth, void *pvParameters,
                                  uint32_t uxPriority, void *pxCreatedTask)
{
    (void)pvTaskCode;
    (void)pcName;
    (void)usStackDepth;
    (void)pvParameters;
    (void)uxPriority;
    (void)pxCreatedTask;
    return 0; // Success
}

#ifdef __cplusplus
}
#endif
