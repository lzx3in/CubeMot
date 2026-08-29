# add the libraries in src/ first so that tests/ can link to them.
add_subdirectory(${CMAKE_SOURCE_DIR}/src/common ${CMAKE_BINARY_DIR}/src/common)
add_subdirectory(${CMAKE_SOURCE_DIR}/src/libs ${CMAKE_BINARY_DIR}/src/libs)
add_subdirectory(${MSGHUB_DIR} ${CMAKE_BINARY_DIR}/shared/msghub)
add_subdirectory(${CMAKE_SOURCE_DIR}/src/topics ${CMAKE_BINARY_DIR}/src/topics)

add_subdirectory(deps)
add_subdirectory(test_common)
add_subdirectory(mocks/zephyr)

# Apply Zephyr mock headers for common library in test builds
target_link_libraries(common PUBLIC zephyr_mocks)

# msghub host unit tests moved into the msghub repo (shared/msghub/tests)
add_subdirectory(lib)
