add_subdirectory(deps)
add_subdirectory(mocks)
add_subdirectory(drivers)

add_executable(cubemot-test
    $<TARGET_OBJECTS:driver_tests>
)

target_include_directories(cubemot-test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(cubemot-test PRIVATE
    board_mocks
    drivers_for_test
    GTest::gtest_main
    GTest::gmock
)

# Register tests with CTest
include(GoogleTest)
gtest_discover_tests(cubemot-test)
