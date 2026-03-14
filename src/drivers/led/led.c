#include <stddef.h>
#include "include/drivers/led/led.h"

__attribute__((weak)) const driver_led_instance *driver_led_get_instance(cubemot_device_led id)
{
    (void)id;
    return NULL;
}
