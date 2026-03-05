#ifndef MSGHUB_STATE_H
#define MSGHUB_STATE_H

#include "msghub/msghub.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Configuration parameters
// ============================================================================

#ifndef MSGHUB_MAX_TOPICS
#define MSGHUB_MAX_TOPICS 8
#endif

#ifndef MSGHUB_MAX_INSTANCES
#define MSGHUB_MAX_INSTANCES 4
#endif

#ifndef MSGHUB_MAX_SUBSCRIBERS
#define MSGHUB_MAX_SUBSCRIBERS 16
#endif

// ============================================================================
// Handle encoding magic numbers
// ============================================================================

#define MSGHUB_PUBLISHER_MAGIC 0xA5
#define MSGHUB_SUBSCRIBER_MAGIC 0x5A

// ============================================================================
// Internal data structures
// ============================================================================

// Topic instance (each topic can have multiple instances)
typedef struct {
    uint8_t advertised;  // Is advertised
    uint16_t generation; // Data generation counter
    uint8_t data[128];   // Data buffer
} msghub_instance_t;

// Topic runtime state (internal use only)
typedef struct {
    msghub_topic_t topic;                              // Pointer to topic definition
    msghub_instance_t instances[MSGHUB_MAX_INSTANCES]; // Multi-instance data
} msghub_topic_state_t;

// Publisher handle slot
typedef struct {
    uint8_t magic;     // Magic number validation
    uint8_t topic_idx; // Topic index
    uint8_t instance;  // Instance number
} msghub_pub_slot_t;

// Subscriber handle slot
typedef struct {
    uint8_t magic;            // Magic number validation
    uint8_t topic_idx;        // Topic index
    uint8_t instance;         // Instance number
    uint16_t last_generation; // Last read generation
} msghub_sub_slot_t;

// ============================================================================
// Global state declarations (defined in msghub_state.c)
// ============================================================================

extern msghub_topic_state_t g_topics[MSGHUB_MAX_TOPICS];
extern msghub_pub_slot_t g_pub_slots[MSGHUB_MAX_TOPICS * MSGHUB_MAX_INSTANCES];
extern msghub_sub_slot_t g_sub_slots[MSGHUB_MAX_SUBSCRIBERS];
extern uint8_t g_num_topics;

// ============================================================================
// Test support functions (for testing only)
// ============================================================================

void msghub_reset(void);

#ifdef __cplusplus
}
#endif

#endif // MSGHUB_STATE_H
