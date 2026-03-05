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
