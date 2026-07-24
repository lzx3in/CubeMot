# FOC tests (foc_types + observer)

# Test foc_types (header-only conversions)
add_executable(foc-types-test
    test_foc_types.cpp
)

target_link_libraries(foc-types-test PRIVATE
    GTest::gtest_main
)

target_include_directories(foc-types-test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..
    ${CMAKE_SOURCE_DIR}/src
)

include(GoogleTest)
gtest_discover_tests(foc-types-test)

# Test observer (sliding mode observer + PLL)
add_executable(observer-test
    test_observer.cpp
)

target_link_libraries(observer-test PRIVATE
    foc_observer
    GTest::gtest_main
)

target_include_directories(observer-test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..
    ${CMAKE_SOURCE_DIR}/src
)

gtest_discover_tests(observer-test)
