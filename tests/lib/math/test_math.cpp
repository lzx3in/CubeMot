// FOC Math Library Test Suite
// Tests for Clarke/Park transforms, SVPWM, and digital filters

#include <gtest/gtest.h>
#include <cmath>

extern "C" {
#include "transform.h"
#include "svpwm.h"
#include "filter.h"
}

namespace cubemot::test
{

// ============================================================================
// Clarke Transform Tests
// ============================================================================

class ClarkeTest : public ::testing::Test {};

TEST_F(ClarkeTest, Balanced_ThreePhase_AlphaEqualsIa)
{
    // Balanced: Ia=1, Ib=-0.5, Ic=-0.5
    alphabeta_t ab = clarke_transform(1.0f, -0.5f, -0.5f);
    EXPECT_NEAR(ab.alpha, 1.0f, 1e-6f);
    EXPECT_NEAR(ab.beta, 0.0f, 1e-6f);
}

TEST_F(ClarkeTest, PureBeta_AlphaZero)
{
    // Ia=0, Ib=1, Ic=-1: pure β component
    alphabeta_t ab = clarke_transform(0.0f, 1.0f, -1.0f);
    EXPECT_NEAR(ab.alpha, 0.0f, 1e-6f);
    EXPECT_NEAR(ab.beta, 2.0f / std::sqrt(3.0f), 1e-6f);
}

TEST_F(ClarkeTest, ZeroCurrent_BothZero)
{
    alphabeta_t ab = clarke_transform(0.0f, 0.0f, 0.0f);
    EXPECT_NEAR(ab.alpha, 0.0f, 1e-6f);
    EXPECT_NEAR(ab.beta, 0.0f, 1e-6f);
}

// ============================================================================
// Inverse Clarke Transform Tests
// ============================================================================

class IClarkeTest : public ::testing::Test {};

TEST_F(IClarkeTest, AlphaOnly_StandardOutput)
{
    // α=1, β=0 → a=1, b=-0.5, c=-0.5
    abc_t abc = iclarke_transform(1.0f, 0.0f);
    EXPECT_NEAR(abc.a, 1.0f, 1e-6f);
    EXPECT_NEAR(abc.b, -0.5f, 1e-6f);
    EXPECT_NEAR(abc.c, -0.5f, 1e-6f);
}

TEST_F(IClarkeTest, BetaOnly_Sqrt3Division)
{
    // α=0, β=1 → a=0, b=√3/2, c=-√3/2
    abc_t abc = iclarke_transform(0.0f, 1.0f);
    EXPECT_NEAR(abc.a, 0.0f, 1e-6f);
    EXPECT_NEAR(abc.b, 0.8660254f, 1e-4f);
    EXPECT_NEAR(abc.c, -0.8660254f, 1e-4f);
}

// ============================================================================
// Park Transform Tests
// ============================================================================

class ParkTest : public ::testing::Test {};

TEST_F(ParkTest, ThetaZero_Passthrough)
{
    // θ=0: Id=Iα, Iq=Iβ (cos=1, sin=0)
    dq_t dq = park_transform(1.0f, 0.5f, 0.0f, 1.0f);
    EXPECT_NEAR(dq.d, 1.0f, 1e-6f);
    EXPECT_NEAR(dq.q, 0.5f, 1e-6f);
}

TEST_F(ParkTest, Theta90Degrees_Rotated)
{
    // θ=90°: Id=Iβ, Iq=-Iα (cos=0, sin=1)
    dq_t dq = park_transform(1.0f, 0.5f, 1.0f, 0.0f);
    EXPECT_NEAR(dq.d, 0.5f, 1e-6f);
    EXPECT_NEAR(dq.q, -1.0f, 1e-6f);
}

// ============================================================================
// Inverse Park Transform Tests
// ============================================================================

class IParkTest : public ::testing::Test {};

TEST_F(IParkTest, ThetaZero_Passthrough)
{
    // θ=0: Vα=Vd, Vβ=Vq
    alphabeta_t ab = ipark_transform(1.0f, 0.5f, 0.0f, 1.0f);
    EXPECT_NEAR(ab.alpha, 1.0f, 1e-6f);
    EXPECT_NEAR(ab.beta, 0.5f, 1e-6f);
}

TEST_F(IParkTest, Theta90Degrees_Rotated)
{
    // θ=90°: Vα=-Vq, Vβ=Vd
    alphabeta_t ab = ipark_transform(1.0f, 0.5f, 1.0f, 0.0f);
    EXPECT_NEAR(ab.alpha, -0.5f, 1e-6f);
    EXPECT_NEAR(ab.beta, 1.0f, 1e-6f);
}

// ============================================================================
// Round-trip Tests: Clarke → Park → iPark → iClarke
// ============================================================================

TEST(TransformRoundTrip, ClarkeParkIParkIClarke_Identity)
{
    float Ia = 0.8f, Ib = -0.3f, Ic = -0.5f; // balanced
    float theta = 1.2f;                        // ~69°

    float sin_t = std::sin(theta);
    float cos_t = std::cos(theta);

    alphabeta_t ab = clarke_transform(Ia, Ib, Ic);
    dq_t dq = park_transform(ab.alpha, ab.beta, sin_t, cos_t);
    alphabeta_t ab2 = ipark_transform(dq.d, dq.q, sin_t, cos_t);

    EXPECT_NEAR(ab2.alpha, ab.alpha, 1e-5f);
    EXPECT_NEAR(ab2.beta, ab.beta, 1e-5f);
}

// ============================================================================
// Fast sin/cos LUT Tests
// ============================================================================

class FastSinCosTest : public ::testing::Test {};

TEST_F(FastSinCosTest, ZeroAngle_SinZeroCosOne)
{
    float s, c;
    fast_sincos(0.0f, &s, &c);
    EXPECT_NEAR(s, 0.0f, 0.02f);
    EXPECT_NEAR(c, 1.0f, 0.02f);
}

TEST_F(FastSinCosTest, QuarterPi_SinEqualsCos)
{
    float s, c;
    fast_sincos(3.14159265f / 4.0f, &s, &c);
    EXPECT_NEAR(s, c, 0.02f);
    EXPECT_NEAR(s, 0.707f, 0.02f);
}

TEST_F(FastSinCosTest, HalfPi_SinOneCosZero)
{
    float s, c;
    fast_sincos(3.14159265f / 2.0f, &s, &c);
    EXPECT_NEAR(s, 1.0f, 0.02f);
    EXPECT_NEAR(c, 0.0f, 0.02f);
}

// ============================================================================
// SVPWM Tests
// ============================================================================

class SVPWMTest : public ::testing::Test {
  protected:
    svpwm_duty_t duty_{};
    static constexpr float kVbus = 13.0f;
};

TEST_F(SVPWMTest, ZeroVector_AllFiftyPercent)
{
    svpwm_minmax(0.0f, 0.0f, kVbus, &duty_);
    EXPECT_NEAR(duty_.duty_a, 0.5f, 1e-4f);
    EXPECT_NEAR(duty_.duty_b, 0.5f, 1e-4f);
    EXPECT_NEAR(duty_.duty_c, 0.5f, 1e-4f);
}

TEST_F(SVPWMTest, AlphaPositive_PhaseADominates)
{
    svpwm_minmax(1.0f, 0.0f, kVbus, &duty_);
    EXPECT_NEAR(duty_.duty_a, 0.5577f, 1e-3f);
    EXPECT_GT(duty_.duty_a, duty_.duty_b);
    EXPECT_GT(duty_.duty_a, duty_.duty_c);
}

TEST_F(SVPWMTest, LargeInput_ClampKeepsInRange)
{
    svpwm_minmax(10.0f, 10.0f, kVbus, &duty_);
    svpwm_clamp(&duty_);
    EXPECT_GE(duty_.duty_a, 0.0f);
    EXPECT_LE(duty_.duty_a, 1.0f);
    EXPECT_GE(duty_.duty_b, 0.0f);
    EXPECT_LE(duty_.duty_b, 1.0f);
    EXPECT_GE(duty_.duty_c, 0.0f);
    EXPECT_LE(duty_.duty_c, 1.0f);
}

// ============================================================================
// Low-Pass Filter Tests
// ============================================================================

class LPFTest : public ::testing::Test {
  protected:
    lpf_t filter_{};

