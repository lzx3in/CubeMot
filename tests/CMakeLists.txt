set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(deps)

# Enable testing
enable_testing()

# Add mock library
add_subdirectory(mocks)

# Create test executable
add_executable(cubemot-test
    drivers/test_led.cpp
)

target_include_directories(cubemot-test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(cubemot-test PRIVATE
    board_led_mock
    drivers_for_test
    GTest::gtest_main
    GTest::gmock
)

# Register tests with CTest
include(GoogleTest)
gtest_discover_tests(cubemot-test)
