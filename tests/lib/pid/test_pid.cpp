#include <gtest/gtest.h>
#include <cmath>
#include <limits>

extern "C" {
#include "lib/pid/pid.h"
}

class PIDTest : public ::testing::Test
{
  protected:
    PID_t pid;

    void SetUp() override
    {
        pid_init(&pid, PID_MODE_DERIVATIV_CALC, 0.001f);
    }
};

// Init and parameters

TEST_F(PIDTest, Init_ZeroState)
{
    EXPECT_EQ(pid.mode, PID_MODE_DERIVATIV_CALC);
    EXPECT_EQ(pid.dt_min, 0.001f);
    EXPECT_EQ(pid.kp, 0.0f);
    EXPECT_EQ(pid.ki, 0.0f);
    EXPECT_EQ(pid.kd, 0.0f);
    EXPECT_EQ(pid.integral, 0.0f);
}

TEST_F(PIDTest, SetParameters_ValidAndInvalid)
{
    // Valid parameters
    EXPECT_EQ(pid_set_parameters(&pid, 1.0f, 0.5f, 0.1f, 10.0f, 100.0f), 0);
    EXPECT_EQ(pid.kp, 1.0f);

    // Invalid (NaN)
    EXPECT_EQ(pid_set_parameters(&pid, NAN, 0.5f, 0.1f, 10.0f, 100.0f), 1);
}

// P/I/D independent calculation

TEST_F(PIDTest, Proportional_Only)
{
    pid_set_parameters(&pid, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    float output = pid_calculate(&pid, 100.0f, 80.0f, 0.0f, 0.01f);
    EXPECT_FLOAT_EQ(output, 40.0f);
}

TEST_F(PIDTest, Integral_Only)
{
    pid_set_parameters(&pid, 0.0f, 1.0f, 0.0f, 100.0f, 0.0f);
    float out1 = pid_calculate(&pid, 10.0f, 0.0f, 0.0f, 0.01f);
    float out2 = pid_calculate(&pid, 10.0f, 0.0f, 0.0f, 0.01f);
    EXPECT_FLOAT_EQ(out1, 0.1f);
    EXPECT_FLOAT_EQ(out2, 0.2f);
}

TEST_F(PIDTest, Derivative_Only)
{
    pid_set_parameters(&pid, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    pid_calculate(&pid, 0.0f, 0.0f, 0.0f, 0.01f); // Init state
    float out = pid_calculate(&pid, 10.0f, 0.0f, 0.0f, 0.01f);
    EXPECT_FLOAT_EQ(out, 1000.0f); // d = (10-0)/0.01 = 1000
}

// Derivative modes

TEST_F(PIDTest, DerivativeMode_Calc_ErrorDerivative)
{
    pid_set_parameters(&pid, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    pid_calculate(&pid, 0.0f, 0.0f, 0.0f, 0.01f);
    float out = pid_calculate(&pid, 10.0f, 0.0f, 0.0f, 0.01f);
    EXPECT_FLOAT_EQ(out, 1000.0f);
}

TEST_F(PIDTest, DerivativeMode_CalcNoSP_NoKickOnSetpointChange)
{
    pid_init(&pid, PID_MODE_DERIVATIV_CALC_NO_SP, 0.001f);
    pid_set_parameters(&pid, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    pid_calculate(&pid, 50.0f, 50.0f, 0.0f, 0.01f);
    float out = pid_calculate(&pid, 100.0f, 50.0f, 0.0f, 0.01f);
    // SP jumps 50->100, but derivative on measurement, output=50 (P only)
    EXPECT_FLOAT_EQ(out, 50.0f);
}

TEST_F(PIDTest, DerivativeMode_Set_ExternalInput)
{
    pid_init(&pid, PID_MODE_DERIVATIV_SET, 0.001f);
    pid_set_parameters(&pid, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    float out = pid_calculate(&pid, 0.0f, 0.0f, -5.0f, 0.01f);
    EXPECT_FLOAT_EQ(out, 5.0f); // d = -(-5.0) = 5.0
}

TEST_F(PIDTest, ControllerTypes_PI_and_P_Mode)
{
    // PI controller (kd=0)
    pid_set_parameters(&pid, 1.0f, 0.5f, 0.0f, 10.0f, 0.0f);
    float out1 = pid_calculate(&pid, 10.0f, 0.0f, 0.0f, 0.01f);
    EXPECT_FLOAT_EQ(out1, 10.05f); // P=10, I=0.05

    // P controller (ki=0, kd=0)
    pid_set_parameters(&pid, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    float out_p1 = pid_calculate(&pid, 10.0f, 0.0f, 0.0f, 0.01f);
    float out_p2 = pid_calculate(&pid, 10.0f, 0.0f, 0.0f, 0.01f);
    EXPECT_FLOAT_EQ(out_p1, 20.0f);
    EXPECT_FLOAT_EQ(out_p2, 20.0f); // Pure proportional, stable output
}

// Limits and anti-windup

TEST_F(PIDTest, Limits_IntegralAndOutputSaturation)
{
    // Integral limit
    pid_set_parameters(&pid, 0.0f, 1.0f, 0.0f, 0.5f, 0.0f);
    for (int i = 0; i < 100; i++) {
        pid_calculate(&pid, 10.0f, 0.0f, 0.0f, 0.01f);
    }
    EXPECT_LE(pid.integral, 0.5f);

    // Output limit
    pid_set_parameters(&pid, 10.0f, 0.0f, 0.0f, 0.0f, 50.0f);
    float output = pid_calculate(&pid, 100.0f, 0.0f, 0.0f, 0.01f);
    EXPECT_FLOAT_EQ(output, 50.0f);
}

TEST_F(PIDTest, AntiWindup_NoAccumulationWhenSaturated)
{
    pid_set_parameters(&pid, 0.0f, 1.0f, 0.0f, 100.0f, 1.0f);
    float out1 = pid_calculate(&pid, 100.0f, 0.0f, 0.0f, 0.01f);
    float out2 = pid_calculate(&pid, 100.0f, 0.0f, 0.0f, 0.01f);
    EXPECT_FLOAT_EQ(out1, out2); // No accumulation when saturated
}

// Boundary conditions

TEST_F(PIDTest, Boundary_NaNInput_ReturnsLastOutput)
{
    pid_set_parameters(&pid, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    float out1 = pid_calculate(&pid, 10.0f, 0.0f, 0.0f, 0.01f);
    float out2 = pid_calculate(&pid, NAN, 0.0f, 0.0f, 0.01f);
    EXPECT_FLOAT_EQ(out1, 10.0f);
    EXPECT_FLOAT_EQ(out2, 10.0f); // Returns last output on NaN
}

TEST_F(PIDTest, ResetIntegral_ZeroesState)
{
    pid_set_parameters(&pid, 0.0f, 1.0f, 0.0f, 100.0f, 0.0f);
    pid_calculate(&pid, 10.0f, 0.0f, 0.0f, 0.01f);
    pid_reset_integral(&pid);
    EXPECT_EQ(pid.integral, 0.0f);
}

// Integration test

TEST_F(PIDTest, StepResponse_ConvergesToTarget)
{
    pid_set_parameters(&pid, 2.0f, 0.5f, 0.1f, 10.0f, 100.0f);
    float target = 100.0f;
    float current = 0.0f;

    for (int i = 0; i < 200; i++) {
        float output = pid_calculate(&pid, target, current, 0.0f, 0.01f);
        current += output * 0.01f;
    }

    EXPECT_GT(current, 90.0f); // Should converge after 200 steps
}
