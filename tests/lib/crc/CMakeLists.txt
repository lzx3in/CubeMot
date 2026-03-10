add_executable(crc-test
    test_crc.cpp
)

target_link_libraries(crc-test PRIVATE
    lib_crc
    GTest::gtest_main
)

target_include_directories(crc-test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..
    ${CMAKE_SOURCE_DIR}/src
)

include(GoogleTest)
gtest_discover_tests(crc-test)

