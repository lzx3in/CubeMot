#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "drivers/led/led.h"
#include "mocks/board_led_mock.h"

using ::testing::_;
using ::testing::IsNull;
using ::testing::NotNull;
using ::testing::Return;

class LedTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        mock_ = &GetBoardLedMock();
        // Initialize a valid LED for tests that need it
        led_valid_.handle = valid_handle_;
    }

    void TearDown() override { mock_ = nullptr; }

    BoardLedMock *mock_;
    // Use a simple non-null pointer as valid handle
    board_led_t valid_handle_ = reinterpret_cast<board_led_t>(0x1234);
    // Pre-initialized valid LED for convenience
    led_t led_valid_;
};

// led_init tests
TEST_F(LedTest, NullLedReturnsError)
{
    led_error_t result = led_init(nullptr, 0);
    EXPECT_EQ(result, LED_ERROR_INVALID_PARAM);
}

TEST_F(LedTest, InvalidLedIdReturnsNotInitialized)
{
    // When get_handle returns NULL, is_valid is called to check the handle
    EXPECT_CALL(*mock_, get_handle(0)).WillOnce(Return(nullptr));
    EXPECT_CALL(*mock_, is_valid(IsNull())).WillOnce(Return(false));

    led_t led;
    led_error_t result = led_init(&led, 0);
    EXPECT_EQ(result, LED_ERROR_NOT_INITIALIZED);
    EXPECT_EQ(led.handle, nullptr);
}

TEST_F(LedTest, ValidLedIdReturnsSuccess)
{
    EXPECT_CALL(*mock_, get_handle(1)).WillOnce(Return(valid_handle_));
    EXPECT_CALL(*mock_, is_valid(valid_handle_)).WillOnce(Return(true));

    led_t led;
    led_error_t result = led_init(&led, 1);
    EXPECT_EQ(result, LED_SUCCESS);
    EXPECT_EQ(led.handle, valid_handle_);
}

// led_set_state tests
TEST_F(LedTest, SetStateNullLedReturnsError)
{
    led_error_t result = led_set_state(nullptr, LED_ON);
    EXPECT_EQ(result, LED_ERROR_INVALID_PARAM);
}

TEST_F(LedTest, SetStateUninitializedLedReturnsError)
{
    led_t led_uninit;
    led_uninit.handle = nullptr;

    EXPECT_CALL(*mock_, is_valid(IsNull())).WillOnce(Return(false));

    led_error_t result = led_set_state(&led_uninit, LED_ON);
    EXPECT_EQ(result, LED_ERROR_NOT_INITIALIZED);
}

TEST_F(LedTest, TurnOnActivatesHardware)
{
    EXPECT_CALL(*mock_, is_valid(valid_handle_)).WillOnce(Return(true));
    EXPECT_CALL(*mock_, set_state(valid_handle_, true)).Times(1);

    led_error_t result = led_set_state(&led_valid_, LED_ON);
    EXPECT_EQ(result, LED_SUCCESS);
}

TEST_F(LedTest, TurnOffDeactivatesHardware)
{
    EXPECT_CALL(*mock_, is_valid(valid_handle_)).WillOnce(Return(true));
    EXPECT_CALL(*mock_, set_state(valid_handle_, false)).Times(1);

    led_error_t result = led_set_state(&led_valid_, LED_OFF);
    EXPECT_EQ(result, LED_SUCCESS);
}

TEST_F(LedTest, SetStateInvalidStateValue)
{
    // Test with an invalid state value (not LED_ON or LED_OFF)
    // The driver does (state == LED_ON), so 99 != 1 results in false
    EXPECT_CALL(*mock_, is_valid(valid_handle_)).WillOnce(Return(true));
    EXPECT_CALL(*mock_, set_state(valid_handle_, false)).Times(1);

    led_error_t result = led_set_state(&led_valid_, (led_state_t)99);
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
    led_t led_uninit;
    led_uninit.handle = nullptr;

    EXPECT_CALL(*mock_, is_valid(IsNull())).WillOnce(Return(false));

    led_error_t result = led_toggle(&led_uninit);
    EXPECT_EQ(result, LED_ERROR_NOT_INITIALIZED);
}

