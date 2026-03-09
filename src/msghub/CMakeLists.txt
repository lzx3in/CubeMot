add_library(msghub STATIC
    msghub_state.c
    msghub_topic.c
    msghub_pub.c
    msghub_sub.c
)

target_include_directories(msghub PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(msghub PUBLIC
    common
    freertos_kernel
)
