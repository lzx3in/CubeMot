#include "msghub/msghub.h"
#include "msghub_topic.h"
#include "msghub_state.h"
#include <string.h>

// ============================================================================
// Publisher API
// ============================================================================

msghub_publisher_t msghub_create_publisher(msghub_topic_t topic)
{
    if (topic == NULL) {
        return MSGHUB_PUBLISHER_INVALID;
    }

    // Management path: use mutex lock
    MSGHUB_LOCK_MGR();

    // Allocate topic
    int8_t topic_idx = msghub_core_alloc_topic(topic);
    if (topic_idx < 0) {
        MSGHUB_UNLOCK_MGR();
        return MSGHUB_PUBLISHER_INVALID;
    }

    // Default to instance 0
    uint8_t instance = 0;

    // Allocate publisher slot
    int8_t slot_idx = msghub_core_alloc_pub_slot();
    if (slot_idx < 0) {
        MSGHUB_UNLOCK_MGR();
        return MSGHUB_PUBLISHER_INVALID;
    }

    // Mark instance as allocated (with critical section for data access)
    MSGHUB_ENTER_CRITICAL();
    msghub_instance_t *inst = &g_topics[topic_idx].instances[instance];
    if (!inst->allocated) {
        inst->allocated = true;
    }
    MSGHUB_EXIT_CRITICAL();

    // Initialize publisher slot
    g_pub_slots[slot_idx].magic = MSGHUB_PUBLISHER_MAGIC;
    g_pub_slots[slot_idx].topic_idx = (uint8_t)topic_idx;
    g_pub_slots[slot_idx].instance = instance;

    MSGHUB_UNLOCK_MGR();

    // Encode and return handle
    msghub_publisher_t handle;
    msghub_core_encode_pub_handle(&handle, (uint8_t)slot_idx);
    return handle;
}

msghub_err_t msghub_destroy_publisher(msghub_publisher_t handle)
{
    uint8_t slot_idx;
    if (msghub_core_decode_pub_handle(handle, &slot_idx) < 0) {
        return MSGHUB_ERR_INVALID;
    }

    msghub_pub_slot_t *slot = &g_pub_slots[slot_idx];

    // Management path: use mutex lock
    MSGHUB_LOCK_MGR();

    // Clear instance allocated flag (with critical section)
    MSGHUB_ENTER_CRITICAL();
    msghub_instance_t *inst = &g_topics[slot->topic_idx].instances[slot->instance];
    inst->allocated = false;
    MSGHUB_EXIT_CRITICAL();

    slot->magic = 0;
    MSGHUB_UNLOCK_MGR();

    return MSGHUB_OK;
}

// Internal helper: trigger callbacks for all subscribers of a topic
// Must be called outside critical section
static void msghub_trigger_callbacks(uint8_t topic_idx, uint8_t instance, uint16_t generation)
{
    // Iterate all subscriber slots and trigger callbacks for matching subscribers
    for (int i = 0; i < MSGHUB_MAX_SUBSCRIBERS; i++) {
        msghub_sub_slot_t *sub_slot = &g_sub_slots[i];

        // Check if this subscriber is valid and subscribed to this topic
        if (sub_slot->magic == MSGHUB_SUBSCRIBER_MAGIC && sub_slot->topic_idx == topic_idx &&
            sub_slot->instance == instance && sub_slot->callback != NULL) {

            // Update subscriber's generation before calling callback
            sub_slot->last_generation = generation;

            // Save callback info (may change during callback execution)
            msghub_sub_callback_t cb = sub_slot->callback;
            void *ctx = sub_slot->callback_context;

            // Encode subscriber handle
            msghub_subscriber_t sub_handle;
            msghub_core_encode_sub_handle(&sub_handle, (uint8_t)i);

            // Call callback outside critical section
            cb(sub_handle, ctx);
        }
    }
}

// Publish data to topic (task context)
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

    // Critical section: protect against concurrent access from other tasks or ISRs
    uint16_t new_generation;
    MSGHUB_ENTER_CRITICAL();
    memcpy(inst->data, data, topic_state->topic->msg_size);
    inst->generation++;
    new_generation = inst->generation;
    MSGHUB_EXIT_CRITICAL();

    // Trigger callbacks for all subscribers (outside critical section)
    msghub_trigger_callbacks(slot->topic_idx, slot->instance, new_generation);

    return MSGHUB_OK;
}

// Publish data to topic from ISR context
// Note: Subscriber callbacks will also be called from ISR context!
//       Callbacks must use ISR-safe APIs (e.g., k_sem_give, k_msgq_put).
msghub_err_t msghub_publish_from_isr(msghub_publisher_t handle, const void *data)
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

    // Critical section: protect against concurrent access from tasks or other ISRs
    uint16_t new_generation;
    MSGHUB_ENTER_CRITICAL_ISR();
    memcpy(inst->data, data, topic_state->topic->msg_size);
    inst->generation++;
    new_generation = inst->generation;
    MSGHUB_EXIT_CRITICAL_ISR();

    // Trigger callbacks for all subscribers from ISR context
    // Callbacks must use ISR-safe APIs!
    msghub_trigger_callbacks(slot->topic_idx, slot->instance, new_generation);

    return MSGHUB_OK;
}

// ============================================================================
// Multi-instance support
// ============================================================================

msghub_publisher_t msghub_create_publisher_multi(msghub_topic_t topic, int *instance)
{
    if (topic == NULL || instance == NULL) {
        return MSGHUB_PUBLISHER_INVALID;
    }

    // Management path: use mutex lock
    MSGHUB_LOCK_MGR();

    // Allocate topic
    int8_t topic_idx = msghub_core_alloc_topic(topic);
    if (topic_idx < 0) {
        MSGHUB_UNLOCK_MGR();
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
            MSGHUB_UNLOCK_MGR();
            return MSGHUB_PUBLISHER_INVALID;
        }
    } else {
        // Use specified instance
        if ((uint8_t)*instance >= MSGHUB_MAX_INSTANCES) {
            MSGHUB_UNLOCK_MGR();
            return MSGHUB_PUBLISHER_INVALID;
        }
        if (g_topics[topic_idx].instances[*instance].allocated) {
            MSGHUB_UNLOCK_MGR();
            return MSGHUB_PUBLISHER_INVALID;
        }
        target_instance = (uint8_t)*instance;
    }

    // Allocate publisher slot
    int8_t slot_idx = msghub_core_alloc_pub_slot();
    if (slot_idx < 0) {
        MSGHUB_UNLOCK_MGR();
        return MSGHUB_PUBLISHER_INVALID;
    }

    // Initialize instance data (with critical section)
    MSGHUB_ENTER_CRITICAL();
    msghub_instance_t *inst = &g_topics[topic_idx].instances[target_instance];
    inst->allocated = true;
    MSGHUB_EXIT_CRITICAL();

    // Initialize publisher slot
    g_pub_slots[slot_idx].magic = MSGHUB_PUBLISHER_MAGIC;
    g_pub_slots[slot_idx].topic_idx = (uint8_t)topic_idx;
    g_pub_slots[slot_idx].instance = target_instance;

    *instance = target_instance;

    MSGHUB_UNLOCK_MGR();

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
