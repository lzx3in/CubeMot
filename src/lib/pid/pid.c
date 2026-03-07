#include "pid.h"
#include <math.h>
#include <stdbool.h>

#define SIGMA 0.000001f

static inline bool is_finite(float val)
{
    return isfinite(val);
}

void pid_init(PID_t *pid, pid_mode_t mode, float dt_min)
{
    pid->mode = mode;
    pid->dt_min = dt_min;
    pid->kp = 0.0f;
    pid->ki = 0.0f;
    pid->kd = 0.0f;
    pid->integral = 0.0f;
    pid->integral_limit = 0.0f;
    pid->output_limit = 0.0f;
    pid->error_previous = 0.0f;
    pid->last_output = 0.0f;
}

int pid_set_parameters(PID_t *pid, float kp, float ki, float kd, float integral_limit, float output_limit)
{
    int ret = 0;

    if (is_finite(kp)) {
        pid->kp = kp;
    } else {
        ret = 1;
    }

    if (is_finite(ki)) {
        pid->ki = ki;
    } else {
        ret = 1;
    }

    if (is_finite(kd)) {
        pid->kd = kd;
    } else {
        ret = 1;
    }

    if (is_finite(integral_limit)) {
        pid->integral_limit = integral_limit;
    } else {
        ret = 1;
    }

    if (is_finite(output_limit)) {
        pid->output_limit = output_limit;
    } else {
        ret = 1;
    }

    return ret;
}

float pid_calculate(PID_t *pid, float sp, float val, float val_dot, float dt)
{
    if (!is_finite(sp) || !is_finite(val) || !is_finite(val_dot) || !is_finite(dt)) {
        return pid->last_output;
    }

    float i, d = 0.0f;
    float error = sp - val;

    if (pid->kd > SIGMA && pid->mode < PID_MODE_DERIVATIV_COUNT) {
        if (pid->mode == PID_MODE_DERIVATIV_CALC) {
            d = (error - pid->error_previous) / fmaxf(dt, pid->dt_min);
            pid->error_previous = error;

        } else if (pid->mode == PID_MODE_DERIVATIV_CALC_NO_SP) {
            d = (-val - pid->error_previous) / fmaxf(dt, pid->dt_min);
            pid->error_previous = -val;

        } else if (pid->mode == PID_MODE_DERIVATIV_SET) {
            d = -val_dot;
        }

        if (!is_finite(d)) {
            d = 0.0f;
        }
    }

    float output = (error * pid->kp) + (d * pid->kd);

    if (pid->ki > SIGMA) {
        i = pid->integral + (error * dt);

        if (is_finite(i)) {
            if ((pid->output_limit < SIGMA || (fabsf(output + (i * pid->ki)) <= pid->output_limit)) &&
                fabsf(i) <= pid->integral_limit) {
                pid->integral = i;
            }
        }

        output += pid->integral * pid->ki;
    }

    if (is_finite(output)) {
        if (pid->output_limit > SIGMA) {
            if (output > pid->output_limit) {
                output = pid->output_limit;
            } else if (output < -pid->output_limit) {
                output = -pid->output_limit;
            }
        }
    }

    pid->last_output = output;

    return pid->last_output;
}

void pid_reset_integral(PID_t *pid)
{
    pid->integral = 0.0f;
}
