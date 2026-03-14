add_library(topics STATIC
    topics.c
)

target_include_directories(topics PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(topics PUBLIC
    msghub
    common
)
