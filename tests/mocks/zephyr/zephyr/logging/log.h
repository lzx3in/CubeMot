/**
 * @file log.h
 * @brief Zephyr logging API mock for host-based unit testing
 *
 * Provides no-op macros for Zephyr logging functions.
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Log levels */
#define LOG_LEVEL_NONE 0
#define LOG_LEVEL_ERR  1
#define LOG_LEVEL_WRN  2
#define LOG_LEVEL_INF  3
#define LOG_LEVEL_DBG  4

/* Module registration — no-op on host */
#define LOG_MODULE_REGISTER(...)

/* Logging macros — no-op on host */
#define LOG_ERR(...)
#define LOG_WRN(...)
#define LOG_INF(...)
#define LOG_DBG(...)

#ifdef __cplusplus
}
#endif
