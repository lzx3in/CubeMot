add_executable(math-test
    test_math.cpp
)

target_include_directories(math-test PRIVATE
    ${CMAKE_SOURCE_DIR}/src/libs/math
)

target_link_libraries(math-test PRIVATE
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(math-test)
