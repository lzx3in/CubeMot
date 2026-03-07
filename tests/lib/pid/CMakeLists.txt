add_executable(pid-test
    test_pid.cpp
)

target_link_libraries(pid-test PRIVATE
    pid
    GTest::gtest_main
)

target_include_directories(pid-test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..
    ${CMAKE_SOURCE_DIR}/src
)

include(GoogleTest)
gtest_discover_tests(pid-test)
