/* Unit tests for led_level layer */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <vector>
#include <algorithm>

extern "C" {
#include "drivers/led/led_level.h"

extern led_err_t led_level_set_mode(led_level_t led, bool animating);
}

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

class MockLevelBackend
{
  public:
    MOCK_METHOD(int, set_brightness, (void *user_data, uint16_t brightness));
};

static int mock_set_brightness(void *user_data, uint16_t brightness)
{
    return static_cast<MockLevelBackend *>(user_data)->set_brightness(user_data, brightness);
}

class LedLevelTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        mock_ = new MockLevelBackend();
        backend_ = {.set_brightness = mock_set_brightness, .user_data = mock_};

        EXPECT_CALL(*mock_, set_brightness(mock_, 0)).WillOnce(Return(0));

        led_err_t err = led_level_create(&backend_, &led_);
        ASSERT_EQ(err, LED_OK);
        ASSERT_NE(led_, nullptr);

        ::testing::Mock::VerifyAndClearExpectations(mock_);
    }

    void TearDown() override
    {
        led_level_destroy(led_);
        delete mock_;
    }

    MockLevelBackend *mock_;
    led_level_backend_t backend_;
    led_level_t led_;
};

TEST(LedLevelCreateTest, CreateWithNullOutParamReturnsError)
{
    led_err_t err = led_level_create(nullptr, nullptr);
    EXPECT_EQ(err, LED_ERR_NULL);
}

TEST(LedLevelCreateTest, CreateWithDelayedBackend)
{
    MockLevelBackend mock;

    led_level_t led;
    led_err_t err = led_level_create(nullptr, &led);
    EXPECT_EQ(err, LED_OK);
    EXPECT_NE(led, nullptr);

    led_level_backend_t backend = {.set_brightness = mock_set_brightness, .user_data = &mock};
    EXPECT_CALL(mock, set_brightness(&mock, 0)).WillOnce(Return(0));

    err = led_level_register_backend(led, &backend);
    EXPECT_EQ(err, LED_OK);

    led_level_destroy(led);
}

TEST_F(LedLevelTest, DestroyInvalidHandleReturnsError)
{
    led_err_t err = led_level_destroy(nullptr);
    EXPECT_EQ(err, LED_ERR_NOT_FOUND);
}

TEST_F(LedLevelTest, DestroyWhenBusyReturnsError)
{
    extern led_err_t led_level_set_mode(led_level_t led, bool animating);
    led_level_set_mode(led_, true);

    led_err_t err = led_level_destroy(led_);
    EXPECT_EQ(err, LED_ERR_BUSY);

    led_level_set_mode(led_, false);
}

TEST_F(LedLevelTest, SetBrightnessSetsValue)
{
    EXPECT_CALL(*mock_, set_brightness(mock_, 500)).WillOnce(Return(0));

    led_err_t err = led_level_set(led_, 500);
    EXPECT_EQ(err, LED_OK);

    uint16_t value;
    err = led_level_get(led_, &value);
    EXPECT_EQ(err, LED_OK);
    EXPECT_EQ(value, 500);
}

TEST_F(LedLevelTest, SetBrightnessMaxValue)
{
    EXPECT_CALL(*mock_, set_brightness(mock_, 1000)).WillOnce(Return(0));

    led_err_t err = led_level_set(led_, 1000);
    EXPECT_EQ(err, LED_OK);
}

TEST_F(LedLevelTest, SetBrightnessOutOfRangeReturnsError)
{
    led_err_t err = led_level_set(led_, 1001);
    EXPECT_EQ(err, LED_ERR_INVALID);

    err = led_level_set(led_, 2000);
    EXPECT_EQ(err, LED_ERR_INVALID);
}

TEST_F(LedLevelTest, SetBrightnessWithNullHandleReturnsError)
{
    led_err_t err = led_level_set(nullptr, 500);
    EXPECT_EQ(err, LED_ERR_NOT_FOUND);
}

TEST_F(LedLevelTest, SetBrightnessWithoutBackendReturnsError)
{
    led_level_t led;
    led_err_t err = led_level_create(nullptr, &led);
    ASSERT_EQ(err, LED_OK);

    err = led_level_set(led, 500);
    EXPECT_EQ(err, LED_ERR_INVALID);

    led_level_destroy(led);
}

TEST_F(LedLevelTest, BackendErrorPropagates)
{
    EXPECT_CALL(*mock_, set_brightness(mock_, 500)).WillOnce(Return(-1));

    led_err_t err = led_level_set(led_, 500);
    EXPECT_EQ(err, LED_ERR_BACKEND);
}

TEST_F(LedLevelTest, GetBrightnessWithNullOutputReturnsError)
{
    led_err_t err = led_level_get(led_, nullptr);
    EXPECT_EQ(err, LED_ERR_NULL);
}

TEST_F(LedLevelTest, IsActiveReturnsFalseInitially)
{
    bool active;
    led_err_t err = led_level_is_active(led_, &active);
    EXPECT_EQ(err, LED_OK);
    EXPECT_FALSE(active);
}

TEST_F(LedLevelTest, IsActiveWithNullOutputReturnsError)
{
    led_err_t err = led_level_is_active(led_, nullptr);
    EXPECT_EQ(err, LED_ERR_NULL);
}

TEST(LedLevelCurveTest, SineTableValues)
{
    /* Sine(0) should be ~500 (midpoint) */
    EXPECT_EQ(led_level_curve_sine(0), 500);

    /* Sine(64) should be ~1000 (peak) */
    EXPECT_GE(led_level_curve_sine(64), 990u);

    /* Sine(192) should be ~0 (trough) */
    EXPECT_LE(led_level_curve_sine(192), 10u);

    /* Sine(128) should be ~500 (180 degrees) */
    EXPECT_NEAR(led_level_curve_sine(128), 500, 20);
}

TEST(LedLevelCurveTest, GammaValues)
{
    /* Gamma(0) = 0 */
    EXPECT_EQ(led_level_curve_gamma(0), 0);

    /* Gamma(1000) = 1000 */
    EXPECT_EQ(led_level_curve_gamma(1000), 1000);

    /* Gamma(500) should be less than 500 (darker due to gamma) */
    EXPECT_LT(led_level_curve_gamma(500), 500u);

    /* Gamma is monotonically increasing */
    for (int i = 10; i <= 1000; i += 10) {
        EXPECT_GE(led_level_curve_gamma(i), led_level_curve_gamma(i - 10));
    }
}

TEST(LedLevelCurveTest, ApplyCurveLinear)
{
    EXPECT_EQ(led_level_curve_apply(500, LED_CURVE_LINEAR), 500);
    EXPECT_EQ(led_level_curve_apply(0, LED_CURVE_LINEAR), 0);
    EXPECT_EQ(led_level_curve_apply(1000, LED_CURVE_LINEAR), 1000);
}

TEST(LedLevelCurveTest, ApplyCurveSine)
{
    /* Sine maps value as phase */
    uint16_t result = led_level_curve_apply(0, LED_CURVE_SINE);
    EXPECT_EQ(result, 500); /* sin(0) mapped to 500 */
}

TEST(LedLevelCurveTest, ApplyCurveGammaDarkensMidtones)
{
    uint16_t result = led_level_curve_apply(500, LED_CURVE_GAMMA);
    EXPECT_LT(result, 500u);
}

TEST(LedLevelCurveTest, ApplyCurveClampsOverflow)
{
    uint16_t result = led_level_curve_apply(1500, LED_CURVE_LINEAR);
    EXPECT_EQ(result, 1000u);
}
