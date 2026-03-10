add_library(app OBJECT
    app_main.c
    app_init.c
)

target_include_directories(app PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/include
)

target_link_libraries(app PUBLIC
    modules
    freertos_kernel
    ${TOOLCHAIN_LINK_LIBRARIES}
)
