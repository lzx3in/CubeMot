#include "boards/init.h"
#include "modules/blink/blink.h"
#include "FreeRTOS.h"
#include "task.h"

int main(void)
{
    board_init();
    blink_module_init();
    vTaskStartScheduler();

    // Should never reach here
    for (;;) {
    }
}
