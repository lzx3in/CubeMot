add_library(cubemot_button_detector button_detector.c)

target_include_directories(cubemot_button_detector PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
