#pragma once

#include <gmock/gmock.h>
#include <stdint.h>

// Include C header with proper extern "C" wrapper
extern "C" {
#include "boards/led.h"
}

class BoardLedMock
{
  public:
    MOCK_METHOD(board_led_t, get_handle, (int led_id));
    MOCK_METHOD(bool, is_valid, (board_led_t led));
    MOCK_METHOD(int, get_count, ());
    MOCK_METHOD(void, set_state, (board_led_t led, bool state));
    MOCK_METHOD(void, toggle, (board_led_t led));
    MOCK_METHOD(bool, get_state, (board_led_t led));
};

// Get the global mock instance
BoardLedMock &GetBoardLedMock();
