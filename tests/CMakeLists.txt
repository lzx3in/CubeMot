# add the libraries in src/ first so that tests/ can link to them.
add_subdirectory(${CMAKE_SOURCE_DIR}/src/common ${CMAKE_BINARY_DIR}/src/common)
add_subdirectory(${CMAKE_SOURCE_DIR}/src/libs ${CMAKE_BINARY_DIR}/src/libs)
add_subdirectory(${CMAKE_SOURCE_DIR}/src/msghub ${CMAKE_BINARY_DIR}/src/msghub)
add_subdirectory(${CMAKE_SOURCE_DIR}/src/topics ${CMAKE_BINARY_DIR}/src/topics)

target_include_directories(msghub PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/freertos
)

add_subdirectory(deps)
add_subdirectory(test_common)
add_subdirectory(mocks/freertos)
add_subdirectory(lib)
add_subdirectory(msghub)
