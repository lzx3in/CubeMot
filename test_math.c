/**
 * @file test_math.c
 * @brief Unit tests for FOC math library (host-native build)
 *
 * Build: gcc -o test_math test_math.c -lm && ./test_math
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>

/* ── Include math lib headers (copy definitions) ──── */
#include "transform.h"
#include "svpwm.h"
#include "filter.h"

#define TEST_PASS(fmt, ...) printf("  ✅ " fmt "\n", ##__VA_ARGS__)
#define TEST_FAIL(fmt, ...) do { printf("  ❌ " fmt "\n", ##__VA_ARGS__); exit(1); } while(0)

static int tests_run = 0;
static int tests_passed = 0;

#define ASSERT_FLOAT_EQ(val, expected, tolerance, label) do { \
    tests_run++; \
    float diff = fabsf((val) - (expected)); \
    if (diff <= (tolerance)) { \
        tests_passed++; \
        TEST_PASS("%s = %.6f (expected %.6f, diff %.6e)", label, (double)(val), (double)(expected), (double)diff); \
    } else { \
        TEST_FAIL("%s = %.6f, expected %.6f, diff %.6e > tol %.6e", label, (double)(val), (double)(expected), (double)diff, (double)(tolerance)); \
    } \
} while(0)

/* ── Clarke Transform Tests ────────────────────────── */

static void test_clarke_balanced(void)
{
    printf("\n── Clarke Transform ──\n");

    /* Balanced 3-phase: Ia=1, Ib=-0.5, Ic=-0.5 */
    alphabeta_t ab = clarke_transform(1.0f, -0.5f, -0.5f);
    ASSERT_FLOAT_EQ(ab.alpha, 1.0f, 1e-6f, "Iα (balanced)");
    ASSERT_FLOAT_EQ(ab.beta,  0.0f, 1e-6f, "Iβ (balanced)");

    /* Ia=0, Ib=1, Ic=-1: pure β component */
    ab = clarke_transform(0.0f, 1.0f, -1.0f);
    ASSERT_FLOAT_EQ(ab.alpha, 0.0f, 1e-6f, "Iα (β-only)");
    ASSERT_FLOAT_EQ(ab.beta, 2.0f / sqrtf(3.0f), 1e-6f, "Iβ (β-only)");

    /* Zero current */
    ab = clarke_transform(0.0f, 0.0f, 0.0f);
    ASSERT_FLOAT_EQ(ab.alpha, 0.0f, 1e-6f, "Iα (zero)");
    ASSERT_FLOAT_EQ(ab.beta,  0.0f, 1e-6f, "Iβ (zero)");
}

/* ── Inverse Clarke Tests ──────────────────────────── */

static void test_iclarke(void)
{
    printf("\n── Inverse Clarke Transform ──\n");

    /* α=1, β=0 → a=1, b=-0.5, c=-0.5 */
    abc_t abc = iclarke_transform(1.0f, 0.0f);
    ASSERT_FLOAT_EQ(abc.a,  1.0f, 1e-6f, "Va (α-only)");
    ASSERT_FLOAT_EQ(abc.b, -0.5f, 1e-6f, "Vb (α-only)");
    ASSERT_FLOAT_EQ(abc.c, -0.5f, 1e-6f, "Vc (α-only)");

    /* α=0, β=1 → a=0, b=√3/2, c=-√3/2 */
    abc = iclarke_transform(0.0f, 1.0f);
    ASSERT_FLOAT_EQ(abc.a, 0.0f, 1e-6f, "Va (β-only)");
    ASSERT_FLOAT_EQ(abc.b, 0.8660254f, 1e-4f, "Vb (β-only)");
    ASSERT_FLOAT_EQ(abc.c, -0.8660254f, 1e-4f, "Vc (β-only)");
}

/* ── Park Transform Tests ──────────────────────────── */

