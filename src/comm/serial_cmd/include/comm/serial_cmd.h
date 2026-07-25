#pragma once

/**
 * @file serial_cmd.h
 * @brief Serial command protocol: UART bridge for remote control
 *
 * Frame format:
 *   [0xAA 0x55] [CMD_ID] [LEN] [PAYLOAD...] [CRC8]
 *
 * CMD_IDs:
 *   0x01  CMD_VEL        → {linear_x(f32), angular_z(f32)}  (8 bytes)
 *   0x02  CMD_ARM        → {} (0 bytes)
 *   0x03  CMD_DISARM     → {} (0 bytes)
 *   0x04  CMD_ESTOP      → {} (0 bytes)
 *   0x05  CMD_RESET      → {} (0 bytes)
 *   0x06  CMD_PING       → {} (0 bytes)
 *   0x07  CMD_MOTOR_START→ {motor_id(u8), speed_rpm(f32)} (5 bytes)
 *   0x08  CMD_MOTOR_STOP → {motor_id(u8)} (1 byte)
 *   0x10  CMD_TEST       → FOC test command
 *   0x80  RSP_STATUS     → {state(u8), fault(u8)}  (2 bytes)
 *   0x81  RSP_TELEMETRY  → {x(f32), y(f32), yaw(f32), vx(f32), wz(f32)} (20 bytes)
 *   0x82  RSP_MOTOR      → {motor_id(u8), state(u8), speed(i16), vbus(u16), id(f32), iq(f32)} (14 bytes)
 *
 * This module bridges UART ↔ msghub topics.
 * Future: ESP32 AT firmware can translate these frames to MQTT.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Protocol constants are defined in serial_protocol.h (SP_*) */

/* Command IDs (host → device) */
#define CMD_ID_VEL              0x01
#define CMD_ID_ARM              0x02
#define CMD_ID_DISARM           0x03
#define CMD_ID_ESTOP            0x04
#define CMD_ID_RESET            0x05
#define CMD_ID_PING             0x06
#define CMD_ID_MOTOR_START      0x07  /* Start motor startup sequence */
#define CMD_ID_MOTOR_STOP       0x08  /* Stop motor */
#define CMD_ID_TEST             0x10  /* FOC test command */

/* Response IDs (device → host) */
#define RSP_ID_STATUS           0x80
#define RSP_ID_TELEMETRY        0x81
#define RSP_ID_MOTOR            0x82
#define RSP_ID_PONG             0x86
#define RSP_ID_DIAG             0x87  /* FOC debug diagnostics */

/* Thread priority */
#define SERIAL_CMD_THREAD_PRIORITY  K_PRIO_COOP(8)

/* ── API ────────────────────────────────────────────── */

/**
 * @brief  Initialize serial command module
 *
 * Opens UART, subscribes to msghub topics for TX,
 * enables IRQ-driven RX.
 *
 * @return 0 on success
 */
int serial_cmd_init(void);

/**
 * @brief  Serial command thread (merged RX + TX)
 *
 * Never returns. Each iteration:
 *   1. Waits on rx_sem (10 ms timeout) for ISR-signaled RX data
 *   2. Parses all available frames and dispatches commands
 *   3. Sends periodic telemetry (status / vehicle / motor / diag)
 *
 * @param  arg1  Unused
 * @param  arg2  Unused
 * @param  arg3  Unused
 */
void serial_cmd_thread(void *arg1, void *arg2, void *arg3);

#ifdef __cplusplus
}
#endif
