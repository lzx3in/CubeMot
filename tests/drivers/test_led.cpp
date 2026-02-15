#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "drivers/led/led.h"
#include "mocks/board_led_mock.h"

using ::testing::_;
using ::testing::Return;

class LedTest : public ::testing::Test
{
  protected:
    void SetUp() override { mock_ = &GetBoardLedMock(); }

    void TearDown() override { mock_ = nullptr; }

    BoardLedMock *mock_;
    board_led_config_t config_{0, 1};
};

// led_init tests
TEST_F(LedTest, NullLedReturnsError)
{
    led_error_t result = led_init(nullptr, &config_);
    EXPECT_EQ(result, LED_ERROR_INVALID_PARAM);
}

TEST_F(LedTest, NullConfigReturnsError)
{
    led_t led;
    led_error_t result = led_init(&led, nullptr);
    EXPECT_EQ(result, LED_ERROR_INVALID_PARAM);
}

TEST_F(LedTest, ValidParamsReturnsSuccess)
{
    led_t led;
    led_error_t result = led_init(&led, &config_);
    EXPECT_EQ(result, LED_SUCCESS);
    EXPECT_EQ(led.hw_config, &config_);
}

// led_set_state tests
TEST_F(LedTest, SetStateNullLedReturnsError)
{
    led_error_t result = led_set_state(nullptr, LED_ON);
    EXPECT_EQ(result, LED_ERROR_INVALID_PARAM);
}

TEST_F(LedTest, SetStateUninitializedLedReturnsError)
{
    led_t led;
    led.hw_config = nullptr;
    led_error_t result = led_set_state(&led, LED_ON);
    EXPECT_EQ(result, LED_ERROR_NOT_INITIALIZED);
}

TEST_F(LedTest, SetOnCallsBoardLedSetStateWithTrue)
{
    led_t led;
    led_init(&led, &config_);

    EXPECT_CALL(*mock_, set_state(&config_, true)).Times(1);

    led_error_t result = led_set_state(&led, LED_ON);
    EXPECT_EQ(result, LED_SUCCESS);
}

TEST_F(LedTest, SetOffCallsBoardLedSetStateWithFalse)
{
    led_t led;
    led_init(&led, &config_);

    EXPECT_CALL(*mock_, set_state(&config_, false)).Times(1);

    led_error_t result = led_set_state(&led, LED_OFF);
    EXPECT_EQ(result, LED_SUCCESS);
}

// led_toggle tests
TEST_F(LedTest, ToggleNullLedReturnsError)
{
    led_error_t result = led_toggle(nullptr);
    EXPECT_EQ(result, LED_ERROR_INVALID_PARAM);
}

TEST_F(LedTest, ToggleUninitializedLedReturnsError)
{
    led_t led;
    led.hw_config = nullptr;
    led_error_t result = led_toggle(&led);
    EXPECT_EQ(result, LED_ERROR_NOT_INITIALIZED);
}

TEST_F(LedTest, ToggleCallsBoardLedToggle)
{
    led_t led;
    led_init(&led, &config_);

    EXPECT_CALL(*mock_, toggle(&config_)).Times(1);

    led_error_t result = led_toggle(&led);
    EXPECT_EQ(result, LED_SUCCESS);
}

// led_get_state tests
TEST_F(LedTest, GetStateNullLedReturnsError)
{
    led_state_t state;
    led_error_t result = led_get_state(nullptr, &state);
    EXPECT_EQ(result, LED_ERROR_INVALID_PARAM);
}

TEST_F(LedTest, GetStateNullStateReturnsError)
{
    led_t led;
    led_init(&led, &config_);

    led_error_t result = led_get_state(&led, nullptr);
    EXPECT_EQ(result, LED_ERROR_INVALID_PARAM);
}

TEST_F(LedTest, GetStateUninitializedLedReturnsError)
{
    led_t led;
    led.hw_config = nullptr;
    led_state_t state;
    led_error_t result = led_get_state(&led, &state);
    EXPECT_EQ(result, LED_ERROR_NOT_INITIALIZED);
}

TEST_F(LedTest, GetStateTrueReturnsLedOn)
{
    led_t led;
    led_init(&led, &config_);
    led_state_t state;

    EXPECT_CALL(*mock_, get_state(&config_)).WillOnce(Return(true));

    led_error_t result = led_get_state(&led, &state);
    EXPECT_EQ(result, LED_SUCCESS);
    EXPECT_EQ(state, LED_ON);
}

TEST_F(LedTest, GetStateFalseReturnsLedOff)
{
    led_t led;
    led_init(&led, &config_);
    led_state_t state;

    EXPECT_CALL(*mock_, get_state(&config_)).WillOnce(Return(false));

    led_error_t result = led_get_state(&led, &state);
    EXPECT_EQ(result, LED_SUCCESS);
    EXPECT_EQ(state, LED_OFF);
}
