#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "mocks/board/led_hal_mock.h"

extern "C" {
#include "drivers/led/led.h"
#include "drivers/led/led_msg.h"
#include "msghub/msghub.h"

// Test support functions
void msghub_reset(void);
void led_driver_deinit(void);
}

using ::testing::_;
using ::testing::Return;

class LedDriverTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        mock_ = &GetLedHalMock();
        setup_led_hal_mock();

        // Reset msghub and LED driver state before each test
        msghub_reset();
        led_driver_deinit();
    }

    void TearDown() override
    {
        // Cleanup handled by mock reset between tests
    }

    // Helper: Setup mock expectations for LED operations
    void ExpectLedState(uint8_t id, bool state)
    {
        EXPECT_CALL(*mock_, set_state(id, state)).Times(1);
    }

    LedHalMock *mock_;
};

// ============================================================================
// Driver Initialization Tests
// ============================================================================

TEST_F(LedDriverTest, InitReturnsSuccess)
{
    // Setup mock expectations
    EXPECT_CALL(*mock_, get_count()).WillRepeatedly(Return(1));

    int result = led_driver_init();
    EXPECT_EQ(result, 0);
}

TEST_F(LedDriverTest, InitFailsWhenCalledTwice)
{
    EXPECT_CALL(*mock_, get_count()).WillRepeatedly(Return(1));

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
    EXPECT_CALL(*mock_, get_count()).WillRepeatedly(Return(1));
    ASSERT_EQ(led_driver_init(), 0);

    // Expect hardware operation
    ExpectLedState(0, true);

    led_set(0, true);
}

TEST_F(LedDriverTest, SetTurnsLedOff)
{
    EXPECT_CALL(*mock_, get_count()).WillRepeatedly(Return(1));
    ASSERT_EQ(led_driver_init(), 0);

    // Expect hardware operation
    ExpectLedState(0, false);

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
    EXPECT_CALL(*mock_, get_count()).WillRepeatedly(Return(1));
    ASSERT_EQ(led_driver_init(), 0);

    // Initial state is off, toggle should turn on
    ExpectLedState(0, true);

    led_toggle(0);
}

TEST_F(LedDriverTest, ToggleChangesStateFromOnToOff)
{
    EXPECT_CALL(*mock_, get_count()).WillRepeatedly(Return(1));
    ASSERT_EQ(led_driver_init(), 0);

    // First toggle: off -> on
    ExpectLedState(0, true);
    led_toggle(0);

    // Second toggle: on -> off
    ExpectLedState(0, false);
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
    EXPECT_CALL(*mock_, get_count()).WillRepeatedly(Return(2));
    ASSERT_EQ(led_driver_init(), 0);

    // Setup expectations for both LEDs
    ExpectLedState(0, true);
    ExpectLedState(1, false);

    led_set(0, true);
    led_set(1, false);
}
