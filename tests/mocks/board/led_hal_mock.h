#pragma once

#include <gmock/gmock.h>
#include <stdint.h>

// Include the HAL framework interface
extern "C" {
#include "drivers/framework/led_hal_framework.h"
}

/**
 * Mock class for LED HAL Framework
 *
 * This mock implements the led_hal_ops_t interface.
 * Tests use this to verify driver behavior without hardware dependencies.
 */
class LedHalMock
{
  public:
    MOCK_METHOD(int, get_count, ());
    MOCK_METHOD(void, set_state, (uint8_t led_id, bool state));
    MOCK_METHOD(void, toggle, (uint8_t led_id));
    MOCK_METHOD(bool, get_state, (uint8_t led_id));
};

/**
 * Get the global LED HAL mock instance
 *
 * @return Reference to the singleton mock instance
 */
LedHalMock &GetLedHalMock();

/**
 * Setup LED HAL mock
 * Registers the mock implementation with the framework
 */
void setup_led_hal_mock(void);
