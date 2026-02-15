#include "board_led_mock.h"

static BoardLedMock *g_mock = nullptr;

BoardLedMock &GetBoardLedMock()
{
    static BoardLedMock instance;
    g_mock = &instance;
    return instance;
}

extern "C" {

void board_led_set_state(const board_led_config_t *config, bool state)
{
    if (g_mock) {
        g_mock->set_state(config, state);
    }
}

void board_led_toggle(const board_led_config_t *config)
{
    if (g_mock) {
        g_mock->toggle(config);
    }
}

bool board_led_get_state(const board_led_config_t *config)
{
    if (g_mock) {
        return g_mock->get_state(config);
    }
    return false;
}

} // extern "C"
