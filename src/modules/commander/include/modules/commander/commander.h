#pragma once

/**
 * @file commander.h
 * @brief Commander module: system state machine + command routing
 *
 * Architecture (PX4-inspired):
 *   - Manages system-wide state: INIT → STANDBY → ARMED → ACTIVE
 *   - Routes cmd_vel → vehicle/motor commands
 *   - Handles emergency stop (high priority)
 *   - Publishes commander_status at 10Hz
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* ── Commander States ────────────────────────────────── */

typedef enum {
    CMD_STATE_INIT = 0,
    CMD_STATE_STANDBY,
    CMD_STATE_ARMED,
    CMD_STATE_ACTIVE,
    CMD_STATE_FAULT
} cmd_state_t;

/* ── Fault Codes ─────────────────────────────────────── */

typedef enum {
    CMD_FAULT_NONE = 0,
    CMD_FAULT_MOTOR_FAIL = 1,
    CMD_FAULT_COMM_LOSS = 2,
    CMD_FAULT_EMERGENCY = 3,
    CMD_FAULT_OVERCURRENT = 4,
} cmd_fault_t;

/* ── API ─────────────────────────────────────────────── */

/**
 * @brief  Initialize commander module
 *
 * Subscribes to cmd_vel, commander_cmd topics.
 * Creates commander thread (100Hz).
 *
 * @return 0 on success
 */
int commander_init(void);

/**
 * @brief  Commander thread entry point
 *
 * Never returns. Runs at 100Hz:
 *   1. Check incoming commands
 *   2. Update state machine
 *   3. Route cmd_vel to vehicle/motor
 *   4. Publish commander_status
 *
 * @param  arg1  Unused
 * @param  arg2  Unused
 * @param  arg3  Unused
 */
void commander_thread(void *arg1, void *arg2, void *arg3);

/**
 * @brief  Get current commander state
 * @return Current state enum
 */
cmd_state_t commander_get_state(void);

#ifdef __cplusplus
}
#endif
