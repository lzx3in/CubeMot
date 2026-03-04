add_library(freertos_mocks INTERFACE)

target_include_directories(freertos_mocks INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