static void test_park(void)
{
    printf("\n── Park Transform ──\n");

    /* θ=0: Id=Iα, Iq=Iβ */
    dq_t dq = park_transform(1.0f, 0.5f, 0.0f, 1.0f);
    ASSERT_FLOAT_EQ(dq.d, 1.0f, 1e-6f, "Id (θ=0)");
    ASSERT_FLOAT_EQ(dq.q, 0.5f, 1e-6f, "Iq (θ=0)");

    /* θ=90°: Id=Iβ, Iq=-Iα */
    dq = park_transform(1.0f, 0.5f, 1.0f, 0.0f);
    ASSERT_FLOAT_EQ(dq.d,  0.5f, 1e-6f, "Id (θ=90°)");
    ASSERT_FLOAT_EQ(dq.q, -1.0f, 1e-6f, "Iq (θ=90°)");
}

/* ── Inverse Park Tests ────────────────────────────── */

static void test_ipark(void)
{
    printf("\n── Inverse Park Transform ──\n");

    /* θ=0: Vα=Vd, Vβ=Vq */
    alphabeta_t ab = ipark_transform(1.0f, 0.5f, 0.0f, 1.0f);
    ASSERT_FLOAT_EQ(ab.alpha, 1.0f, 1e-6f, "Vα (θ=0)");
    ASSERT_FLOAT_EQ(ab.beta,  0.5f, 1e-6f, "Vβ (θ=0)");

    /* θ=90°: Vα=-Vq, Vβ=Vd */
    ab = ipark_transform(1.0f, 0.5f, 1.0f, 0.0f);
    ASSERT_FLOAT_EQ(ab.alpha, -0.5f, 1e-6f, "Vα (θ=90°)");
    ASSERT_FLOAT_EQ(ab.beta,   1.0f, 1e-6f, "Vβ (θ=90°)");
}

/* ── Round-trip Tests ──────────────────────────────── */

static void test_roundtrip(void)
{
    printf("\n── Round-trip: Clarke→Park→iPark→iClarke ──\n");

    /* Test with arbitrary currents and angle */
    float Ia = 0.8f, Ib = -0.3f, Ic = -0.5f; // balanced
    float theta = 1.2f; // ~69°

    float sin_t = sinf(theta);
    float cos_t = cosf(theta);

    alphabeta_t ab = clarke_transform(Ia, Ib, Ic);
    dq_t dq = park_transform(ab.alpha, ab.beta, sin_t, cos_t);
    alphabeta_t ab2 = ipark_transform(dq.d, dq.q, sin_t, cos_t);

    ASSERT_FLOAT_EQ(ab2.alpha, ab.alpha, 1e-5f, "α roundtrip");
    ASSERT_FLOAT_EQ(ab2.beta,  ab.beta,  1e-5f, "β roundtrip");
}

/* ── SVPWM Tests ───────────────────────────────────── */

