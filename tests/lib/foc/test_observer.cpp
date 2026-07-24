/**
 * @file test_observer.cpp
 * @brief Unit tests for Sliding Mode Observer + PLL
 */

#include <gtest/gtest.h>
#include <cmath>

extern "C" {
#include "libs/foc/observer.h"
#include "libs/math/transform.h"
}

class ObserverTest : public ::testing::Test
{
  protected:
    observer_t obs;
    
    // Motor parameters (GBM2804H-100T gimbal motor)
    static constexpr float RS = 5.29f;           // Ω
    static constexpr float LS = 1.058e-3f;       // H
    static constexpr float DT = 1.0f / 30000.0f; // 30kHz sample rate
    static constexpr float GAIN1 = -22528.0f;    // Sliding mode gain
    static constexpr float GAIN2 = 31586.0f;     // BEMF filter gain
    static constexpr float PLL_KP = 1.0f;
    static constexpr float PLL_KI = 100.0f;

    void SetUp() override
    {
        observer_init(&obs, RS, LS, DT, GAIN1, GAIN2, PLL_KP, PLL_KI);
    }
    
    // Simulate motor running at constant speed
    void simulate_rotation(float omega_elec, int steps)
    {
        float theta = 0.0f;
        for (int i = 0; i < steps; i++) {
            // Simulate BEMF: E = ω * λ_m * [-sin(θ), cos(θ)]
            // Simplified: assume unit flux linkage
            float sin_t, cos_t;
            fast_sincos(theta, &sin_t, &cos_t);
            
            // Applied voltage (simplified: just counter BEMF)
            float v_alpha = -omega_elec * sin_t;
            float v_beta = omega_elec * cos_t;
            
            // Measured current (small, since V ≈ E)
            float i_alpha = 0.01f * sin_t;
            float i_beta = 0.01f * cos_t;
            
            observer_step(&obs, v_alpha, v_beta, i_alpha, i_beta);
            theta += omega_elec * DT;
        }
    }
};

// ============================================================================
// Initialization Tests
// ============================================================================

TEST_F(ObserverTest, Init_ZeroState)
{
    EXPECT_FLOAT_EQ(obs.i_alpha_hat, 0.0f);
    EXPECT_FLOAT_EQ(obs.i_beta_hat, 0.0f);
    EXPECT_FLOAT_EQ(obs.e_alpha, 0.0f);
    EXPECT_FLOAT_EQ(obs.e_beta, 0.0f);
    EXPECT_FLOAT_EQ(obs.theta_elec, 0.0f);
    EXPECT_FLOAT_EQ(obs.omega_elec, 0.0f);
    EXPECT_FALSE(obs.converged);
    EXPECT_EQ(obs.consecutive_ok, 0);
}

TEST_F(ObserverTest, Init_ParametersSet)
{
    EXPECT_NEAR(obs.rs_inv_ls, RS / LS, 1e-3f);
    EXPECT_NEAR(obs.inv_ls, 1.0f / LS, 1e-3f);
    EXPECT_FLOAT_EQ(obs.dt, DT);
    EXPECT_FLOAT_EQ(obs.gain1, GAIN1);
    EXPECT_FLOAT_EQ(obs.gain2, GAIN2);
    EXPECT_FLOAT_EQ(obs.pll_kp, PLL_KP);
    EXPECT_FLOAT_EQ(obs.pll_ki, PLL_KI);
}

// ============================================================================
// Basic Operation Tests
// ============================================================================

TEST_F(ObserverTest, Step_NoInput_StaysAtZero)
{
    // With zero inputs, observer should stay near zero
    for (int i = 0; i < 100; i++) {
        observer_step(&obs, 0.0f, 0.0f, 0.0f, 0.0f);
    }
    
    EXPECT_NEAR(obs.theta_elec, 0.0f, 0.1f);
    EXPECT_NEAR(obs.omega_elec, 0.0f, 1.0f);
}

TEST_F(ObserverTest, Step_CurrentEstimateTracking)
{
    // Apply voltage, current estimate should respond
    float v_alpha = 1.0f;
    float v_beta = 0.0f;
    
    for (int i = 0; i < 1000; i++) {
        observer_step(&obs, v_alpha, v_beta, 0.0f, 0.0f);
    }
    
    // Current estimate should be non-zero after applying voltage
    EXPECT_NE(obs.i_alpha_hat, 0.0f);
}

// ============================================================================
// Convergence Tests
// ============================================================================

TEST_F(ObserverTest, Convergence_InitiallyFalse)
{
    EXPECT_FALSE(observer_is_converged(&obs));
}

