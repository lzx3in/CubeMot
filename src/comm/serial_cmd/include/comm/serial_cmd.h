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
 *   0x80  RSP_STATUS     → {state(u8), fault(u8)}  (2 bytes)
 *   0x81  RSP_TELEMETRY  → {x(f32), y(f32), yaw(f32), vx(f32), wz(f32)} (20 bytes)
 *   0x82  RSP_MOTOR      → {motor_id(u8), state(u8), speed(i16), vbus(u16)} (6 bytes)
 *
 * This module bridges UART ↔ msghub topics.
 * Future: ESP32 AT firmware can translate these frames to MQTT.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ── Protocol constants ──────────────────────────────── */

#define SERIAL_FRAME_HEAD_0     0xAA
#define SERIAL_FRAME_HEAD_1     0x55
#define SERIAL_MAX_PAYLOAD      32
#define SERIAL_MAX_FRAME        (2 + 1 + 1 + SERIAL_MAX_PAYLOAD + 1)

/* Command IDs (host → device) */
#define CMD_ID_VEL              0x01
#define CMD_ID_ARM              0x02
#define CMD_ID_DISARM           0x03
#define CMD_ID_ESTOP            0x04
#define CMD_ID_RESET            0x05
#define CMD_ID_PING             0x06

/* Response IDs (device → host) */
#define RSP_ID_STATUS           0x80
#define RSP_ID_TELEMETRY        0x81
#define RSP_ID_MOTOR            0x82
#define RSP_ID_PONG             0x86

/* Thread priorities */
#define SERIAL_RX_THREAD_PRIORITY  8
#define SERIAL_TX_THREAD_PRIORITY  8

/* ── API ─────────────────────────────────────────────── */

/**
 * @brief  Initialize serial command module
 *
 * Opens UART, subscribes to msghub topics for TX,
 * creates RX thread.
 *
 * @return 0 on success
 */
int serial_cmd_init(void);

/**
 * @brief  Serial command RX thread
 *
 * Never returns. Reads UART, parses frames,
 * publishes to msghub topics.
 */
void serial_cmd_rx_thread(void);

/**
 * @brief  Serial command TX thread
 *
 * Never returns. Subscribes to msghub topics,
 * encodes frames, writes to UART.
 */
void serial_cmd_tx_thread(void);

#ifdef __cplusplus
}
#endif
