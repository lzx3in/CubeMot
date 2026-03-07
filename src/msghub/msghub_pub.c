#include "msghub/msghub.h"
#include "msghub_topic.h"
#include "msghub_state.h"
#include <string.h>

// ============================================================================
// Publisher API
// ============================================================================

// Create a publisher for a topic. Returns MSGHUB_PUBLISHER_INVALID on failure.
msghub_publisher_t msghub_create_publisher(msghub_topic_t topic)
{
    if (topic == NULL) {
        return MSGHUB_PUBLISHER_INVALID;
    }

    // Allocate topic
    int8_t topic_idx = msghub_core_alloc_topic(topic);
    if (topic_idx < 0) {
        return MSGHUB_PUBLISHER_INVALID;
    }

    // Default to instance 0
    uint8_t instance = 0;
    msghub_instance_t *inst = &g_topics[topic_idx].instances[instance];

    // Allocate publisher slot
    int8_t slot_idx = msghub_core_alloc_pub_slot();
    if (slot_idx < 0) {
        return MSGHUB_PUBLISHER_INVALID;
    }

    // Mark instance as allocated on first creation
    // Do NOT reset generation to preserve data for existing subscribers
    if (!inst->allocated) {
        inst->allocated = true;
        // generation initialized to 0 by global/static initialization
        // Subsequent publishers reuse the same instance without resetting generation
    }

    // Initialize publisher slot
    g_pub_slots[slot_idx].magic = MSGHUB_PUBLISHER_MAGIC;
    g_pub_slots[slot_idx].topic_idx = (uint8_t)topic_idx;
    g_pub_slots[slot_idx].instance = instance;

    // Encode and return handle
    msghub_publisher_t handle;
    msghub_core_encode_pub_handle(&handle, (uint8_t)slot_idx);
    return handle;
}

// Destroy a publisher
msghub_err_t msghub_destroy_publisher(msghub_publisher_t handle)
{
    uint8_t slot_idx;
    if (msghub_core_decode_pub_handle(handle, &slot_idx) < 0) {
        return MSGHUB_ERR_INVALID;
    }

    msghub_pub_slot_t *slot = &g_pub_slots[slot_idx];
    msghub_instance_t *inst = &g_topics[slot->topic_idx].instances[slot->instance];

    // Clear instance allocated flag to allow reallocation
    // Note: Data is preserved, only the flag is cleared
    // Subscribers can still access the data via subscriber_check
    inst->allocated = false;
    slot->magic = 0;

    return MSGHUB_OK;
}

// Publish data to topic
msghub_err_t msghub_publish(msghub_publisher_t handle, const void *data)
{
    if (data == NULL) {
        return MSGHUB_ERR_INVALID;
    }

    uint8_t slot_idx;
    if (msghub_core_decode_pub_handle(handle, &slot_idx) < 0) {
        return MSGHUB_ERR_INVALID;
    }

    msghub_pub_slot_t *slot = &g_pub_slots[slot_idx];
    msghub_topic_state_t *topic_state = &g_topics[slot->topic_idx];
    msghub_instance_t *inst = &topic_state->instances[slot->instance];

    // Check instance validity
    if (!inst->allocated) {
        return MSGHUB_ERR_NOT_FOUND;
    }

    // Copy data and increment generation
    memcpy(inst->data, data, topic_state->topic->msg_size);
    inst->generation++;
    return MSGHUB_OK;
}

// ============================================================================
// Multi-instance support
// ============================================================================

// Create a publisher with multi-instance support. Instance: -1 for auto-allocate.
msghub_publisher_t msghub_create_publisher_multi(msghub_topic_t topic, int *instance)
{
    if (topic == NULL || instance == NULL) {
        return MSGHUB_PUBLISHER_INVALID;
    }

    // Allocate topic
    int8_t topic_idx = msghub_core_alloc_topic(topic);
    if (topic_idx < 0) {
        return MSGHUB_PUBLISHER_INVALID;
    }

    uint8_t target_instance;

    // Determine target instance
    if (*instance < 0) {
        // Auto-allocate: find first free instance
        target_instance = 0xFF;
        for (uint8_t i = 0; i < MSGHUB_MAX_INSTANCES; i++) {
            if (!g_topics[topic_idx].instances[i].allocated) {
                target_instance = i;
                break;
            }
        }
        if (target_instance == 0xFF) {
            return MSGHUB_PUBLISHER_INVALID;
        }
    } else {
        // Use specified instance
        if ((uint8_t)*instance >= MSGHUB_MAX_INSTANCES) {
            return MSGHUB_PUBLISHER_INVALID;
        }
        if (g_topics[topic_idx].instances[*instance].allocated) {
            return MSGHUB_PUBLISHER_INVALID;
        }
        target_instance = (uint8_t)*instance;
    }

    // Allocate publisher slot
    int8_t slot_idx = msghub_core_alloc_pub_slot();
    if (slot_idx < 0) {
        return MSGHUB_PUBLISHER_INVALID;
    }

    // Initialize instance data
    msghub_instance_t *inst = &g_topics[topic_idx].instances[target_instance];
    inst->allocated = true;
    // Note: Do NOT reset generation to preserve data for existing subscribers

    // Initialize publisher slot
    g_pub_slots[slot_idx].magic = MSGHUB_PUBLISHER_MAGIC;
    g_pub_slots[slot_idx].topic_idx = (uint8_t)topic_idx;
    g_pub_slots[slot_idx].instance = target_instance;

    // Output actual allocated instance
    *instance = target_instance;

    // Encode and return handle
    msghub_publisher_t handle;
    msghub_core_encode_pub_handle(&handle, (uint8_t)slot_idx);
    return handle;
}

// ============================================================================
// Topic query functions
// ============================================================================

// Check if topic instance exists
bool msghub_topic_exists(msghub_topic_t topic, uint8_t instance)
{
    int8_t topic_idx = msghub_core_find_topic(topic);
    if (topic_idx < 0) {
        return false;
    }
    if (instance >= MSGHUB_MAX_INSTANCES) {
        return false;
    }
    return g_topics[topic_idx].instances[instance].allocated != 0;
}

// Get number of published topic instances
int msghub_topic_publisher_count(msghub_topic_t topic)
{
    int8_t topic_idx = msghub_core_find_topic(topic);
    if (topic_idx < 0) {
        return 0;
    }

    int count = 0;
    for (uint8_t i = 0; i < MSGHUB_MAX_INSTANCES; i++) {
        if (g_topics[topic_idx].instances[i].allocated) {
            count++;
        }
    }
    return count;
}

// ============================================================================
// Utility functions
// ============================================================================

// Validate publisher handle
bool msghub_publisher_valid(msghub_publisher_t handle)
{
    uint8_t slot_idx;
    return msghub_core_decode_pub_handle(handle, &slot_idx) == 0;
}
