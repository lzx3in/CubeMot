#include "app.h"
#include "cubemot_config.h"
#include "modules/blink/blink.h"
#include <stdint.h>

volatile app_state_t g_app_state = APP_STATE_UNINITIALIZED;

static const app_config_t k_default_config = {
    .auto_start = true, .startup_delay_ms = 1000, .enable_safety = false, .enable_debug = false};

static int init_modules(void)
{
#if CONFIG_MODULE_BLINK_ENABLE
    blink_module_init();
#endif

    return 0;
}

int app_init(void)
{
    g_app_state = APP_STATE_INITIALIZING;

    int ret = init_modules();
    if (ret != 0) {
        g_app_state = APP_STATE_ERROR;
        return ret;
    }

    g_app_state = APP_STATE_READY;
    return 0;
}

int app_start(void)
{
    if (g_app_state != APP_STATE_READY) {
        return -1;
    }

    g_app_state = APP_STATE_RUNNING;
    return 0;
}

app_state_t app_get_state(void)
{
    return g_app_state;
}

const app_config_t *app_get_default_config(void)
{
    return &k_default_config;
}
