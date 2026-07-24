add_library(cubemot_drivers button.c)

target_include_directories(cubemot_drivers PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
