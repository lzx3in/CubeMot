# FOC library (observer only for host tests — foc_core.c has HW dependencies)
add_library(foc_observer STATIC
    observer.c
)

target_include_directories(foc_observer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/libs/math
)

target_link_libraries(foc_observer PUBLIC
    zephyr_mocks
)
# FOC library