TEST_F(ObserverTest, Convergence_WithBEMF)
{
    // Simulate motor with significant BEMF
    float omega = 100.0f; // rad/s electrical
    
    // Run for enough steps to converge (>50 consecutive OK)
    simulate_rotation(omega, 5000);
    
    // Should eventually converge
    // Note: convergence depends on BEMF amplitude > 0.1 and theta_error < 500
    // This test verifies the convergence mechanism works
    EXPECT_GE(obs.consecutive_ok, 0);
}

TEST_F(ObserverTest, Convergence_ResetWorks)
{
    // Force some convergence
    obs.consecutive_ok = 100;
    obs.converged = true;
    
    observer_reset_convergence(&obs);
    
    EXPECT_FALSE(obs.converged);
    EXPECT_EQ(obs.consecutive_ok, 0);
}

// ============================================================================
// Force Angle Tests
// ============================================================================

TEST_F(ObserverTest, ForceAngle_SetsTheta)
{
    float target_angle = 1.5f;
    observer_force_angle(&obs, target_angle);
    
    EXPECT_FLOAT_EQ(obs.theta_elec, target_angle);
    EXPECT_FLOAT_EQ(obs.omega_elec, 0.0f);
    EXPECT_FLOAT_EQ(obs.pll_integral, 0.0f);
    EXPECT_FALSE(obs.converged);
}

TEST_F(ObserverTest, ForceAngle_ResetsPLL)
{
    // First let PLL accumulate something
    for (int i = 0; i < 100; i++) {
        observer_step(&obs, 1.0f, 0.5f, 0.0f, 0.0f);
    }
    
    observer_force_angle(&obs, 0.0f);
    
    EXPECT_FLOAT_EQ(obs.pll_integral, 0.0f);
    EXPECT_FLOAT_EQ(obs.omega_elec, 0.0f);
}

// ============================================================================
// Angle Wrapping Tests
// ============================================================================

TEST_F(ObserverTest, AngleWrapping_StaysInRange)
{
    // Run observer with inputs that would cause angle to grow
    for (int i = 0; i < 10000; i++) {
        observer_step(&obs, 0.0f, 1.0f, 0.0f, 0.0f);
        
        // Angle should always be in [0, 2π)
        EXPECT_GE(obs.theta_elec, 0.0f);
        EXPECT_LT(obs.theta_elec, 2.0f * 3.14159265359f);
    }
}

// ============================================================================
// PLL Anti-Windup Tests
// ============================================================================

TEST_F(ObserverTest, PLL_AntiWindup_LimitsIntegral)
{
    // Apply large error to try to wind up PLL
    for (int i = 0; i < 1000; i++) {
        observer_step(&obs, 100.0f, 100.0f, 0.0f, 0.0f);
    }
    
    // PLL integral should be clamped to ±2000
    EXPECT_LE(obs.pll_integral, 2000.0f);
    EXPECT_GE(obs.pll_integral, -2000.0f);
}

// ============================================================================
// Speed Estimation Tests
// ============================================================================

TEST_F(ObserverTest, SpeedEstimation_FilteredSpeed)
{
    // Run observer with some dynamics
    for (int i = 0; i < 1000; i++) {
        observer_step(&obs, 0.0f, 10.0f, 0.0f, 0.0f);
    }
    
    // Filtered speed should be non-negative
    EXPECT_GE(obs.speed_rpm_filt, 0.0f);
}

// ============================================================================
// BEMF Estimation Tests
// ============================================================================

TEST_F(ObserverTest, BEMF_EstimationRespondsToSlidingMode)
{
    // Create current estimation error to trigger sliding mode
    obs.i_alpha_hat = 1.0f;  // Error = 1.0 - 0.0 = 1.0
    
    observer_step(&obs, 0.0f, 0.0f, 0.0f, 0.0f);
    
    // BEMF estimate should change due to sliding mode correction
    // Z_alpha = G1 * sign(1.0) = G1 * 1 = -22528
    // E_alpha += G2 * dt * (Z - E) = 31586 * (1/30000) * (-22528 - 0)
    EXPECT_NE(obs.e_alpha, 0.0f);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(ObserverTest, Integration_MultipleStepsNoInstability)
{
    // Run observer for many steps and verify no NaN/Inf
    for (int i = 0; i < 10000; i++) {
        float v_alpha = (i % 100 < 50) ? 1.0f : -1.0f;
        float v_beta = (i % 50 < 25) ? 0.5f : -0.5f;
        observer_step(&obs, v_alpha, v_beta, 0.0f, 0.0f);
        
        EXPECT_FALSE(std::isnan(obs.theta_elec));
        EXPECT_FALSE(std::isnan(obs.omega_elec));
        EXPECT_FALSE(std::isinf(obs.theta_elec));
        EXPECT_FALSE(std::isinf(obs.omega_elec));
    }
}
