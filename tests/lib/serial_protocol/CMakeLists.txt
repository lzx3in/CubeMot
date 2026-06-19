add_executable(serial-protocol-test
    test_serial_protocol.cpp
)

target_link_libraries(serial-protocol-test PRIVATE
    lib_serial_protocol
    GTest::gtest_main
)

target_include_directories(serial-protocol-test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..
    ${CMAKE_SOURCE_DIR}/src
)

include(GoogleTest)
gtest_discover_tests(serial-protocol-test)
