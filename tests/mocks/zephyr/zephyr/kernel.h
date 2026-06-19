/**
 * @file kernel.h
 * @brief Zephyr kernel API mock for host-based unit testing
 *
 * Provides stub implementations of Zephyr kernel primitives used by
 * msghub and common modules. Mutexes delegate to pthread; scheduling
 * and interrupt locks are no-ops on a single-threaded test host.
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* k_mutex — wraps pthread_mutex_t                                            */
/* ========================================================================== */

struct k_mutex {
    pthread_mutex_t mtx;
};

static inline int k_mutex_init(struct k_mutex *mutex)
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&mutex->mtx, &attr);
    pthread_mutexattr_destroy(&attr);
    return 0;
}

static inline int k_mutex_lock(struct k_mutex *mutex, /*k_timeout_t*/ int timeout)
{
    (void)timeout;
    return pthread_mutex_lock(&mutex->mtx);
}

static inline int k_mutex_unlock(struct k_mutex *mutex)
{
    return pthread_mutex_unlock(&mutex->mtx);
}

/* K_FOREVER timeout constant */
#define K_FOREVER (-1)

/* ========================================================================== */
/* Scheduler lock — no-op on host (single-threaded test context)              */
/* ========================================================================== */

static inline void k_sched_lock(void) {}
static inline void k_sched_unlock(void) {}

/* ========================================================================== */
/* Interrupt lock — no-op on host                                             */
/* ========================================================================== */

static inline unsigned int irq_lock(void)
{
    return 0;
}

static inline void irq_unlock(unsigned int key)
{
    (void)key;
}

/* ========================================================================== */
/* k_sleep / K_MSEC / K_NO_WAIT                                              */
/* ========================================================================== */

struct k_timespec {
    int32_t duration;
};

static inline int32_t k_sleep(struct k_timespec duration)
{
    struct timespec ts;
    ts.tv_sec = duration.duration / 1000;
    ts.tv_nsec = (duration.duration % 1000) * 1000000L;
    nanosleep(&ts, NULL);
    return duration.duration;
}

#define K_MSEC(_ms) ((struct k_timespec){.duration = (_ms)})
#define K_NO_WAIT ((struct k_timespec){.duration = 0})

/* ========================================================================== */
/* k_uptime_get — monotonic clock in milliseconds                             */
/* ========================================================================== */

static inline int64_t k_uptime_get(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

#ifdef __cplusplus
}
#endif
