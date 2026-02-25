# Driver tests library
# Creates an object library containing all driver test files

add_library(driver_tests OBJECT
    test_led.cpp
    test_led_level.cpp
    test_led_animate.cpp
)

target_include_directories(driver_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(driver_tests PRIVATE
    board_mocks
    drivers_for_test
    GTest::gtest
    GTest::gmock
)
