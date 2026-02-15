# Mock library for board LED functions

add_library(board_led_mock STATIC
    board_led_mock.cpp
)

target_include_directories(board_led_mock PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/boards/include
)

target_link_libraries(board_led_mock PUBLIC
    gmock
)

# Create a drivers library for testing (host build, not embedded)
add_library(drivers_for_test STATIC
    ../../src/drivers/led/led.c
)

target_include_directories(drivers_for_test PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/boards/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/drivers/include
)

target_link_libraries(drivers_for_test PUBLIC
    board_led_mock
)
