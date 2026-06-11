#include "app.h"
#include <zephyr/kernel.h>

extern volatile app_state_t g_app_state;

void app_launch(void)
{
    if (app_init() != 0) {
        g_app_state = APP_STATE_ERROR;
        for (;;) {
            k_msleep(1000);
        }
    }
}
