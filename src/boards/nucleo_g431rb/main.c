#include "boards/init.h"
#include "app/app.h"

int main(void)
{
    board_init();
    app_launch();

    // Should never reach here
    for (;;) {
    }
}
