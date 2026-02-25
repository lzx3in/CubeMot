#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "mocks/board_led_mock.h"

extern "C" {
#include "drivers/led/led.h"
}

using ::testing::_;
using ::testing::Return;

class LedTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        mock_ = &GetBoardLedMock();
    }

    void TearDown() override
    {
        // Mock will be destroyed after each test
    }

    // Helper: Expect a valid handle to be returned for given ID
    void ExpectValidHandle(int id, board_led_t handle)
    {
        EXPECT_CALL(*mock_, get_handle(id)).WillOnce(Return(handle));
        EXPECT_CALL(*mock_, is_valid(handle)).WillRepeatedly(Return(true));
    }

    // Helper: Expect an invalid handle (null or is_valid returns false)
    void ExpectInvalidHandle(int id)
    {
        EXPECT_CALL(*mock_, get_handle(id)).WillOnce(Return(nullptr));
        EXPECT_CALL(*mock_, is_valid(nullptr)).WillOnce(Return(false));
    }

    // Helper: Setup a valid LED instance for tests that need one
    led_err_t SetupLed(led_t *led, int id, board_led_t handle)
    {
        ExpectValidHandle(id, handle);
        return led_init(led, id);
    }

    BoardLedMock *mock_;
};

TEST_F(LedTest, InitWithNullPointerReturnsError)
{
    led_err_t err = led_init(nullptr, 0);
    EXPECT_EQ(err, LED_ERR_NULL);
}

TEST_F(LedTest, InitWithInvalidIdReturnsError)
{
    ExpectInvalidHandle(99);

    led_t led;
    led_err_t err = led_init(&led, 99);
    EXPECT_EQ(err, LED_ERR_INVALID);
}

TEST_F(LedTest, InitSuccess)
{
    board_led_t dummy_handle = (board_led_t)0x1234;
    ExpectValidHandle(0, dummy_handle);

    led_t led;
    led_err_t err = led_init(&led, 0);
    EXPECT_EQ(err, LED_OK);
}

TEST_F(LedTest, SetOnTurnsLedOn)
{
    board_led_t dummy_handle = (board_led_t)0x1234;
    led_t led;
    ASSERT_EQ(SetupLed(&led, 0, dummy_handle), LED_OK);

    EXPECT_CALL(*mock_, set_state(dummy_handle, true));

    led_err_t err = led_set(&led, true);
    EXPECT_EQ(err, LED_OK);
}

TEST_F(LedTest, SetOffTurnsLedOff)
{
    board_led_t dummy_handle = (board_led_t)0x1234;
    led_t led;
    ASSERT_EQ(SetupLed(&led, 0, dummy_handle), LED_OK);

    EXPECT_CALL(*mock_, set_state(dummy_handle, false));

    led_err_t err = led_set(&led, false);
    EXPECT_EQ(err, LED_OK);
}

TEST_F(LedTest, SetWithNullPointerReturnsError)
{
    led_err_t err = led_set(nullptr, true);
    EXPECT_EQ(err, LED_ERR_NULL);
}

TEST_F(LedTest, GetReturnsCurrentStateOn)
{
    board_led_t dummy_handle = (board_led_t)0x1234;
    led_t led;
    ASSERT_EQ(SetupLed(&led, 0, dummy_handle), LED_OK);

    EXPECT_CALL(*mock_, get_state(dummy_handle)).WillOnce(Return(true));

    bool state;
    led_err_t err = led_get(&led, &state);
    EXPECT_EQ(err, LED_OK);
    EXPECT_TRUE(state);
}

TEST_F(LedTest, GetReturnsCurrentStateOff)
{
    board_led_t dummy_handle = (board_led_t)0x1234;
    led_t led;
    ASSERT_EQ(SetupLed(&led, 0, dummy_handle), LED_OK);

    EXPECT_CALL(*mock_, get_state(dummy_handle)).WillOnce(Return(false));

    bool state;
    led_err_t err = led_get(&led, &state);
    EXPECT_EQ(err, LED_OK);
    EXPECT_FALSE(state);
}

TEST_F(LedTest, GetWithNullLedPointerReturnsError)
{
    bool state;
    led_err_t err = led_get(nullptr, &state);
    EXPECT_EQ(err, LED_ERR_NULL);
}

TEST_F(LedTest, GetWithNullStatePointerReturnsError)
{
    board_led_t dummy_handle = (board_led_t)0x1234;
    led_t led;
    ASSERT_EQ(SetupLed(&led, 0, dummy_handle), LED_OK);

    led_err_t err = led_get(&led, nullptr);
    EXPECT_EQ(err, LED_ERR_NULL);
}

TEST_F(LedTest, ToggleChangesState)
{
    board_led_t dummy_handle = (board_led_t)0x1234;
    led_t led;
    ASSERT_EQ(SetupLed(&led, 0, dummy_handle), LED_OK);

    EXPECT_CALL(*mock_, toggle(dummy_handle));

    led_err_t err = led_toggle(&led);
    EXPECT_EQ(err, LED_OK);
}

TEST_F(LedTest, ToggleWithNullPointerReturnsError)
{
    led_err_t err = led_toggle(nullptr);
    EXPECT_EQ(err, LED_ERR_NULL);
}

TEST_F(LedTest, MultipleLedsCanBeControlledIndependently)
{
    board_led_t handle1 = (board_led_t)0x1000;
    board_led_t handle2 = (board_led_t)0x2000;

    ExpectValidHandle(0, handle1);
    led_t led1;
    EXPECT_EQ(led_init(&led1, 0), LED_OK);

    ExpectValidHandle(1, handle2);
    led_t led2;
    EXPECT_EQ(led_init(&led2, 1), LED_OK);

    EXPECT_CALL(*mock_, set_state(handle1, true));
    EXPECT_CALL(*mock_, set_state(handle2, false));

    EXPECT_EQ(led_set(&led1, true), LED_OK);
    EXPECT_EQ(led_set(&led2, false), LED_OK);
}

TEST_F(LedTest, MultipleLedsGetStateIndependently)
{
    board_led_t handle1 = (board_led_t)0x1000;
    board_led_t handle2 = (board_led_t)0x2000;

    led_t led1, led2;
    ASSERT_EQ(SetupLed(&led1, 0, handle1), LED_OK);
    ASSERT_EQ(SetupLed(&led2, 1, handle2), LED_OK);

    EXPECT_CALL(*mock_, get_state(handle1)).WillOnce(Return(true));
    EXPECT_CALL(*mock_, get_state(handle2)).WillOnce(Return(false));

    bool state1, state2;
    EXPECT_EQ(led_get(&led1, &state1), LED_OK);
    EXPECT_EQ(led_get(&led2, &state2), LED_OK);

    EXPECT_TRUE(state1);
    EXPECT_FALSE(state2);
}

TEST_F(LedTest, OperationsFailWithUninitializedLed)
{
    led_t uninitialized_led = {nullptr};

    EXPECT_EQ(led_set(&uninitialized_led, true), LED_ERR_INVALID);
    EXPECT_EQ(led_get(&uninitialized_led, nullptr), LED_ERR_NULL);

    bool state;
    EXPECT_EQ(led_get(&uninitialized_led, &state), LED_ERR_INVALID);
    EXPECT_EQ(led_toggle(&uninitialized_led), LED_ERR_INVALID);
}
