/**
 * @file test_foc_types.cpp
 * @brief Unit tests for FOC data types and conversion functions
 */

#include <gtest/gtest.h>
#include <cmath>

extern "C" {
#include "libs/foc/foc_types.h"
}

class FocTypesTest : public ::testing::Test
{
  protected:
    static constexpr float TOLERANCE = 1e-4f;
};

// ============================================================================
// ADC to Current Conversion
// ============================================================================

TEST_F(FocTypesTest, AdcToCurrent_ZeroOffset_ZeroCurrent)
{
    // When raw == offset, current should be 0
    float current = foc_adc_to_current(2048, 2048);
    EXPECT_NEAR(current, 0.0f, TOLERANCE);
}

TEST_F(FocTypesTest, AdcToCurrent_PositiveCurrent)
{
    // raw > offset → positive current
    // scale = 3.3 / (4096 * 0.33 * 1.53) ≈ 0.001597
    int16_t raw = 2048 + 100;
    int16_t offset = 2048;
    float current = foc_adc_to_current(raw, offset);
    float expected = 100.0f * 3.3f / (4096.0f * 0.33f * 1.53f);
    EXPECT_NEAR(current, expected, TOLERANCE);
}

TEST_F(FocTypesTest, AdcToCurrent_NegativeCurrent)
{
    // raw < offset → negative current
    int16_t raw = 2048 - 100;
    int16_t offset = 2048;
    float current = foc_adc_to_current(raw, offset);
    float expected = -100.0f * 3.3f / (4096.0f * 0.33f * 1.53f);
    EXPECT_NEAR(current, expected, TOLERANCE);
}

TEST_F(FocTypesTest, AdcToCurrent_ScaleFactor)
{
    // Verify scale factor: 3.3 / (4096 * 0.33 * 1.53) ≈ 0.001597 A/LSB
    float scale = FOC_ADC_VREF / (FOC_ADC_RESOLUTION * 0.33f * 1.53f);
    EXPECT_NEAR(scale, 0.001597f, 1e-5f);
}

// ============================================================================
// ADC to Bus Voltage Conversion
// ============================================================================

TEST_F(FocTypesTest, AdcToVbus_ZeroRaw_ZeroVoltage)
{
    float vbus = foc_adc_to_vbus(0);
    EXPECT_NEAR(vbus, 0.0f, TOLERANCE);
}

TEST_F(FocTypesTest, AdcToVbus_NominalVoltage)
{
    // For 24V bus with partition factor 0.0625:
    // ADC_raw = 24 * 0.0625 * 4096 / 3.3 ≈ 1838
    int16_t raw_24v = (int16_t)(24.0f * 0.0625f * 4096.0f / 3.3f);
    float vbus = foc_adc_to_vbus(raw_24v);
    EXPECT_NEAR(vbus, 24.0f, 0.1f);
}

TEST_F(FocTypesTest, AdcToVbus_ScaleFactor)
{
    // scale = 3.3 / (4096 * 0.0625) ≈ 0.01289 V/LSB
    float scale = FOC_ADC_VREF / (FOC_ADC_RESOLUTION * 0.0625f);
    EXPECT_NEAR(scale, 0.01289f, 1e-4f);
}

// ============================================================================
// Speed Unit Conversions
// ============================================================================

TEST_F(FocTypesTest, RpmToRads_ZeroSpeed)
{
    float rads = foc_rpm_to_rads(0.0f, 7);
    EXPECT_NEAR(rads, 0.0f, TOLERANCE);
}

TEST_F(FocTypesTest, RpmToRads_NominalSpeed)
{
    // 1000 RPM with 7 pole pairs
    // ω_elec = RPM * 2π * pole_pairs / 60
    float rpm = 1000.0f;
    uint8_t pole_pairs = 7;
    float rads = foc_rpm_to_rads(rpm, pole_pairs);
    float expected = rpm * 2.0f * 3.14159265359f * pole_pairs / 60.0f;
    EXPECT_NEAR(rads, expected, 0.01f);
}

TEST_F(FocTypesTest, RadsToRpm_ZeroSpeed)
{
    float rpm = foc_rads_to_rpm(0.0f, 7);
    EXPECT_NEAR(rpm, 0.0f, TOLERANCE);
}

TEST_F(FocTypesTest, RadsToRpm_NominalSpeed)
{
    // Round-trip test: RPM → rad/s → RPM
    float original_rpm = 1500.0f;
    uint8_t pole_pairs = 7;
    float rads = foc_rpm_to_rads(original_rpm, pole_pairs);
    float recovered_rpm = foc_rads_to_rpm(rads, pole_pairs);
    EXPECT_NEAR(recovered_rpm, original_rpm, 0.01f);
}

TEST_F(FocTypesTest, SpeedConversion_DifferentPolePairs)
{
    // Test with different pole pair counts
    float rpm = 3000.0f;
    
    for (uint8_t pp : {1, 4, 7, 11}) {
        float rads = foc_rpm_to_rads(rpm, pp);
        float recovered = foc_rads_to_rpm(rads, pp);
        EXPECT_NEAR(recovered, rpm, 0.01f) << "Failed for pole_pairs=" << (int)pp;
    }
}

// ============================================================================
// Motor Config Structure
// ============================================================================

TEST_F(FocTypesTest, MotorConfig_DefaultValues)
{
    // Verify typical gimbal motor parameters (GBM2804H-100T)
    foc_motor_config_t config = {
        .rs = 5.29f,           // Ω
        .ls = 1.058e-3f,       // H (1.058 mH)
        .ld_lq_ratio = 1.0f,   // SPMSM
        .pole_pairs = 7,
        .voltage_const = 0.0f, // Not used
        .rated_flux = 0.0f,    // Not used
        .max_speed_rpm = 1000.0f,
        .nominal_current = 0.8f,
        .iq_max = 1.5f,
        .id_demag = -2.0f,
        .observer_gain1 = -22528.0f,
        .observer_gain2 = 31586.0f,
        .pll_kp = 1.0f,
        .pll_ki = 100.0f,
        .observer_min_speed_rpm = 50,
    };
    
    EXPECT_EQ(config.pole_pairs, 7);
    EXPECT_NEAR(config.rs, 5.29f, 0.01f);
    EXPECT_NEAR(config.ls, 1.058e-3f, 1e-5f);
}
