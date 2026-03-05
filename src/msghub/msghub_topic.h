#ifndef MSGHUB_TOPIC_H
#define MSGHUB_TOPIC_H

#include "msghub/msghub.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Topic management
// ============================================================================

// Find registered topic by topic handle. Returns topic index, -1 on failure.
int8_t msghub_core_find_topic(msghub_topic_t topic);

// Allocate new topic slot. Returns existing index if topic already exists.
int8_t msghub_core_alloc_topic(msghub_topic_t topic);

// ============================================================================
// Publisher slot management
// ============================================================================

// Allocate publisher slot. Returns slot index, -1 on failure.
int8_t msghub_core_alloc_pub_slot(void);

// Encode publisher handle from slot index
void msghub_core_encode_pub_handle(msghub_publisher_t *handle, uint8_t slot_idx);

// Decode publisher handle. Returns 0 success, -1 failure.
int8_t msghub_core_decode_pub_handle(msghub_publisher_t handle, uint8_t *slot_idx);

// ============================================================================
// Subscriber slot management
// ============================================================================

// Allocate subscriber slot. Returns slot index, -1 on failure.
int8_t msghub_core_alloc_sub_slot(void);

// Encode subscriber handle from slot index
void msghub_core_encode_sub_handle(msghub_subscriber_t *handle, uint8_t slot_idx);

// Decode subscriber handle. Returns 0 success, -1 failure.
int8_t msghub_core_decode_sub_handle(msghub_subscriber_t handle, uint8_t *slot_idx);

#ifdef __cplusplus
}
#endif

#endif // MSGHUB_TOPIC_H
