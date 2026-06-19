#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * CRC8 (Polynomial 0x07)
 * ============================================================================ */

/**
 * @brief One-shot CRC8 calculation (polynomial 0x07)
 * @param data Data buffer
 * @param len Data length
 * @return CRC8 value
 */
uint8_t crc8_calculate(const uint8_t *data, size_t len);

/* ============================================================================
 * CRC16-CCITT
 * ============================================================================ */

/**
 * CRC16-CCITT context structure
 *
 * Context-based stateful CRC16 calculation.
 * Each context only needs to track its current state (2 bytes).
 */
typedef struct crc16_ctx {
    uint16_t state; /**< Current CRC state */
} crc16_ctx_t;

/**
 * @brief Initialize CRC16-CCITT context
 * @param ctx Pointer to context structure (must not be NULL)
 * @return 0 on success, -1 on error
 */
int crc16_ctx_init(crc16_ctx_t *ctx);

/**
 * @brief Reset CRC16-CCITT context to initial state
 * @param ctx Pointer to context structure
 *
 * Context can be reused without re-initializing.
 */
void crc16_ctx_reset(crc16_ctx_t *ctx);

/**
 * @brief Update CRC16-CCITT with new data
 * @param ctx Context structure
 * @param data Data buffer
 * @param len Data length in bytes
 */
void crc16_ctx_update(crc16_ctx_t *ctx, const uint8_t *data, size_t len);

/**
 * @brief Finalize CRC16 calculation and return result
 * @param ctx Context structure
 * @return Final CRC16 value
 * @note This resets the context to initial state for reuse
 */
uint16_t crc16_ctx_finalize(crc16_ctx_t *ctx);

/**
 * @brief One-shot CRC16-CCITT calculation
 * @param data Data buffer
 * @param len Data length
 * @return CRC16 value
 */
uint16_t crc16_calculate(const uint8_t *data, size_t len);

/* ============================================================================
 * CRC32 (IEEE 802.3)
 * ============================================================================ */

/**
 * CRC32 context structure
 *
 * Context-based stateful CRC32 calculation.
 * Each context only needs to track its current state (4 bytes).
 */
typedef struct crc32_ctx {
    uint32_t state; /**< Current CRC state */
} crc32_ctx_t;

/**
 * @brief Initialize CRC32 context
 * @param ctx Pointer to context structure (must not be NULL)
 * @return 0 on success, -1 on error
 */
int crc32_ctx_init(crc32_ctx_t *ctx);

/**
 * @brief Reset CRC32 context to initial state
 * @param ctx Pointer to context structure
 *
 * Context can be reused without re-initializing.
 */
void crc32_ctx_reset(crc32_ctx_t *ctx);

/**
 * @brief Update CRC32 with new data
 * @param ctx Context structure
 * @param data Data buffer
 * @param len Data length in bytes
 */
void crc32_ctx_update(crc32_ctx_t *ctx, const uint8_t *data, size_t len);

/**
 * @brief Finalize CRC32 calculation and return result
 * @param ctx Context structure
 * @return Final CRC32 value
 * @note This resets the context to initial state for reuse
 */
uint32_t crc32_ctx_finalize(crc32_ctx_t *ctx);

/**
 * @brief One-shot CRC32 calculation
 * @param data Data buffer
 * @param len Data length
 * @return CRC32 value
 */
uint32_t crc32_calculate(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif
