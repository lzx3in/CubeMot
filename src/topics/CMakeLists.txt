add_library(topics topics.c)

target_include_directories(topics PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(topics PRIVATE msghub common)
