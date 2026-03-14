#include <stddef.h>
#include "include/drivers/button/button.h"

__attribute__((weak)) driver_button_instance *driver_button_get_instance(cubemot_device_button id)
{
    (void)id;
    return NULL;
}
