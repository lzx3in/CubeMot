#include "msghub/msghub.h"
#include "msghub_topic.h"
#include "msghub_state.h"
#include <string.h>

// ============================================================================
// Subscriber API
// ============================================================================

msghub_subscriber_t msghub_create_subscriber(msghub_topic_t topic, uint8_t instance)
{
    if (topic == NULL || instance >= MSGHUB_MAX_INSTANCES) {
        return MSGHUB_SUBSCRIBER_INVALID;
    }

    // Management path: use mutex lock
    MSGHUB_LOCK_MGR();

    // Find or allocate topic
    int8_t topic_idx = msghub_core_find_topic(topic);
    if (topic_idx < 0) {
        topic_idx = msghub_core_alloc_topic(topic);
        if (topic_idx < 0) {
            MSGHUB_UNLOCK_MGR();
            return MSGHUB_SUBSCRIBER_INVALID;
        }
    }

    // Allocate subscriber slot
    int8_t slot_idx = msghub_core_alloc_sub_slot();
    if (slot_idx < 0) {
        MSGHUB_UNLOCK_MGR();
        return MSGHUB_SUBSCRIBER_INVALID;
    }

    // Initialize subscriber slot
    g_sub_slots[slot_idx].magic = MSGHUB_SUBSCRIBER_MAGIC;
    g_sub_slots[slot_idx].topic_idx = (uint8_t)topic_idx;
    g_sub_slots[slot_idx].instance = instance;

    // Initialize generation tracking (with critical section for data access)
    MSGHUB_ENTER_CRITICAL();
    msghub_instance_t *inst = &g_topics[topic_idx].instances[instance];
    if (inst->allocated) {
        g_sub_slots[slot_idx].last_generation = inst->generation;
    } else {
        g_sub_slots[slot_idx].last_generation = 0;
    }
    MSGHUB_EXIT_CRITICAL();

    MSGHUB_UNLOCK_MGR();

    // Encode and return handle
    msghub_subscriber_t handle;
    msghub_core_encode_sub_handle(&handle, (uint8_t)slot_idx);
    return handle;
}

msghub_err_t msghub_destroy_subscriber(msghub_subscriber_t handle)
{
    uint8_t slot_idx;
    if (msghub_core_decode_sub_handle(handle, &slot_idx) < 0) {
        return MSGHUB_ERR_INVALID;
    }

    // Management path: use mutex lock
    MSGHUB_LOCK_MGR();
    g_sub_slots[slot_idx].magic = 0;
    MSGHUB_UNLOCK_MGR();

    return MSGHUB_OK;
}

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

    // Validate topic exists
    if (topic_state->topic == NULL) {
        return MSGHUB_ERR_NOT_FOUND;
    }

    msghub_instance_t *inst = &topic_state->instances[slot->instance];

    // Data path: use critical section for atomic read
    MSGHUB_ENTER_CRITICAL();
    memcpy(buffer, inst->data, topic_state->topic->msg_size);
    slot->last_generation = inst->generation;
    MSGHUB_EXIT_CRITICAL();

    return MSGHUB_OK;
}

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

    // Validate topic exists
    if (topic_state->topic == NULL) {
        return MSGHUB_ERR_NOT_FOUND;
    }

    msghub_instance_t *inst = &topic_state->instances[slot->instance];

    // Data path: use critical section for atomic generation check
    MSGHUB_ENTER_CRITICAL();
    *updated = (inst->generation != slot->last_generation);
    MSGHUB_EXIT_CRITICAL();

    return MSGHUB_OK;
}

// ============================================================================
// Advanced features
// ============================================================================

uint16_t msghub_get_generation(msghub_subscriber_t handle)
{
    uint8_t slot_idx;
    if (msghub_core_decode_sub_handle(handle, &slot_idx) < 0) {
        return 0;
    }

    msghub_sub_slot_t *slot = &g_sub_slots[slot_idx];
    msghub_topic_state_t *topic_state = &g_topics[slot->topic_idx];
    msghub_instance_t *inst = &topic_state->instances[slot->instance];

    uint16_t generation;
    MSGHUB_ENTER_CRITICAL();
    generation = inst->generation;
    MSGHUB_EXIT_CRITICAL();

    return generation;
}

msghub_err_t msghub_subscriber_poll(msghub_subscriber_t handle, uint32_t timeout_ms, bool *updated)
{
    if (updated == NULL) {
        return MSGHUB_ERR_INVALID;
    }

    // Timeout 0: degrade to check
    if (timeout_ms == 0) {
        return msghub_subscriber_check(handle, updated);
    }

    // Use FreeRTOS/RT-Thread for blocking wait
    uint32_t elapsed = 0;
    while (elapsed < timeout_ms) {
        msghub_err_t err = msghub_subscriber_check(handle, updated);
        if (err != MSGHUB_OK) {
            return err;
        }
        if (*updated) {
            return MSGHUB_OK;
        }
        // TODO: Use RTOS-specific wait mechanism instead of polling
#if defined(FREERTOS_ENV)
        vTaskDelay(pdMS_TO_TICKS(1));
#elif defined(RTTHREAD_ENV)
        rt_thread_mdelay(1);
#else
        // PC/bare-metal: busy wait (not recommended for long timeouts)
        for (volatile int i = 0; i < 100000; i++)
            ;
#endif
        elapsed++;
    }

    *updated = false;
    return MSGHUB_ERR_TIMEOUT;
}

msghub_err_t msghub_subscriber_update(msghub_subscriber_t handle, void *buffer, bool *updated)
{
    if (buffer == NULL || updated == NULL) {
        return MSGHUB_ERR_INVALID;
    }

    uint8_t slot_idx;
    if (msghub_core_decode_sub_handle(handle, &slot_idx) < 0) {
        return MSGHUB_ERR_INVALID;
    }

    msghub_sub_slot_t *slot = &g_sub_slots[slot_idx];
    msghub_topic_state_t *topic_state = &g_topics[slot->topic_idx];

    if (topic_state->topic == NULL) {
        return MSGHUB_ERR_NOT_FOUND;
    }

    msghub_instance_t *inst = &topic_state->instances[slot->instance];

    // Data path: CRITICAL - entire check+receive must be atomic
    MSGHUB_ENTER_CRITICAL();
    *updated = (inst->generation != slot->last_generation);
    if (*updated) {
        memcpy(buffer, inst->data, topic_state->topic->msg_size);
        slot->last_generation = inst->generation;
    }
    MSGHUB_EXIT_CRITICAL();

    return MSGHUB_OK;
}

// ============================================================================
// Utility functions
// ============================================================================

bool msghub_subscriber_valid(msghub_subscriber_t handle)
{
    uint8_t slot_idx;
    return msghub_core_decode_sub_handle(handle, &slot_idx) == 0;
}
