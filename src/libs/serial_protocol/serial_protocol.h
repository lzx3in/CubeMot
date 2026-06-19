#pragma once

/**
 * @file serial_protocol.h
 * @brief Serial protocol frame codec (hardware-independent)
 *
 * Frame format: [0xAA 0x55] [CMD_ID] [LEN] [PAYLOAD...] [CRC8]
 *
 * This library provides pure encode/Decode functions for the serial
 * protocol frames, with no hardware dependencies.
 */

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Protocol constants ──────────────────────────────── */

#define SP_FRAME_HEAD_0     0xAA
#define SP_FRAME_HEAD_1     0x55
#define SP_MAX_PAYLOAD      32
#define SP_MAX_FRAME        (2 + 1 + 1 + SP_MAX_PAYLOAD + 1)  /* 37 */
#define SP_HEADER_SIZE      2
#define SP_OVERHEAD         (SP_HEADER_SIZE + 1 + 1 + 1)       /* head + id + len + crc = 5 */

/* ── Parsed frame structure ──────────────────────────── */

typedef struct {
    uint8_t cmd_id;
    uint8_t len;
    uint8_t payload[SP_MAX_PAYLOAD];
} sp_frame_t;

/* ── API ─────────────────────────────────────────────── */

/**
 * @brief Encode a frame into a buffer
 *
 * Writes: [0xAA 0x55] [cmd_id] [len] [payload...] [crc8]
 *
 * @param buf       Output buffer (must be >= SP_MAX_FRAME bytes)
 * @param cmd_id    Command/response ID
 * @param payload   Payload data (may be NULL if len == 0)
 * @param len       Payload length (0..SP_MAX_PAYLOAD)
 * @return Total frame length in bytes, or 0 on error
 */
size_t sp_encode_frame(uint8_t *buf, uint8_t cmd_id, const uint8_t *payload, uint8_t len);

/**
 * @brief Decode a frame from a buffer
 *
 * Validates header bytes and CRC8 checksum.
 *
 * @param buf       Input buffer containing raw frame bytes
 * @param buf_len   Number of bytes available in buf
 * @param frame     Output parsed frame structure
 * @return Number of bytes consumed on success (>= 5), or -1 on error
 */
int sp_decode_frame(const uint8_t *buf, size_t buf_len, sp_frame_t *frame);

/**
 * @brief Compute CRC8 over a byte range
 *
 * Exposed for testing; prefer sp_encode_frame/sp_decode_frame
 * for normal protocol use.
 *
 * @param data  Data buffer
 * @param len   Data length
 * @return CRC8 value
 */
uint8_t sp_crc8(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif
