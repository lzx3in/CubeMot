#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t TickType_t;
typedef void *TaskHandle_t;
typedef int32_t BaseType_t;
typedef uint32_t UBaseType_t;
typedef uint32_t uint32_t;

// eNotifyAction enum for task notifications
typedef enum {
    eSetBits = 0,
    eIncrement,
    eSetValueWithOverwrite,
    eSetValueWithoutOverwrite,
    eNoAction
} eNotifyAction;

#define pdTRUE 1
#define pdFALSE 0
#define pdMS_TO_TICKS(xTimeInMs) ((xTimeInMs) / portTICK_PERIOD_MS)
#define portTICK_PERIOD_MS 1
#define portMAX_DELAY ((TickType_t)0xffffffffUL)

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
static inline BaseType_t xTaskCreate(void *pvTaskCode, const char *const pcName, uint32_t usStackDepth,
                                     void *pvParameters, UBaseType_t uxPriority, TaskHandle_t *pxCreatedTask)
{
    (void)pvTaskCode;
    (void)pcName;
    (void)usStackDepth;
    (void)pvParameters;
    (void)uxPriority;
    (void)pxCreatedTask;
    return 0; // Success
}

// Task notification stubs for testing (no-op in unit test environment)
static inline BaseType_t xTaskNotifyFromISR(TaskHandle_t xTaskToNotify, uint32_t ulValue, eNotifyAction eAction,
                                            BaseType_t *pxHigherPriorityTaskWoken)
{
    (void)xTaskToNotify;
    (void)ulValue;
    (void)eAction;
    (void)pxHigherPriorityTaskWoken;
    return pdTRUE;
}

static inline uint32_t ulTaskNotifyTake(BaseType_t xClearCountOnExit, TickType_t xTicksToWait)
{
    (void)xClearCountOnExit;
    (void)xTicksToWait;
    // In test environment, just return immediately
    return 1;
}

#ifdef __cplusplus
}
#endif
