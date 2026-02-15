#ifndef BOARD_LED_MOCK_H
#define BOARD_LED_MOCK_H

#include <gmock/gmock.h>
#include <stdint.h>

// Include C header with proper extern "C" wrapper
extern "C" {
#include "boards/led.h"
}

class BoardLedMock
{
  public:
    MOCK_METHOD(void, set_state, (const board_led_config_t *config, bool state));
    MOCK_METHOD(void, toggle, (const board_led_config_t *config));
    MOCK_METHOD(bool, get_state, (const board_led_config_t *config));
};

// Get the global mock instance
BoardLedMock &GetBoardLedMock();

#endif // BOARD_LED_MOCK_H
