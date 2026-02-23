add_library(boards STATIC)

target_include_directories(boards PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/${BOARD})
