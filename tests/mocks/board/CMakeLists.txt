add_library(board_mocks STATIC
    led_mock.cpp
)

target_include_directories(board_mocks PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/boards/include
)

target_link_libraries(board_mocks PUBLIC
    gmock
)