static void test_svpwm(void)
{
    printf("\n── SVPWM ──\n");

    svpwm_duty_t duty;
    float vbus = 13.0f;

    /* Zero vector: all phases at 50% duty */
    svpwm_minmax(0.0f, 0.0f, vbus, &duty);
    ASSERT_FLOAT_EQ(duty.duty_a, 0.5f, 1e-4f, "duty_a (zero)");
    ASSERT_FLOAT_EQ(duty.duty_b, 0.5f, 1e-4f, "duty_b (zero)");
    ASSERT_FLOAT_EQ(duty.duty_c, 0.5f, 1e-4f, "duty_c (zero)");

    /* Alpha-axis positive → duty_a > 0.5 */
    svpwm_minmax(1.0f, 0.0f, vbus, &duty);
    ASSERT_FLOAT_EQ(duty.duty_a, 0.5577f, 1e-3f, "duty_a (Vα=1)");
    /* duty_a should be the highest */
    if (duty.duty_a > duty.duty_b && duty.duty_a > duty.duty_c) {
        TEST_PASS("Phase A duty dominates for Vα>0 (%.3f > %.3f, %.3f)",
                  (double)duty.duty_a, (double)duty.duty_b, (double)duty.duty_c);
        tests_run++; tests_passed++;
    } else {
        TEST_FAIL("Phase A should dominate: a=%.3f b=%.3f c=%.3f",
                  (double)duty.duty_a, (double)duty.duty_b, (double)duty.duty_c);
    }

    /* Clamp test */
    svpwm_minmax(10.0f, 10.0f, vbus, &duty);
    svpwm_clamp(&duty);
    if (duty.duty_a >= 0.0f && duty.duty_a <= 1.0f &&
        duty.duty_b >= 0.0f && duty.duty_b <= 1.0f &&
        duty.duty_c >= 0.0f && duty.duty_c <= 1.0f) {
        TEST_PASS("SVPWM clamp: all duties in [0,1] (a=%.3f b=%.3f c=%.3f)",
                  (double)duty.duty_a, (double)duty.duty_b, (double)duty.duty_c);
        tests_run++; tests_passed++;
    } else {
        TEST_FAIL("SVPWM clamp failed: a=%.3f b=%.3f c=%.3f",
                  (double)duty.duty_a, (double)duty.duty_b, (double)duty.duty_c);
    }
}

/* ── Filter Tests ──────────────────────────────────── */

static void test_lpf(void)
{
    printf("\n── Low-Pass Filter ──\n");

    lpf_t f;
    lpf_init(&f, 10.0f, 1000.0f); // 10Hz cutoff, 1kHz sample

    /* First sample */
    float y = lpf_update(&f, 1.0f);
    /* alpha ≈ 2π·10/(2π·10 + 1000) ≈ 0.0591 */
    ASSERT_FLOAT_EQ(y, 0.0591f, 0.01f, "LPF first sample");

    /* Converge after many samples */
    for (int i = 0; i < 100; i++) {
        lpf_update(&f, 1.0f);
    }
    y = lpf_update(&f, 1.0f);
    ASSERT_FLOAT_EQ(y, 1.0f, 0.01f, "LPF converged to DC");
}

static void test_maf(void)
{
    printf("\n── Moving Average Filter ──\n");

    maf_t m;
    maf_init(&m, 4);

    float y = maf_update(&m, 1.0f);
    ASSERT_FLOAT_EQ(y, 1.0f, 1e-6f, "MAF[0]: single value");

    y = maf_update(&m, 2.0f);
    ASSERT_FLOAT_EQ(y, 1.5f, 1e-6f, "MAF[1]: (1+2)/2");

    y = maf_update(&m, 3.0f);
    ASSERT_FLOAT_EQ(y, 2.0f, 1e-6f, "MAF[2]: (1+2+3)/3");

    y = maf_update(&m, 4.0f);
    ASSERT_FLOAT_EQ(y, 2.5f, 1e-6f, "MAF[3]: (1+2+3+4)/4");

    y = maf_update(&m, 0.0f);
    ASSERT_FLOAT_EQ(y, 2.25f, 1e-6f, "MAF[4]: (2+3+4+0)/4");
}

/* ── Main ──────────────────────────────────────────── */

int main(void)
{
    printf("╔══════════════════════════════════════════╗\n");
    printf("║   CubeMot FOC Math Unit Tests           ║\n");
    printf("╚══════════════════════════════════════════╝\n");

    test_clarke_balanced();
    test_iclarke();
    test_park();
    test_ipark();
    test_roundtrip();
    test_svpwm();
    test_lpf();
    test_maf();

    printf("\n═══════════════════════════════════════════\n");
    printf("  Results: %d/%d passed\n", tests_passed, tests_run);
    printf("═══════════════════════════════════════════\n");

    if (tests_passed == tests_run) {
        printf("  ✅ ALL TESTS PASSED\n\n");
        return 0;
    } else {
        printf("  ❌ %d FAILED\n\n", tests_run - tests_passed);
        return 1;
    }
}
