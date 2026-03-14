#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// PID formula: output = kp*error + ki*∫error*dt + kd*d(measurement)/dt
typedef enum {
    // Derivative on error: d = (error - error_prev) / dt
    // Warning: causes derivative kick on setpoint changes
    PID_MODE_DERIVATIV_CALC = 0,

    // Derivative on measurement: d = -(PV - PV_prev) / dt
    // Recommended: no derivative kick on setpoint changes
    PID_MODE_DERIVATIV_CALC_NO_SP,

    // External derivative input: d = -val_dot
    // Use case: gyroscope rate, encoder velocity
    PID_MODE_DERIVATIV_SET,

    PID_MODE_DERIVATIV_COUNT
} pid_mode_t;

typedef struct {
    pid_mode_t mode; // Derivative mode

    float dt_min; // Min sample time (s), prevents dt=0
    float kp;     // Proportional gain
    float ki;     // Integral gain
    float kd;     // Derivative gain (0 to disable)

    float integral;       // Integral state: Σ(error × dt)
    float integral_limit; // Anti-windup limit
    float output_limit;   // Output saturation limit

    float error_previous; // Previous error for derivative
    float last_output;    // Previous output (NaN fallback)
} PID_t;

// Initialize PID controller (gains set to 0)
void pid_init(PID_t *pid, pid_mode_t mode, float dt_min);

// Set PID parameters, returns 0 on success, 1 on NaN
int pid_set_parameters(PID_t *pid, float kp, float ki, float kd, float integral_limit, float output_limit);

// Calculate PID output, dt in seconds
float pid_calculate(PID_t *pid, float sp, float val, float val_dot, float dt);

// Reset integral accumulator
void pid_reset_integral(PID_t *pid);

#ifdef __cplusplus
}
#endif
