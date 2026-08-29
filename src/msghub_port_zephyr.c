// CubeMot ESC — msghub Zephyr adapter (framework glue, lives in the consumer repo).
//
// The msghub repo is framework-agnostic: its core only calls the msghub_port_*
// abstract interface and ships weak no-op defaults. This file provides the
// strong Zephyr implementations, overriding the weak ones at link time.

#include "msghub/msghub_port.h"

#include <zephyr/kernel.h>

static struct k_mutex s_msghub_mgr_mutex;

void msghub_port_init(void)
{
    k_mutex_init(&s_msghub_mgr_mutex);
}

void msghub_port_deinit(void)
{
    // Zephyr k_mutex does not need explicit deinit
}

void msghub_port_mgr_lock(void)
{
    k_mutex_lock(&s_msghub_mgr_mutex, K_FOREVER);
}

void msghub_port_mgr_unlock(void)
{
    k_mutex_unlock(&s_msghub_mgr_mutex);
}

void msghub_port_enter_critical(void)
{
    k_sched_lock();
}

void msghub_port_exit_critical(void)
{
    k_sched_unlock();
}

uint32_t msghub_port_enter_critical_isr(void)
{
    return (uint32_t)irq_lock();
}

void msghub_port_exit_critical_isr(uint32_t key)
{
    irq_unlock((int)key);
}

void msghub_port_sleep_ms(uint32_t ms)
{
    k_sleep(K_MSEC(ms));
}
