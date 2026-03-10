#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

int app_init(void);
int app_start(void);
void app_launch(void);

typedef enum {
    APP_STATE_UNINITIALIZED = 0,
    APP_STATE_INITIALIZING,
    APP_STATE_READY,
    APP_STATE_RUNNING,
    APP_STATE_ERROR,
    APP_STATE_STOPPED
} app_state_t;

app_state_t app_get_state(void);

typedef struct {
    bool auto_start;
    uint32_t startup_delay_ms;
    bool enable_safety;
    bool enable_debug;
} app_config_t;

const app_config_t *app_get_default_config(void);

#ifdef __cplusplus
}
#endif
