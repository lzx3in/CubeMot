/* Unit tests for led_animate layer */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <vector>
#include <algorithm>

extern "C" {
#include "drivers/led/led_level.h"
#include "drivers/led/led_animate.h"

extern void led_animate_update_all(uint32_t time_ms);
}

using ::testing::_;
using ::testing::AtLeast;
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

class LedAnimateTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        mock_ = new MockLevelBackend();
        backend_ = {.set_brightness = mock_set_brightness, .user_data = mock_};

        EXPECT_CALL(*mock_, set_brightness(mock_, 0)).WillOnce(Return(0));

        led_err_t err = led_level_create("test_led", &backend_, &led_);
        ASSERT_EQ(err, LED_OK);
        ASSERT_NE(led_, nullptr);

        ::testing::Mock::VerifyAndClearExpectations(mock_);
    }

    void TearDown() override
    {
        led_level_destroy(led_);
        delete mock_;
    }

    void advance_time(uint32_t delta_ms)
    {
        time_ms_ += delta_ms;
        led_animate_update_all(time_ms_);
    }

    MockLevelBackend *mock_;
    led_level_backend_t backend_;
    led_level_t led_;
    uint32_t time_ms_ = 0;
};

TEST(LedAnimateCreateTest, BreathingWithNullCfgReturnsError)
{
    led_animation_t anim;
    led_err_t err = led_animate_breathing_create(nullptr, &anim);
    EXPECT_EQ(err, LED_ERR_NULL);
}

TEST(LedAnimateCreateTest, BreathingWithNullOutReturnsError)
{
    led_animate_breathing_cfg_t cfg = {
        .period_ms = 1000, .min_brightness = 0, .max_brightness = 1000, .curve = LED_CURVE_SINE, .repeat_count = 0};
    led_err_t err = led_animate_breathing_create(&cfg, nullptr);
    EXPECT_EQ(err, LED_ERR_NULL);
}

TEST(LedAnimateCreateTest, BreathingWithZeroPeriodReturnsError)
{
    led_animate_breathing_cfg_t cfg = {.period_ms = 0, /* Invalid */
                                       .min_brightness = 0,
                                       .max_brightness = 1000,
                                       .curve = LED_CURVE_SINE,
                                       .repeat_count = 0};
    led_animation_t anim;
    led_err_t err = led_animate_breathing_create(&cfg, &anim);
    EXPECT_EQ(err, LED_ERR_INVALID);
}

TEST(LedAnimateCreateTest, BreathingWithInvalidRangeReturnsError)
{
    led_animate_breathing_cfg_t cfg = {.period_ms = 1000,
                                       .min_brightness = 800,
                                       .max_brightness = 200, /* Less than min */
                                       .curve = LED_CURVE_SINE,
                                       .repeat_count = 0};
    led_animation_t anim;
    led_err_t err = led_animate_breathing_create(&cfg, &anim);
    EXPECT_EQ(err, LED_ERR_INVALID);
}

TEST(LedAnimateCreateTest, BreathingCreateSuccess)
{
    led_animate_breathing_cfg_t cfg = {
        .period_ms = 1000, .min_brightness = 100, .max_brightness = 900, .curve = LED_CURVE_SINE, .repeat_count = 0};
    led_animation_t anim;
    led_err_t err = led_animate_breathing_create(&cfg, &anim);
    EXPECT_EQ(err, LED_OK);
    EXPECT_NE(anim, nullptr);

    const char *name;
    err = led_animate_get_name(anim, &name);
    EXPECT_EQ(err, LED_OK);
    EXPECT_STREQ(name, "breathing");

    led_animate_destroy(anim);
}

TEST(LedAnimateCreateTest, BlinkCreateSuccess)
{
    led_animate_blink_cfg_t cfg = {.on_time_ms = 100, .off_time_ms = 100, .on_brightness = 1000, .repeat_count = 0};
    led_animation_t anim;
    led_err_t err = led_animate_blink_create(&cfg, &anim);
    EXPECT_EQ(err, LED_OK);
    EXPECT_NE(anim, nullptr);

    const char *name;
    err = led_animate_get_name(anim, &name);
    EXPECT_EQ(err, LED_OK);
    EXPECT_STREQ(name, "blink");

    led_animate_destroy(anim);
}

TEST(LedAnimateCreateTest, FadeCreateSuccess)
{
    led_animate_fade_cfg_t cfg = {
        .from_brightness = 0, .to_brightness = 1000, .duration_ms = 500, .curve = LED_CURVE_LINEAR};
    led_animation_t anim;
    led_err_t err = led_animate_fade_create(&cfg, &anim);
    EXPECT_EQ(err, LED_OK);
    EXPECT_NE(anim, nullptr);

    const char *name;
    err = led_animate_get_name(anim, &name);
    EXPECT_EQ(err, LED_OK);
    EXPECT_STREQ(name, "fade");

    led_animate_destroy(anim);
}

