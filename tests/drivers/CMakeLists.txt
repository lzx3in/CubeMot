# Driver tests library
# Creates an object library containing all driver test files

add_library(driver_tests STATIC
    test_led.cpp
)

target_include_directories(driver_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_SOURCE_DIR}/src/msghub/include
)

target_link_libraries(driver_tests PRIVATE
    board_mocks
    drivers
    msghub
    GTest::gtest
    GTest::gmock
)
