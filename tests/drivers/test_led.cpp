#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "mocks/board/led_mock.h"

extern "C" {
#include "drivers/led/led.h"
#include "drivers/led/led_msg.h"
#include "msghub/msghub.h"
}

using ::testing::_;
using ::testing::Return;

class LedDriverTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        mock_ = &GetBoardLedMock();
    }

    void TearDown() override
    {
        // Cleanup handled by mock reset between tests
    }

    // Helper: Setup mock expectations for a valid LED handle
    void ExpectValidHandle(int id, board_led_t handle)
    {
        EXPECT_CALL(*mock_, get_handle(id)).WillOnce(Return(handle));
        EXPECT_CALL(*mock_, is_valid(handle)).WillRepeatedly(Return(true));
    }

    BoardLedMock *mock_;
};

// ============================================================================
// Driver Initialization Tests
// ============================================================================

TEST_F(LedDriverTest, InitReturnsSuccess)
{
    // Setup mock for LED 0
    board_led_t dummy_handle = (board_led_t)0x1234;
    ExpectValidHandle(0, dummy_handle);

    int result = led_driver_init();
    EXPECT_EQ(result, 0);
}

TEST_F(LedDriverTest, InitFailsWhenCalledTwice)
{
    board_led_t dummy_handle = (board_led_t)0x1234;
    ExpectValidHandle(0, dummy_handle);

    // First init should succeed
    EXPECT_EQ(led_driver_init(), 0);

    // Second init should fail
    EXPECT_EQ(led_driver_init(), -1);
}

// ============================================================================
// LED Set Tests
// ============================================================================

TEST_F(LedDriverTest, SetTurnsLedOn)
{
    board_led_t dummy_handle = (board_led_t)0x1234;
    ExpectValidHandle(0, dummy_handle);

    ASSERT_EQ(led_driver_init(), 0);

    // Expect hardware operation
    EXPECT_CALL(*mock_, set_state(dummy_handle, true));

    led_set(0, true);
}

TEST_F(LedDriverTest, SetTurnsLedOff)
{
    board_led_t dummy_handle = (board_led_t)0x1234;
    ExpectValidHandle(0, dummy_handle);

    ASSERT_EQ(led_driver_init(), 0);

    // Expect hardware operation
    EXPECT_CALL(*mock_, set_state(dummy_handle, false));

    led_set(0, false);
}

TEST_F(LedDriverTest, SetWithInvalidLedId)
{
    ASSERT_EQ(led_driver_init(), 0);

    // Invalid LED ID should be silently ignored (no mock expectations)
    led_set(99, true);
}

TEST_F(LedDriverTest, SetBeforeInitDoesNothing)
{
    // Should not crash, just silently return
    led_set(0, true);
}

// ============================================================================
// LED Toggle Tests
// ============================================================================

TEST_F(LedDriverTest, ToggleChangesStateFromOffToOn)
{
    board_led_t dummy_handle = (board_led_t)0x1234;
    ExpectValidHandle(0, dummy_handle);

    ASSERT_EQ(led_driver_init(), 0);

    // Initial state is off, toggle should turn on
    EXPECT_CALL(*mock_, set_state(dummy_handle, true));

    led_toggle(0);
}

TEST_F(LedDriverTest, ToggleChangesStateFromOnToOff)
{
    board_led_t dummy_handle = (board_led_t)0x1234;
    ExpectValidHandle(0, dummy_handle);

    ASSERT_EQ(led_driver_init(), 0);

    // First toggle: off -> on
    EXPECT_CALL(*mock_, set_state(dummy_handle, true));
    led_toggle(0);

    // Second toggle: on -> off
    EXPECT_CALL(*mock_, set_state(dummy_handle, false));
    led_toggle(0);
}

TEST_F(LedDriverTest, ToggleWithInvalidLedId)
{
    ASSERT_EQ(led_driver_init(), 0);

    // Invalid LED ID should be silently ignored
    led_toggle(99);
}

TEST_F(LedDriverTest, ToggleBeforeInitDoesNothing)
{
    // Should not crash, just silently return
    led_toggle(0);
}

// ============================================================================
// State Subscriber Tests
// ============================================================================

TEST_F(LedDriverTest, GetStateSubscriberReturnsValidHandle)
{
    ASSERT_EQ(led_driver_init(), 0);

    msghub_subscriber_t sub = led_get_state_subscriber(0);
    EXPECT_NE(sub, MSGHUB_SUBSCRIBER_INVALID);

    msghub_destroy_subscriber(sub);
}

// ============================================================================
// Multiple LED Tests
// ============================================================================

TEST_F(LedDriverTest, MultipleLedsCanBeControlledIndependently)
{
    board_led_t handle1 = (board_led_t)0x1000;
    board_led_t handle2 = (board_led_t)0x2000;

    ExpectValidHandle(0, handle1);
    ASSERT_EQ(led_driver_init(), 0);

    // Setup expectations for both LEDs
    EXPECT_CALL(*mock_, set_state(handle1, true));
    EXPECT_CALL(*mock_, set_state(handle2, false));

    led_set(0, true);
    led_set(1, false);
}
