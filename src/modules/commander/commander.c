/**
 * @file commander.c
 * @brief Commander module: PX4-inspired state machine + command routing
 *
 * State Machine:
 *   INIT → STANDBY → ARMED → ACTIVE
 *              ↑         ↓
 *              └─ FAULT ←┘
 *
 * Responsibilities:
 *   - System state management
 *   - Route cmd_vel → vehicle/motor
 *   - Emergency stop handling
 *   - Fault detection and recovery
 */

#include "modules/commander/commander.h"
#include "topics/topics.h"
#include "common_time.h"
#include "common_error.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(commander, LOG_LEVEL_INF);

/* ── Configuration ───────────────────────────────────── */

#define COMMANDER_HZ        100
#define COMMANDER_PERIOD    K_MSEC(10)
#define STATUS_PUB_HZ       10
#define STATUS_PUB_DIV      (COMMANDER_HZ / STATUS_PUB_HZ)

/* ── State ───────────────────────────────────────────── */

static cmd_state_t g_state = CMD_STATE_INIT;
static cmd_fault_t g_fault = CMD_FAULT_NONE;
static uint32_t g_last_cmd_time_ms = 0;

/* ── msghub ──────────────────────────────────────────── */

static msghub_subscriber_t g_cmd_vel_sub;
static msghub_subscriber_t g_cmd_sub;
static msghub_publisher_t g_status_pub;
static msghub_publisher_t g_motor_cmd_pub;

/* ── Timeout ─────────────────────────────────────────── */

#define CMD_TIMEOUT_MS      500  // 500ms without cmd_vel → stop

/* ── State transitions ───────────────────────────────── */

static void transition_to(cmd_state_t new_state)
{
    if (g_state != new_state) {
        LOG_INF("State: %d → %d", g_state, new_state);
        g_state = new_state;
    }
}

static void set_fault(cmd_fault_t fault)
{
    g_fault = fault;
    transition_to(CMD_STATE_FAULT);
    LOG_ERR("Fault: %d", fault);
}

static void clear_fault(void)
{
    if (g_state == CMD_STATE_FAULT) {
        g_fault = CMD_FAULT_NONE;
        transition_to(CMD_STATE_STANDBY);
        LOG_INF("Fault cleared → STANDBY");
    }
}

/* ── Motor command routing ───────────────────────────── */

static void send_motor_cmd(uint8_t motor_id, motor_cmd_type_t type, float speed_rpm)
{
    motor_cmd_t cmd = {
        .motor_id = motor_id,
        .cmd = type,
        .target_speed_rpm = speed_rpm,
        .ramp_time_ms = 0.0f,
    };
    msghub_publish(g_motor_cmd_pub, &cmd);
}

/* ── cmd_vel processing ──────────────────────────────── */

/**
 * @brief  Process incoming cmd_vel command
 *
 * Commander's role is state gating and timeout tracking only.
 * Actual kinematics are handled by the Vehicle module.
 * Vehicle checks commander_get_state() before processing.
 */
static void process_cmd_vel(const cmd_vel_t *vel)
{
    (void)vel;  // Content handled by Vehicle module
    
    // Track last command time for timeout detection
    g_last_cmd_time_ms = common_get_timestamp_ms();
}

/* ── Commander command processing ────────────────────── */

static void process_commander_cmd(const commander_cmd_t *cmd)
{
    switch (cmd->op) {
    case CMD_OP_ARM:
        if (g_state == CMD_STATE_STANDBY) {
            transition_to(CMD_STATE_ARMED);
            LOG_INF("System ARMED");
        }
        break;
        
    case CMD_OP_DISARM:
        if (g_state == CMD_STATE_ARMED || g_state == CMD_STATE_ACTIVE) {
            // Stop all motors
            send_motor_cmd(0, MOTOR_CMD_STOP, 0.0f);
            send_motor_cmd(1, MOTOR_CMD_STOP, 0.0f);
            transition_to(CMD_STATE_STANDBY);
            LOG_INF("System DISARMED");
        }
        break;
        
    case CMD_OP_ESTOP:
        // Emergency stop - immediate
        send_motor_cmd(0, MOTOR_CMD_EMERGENCY, 0.0f);
        send_motor_cmd(1, MOTOR_CMD_EMERGENCY, 0.0f);
        set_fault(CMD_FAULT_EMERGENCY);
        LOG_ERR("EMERGENCY STOP");
        break;
        
    case CMD_OP_RESET_FAULT:
        clear_fault();
        break;
        
    default:
        break;
    }
}

/* ── Init ────────────────────────────────────────────── */

int commander_init(void)
{
    g_cmd_vel_sub = msghub_create_subscriber(MSGHUB_TOPIC(cmd_vel), 0);
    g_cmd_sub = msghub_create_subscriber(MSGHUB_TOPIC(commander_cmd), 0);
    g_status_pub = msghub_create_publisher(MSGHUB_TOPIC(commander_status));
    g_motor_cmd_pub = msghub_create_publisher(MSGHUB_TOPIC(motor_cmd));
    
    g_state = CMD_STATE_INIT;
    g_fault = CMD_FAULT_NONE;
    g_last_cmd_time_ms = common_get_timestamp_ms();
    
    LOG_INF("Commander initialized");
    return 0;
}

/* ── Main thread ─────────────────────────────────────── */

void commander_thread(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    /* Startup delay: allow msghub topics and downstream modules to initialize */
    k_sleep(K_MSEC(100));
    
    transition_to(CMD_STATE_STANDBY);
    LOG_INF("Commander thread started");
    
    uint32_t pub_counter = 0;
    
    while (1) {
        /* ── Check cmd_vel ───────────────────────────── */
        cmd_vel_t vel;
        bool updated = false;
        msghub_subscriber_update(g_cmd_vel_sub, &vel, &updated);
        if (updated) {
            process_cmd_vel(&vel);
        }
        
        /* ── Check commander commands ────────────────── */
        commander_cmd_t cmd;
        updated = false;
        msghub_subscriber_update(g_cmd_sub, &cmd, &updated);
        if (updated) {
            process_commander_cmd(&cmd);
        }
        
        /* ── Timeout check (ACTIVE state) ────────────── */
        if (g_state == CMD_STATE_ACTIVE) {
            uint32_t now = common_get_timestamp_ms();
            if (now - g_last_cmd_time_ms > CMD_TIMEOUT_MS) {
                // No command for 500ms → stop motors
                send_motor_cmd(0, MOTOR_CMD_STOP, 0.0f);
                send_motor_cmd(1, MOTOR_CMD_STOP, 0.0f);
                transition_to(CMD_STATE_ARMED);
                LOG_WRN("Command timeout → ARMED");
            }
        }
        
        /* ── Publish status @ 10Hz ───────────────────── */
        if (++pub_counter >= STATUS_PUB_DIV) {
            pub_counter = 0;
            
            commander_status_t status = {
                .state = g_state,
                .fault_code = (uint16_t)g_fault,
                .timestamp = common_get_timestamp_ms(),
            };
            msghub_publish(g_status_pub, &status);
        }
        
        k_sleep(COMMANDER_PERIOD);
    }
}

/* ── Getter ──────────────────────────────────────────── */

cmd_state_t commander_get_state(void)
{
    return g_state;
}
