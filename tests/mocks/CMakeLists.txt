# Unified mock library for all board-level interfaces

add_library(board_mocks STATIC
    board_led_mock.cpp
)

target_include_directories(board_mocks PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/boards/include
)

target_link_libraries(board_mocks PUBLIC
    gmock
)

# Unified drivers library for testing (host build, not embedded)
add_library(drivers_for_test STATIC
    ../../src/drivers/led/led.c
)

target_include_directories(drivers_for_test PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/boards/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/drivers/include
)

target_link_libraries(drivers_for_test PUBLIC
    board_mocks
)
