add_subdirectory(board)
add_subdirectory(freertos)

# Unified drivers library for testing (host build, not embedded)
add_library(drivers_for_test STATIC
    ../../src/drivers/led/led.c
)

target_include_directories(drivers_for_test PUBLIC
    ${CMAKE_SOURCE_DIR}/src/boards/include
    ${CMAKE_SOURCE_DIR}/src/drivers/include
)

target_link_libraries(drivers_for_test PUBLIC
    board_mocks
)