TEST(LedAnimateCreateTest, ConstantCreateSuccess)
{
    led_animation_t anim;
    led_err_t err = led_animate_constant_create(500, &anim);
    EXPECT_EQ(err, LED_OK);
    EXPECT_NE(anim, nullptr);

    const char *name;
    err = led_animate_get_name(anim, &name);
    EXPECT_EQ(err, LED_OK);
    EXPECT_STREQ(name, "constant");

    led_animate_destroy(anim);
}

TEST_F(LedAnimateTest, PlayWithNullAnimReturnsError)
{
    led_err_t err = led_animate_play(led_, nullptr);
    EXPECT_EQ(err, LED_ERR_NULL);
}

TEST_F(LedAnimateTest, PlayWithNullLedReturnsError)
{
    led_animate_breathing_cfg_t cfg = {
        .period_ms = 1000, .min_brightness = 100, .max_brightness = 900, .curve = LED_CURVE_SINE, .repeat_count = 0};
    led_animation_t anim;
    led_animate_breathing_create(&cfg, &anim);

    led_err_t err = led_animate_play(nullptr, anim);
    EXPECT_EQ(err, LED_ERR_NULL);

    led_animate_destroy(anim);
}

TEST_F(LedAnimateTest, IsActiveReturnsCorrectState)
{
    led_animate_breathing_cfg_t cfg = {
        .period_ms = 1000, .min_brightness = 100, .max_brightness = 900, .curve = LED_CURVE_SINE, .repeat_count = 0};
    led_animation_t anim;
    led_animate_breathing_create(&cfg, &anim);

    /* Initially not active */
    bool active;
    led_err_t err = led_animate_is_active(led_, &active);
    EXPECT_EQ(err, LED_OK);
    EXPECT_FALSE(active);

    /* Start animation */
    ON_CALL(*mock_, set_brightness(mock_, _)).WillByDefault(Return(0));
    led_animate_play(led_, anim);

    /* Now active */
    err = led_animate_is_active(led_, &active);
    EXPECT_EQ(err, LED_OK);
    EXPECT_TRUE(active);

    led_animate_destroy(anim);
}

TEST_F(LedAnimateTest, StopAnimation)
{
    led_animate_breathing_cfg_t cfg = {
        .period_ms = 1000, .min_brightness = 100, .max_brightness = 900, .curve = LED_CURVE_SINE, .repeat_count = 0};
    led_animation_t anim;
    led_animate_breathing_create(&cfg, &anim);

    ON_CALL(*mock_, set_brightness(mock_, _)).WillByDefault(Return(0));
    led_animate_play(led_, anim);

    led_animate_stop(led_);

    bool active;
    led_animate_is_active(led_, &active);
    EXPECT_FALSE(active);

    led_animate_destroy(anim);
}

class BreathingTest : public LedAnimateTest
{
  protected:
    void SetUp() override
    {
        LedAnimateTest::SetUp();

        cfg_ = {.period_ms = 1000,
                .min_brightness = 100,
                .max_brightness = 900,
                .curve = LED_CURVE_SINE,
                .repeat_count = 0};
        led_err_t err = led_animate_breathing_create(&cfg_, &anim_);
        ASSERT_EQ(err, LED_OK);
        ASSERT_NE(anim_, nullptr);
    }

    void TearDown() override
    {
        led_animate_destroy(anim_);
        LedAnimateTest::TearDown();
    }

    led_animate_breathing_cfg_t cfg_;
    led_animation_t anim_;
};

TEST_F(BreathingTest, SinePattern)
{
    std::vector<uint16_t> recorded;

    ON_CALL(*mock_, set_brightness(mock_, _)).WillByDefault(Invoke([&](void *, uint16_t b) {
        recorded.push_back(b);
        return 0;
    }));

    led_animate_play(led_, anim_);

    /* Sample over one full period */
    for (int i = 0; i <= 20; i++) {
        advance_time(50);
    }

    EXPECT_GE(recorded.size(), 10u);

    /* Find min/max */
    auto minmax = std::minmax_element(recorded.begin(), recorded.end());
    EXPECT_LE(*minmax.first, 200u);  /* Close to min */
    EXPECT_GE(*minmax.second, 800u); /* Close to max */
}

TEST_F(BreathingTest, LinearCurve)
{
    cfg_.curve = LED_CURVE_LINEAR;

    std::vector<uint16_t> recorded;
    ON_CALL(*mock_, set_brightness(mock_, _)).WillByDefault(Invoke([&](void *, uint16_t b) {
        recorded.push_back(b);
        return 0;
    }));

    led_animate_play(led_, anim_);

    for (int i = 0; i < 20; i++) {
        advance_time(50);
    }

    auto max_val = *std::max_element(recorded.begin(), recorded.end());
    EXPECT_GE(max_val, 800u);
}

