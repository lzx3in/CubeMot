#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void board_delay_ms(uint32_t ms);
uint32_t board_get_tick(void);

#ifdef __cplusplus
}
#endif
