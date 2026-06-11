// MSGHUB global state definitions — Zephyr RTOS only
// This file contains ONLY global state variable definitions.
// No logic or functions should be placed here.

#include "msghub_state.h"
#include <string.h>
#include <zephyr/kernel.h>

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

// Management mutex (Zephyr k_mutex, initialized in msghub_init)
struct k_mutex g_msghub_mgr_mutex;

// ============================================================================
// Initialization
// ============================================================================

msghub_err_t msghub_init(void)
{
    k_mutex_init(&g_msghub_mgr_mutex);

    // Zero-initialize all global state
    memset(g_topics,   0, sizeof(g_topics));
    memset(g_pub_slots, 0, sizeof(g_pub_slots));
    memset(g_sub_slots, 0, sizeof(g_sub_slots));
    g_num_topics = 0;

    return MSGHUB_OK;
}

void msghub_deinit(void)
{
    // Zephyr k_mutex does not need explicit deinit
}

// ============================================================================
// Test support functions
// ============================================================================

void msghub_reset(void)
{
    memset(g_topics,   0, sizeof(g_topics));
    memset(g_pub_slots, 0, sizeof(g_pub_slots));
    memset(g_sub_slots, 0, sizeof(g_sub_slots));
    g_num_topics = 0;
}
