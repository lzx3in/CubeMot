// MSGHUB global state definitions
// This file contains ONLY global state variable definitions.
// No logic or functions should be placed here.

#include "msghub_state.h"
#include <string.h>

// ============================================================================
// Global state definitions
// ============================================================================

// All registered topics (runtime state)
msghub_topic_state_t g_topics[MSGHUB_MAX_TOPICS];

// Publisher handle slot table
msghub_pub_slot_t g_pub_slots[MSGHUB_MAX_TOPICS * MSGHUB_MAX_INSTANCES];

// Subscriber handle slot table
msghub_sub_slot_t g_sub_slots[MSGHUB_MAX_SUBSCRIBERS];

// Number of registered topics
uint8_t g_num_topics = 0;

// ============================================================================
// Management mutex (environment-specific)
// ============================================================================

#if defined(RTTHREAD_ENV)
rt_mutex_t g_msghub_mgr_mutex = RT_NULL;
#elif defined(FREERTOS_ENV)
SemaphoreHandle_t g_msghub_mgr_mutex = NULL;
#elif defined(ZEPHYR_ENV)
struct k_mutex g_msghub_mgr_mutex;
#elif defined(UNIT_TEST_HOST)
pthread_mutex_t g_msghub_mgr_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t g_msghub_crit_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

// ============================================================================
// Initialization
// ============================================================================

msghub_err_t msghub_init(void)
{
#if defined(RTTHREAD_ENV)
    // Create management mutex (IPC flag = priority inheritance)
    g_msghub_mgr_mutex = rt_mutex_create("msghub_mgr", RT_IPC_FLAG_PRIO);
    if (g_msghub_mgr_mutex == RT_NULL) {
        return MSGHUB_ERR_NO_MEM;
    }
#elif defined(FREERTOS_ENV)
    // Create management mutex (recursive for safety)
    g_msghub_mgr_mutex = xSemaphoreCreateMutex();
    if (g_msghub_mgr_mutex == NULL) {
        return MSGHUB_ERR_NO_MEM;
    }
#elif defined(ZEPHYR_ENV)
    // Initialize management mutex
    k_mutex_init(&g_msghub_mgr_mutex);
#elif defined(UNIT_TEST_HOST)
    // Initialize pthread mutexes
    pthread_mutex_init(&g_msghub_crit_mutex, NULL);
    pthread_mutex_init(&g_msghub_mgr_mutex, NULL);
#endif

    // Zero-initialize all global state
    memset(g_topics, 0, sizeof(g_topics));
    memset(g_pub_slots, 0, sizeof(g_pub_slots));
    memset(g_sub_slots, 0, sizeof(g_sub_slots));
    g_num_topics = 0;

    return MSGHUB_OK;
}

void msghub_deinit(void)
{
#if defined(RTTHREAD_ENV)
    if (g_msghub_mgr_mutex != RT_NULL) {
        rt_mutex_delete(g_msghub_mgr_mutex);
        g_msghub_mgr_mutex = RT_NULL;
    }
#elif defined(FREERTOS_ENV)
    if (g_msghub_mgr_mutex != NULL) {
        vSemaphoreDelete(g_msghub_mgr_mutex);
        g_msghub_mgr_mutex = NULL;
    }
#elif defined(ZEPHYR_ENV)
    // Zephyr k_mutex does not need explicit deinit
#elif defined(UNIT_TEST_HOST)
    pthread_mutex_destroy(&g_msghub_crit_mutex);
    pthread_mutex_destroy(&g_msghub_mgr_mutex);
#endif
}

// ============================================================================
// Test support functions
// ============================================================================

void msghub_reset(void)
{
    // Clear all global state
    memset(g_topics, 0, sizeof(g_topics));
    memset(g_pub_slots, 0, sizeof(g_pub_slots));
    memset(g_sub_slots, 0, sizeof(g_sub_slots));
    g_num_topics = 0;
}
