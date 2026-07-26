/**
 * @file scope.h
 * @brief High-resolution diagnostic scope (ring buffer @ 1kHz)
 *
 * Captures 9 channels of FOC state every speed-loop tick (1kHz)
 * into a fixed-size ring buffer. Dump via shell command for offline analysis.
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SCOPE_CHANNELS  9
#define SCOPE_DEPTH     128  /* 128ms @ 1kHz */

typedef struct {
    int16_t ch[SCOPE_CHANNELS];
} scope_sample_t;

/* Channel indices */
enum {
    SC_THETA_FOC = 0,  /* rad × 1000 (mrad) */
    SC_THETA_OBS,      /* rad × 1000 (mrad) */
    SC_OMEGA,          /* rad/s × 10 (deci-rad/s) */
    SC_ID_MA,          /* mA */
    SC_IQ_MA,          /* mA */
    SC_IQ_REF_MA,      /* mA */
    SC_RPM,            /* RPM (integer) */
    SC_BEMF_CV,        /* V × 100 (centi-volts) */
    SC_STATE,          /* motor state enum */
};

void scope_start(void);
void scope_stop(void);
bool scope_is_active(void);
void scope_record(int8_t motor_state);  /* call from motor_ctrl thread each tick */
uint16_t scope_get_count(void);
const scope_sample_t *scope_get_buf(void);

#ifdef __cplusplus
}
#endif