    void SetUp() override
    {
        lpf_init(&filter_, 10.0f, 1000.0f); // 10Hz cutoff, 1kHz sample
    }
};

TEST_F(LPFTest, FirstSample_SmallAlpha)
{
    // alpha ≈ 2π·10/(2π·10 + 1000) ≈ 0.0591
    float y = lpf_update(&filter_, 1.0f);
    EXPECT_NEAR(y, 0.0591f, 0.01f);
}

TEST_F(LPFTest, ConvergesToDC_After100Samples)
{
    for (int i = 0; i < 100; i++) {
        lpf_update(&filter_, 1.0f);
    }
    float y = lpf_update(&filter_, 1.0f);
    EXPECT_NEAR(y, 1.0f, 0.01f);
}

TEST_F(LPFTest, Reset_OutputReturnsToZero)
{
    lpf_update(&filter_, 5.0f);
    lpf_reset(&filter_);
    EXPECT_NEAR(filter_.y_prev, 0.0f, 1e-6f);
}

// ============================================================================
// Moving Average Filter Tests
// ============================================================================

class MAFTest : public ::testing::Test {
  protected:
    maf_t filter_{};

    void SetUp() override
    {
        maf_init(&filter_, 4);
    }
};

TEST_F(MAFTest, SingleValue_ReturnsSame)
{
    float y = maf_update(&filter_, 1.0f);
    EXPECT_NEAR(y, 1.0f, 1e-6f);
}

TEST_F(MAFTest, TwoValues_Average)
{
    maf_update(&filter_, 1.0f);
    float y = maf_update(&filter_, 2.0f);
    EXPECT_NEAR(y, 1.5f, 1e-6f);
}

TEST_F(MAFTest, ThreeValues_Average)
{
    maf_update(&filter_, 1.0f);
    maf_update(&filter_, 2.0f);
    float y = maf_update(&filter_, 3.0f);
    EXPECT_NEAR(y, 2.0f, 1e-6f);
}

TEST_F(MAFTest, FullWindow_Average)
{
    maf_update(&filter_, 1.0f);
    maf_update(&filter_, 2.0f);
    maf_update(&filter_, 3.0f);
    float y = maf_update(&filter_, 4.0f);
    EXPECT_NEAR(y, 2.5f, 1e-6f);
}

TEST_F(MAFTest, WindowSlides_OldestDropped)
{
    maf_update(&filter_, 1.0f);
    maf_update(&filter_, 2.0f);
    maf_update(&filter_, 3.0f);
    maf_update(&filter_, 4.0f);
    // Window full: [1,2,3,4], now add 0 → [2,3,4,0]
    float y = maf_update(&filter_, 0.0f);
    EXPECT_NEAR(y, 2.25f, 1e-6f);
}

} // namespace cubemot::test
