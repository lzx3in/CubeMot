#include "led_hal_mock.h"
#include <cassert>

static LedHalMock *g_mock = nullptr;
static led_hal_ops_t g_mock_ops;

LedHalMock &GetLedHalMock()
{
    static LedHalMock instance;
    g_mock = &instance;
    return instance;
}

static LedHalMock *GetMock()
{
    // Mock must be initialized before use
    assert(g_mock != nullptr && "LedHalMock not initialized - call GetLedHalMock() first");
    return g_mock;
}

// ============================================================================
// Mock Implementation Functions
// ============================================================================

static int mock_get_count(void)
{
    return GetMock()->get_count();
}

static void mock_set_state(uint8_t led_id, bool state)
{
    GetMock()->set_state(led_id, state);
}

static void mock_toggle(uint8_t led_id)
{
    GetMock()->toggle(led_id);
}

static bool mock_get_state(uint8_t led_id)
{
    return GetMock()->get_state(led_id);
}

// ============================================================================
// Mock Setup
// ============================================================================

void setup_led_hal_mock(void)
{
    g_mock_ops = {
        .base =
            {
                .name = "mock_led",
                .init = nullptr,
                .deinit = nullptr,
            },
        .get_count = mock_get_count,
        .set_state = mock_set_state,
        .toggle = mock_toggle,
        .get_state = mock_get_state,
    };

    led_hal_register(&g_mock_ops);
}