TEST_F(LedTest, ToggleSwitchesHardwareState)
{
    EXPECT_CALL(*mock_, is_valid(valid_handle_)).WillOnce(Return(true));
    EXPECT_CALL(*mock_, toggle(valid_handle_)).Times(1);

    led_error_t result = led_toggle(&led_valid_);
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
    led_error_t result = led_get_state(&led_valid_, nullptr);
    EXPECT_EQ(result, LED_ERROR_INVALID_PARAM);
}

TEST_F(LedTest, GetStateUninitializedLedReturnsError)
{
    led_t led_uninit;
    led_uninit.handle = nullptr;
    led_state_t state;

    EXPECT_CALL(*mock_, is_valid(IsNull())).WillOnce(Return(false));

    led_error_t result = led_get_state(&led_uninit, &state);
    EXPECT_EQ(result, LED_ERROR_NOT_INITIALIZED);
}

TEST_F(LedTest, ReadingStateWhenHardwareIsOnReturnsLedOn)
{
    led_state_t state;

    EXPECT_CALL(*mock_, is_valid(valid_handle_)).WillOnce(Return(true));
    EXPECT_CALL(*mock_, get_state(valid_handle_)).WillOnce(Return(true));

    led_error_t result = led_get_state(&led_valid_, &state);
    EXPECT_EQ(result, LED_SUCCESS);
    EXPECT_EQ(state, LED_ON);
}

TEST_F(LedTest, ReadingStateWhenHardwareIsOffReturnsLedOff)
{
    led_state_t state;

    EXPECT_CALL(*mock_, is_valid(valid_handle_)).WillOnce(Return(true));
    EXPECT_CALL(*mock_, get_state(valid_handle_)).WillOnce(Return(false));

    led_error_t result = led_get_state(&led_valid_, &state);
    EXPECT_EQ(result, LED_SUCCESS);
    EXPECT_EQ(state, LED_OFF);
}

// board_led_get_count test
TEST_F(LedTest, QueryingLedCountReturnsBoardConfiguration)
{
    EXPECT_CALL(*mock_, get_count()).WillOnce(Return(3));

    int count = board_led_get_count();
    EXPECT_EQ(count, 3);
}

// ============================================================================
// Boundary and edge case tests
// ============================================================================

TEST_F(LedTest, InitWithNegativeLedId)
{
    EXPECT_CALL(*mock_, get_handle(-1)).WillOnce(Return(nullptr));
    EXPECT_CALL(*mock_, is_valid(IsNull())).WillOnce(Return(false));

    led_t led;
    led_error_t result = led_init(&led, -1);
    EXPECT_EQ(result, LED_ERROR_NOT_INITIALIZED);
}

TEST_F(LedTest, InitWithLargeLedId)
{
    EXPECT_CALL(*mock_, get_handle(9999)).WillOnce(Return(nullptr));
    EXPECT_CALL(*mock_, is_valid(IsNull())).WillOnce(Return(false));

    led_t led;
    led_error_t result = led_init(&led, 9999);
    EXPECT_EQ(result, LED_ERROR_NOT_INITIALIZED);
}

TEST_F(LedTest, InitSetsHandleEvenWhenInvalid)
{
    // Verify that led->handle is set even when board returns invalid handle
    EXPECT_CALL(*mock_, get_handle(5)).WillOnce(Return(nullptr));
    EXPECT_CALL(*mock_, is_valid(IsNull())).WillOnce(Return(false));

    led_t led;
    // Pre-set handle to non-null to verify it gets overwritten
    led.handle = valid_handle_;
    led_error_t result = led_init(&led, 5);
    EXPECT_EQ(result, LED_ERROR_NOT_INITIALIZED);
    EXPECT_EQ(led.handle, nullptr);
}
