add_executable(msghub_test
    test_msghub.cpp
)

target_include_directories(msghub_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src/msghub/include
    ${CMAKE_SOURCE_DIR}/src/msghub
)

target_link_libraries(msghub_test PRIVATE
    msghub
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(msghub_test)
