#include "msghub_topic.h"
#include "msghub_state.h"
#include <string.h>

// ============================================================================
// Topic management
// Note: These functions are called from management paths (create/destroy)
//       The caller is responsible for holding MSGHUB_LOCK_MGR().
// ============================================================================

// Find registered topic by topic handle. Returns topic index, -1 on failure.
// Read-only operation, no lock needed.
int8_t msghub_core_find_topic(msghub_topic_t topic)
{
    for (uint8_t i = 0; i < g_num_topics; i++) {
        if (g_topics[i].topic == topic) {
            return (int8_t)i;
        }
    }
    return -1;
}

// Allocate new topic slot. Returns existing index if topic already exists.
// Caller MUST hold MSGHUB_LOCK_MGR() before calling.
int8_t msghub_core_alloc_topic(msghub_topic_t topic)
{
    // Check if already exists (read-only, no lock needed for find)
    int8_t existing = msghub_core_find_topic(topic);
    if (existing >= 0) {
        return existing;
    }

    // Check capacity
    if (g_num_topics >= MSGHUB_MAX_TOPICS) {
        return -1;
    }

    // Allocate new slot (caller holds MSGHUB_LOCK_MGR)
    uint8_t idx = g_num_topics++;
    g_topics[idx].topic = topic;
    memset(g_topics[idx].instances, 0, sizeof(g_topics[idx].instances));
    return (int8_t)idx;
}

// ============================================================================
// Publisher handle management
// Note: These functions are called from management paths (create/destroy)
//       The caller is responsible for holding MSGHUB_LOCK_MGR().
// ============================================================================

// Allocate publisher slot. Returns slot index, -1 on failure.
// Caller MUST hold MSGHUB_LOCK_MGR() before calling.
int8_t msghub_core_alloc_pub_slot(void)
{
    const uint8_t max_slots = sizeof(g_pub_slots) / sizeof(g_pub_slots[0]);
    for (uint8_t i = 0; i < max_slots; i++) {
        if (g_pub_slots[i].magic == 0) {
            return (int8_t)i;
        }
    }
    return -1;
}

// Encode publisher handle from slot index
// No lock needed (pure computation)
void msghub_core_encode_pub_handle(msghub_publisher_t *handle, uint8_t slot_idx)
{
    *handle = ((uint16_t)MSGHUB_PUBLISHER_MAGIC << 8) | slot_idx;
}

// Decode publisher handle. Returns 0 success, -1 failure.
// Read-only validation, no lock needed.
int8_t msghub_core_decode_pub_handle(msghub_publisher_t handle, uint8_t *slot_idx)
{
    uint8_t magic = (handle >> 8) & 0xFF;
    uint8_t idx = handle & 0xFF;
    const uint8_t max_slots = sizeof(g_pub_slots) / sizeof(g_pub_slots[0]);

    // Validate magic number
    if (magic != MSGHUB_PUBLISHER_MAGIC) {
        return -1;
    }

    // Validate index range
    if (idx >= max_slots) {
        return -1;
    }

    // Validate slot validity
    if (g_pub_slots[idx].magic != MSGHUB_PUBLISHER_MAGIC) {
        return -1;
    }

    *slot_idx = idx;
    return 0;
}

// ============================================================================
// Subscriber handle management
// Note: These functions are called from management paths (create/destroy)
//       The caller is responsible for holding MSGHUB_LOCK_MGR().
// ============================================================================

// Allocate subscriber slot. Returns slot index, -1 on failure.
// Caller MUST hold MSGHUB_LOCK_MGR() before calling.
int8_t msghub_core_alloc_sub_slot(void)
{
    const uint8_t max_slots = sizeof(g_sub_slots) / sizeof(g_sub_slots[0]);
    for (uint8_t i = 0; i < max_slots; i++) {
        if (g_sub_slots[i].magic == 0) {
            return (int8_t)i;
        }
    }
    return -1;
}

// Encode subscriber handle from slot index
// No lock needed (pure computation)
void msghub_core_encode_sub_handle(msghub_subscriber_t *handle, uint8_t slot_idx)
{
    *handle = ((uint16_t)MSGHUB_SUBSCRIBER_MAGIC << 8) | slot_idx;
}

// Decode subscriber handle. Returns 0 success, -1 failure.
// Read-only validation, no lock needed.
int8_t msghub_core_decode_sub_handle(msghub_subscriber_t handle, uint8_t *slot_idx)
{
    uint8_t magic = (handle >> 8) & 0xFF;
    uint8_t idx = handle & 0xFF;
    const uint8_t max_slots = sizeof(g_sub_slots) / sizeof(g_sub_slots[0]);

    // Validate magic number
    if (magic != MSGHUB_SUBSCRIBER_MAGIC) {
        return -1;
    }

    // Validate index range
    if (idx >= max_slots) {
        return -1;
    }

    // Validate slot validity
    if (g_sub_slots[idx].magic != MSGHUB_SUBSCRIBER_MAGIC) {
        return -1;
    }

    *slot_idx = idx;
    return 0;
}
