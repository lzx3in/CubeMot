add_library(cubemot_app main.c app_main.c app_init.c)

target_include_directories(cubemot_app PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
