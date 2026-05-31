#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

uint32_t common_get_timestamp_ms(void);
uint32_t common_get_timestamp_ms_isr(void);

#ifdef __cplusplus
}
#endif
