#include "crc.h"

/* ============================================================================
 * CRC16-CCITT Implementation (Direct Calculation)
 * ============================================================================
 * Polynomial: x^16 + x^12 + x^5 + 1
 * Represented as reversed polynomial: 0x8408
 */

#define CRC16_POLYNOMIAL 0x8408u
#define CRC16_INITIAL 0xFFFFu
#define CRC16_FINAL_XOR 0x0000u

/**
 * @brief Process single byte for CRC16 (bit-by-bit calculation)
 */
static uint16_t crc16_byte(uint16_t crc, uint8_t byte)
{
    crc ^= byte;
    for (int i = 0; i < 8; i++) {
        if (crc & 0x0001u) {
            crc = (crc >> 1u) ^ CRC16_POLYNOMIAL;
        } else {
            crc >>= 1u;
        }
    }
    return crc;
}

int crc16_ctx_init(crc16_ctx_t *ctx)
{
    if (ctx == NULL) {
        return -1;
    }

    ctx->state = CRC16_INITIAL;
    return 0;
}

void crc16_ctx_reset(crc16_ctx_t *ctx)
{
    if (ctx != NULL) {
        ctx->state = CRC16_INITIAL;
    }
}

void crc16_ctx_update(crc16_ctx_t *ctx, const uint8_t *data, size_t len)
{
    if ((ctx == NULL) || (data == NULL) || (len == 0)) {
        return;
    }

    uint16_t crc = ctx->state;

    for (size_t i = 0; i < len; i++) {
        crc = crc16_byte(crc, data[i]);
    }

    ctx->state = crc;
}

uint16_t crc16_ctx_finalize(crc16_ctx_t *ctx)
{
    if (ctx == NULL) {
        return 0;
    }

    uint16_t result = ctx->state ^ CRC16_FINAL_XOR;
    ctx->state = CRC16_INITIAL;
    return result;
}

uint16_t crc16_calculate(const uint8_t *data, size_t len)
{
    if (data == NULL) {
        return 0;
    }

    crc16_ctx_t ctx;

    if (crc16_ctx_init(&ctx) != 0) {
        return 0;
    }

    crc16_ctx_update(&ctx, data, len);
    return crc16_ctx_finalize(&ctx);
}

/* ============================================================================
 * CRC32 (IEEE 802.3) Implementation (Direct Calculation)
 * ============================================================================
 * Polynomial: x^32 + x^26 + x^23 + x^22 + x^16 + x^12 + x^11 + x^10 +
 *             x^8 + x^7 + x^5 + x^4 + x^2 + x + 1
 * Represented as reversed polynomial: 0xEDB88320
 */

#define CRC32_POLYNOMIAL 0xEDB88320u
#define CRC32_INITIAL 0xFFFFFFFFu
#define CRC32_FINAL_XOR 0xFFFFFFFFu

/**
 * @brief Process single byte for CRC32 (bit-by-bit calculation)
 */
static uint32_t crc32_byte(uint32_t crc, uint8_t byte)
{
    crc ^= byte;
    for (int i = 0; i < 8; i++) {
        if (crc & 0x00000001u) {
            crc = (crc >> 1u) ^ CRC32_POLYNOMIAL;
        } else {
            crc >>= 1u;
        }
    }
    return crc;
}

int crc32_ctx_init(crc32_ctx_t *ctx)
{
    if (ctx == NULL) {
        return -1;
    }

    ctx->state = CRC32_INITIAL;

    return 0;
}

void crc32_ctx_reset(crc32_ctx_t *ctx)
{
    if (ctx != NULL) {
        ctx->state = CRC32_INITIAL;
    }
}

void crc32_ctx_update(crc32_ctx_t *ctx, const uint8_t *data, size_t len)
{
    if ((ctx == NULL) || (data == NULL) || (len == 0)) {
        return;
    }

    uint32_t crc = ctx->state;

    for (size_t i = 0; i < len; i++) {
        crc = crc32_byte(crc, data[i]);
    }

    ctx->state = crc;
}

uint32_t crc32_ctx_finalize(crc32_ctx_t *ctx)
{
    if (ctx == NULL) {
        return 0;
    }

    uint32_t result = ctx->state ^ CRC32_FINAL_XOR;
    ctx->state = CRC32_INITIAL;

    return result;
}

uint32_t crc32_calculate(const uint8_t *data, size_t len)
{
    if (data == NULL) {
        return 0;
    }

    crc32_ctx_t ctx;

    if (crc32_ctx_init(&ctx) != 0) {
        return 0;
    }

    crc32_ctx_update(&ctx, data, len);
    return crc32_ctx_finalize(&ctx);
}
