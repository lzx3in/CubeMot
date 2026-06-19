/**
 * @file serial_protocol.c
 * @brief Serial protocol frame codec implementation
 */

#include "serial_protocol.h"
#include "crc.h"
#include <string.h>

/* ── CRC8 wrapper ────────────────────────────────────── */

uint8_t sp_crc8(const uint8_t *data, size_t len)
{
    return crc8_calculate(data, len);
}

/* ── Frame encoding ──────────────────────────────────── */

size_t sp_encode_frame(uint8_t *buf, uint8_t cmd_id, const uint8_t *payload, uint8_t len)
{
    if (buf == NULL) {
        return 0;
    }
    if (len > SP_MAX_PAYLOAD) {
        return 0;
    }
    if (len > 0 && payload == NULL) {
        return 0;
    }

    buf[0] = SP_FRAME_HEAD_0;
    buf[1] = SP_FRAME_HEAD_1;
    buf[2] = cmd_id;
    buf[3] = len;

    if (len > 0) {
        memcpy(&buf[4], payload, len);
    }

    /* CRC8 over [cmd_id, len, payload...] */
    buf[4 + len] = crc8_calculate(&buf[2], 2 + len);

    return 4 + len + 1;
}

/* ── Frame decoding ──────────────────────────────────── */

int sp_decode_frame(const uint8_t *buf, size_t buf_len, sp_frame_t *frame)
{
    if (buf == NULL || frame == NULL) {
        return -1;
    }

    /* Minimum frame: head(2) + cmd_id(1) + len(1) + crc(1) = 5 */
    if (buf_len < 5) {
        return -1;
    }

    /* Validate header */
    if (buf[0] != SP_FRAME_HEAD_0 || buf[1] != SP_FRAME_HEAD_1) {
        return -1;
    }

    frame->cmd_id = buf[2];
    frame->len = buf[3];

    if (frame->len > SP_MAX_PAYLOAD) {
        return -1;
    }

    size_t frame_len = 4 + frame->len + 1;
    if (buf_len < frame_len) {
        return -1;
    }

    /* Copy payload */
    if (frame->len > 0) {
        memcpy(frame->payload, &buf[4], frame->len);
    }

    /* Verify CRC8 */
    uint8_t crc_buf[2 + SP_MAX_PAYLOAD];
    crc_buf[0] = frame->cmd_id;
    crc_buf[1] = frame->len;
    memcpy(&crc_buf[2], frame->payload, frame->len);

    uint8_t expected_crc = crc8_calculate(crc_buf, 2 + frame->len);
    uint8_t actual_crc = buf[4 + frame->len];

    if (expected_crc != actual_crc) {
        return -1;
    }

    return (int)frame_len;
}
