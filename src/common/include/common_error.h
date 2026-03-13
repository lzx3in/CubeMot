#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// CubeMot error code type. Negative values indicate errors, zero indicates success.
typedef int32_t cubemot_err_t;

// Enumeration of module categories for error classification
#define CUBEMOT_MODULE_COMMON 0
#define CUBEMOT_MODULE_MSGHUB 1
#define CUBEMOT_MODULE_DRIVER 2

/**
 * Define an error code with compile-time range checking.
 * Error codes are negative integers encoded as: -(module_id << 16 | sub_err_code)
 * @param err_name Error symbol name
 * @param module_id Module identifier (upper 16 bits)
 * @param sub_err_code Sub-error code within module (lower 16 bits, 0-0xFFFF)
 */
#ifdef __cplusplus
#define CUBEMOT_DEF_ERR(err_name, module_id, sub_err_code)                                                             \
    static_assert((sub_err_code) >= 0 && (sub_err_code) <= 0xFFFF,                                                     \
                  #err_name "(" #sub_err_code ") out of range [0, 0xFFFF]");                                           \
    enum {                                                                                                             \
        err_name = (-(int32_t)(((module_id) << 16) | ((sub_err_code) & 0xFFFF)))                                       \
    }
#else
#define CUBEMOT_DEF_ERR(err_name, module_id, sub_err_code)                                                             \
    _Static_assert((sub_err_code) >= 0 && (sub_err_code) <= 0xFFFF,                                                    \
                   #err_name "(" #sub_err_code ") out of range [0, 0xFFFF]");                                          \
    enum {                                                                                                             \
        err_name = (-(int32_t)(((module_id) << 16) | ((sub_err_code) & 0xFFFF)))                                       \
    }
#endif

// Common module error codes
CUBEMOT_DEF_ERR(CUBEMOT_OK, CUBEMOT_MODULE_COMMON, 0);              // Success
CUBEMOT_DEF_ERR(CUBEMOT_ERR, CUBEMOT_MODULE_COMMON, 1);             // Generic error
CUBEMOT_DEF_ERR(CUBEMOT_ERR_INVALID, CUBEMOT_MODULE_COMMON, 2);     // Invalid argument or state
CUBEMOT_DEF_ERR(CUBEMOT_ERR_NOT_FOUND, CUBEMOT_MODULE_COMMON, 3);   // Resource not found
CUBEMOT_DEF_ERR(CUBEMOT_ERR_NO_MEM, CUBEMOT_MODULE_COMMON, 4);      // Out of memory
CUBEMOT_DEF_ERR(CUBEMOT_ERR_TIMEOUT, CUBEMOT_MODULE_COMMON, 5);     // Operation timed out
CUBEMOT_DEF_ERR(CUBEMOT_ERR_BUSY, CUBEMOT_MODULE_COMMON, 6);        // Resource busy
CUBEMOT_DEF_ERR(CUBEMOT_ERR_NOT_INIT, CUBEMOT_MODULE_COMMON, 7);    // Not initialized
CUBEMOT_DEF_ERR(CUBEMOT_ERR_UNSUPPORTED, CUBEMOT_MODULE_COMMON, 8); // Operation unsupported
CUBEMOT_DEF_ERR(CUBEMOT_ERR_IO, CUBEMOT_MODULE_COMMON, 9);          // I/O error
CUBEMOT_DEF_ERR(CUBEMOT_ERR_HARDWARE, CUBEMOT_MODULE_COMMON, 10);   // Hardware error
CUBEMOT_DEF_ERR(CUBEMOT_ERR_NO_DATA, CUBEMOT_MODULE_COMMON, 11);    // No data available
CUBEMOT_DEF_ERR(CUBEMOT_ERR_OVERFLOW, CUBEMOT_MODULE_COMMON, 12);   // Buffer overflow
CUBEMOT_DEF_ERR(CUBEMOT_ERR_UNKNOWN, CUBEMOT_MODULE_COMMON, 13);    // Unknown error

// MSGHUB module error codes
CUBEMOT_DEF_ERR(CUBEMOT_MSGHUB_OK, CUBEMOT_MODULE_MSGHUB, 0);            // Success
CUBEMOT_DEF_ERR(CUBEMOT_MSGHUB_ERR, CUBEMOT_MODULE_MSGHUB, 1);           // Generic error
CUBEMOT_DEF_ERR(CUBEMOT_MSGHUB_ERR_INVALID, CUBEMOT_MODULE_MSGHUB, 2);   // Invalid argument or handle
CUBEMOT_DEF_ERR(CUBEMOT_MSGHUB_ERR_NOT_FOUND, CUBEMOT_MODULE_MSGHUB, 3); // Topic or resource not found
CUBEMOT_DEF_ERR(CUBEMOT_MSGHUB_ERR_NO_MEM, CUBEMOT_MODULE_MSGHUB, 4);    // Out of memory (slot exhausted)
CUBEMOT_DEF_ERR(CUBEMOT_MSGHUB_ERR_TIMEOUT, CUBEMOT_MODULE_MSGHUB, 5);   // Operation timed out

// DRIVER module error codes
CUBEMOT_DEF_ERR(CUBEMOT_DRIVER_OK, CUBEMOT_MODULE_DRIVER, 0);              // Success
CUBEMOT_DEF_ERR(CUBEMOT_DRIVER_ERR, CUBEMOT_MODULE_DRIVER, 1);             // Generic error
CUBEMOT_DEF_ERR(CUBEMOT_DRIVER_ERR_NOT_INIT, CUBEMOT_MODULE_DRIVER, 2);    // Driver not initialized
CUBEMOT_DEF_ERR(CUBEMOT_DRIVER_ERR_INVALID, CUBEMOT_MODULE_DRIVER, 3);     // Invalid argument or state
CUBEMOT_DEF_ERR(CUBEMOT_DRIVER_ERR_NO_MEM, CUBEMOT_MODULE_DRIVER, 4);      // Out of memory
CUBEMOT_DEF_ERR(CUBEMOT_DRIVER_ERR_BUSY, CUBEMOT_MODULE_DRIVER, 5);        // Resource busy
CUBEMOT_DEF_ERR(CUBEMOT_DRIVER_ERR_UNSUPPORTED, CUBEMOT_MODULE_DRIVER, 6); // Operation unsupported
CUBEMOT_DEF_ERR(CUBEMOT_DRIVER_ERR_HARDWARE, CUBEMOT_MODULE_DRIVER, 7);    // Hardware error

// Button driver error codes (DRIVER module sub-range: 0x100-0x1FF)
CUBEMOT_DEF_ERR(CUBEMOT_DRIVER_BUTTON_OK, CUBEMOT_MODULE_DRIVER, 0x100);               // Success
CUBEMOT_DEF_ERR(CUBEMOT_DRIVER_BUTTON_ERR, CUBEMOT_MODULE_DRIVER, 0x101);              // Generic error
CUBEMOT_DEF_ERR(CUBEMOT_DRIVER_BUTTON_ERR_NOT_INIT, CUBEMOT_MODULE_DRIVER, 0x102);     // Driver not initialized
CUBEMOT_DEF_ERR(CUBEMOT_DRIVER_BUTTON_ERR_INVALID, CUBEMOT_MODULE_DRIVER, 0x103);      // Invalid button ID or argument
CUBEMOT_DEF_ERR(CUBEMOT_DRIVER_BUTTON_ERR_NO_MEM, CUBEMOT_MODULE_DRIVER, 0x104);       // Publisher creation failed
CUBEMOT_DEF_ERR(CUBEMOT_DRIVER_BUTTON_ERR_HAL, CUBEMOT_MODULE_DRIVER, 0x105);          // HAL layer error
CUBEMOT_DEF_ERR(CUBEMOT_DRIVER_BUTTON_ERR_HAL_NOT_INIT, CUBEMOT_MODULE_DRIVER, 0x106); // HAL not registered

#ifdef __cplusplus
}
#endif