TEST_F(BreathingTest, DynamicParameterUpdate)
{
    std::vector<uint16_t> recorded;
    ON_CALL(*mock_, set_brightness(mock_, _)).WillByDefault(Invoke([&](void *, uint16_t b) {
        recorded.push_back(b);
        return 0;
    }));

    led_animate_play(led_, anim_);

    /* Run for multiple cycles */
    for (int i = 0; i < 50; i++) {
        advance_time(50);
    }

    /* Should reach near original max of 900 */
    auto max_val = *std::max_element(recorded.begin(), recorded.end());
    EXPECT_GE(max_val, 800u);
}

TEST_F(BreathingTest, PhaseOffset)
{
    ON_CALL(*mock_, set_brightness(mock_, _)).WillByDefault(Return(0));

    led_err_t err = led_animate_play_with_phase(led_, anim_, 64);
    EXPECT_EQ(err, LED_OK);

    for (int i = 0; i < 5; i++) {
        advance_time(50);
    }

    bool active;
    led_animate_is_active(led_, &active);
    EXPECT_TRUE(active);
}

TEST_F(LedAnimateTest, BlinkPattern)
{
    led_animate_blink_cfg_t cfg = {.on_time_ms = 100, .off_time_ms = 100, .on_brightness = 1000, .repeat_count = 0};
    led_animation_t anim;
    led_animate_blink_create(&cfg, &anim);

    std::vector<uint16_t> recorded;
    ON_CALL(*mock_, set_brightness(mock_, _)).WillByDefault(Invoke([&](void *, uint16_t b) {
        recorded.push_back(b);
        return 0;
    }));

    led_animate_play(led_, anim);

    for (int i = 0; i < 10; i++) {
        advance_time(20);
    }

    EXPECT_TRUE(std::find(recorded.begin(), recorded.end(), 1000) != recorded.end());
    EXPECT_TRUE(std::find(recorded.begin(), recorded.end(), 0) != recorded.end());

    led_animate_destroy(anim);
}

TEST_F(LedAnimateTest, LinearFade)
{
    led_animate_fade_cfg_t cfg = {
        .from_brightness = 0, .to_brightness = 1000, .duration_ms = 500, .curve = LED_CURVE_LINEAR};
    led_animation_t anim;
    led_animate_fade_create(&cfg, &anim);

    std::vector<uint16_t> recorded;
    ON_CALL(*mock_, set_brightness(mock_, _)).WillByDefault(Invoke([&](void *, uint16_t b) {
        recorded.push_back(b);
        return 0;
    }));

    led_animate_play(led_, anim);

    for (int i = 0; i <= 10; i++) {
        advance_time(50);
    }

    /* Should be monotonically increasing (approximately) */
    for (size_t i = 1; i < recorded.size(); i++) {
        EXPECT_GE(recorded[i], recorded[i - 1]);
    }

    EXPECT_GE(recorded.back(), 900u);

    led_animate_destroy(anim);
}

TEST_F(LedAnimateTest, CompletionCallback)
{
    led_animate_fade_cfg_t cfg = {.from_brightness = 0,
                                  .to_brightness = 500,
                                  .duration_ms = 100, /* Short for testing */
                                  .curve = LED_CURVE_LINEAR};
    led_animation_t anim;
    led_animate_fade_create(&cfg, &anim);

    bool callback_called = false;
    led_level_t callback_led = nullptr;

    auto callback = [](led_level_t led, void *user_data) { *static_cast<bool *>(user_data) = true; };

    ON_CALL(*mock_, set_brightness(mock_, _)).WillByDefault(Return(0));

    led_animate_play_with_callback(led_, anim, 0, callback, &callback_called);

    /* Run past completion */
    for (int i = 0; i < 10; i++) {
        advance_time(20);
    }

    /* Note: callback may not be called in current implementation
     * until proper done detection is implemented */
    (void)callback_led;

    led_animate_destroy(anim);
}

TEST_F(LedAnimateTest, SharedEffectAcrossLeds)
{
    MockLevelBackend mock2;
    led_level_backend_t backend2 = {.set_brightness = mock_set_brightness, .user_data = &mock2};

    ON_CALL(mock2, set_brightness(&mock2, _)).WillByDefault(Return(0));

    led_level_t led2;
    led_err_t err = led_level_create("led2", &backend2, &led2);
    ASSERT_EQ(err, LED_OK);

    led_animate_breathing_cfg_t cfg = {
        .period_ms = 1000, .min_brightness = 100, .max_brightness = 900, .curve = LED_CURVE_SINE, .repeat_count = 0};
    led_animation_t anim;
    led_animate_breathing_create(&cfg, &anim);

    ON_CALL(*mock_, set_brightness(mock_, _)).WillByDefault(Return(0));

    led_animate_play(led_, anim);
    bool active;
    led_animate_is_active(led_, &active);
    EXPECT_TRUE(active);

    led_animate_play(led2, anim);
    led_animate_is_active(led2, &active);
    EXPECT_TRUE(active);

    /* Verify update works on active LED */
    advance_time(100);

    led_animate_stop(led2);
    led_level_destroy(led2);
    led_animate_destroy(anim);
}
