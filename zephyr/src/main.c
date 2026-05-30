#include "app.h"
#include <zephyr/kernel.h>

int main(void)
{
    if (app_init() != 0) {
        for (;;) {
            k_msleep(1000);
        }
    }
    if (app_start() != 0) {
        for (;;) {
            k_msleep(1000);
        }
    }
    return 0;
}
