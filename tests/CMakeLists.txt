# add the libraries in src/ first so that tests/ can link to them.
add_subdirectory(${CMAKE_SOURCE_DIR}/src/common ${CMAKE_BINARY_DIR}/src/common)
add_subdirectory(${CMAKE_SOURCE_DIR}/src/libs ${CMAKE_BINARY_DIR}/src/libs)
add_subdirectory(${CMAKE_SOURCE_DIR}/src/msghub ${CMAKE_BINARY_DIR}/src/msghub)
add_subdirectory(${CMAKE_SOURCE_DIR}/src/drivers ${CMAKE_BINARY_DIR}/src/drivers)
add_subdirectory(${CMAKE_SOURCE_DIR}/src/topics ${CMAKE_BINARY_DIR}/src/topics)

add_subdirectory(deps)
add_subdirectory(test_common)
add_subdirectory(mocks)
add_subdirectory(drivers)
add_subdirectory(lib)

target_include_directories(msghub PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/freertos
)

add_subdirectory(msghub)

add_executable(cubemot-test
    $<TARGET_OBJECTS:driver_tests>
)

target_include_directories(cubemot-test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(cubemot-test PRIVATE
    board_mocks
    drivers
    msghub
    test_common
    GTest::gtest_main
    GTest::gmock
    topics
)

# Register tests with CTest
include(GoogleTest)
gtest_discover_tests(cubemot-test)
