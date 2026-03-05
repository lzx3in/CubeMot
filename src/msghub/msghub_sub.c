#include "msghub/msghub.h"
#include "msghub_topic.h"
#include "msghub_state.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"

// ============================================================================
// Subscriber API
// ============================================================================

// Create a subscriber for a topic. Returns MSGHUB_SUBSCRIBER_INVALID on failure.
msghub_subscriber_t msghub_create_subscriber(msghub_topic_t topic, uint8_t instance)
{
    if (topic == NULL || instance >= MSGHUB_MAX_INSTANCES) {
        return MSGHUB_SUBSCRIBER_INVALID;
    }

    // Find or allocate topic
    int8_t topic_idx = msghub_core_find_topic(topic);
    if (topic_idx < 0) {
        topic_idx = msghub_core_alloc_topic(topic);
        if (topic_idx < 0) {
            return MSGHUB_SUBSCRIBER_INVALID;
        }
    }

    // Allocate subscriber slot
    int8_t slot_idx = msghub_core_alloc_sub_slot();
    if (slot_idx < 0) {
        return MSGHUB_SUBSCRIBER_INVALID;
    }

    // Initialize subscriber slot
    g_sub_slots[slot_idx].magic = MSGHUB_SUBSCRIBER_MAGIC;
    g_sub_slots[slot_idx].topic_idx = (uint8_t)topic_idx;
    g_sub_slots[slot_idx].instance = instance;

    // Initialize generation tracking
    msghub_instance_t *inst = &g_topics[topic_idx].instances[instance];
    if (inst->advertised) {
        g_sub_slots[slot_idx].last_generation = inst->generation;
    } else {
        g_sub_slots[slot_idx].last_generation = 0;
    }

    // Encode and return handle
    msghub_subscriber_t handle;
    msghub_core_encode_sub_handle(&handle, (uint8_t)slot_idx);
    return handle;
}

// Destroy a subscriber
msghub_err_t msghub_destroy_subscriber(msghub_subscriber_t handle)
{
    uint8_t slot_idx;
    if (msghub_core_decode_sub_handle(handle, &slot_idx) < 0) {
        return MSGHUB_ERR_INVALID;
    }

    g_sub_slots[slot_idx].magic = 0;
    return MSGHUB_OK;
}

// Receive (copy) topic data to buffer
msghub_err_t msghub_receive(msghub_subscriber_t handle, void *buffer)
{
    if (buffer == NULL) {
        return MSGHUB_ERR_INVALID;
    }

    uint8_t slot_idx;
    if (msghub_core_decode_sub_handle(handle, &slot_idx) < 0) {
        return MSGHUB_ERR_INVALID;
    }

    msghub_sub_slot_t *slot = &g_sub_slots[slot_idx];
    msghub_topic_state_t *topic_state = &g_topics[slot->topic_idx];
    msghub_instance_t *inst = &topic_state->instances[slot->instance];

    // Check instance validity
    if (!inst->advertised) {
        return MSGHUB_ERR_NOT_FOUND;
    }

    // Copy data and update generation
    memcpy(buffer, inst->data, topic_state->topic->msg_size);
    slot->last_generation = inst->generation;
    return MSGHUB_OK;
}

// Check if topic has updates
msghub_err_t msghub_subscriber_check(msghub_subscriber_t handle, bool *updated)
{
    if (updated == NULL) {
        return MSGHUB_ERR_INVALID;
    }

    uint8_t slot_idx;
    if (msghub_core_decode_sub_handle(handle, &slot_idx) < 0) {
        return MSGHUB_ERR_INVALID;
    }

    msghub_sub_slot_t *slot = &g_sub_slots[slot_idx];
    msghub_topic_state_t *topic_state = &g_topics[slot->topic_idx];
    msghub_instance_t *inst = &topic_state->instances[slot->instance];

    // Check instance validity
    if (!inst->advertised) {
        return MSGHUB_ERR_NOT_FOUND;
    }

    // Compare generation
    *updated = (inst->generation != slot->last_generation);
    return MSGHUB_OK;
}

// ============================================================================
// Advanced features
// ============================================================================

// Get topic current generation. Returns 0 on failure.
uint16_t msghub_get_generation(msghub_subscriber_t handle)
{
    uint8_t slot_idx;
    if (msghub_core_decode_sub_handle(handle, &slot_idx) < 0) {
        return 0;
    }

    msghub_sub_slot_t *slot = &g_sub_slots[slot_idx];
    msghub_topic_state_t *topic_state = &g_topics[slot->topic_idx];
    msghub_instance_t *inst = &topic_state->instances[slot->instance];

    if (!inst->advertised) {
        return 0;
    }

    return inst->generation;
}

// Block waiting for topic update. Timeout 0 for immediate return.
msghub_err_t msghub_subscriber_poll(msghub_subscriber_t handle, uint32_t timeout_ms, bool *updated)
{
    if (updated == NULL) {
        return MSGHUB_ERR_INVALID;
    }

    // Timeout 0: degrade to check
    if (timeout_ms == 0) {
        return msghub_subscriber_check(handle, updated);
    }

    // Use FreeRTOS for blocking wait
    uint32_t elapsed = 0;
    while (elapsed < timeout_ms) {
        msghub_err_t err = msghub_subscriber_check(handle, updated);
        if (err != MSGHUB_OK) {
            return err;
        }
        if (*updated) {
            return MSGHUB_OK;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
        elapsed++;
    }

    *updated = false;
    return MSGHUB_ERR_TIMEOUT;
}

// Atomic check and receive (if updated)
msghub_err_t msghub_subscriber_update(msghub_subscriber_t handle, void *buffer, bool *updated)
{
    if (buffer == NULL || updated == NULL) {
        return MSGHUB_ERR_INVALID;
    }

    // Check for updates first
    msghub_err_t err = msghub_subscriber_check(handle, updated);
    if (err != MSGHUB_OK) {
        return err;
    }

    // Receive if updated
    if (*updated) {
        err = msghub_receive(handle, buffer);
        return err;
    }

    return MSGHUB_OK;
}

// ============================================================================
// Utility functions
// ============================================================================

// Validate subscriber handle
bool msghub_subscriber_valid(msghub_subscriber_t handle)
{
    uint8_t slot_idx;
    return msghub_core_decode_sub_handle(handle, &slot_idx) == 0;
}
