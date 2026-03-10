#pragma once

#include "msghub/msghub.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Configuration parameters
// ============================================================================

#ifndef MSGHUB_MAX_TOPICS
#define MSGHUB_MAX_TOPICS 8
#endif

#ifndef MSGHUB_MAX_INSTANCES
#define MSGHUB_MAX_INSTANCES 4
#endif

#ifndef MSGHUB_MAX_SUBSCRIBERS
#define MSGHUB_MAX_SUBSCRIBERS 16
#endif

// ============================================================================
// Critical Section Macros - Environment-aware (3 environments)
// ============================================================================

#ifdef UNIT_TEST_HOST
// ============================================================================
// PC Unit Testing: pthread-based
// ============================================================================
#include <pthread.h>

extern pthread_mutex_t g_msghub_crit_mutex; // Data path (simulated critical section)
extern pthread_mutex_t g_msghub_mgr_mutex;  // Management path (mutex)

#define MSGHUB_ENTER_CRITICAL() pthread_mutex_lock(&g_msghub_crit_mutex)
#define MSGHUB_EXIT_CRITICAL() pthread_mutex_unlock(&g_msghub_crit_mutex)
#define MSGHUB_ENTER_CRITICAL_ISR() pthread_mutex_lock(&g_msghub_crit_mutex)
#define MSGHUB_EXIT_CRITICAL_ISR() pthread_mutex_unlock(&g_msghub_crit_mutex)
#define MSGHUB_LOCK_MGR() pthread_mutex_lock(&g_msghub_mgr_mutex)
#define MSGHUB_UNLOCK_MGR() pthread_mutex_unlock(&g_msghub_mgr_mutex)

#elif defined(RTTHREAD_ENV)
// ============================================================================
// RT-Thread: Native RT-Thread primitives
// ============================================================================
#include <rtthread.h>

extern rt_mutex_t g_msghub_mgr_mutex; // Management mutex

// Task context critical section (disable scheduler)
#define MSGHUB_ENTER_CRITICAL() rt_enter_critical()
#define MSGHUB_EXIT_CRITICAL() rt_exit_critical()

// ISR context critical section (disable interrupts)
#define MSGHUB_ENTER_CRITICAL_ISR() rt_hw_interrupt_disable()
#define MSGHUB_EXIT_CRITICAL_ISR() rt_hw_interrupt_enable()

// Management lock (mutex, task context only)
#define MSGHUB_LOCK_MGR() rt_mutex_take(g_msghub_mgr_mutex, RT_WAITING_FOREVER)
#define MSGHUB_UNLOCK_MGR() rt_mutex_release(g_msghub_mgr_mutex)

#elif defined(FREERTOS_ENV)
// ============================================================================
// FreeRTOS: Native FreeRTOS primitives
// ============================================================================
#include "FreeRTOS.h"
#include "semphr.h"

extern SemaphoreHandle_t g_msghub_mgr_mutex; // Management mutex

// Task context critical section (disable scheduler)
#define MSGHUB_ENTER_CRITICAL() taskENTER_CRITICAL()
#define MSGHUB_EXIT_CRITICAL() taskEXIT_CRITICAL()

// ISR context critical section (disable interrupts)
#define MSGHUB_ENTER_CRITICAL_ISR()                                                                                    \
    {                                                                                                                  \
        UBaseType_t uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR()
#define MSGHUB_EXIT_CRITICAL_ISR()                                                                                     \
    taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);                                                                \
    }

// Management lock (mutex, task context only)
#define MSGHUB_LOCK_MGR() xSemaphoreTake(g_msghub_mgr_mutex, portMAX_DELAY)
#define MSGHUB_UNLOCK_MGR() xSemaphoreGive(g_msghub_mgr_mutex)

#else
// ============================================================================
// Bare-metal: No protection (single-threaded)
// ============================================================================
#define MSGHUB_ENTER_CRITICAL()
#define MSGHUB_EXIT_CRITICAL()
#define MSGHUB_ENTER_CRITICAL_ISR()
#define MSGHUB_EXIT_CRITICAL_ISR()
#define MSGHUB_LOCK_MGR()
#define MSGHUB_UNLOCK_MGR()
#endif

// ============================================================================
// Handle encoding magic numbers
// ============================================================================

#define MSGHUB_PUBLISHER_MAGIC 0xA5
#define MSGHUB_SUBSCRIBER_MAGIC 0x5A

// ============================================================================
// Internal data structures
// ============================================================================

// Topic instance (each topic can have multiple instances)
typedef struct {
    bool allocated;      // Is this instance slot allocated to a publisher
    uint16_t generation; // Data generation counter
    uint8_t data[128];   // Data buffer (preserved after publisher destroy)
} msghub_instance_t;

// Topic runtime state (internal use only)
typedef struct {
    msghub_topic_t topic;                              // Pointer to topic definition
    msghub_instance_t instances[MSGHUB_MAX_INSTANCES]; // Multi-instance data
} msghub_topic_state_t;

// Publisher handle slot
typedef struct {
    uint8_t magic;     // Magic number validation
    uint8_t topic_idx; // Topic index
    uint8_t instance;  // Instance number
} msghub_pub_slot_t;

// Subscriber callback function type
// Called when new data is published to the subscribed topic
// Parameters:
//   - sub: Subscriber handle
//   - context: User-provided context pointer
// Note: This callback may be called from ISR context if publish is from ISR
typedef void (*msghub_sub_callback_t)(msghub_subscriber_t sub, void *context);

// Subscriber handle slot
typedef struct {
    uint8_t magic;                  // Magic number validation
    uint8_t topic_idx;              // Topic index
    uint8_t instance;               // Instance number
    uint16_t last_generation;       // Last read generation
    msghub_sub_callback_t callback; // Callback function (NULL = polling mode)
    void *callback_context;         // User-provided context for callback
} msghub_sub_slot_t;

// ============================================================================
// Global state declarations (defined in msghub_state.c)
// ============================================================================

extern msghub_topic_state_t g_topics[MSGHUB_MAX_TOPICS];
extern msghub_pub_slot_t g_pub_slots[MSGHUB_MAX_TOPICS * MSGHUB_MAX_INSTANCES];
extern msghub_sub_slot_t g_sub_slots[MSGHUB_MAX_SUBSCRIBERS];
extern uint8_t g_num_topics;

// ============================================================================
// Initialization (must be called before any other msghub function)
// ============================================================================

// Initialize msghub (create management mutex, etc.)
// Returns MSGHUB_OK on success, error code on failure
msghub_err_t msghub_init(void);

// Cleanup msghub (destroy management mutex, etc.)
void msghub_deinit(void);

// ============================================================================
// Test support functions (for testing only)
// ============================================================================

void msghub_reset(void);

#ifdef __cplusplus
}
#endif
