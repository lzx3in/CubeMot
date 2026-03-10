add_executable(msghub_test
    test_msghub.cpp
)

target_include_directories(msghub_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src/msghub/include
    ${CMAKE_SOURCE_DIR}/src/msghub
)

# Enable PC unit test mode
target_compile_definitions(msghub_test PRIVATE
    UNIT_TEST_HOST
)

target_link_libraries(msghub_test PRIVATE
    msghub
    GTest::gtest_main
    pthread
)

include(GoogleTest)
gtest_discover_tests(msghub_test)
