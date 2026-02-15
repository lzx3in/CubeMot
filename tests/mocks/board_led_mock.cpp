#include "board_led_mock.h"
#include <cassert>

static BoardLedMock *g_mock = nullptr;

BoardLedMock &GetBoardLedMock()
{
    static BoardLedMock instance;
    g_mock = &instance;
    return instance;
}

static BoardLedMock *GetMock()
{
    // Mock must be initialized before use
    assert(g_mock != nullptr && "BoardLedMock not initialized - call GetBoardLedMock() first");
    return g_mock;
}

extern "C" {

board_led_t board_led_get_handle(int led_id)
{
    return GetMock()->get_handle(led_id);
}

bool board_led_is_valid(board_led_t led)
{
    return GetMock()->is_valid(led);
}

int board_led_get_count(void)
{
    return GetMock()->get_count();
}

void board_led_set_state(board_led_t led, bool state)
{
    GetMock()->set_state(led, state);
}

void board_led_toggle(board_led_t led)
{
    GetMock()->toggle(led);
}

bool board_led_get_state(board_led_t led)
{
    return GetMock()->get_state(led);
}

} // extern "C"
