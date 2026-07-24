#pragma once

#include "msghub/msghub.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Configuration parameters
// ============================================================================

// Use Kconfig values when building with Zephyr, otherwise use defaults
#ifdef ZEPHYR_ENV
#include <zephyr/autoconf.h>
#endif

#ifndef MSGHUB_MAX_TOPICS
#ifdef CONFIG_MSGHUB_MAX_TOPICS
#define MSGHUB_MAX_TOPICS CONFIG_MSGHUB_MAX_TOPICS
#else
#define MSGHUB_MAX_TOPICS 8
#endif
#endif

#ifndef MSGHUB_MAX_INSTANCES
#ifdef CONFIG_MSGHUB_MAX_INSTANCES
#define MSGHUB_MAX_INSTANCES CONFIG_MSGHUB_MAX_INSTANCES
#else
#define MSGHUB_MAX_INSTANCES 4
#endif
#endif

#ifndef MSGHUB_MAX_SUBSCRIBERS
#ifdef CONFIG_MSGHUB_MAX_SUBSCRIBERS
#define MSGHUB_MAX_SUBSCRIBERS CONFIG_MSGHUB_MAX_SUBSCRIBERS
#else
#define MSGHUB_MAX_SUBSCRIBERS 16
#endif
#endif

// ============================================================================
// Critical Section Macros — Zephyr RTOS primitives
// ============================================================================

#include <zephyr/kernel.h>

extern struct k_mutex g_msghub_mgr_mutex; // Management mutex

// Task context critical section (disable scheduler)
#define MSGHUB_ENTER_CRITICAL()    k_sched_lock()
#define MSGHUB_EXIT_CRITICAL()     k_sched_unlock()

// ISR context critical section (disable interrupts)
#define MSGHUB_ENTER_CRITICAL_ISR() \
    {                               \
        unsigned int uxSavedInterruptStatus = irq_lock()
#define MSGHUB_EXIT_CRITICAL_ISR()  \
        irq_unlock(uxSavedInterruptStatus); \
    }

// Management lock (mutex, task context only)
#define MSGHUB_LOCK_MGR()   k_mutex_lock(&g_msghub_mgr_mutex, K_FOREVER)
#define MSGHUB_UNLOCK_MGR() k_mutex_unlock(&g_msghub_mgr_mutex)

// ============================================================================
// Handle encoding magic numbers
// ============================================================================

#define MSGHUB_PUBLISHER_MAGIC  0xA5
#define MSGHUB_SUBSCRIBER_MAGIC 0x5A

// ============================================================================
// Internal data structures
// ============================================================================

// Topic instance (each topic can have multiple instances)
typedef struct {
    bool      allocated;   // Is this instance slot allocated to a publisher
    uint16_t  generation;  // Data generation counter
    uint8_t   data[128];   // Data buffer (preserved after publisher destroy)
} msghub_instance_t;

// Topic runtime state (internal use only)
typedef struct {
    msghub_topic_t    topic;                             // Pointer to topic definition
    msghub_instance_t instances[MSGHUB_MAX_INSTANCES];   // Multi-instance data
} msghub_topic_state_t;

// Publisher handle slot
typedef struct {
    uint8_t magic;      // Magic number validation
    uint8_t topic_idx;  // Topic index
    uint8_t instance;   // Instance number
} msghub_pub_slot_t;

// Subscriber callback function type
typedef void (*msghub_sub_callback_t)(msghub_subscriber_t sub, void *context);

// Subscriber handle slot
typedef struct {
    uint8_t                  magic;              // Magic number validation
    uint8_t                  topic_idx;          // Topic index
    uint8_t                  instance;           // Instance number
    uint16_t                 last_generation;    // Last read generation
    msghub_sub_callback_t    callback;           // Callback function (NULL = polling mode)
    void                    *callback_context;   // User-provided context for callback
} msghub_sub_slot_t;

// ============================================================================
// Global state declarations (defined in msghub_state.c)
// ============================================================================

extern msghub_topic_state_t g_topics[MSGHUB_MAX_TOPICS];
extern msghub_pub_slot_t    g_pub_slots[MSGHUB_MAX_TOPICS * MSGHUB_MAX_INSTANCES];
extern msghub_sub_slot_t    g_sub_slots[MSGHUB_MAX_SUBSCRIBERS];
extern uint8_t              g_num_topics;

// ============================================================================
// Initialization
// ============================================================================

// Initialize msghub (create management mutex, etc.)
msghub_err_t msghub_init(void);

// Cleanup msghub (destroy management mutex, etc.)
void msghub_deinit(void);

// ============================================================================
// Test support functions
// ============================================================================

void msghub_reset(void);

#ifdef __cplusplus
}
#endif
