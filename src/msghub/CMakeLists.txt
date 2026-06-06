add_library(msghub
    msghub_state.c
    msghub_topic.c
    msghub_pub.c
    msghub_sub.c
)

target_include_directories(msghub PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(msghub PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(msghub PUBLIC common)
