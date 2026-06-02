# CubeMot Zephyr Application
# ZEPHYR_ENV is defined so app code knows it's on Zephyr

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/common/include
    ${CMAKE_CURRENT_SOURCE_DIR}/msghub/include
    ${CMAKE_CURRENT_SOURCE_DIR}/topics/include
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers/include
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/blink/include
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/led_controller/include
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/button_detector/include
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/pid
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/crc
)

target_sources(app PRIVATE
    app/main.c
    app/app_main.c
    app/app_init.c
    common/common_time.c
    msghub/msghub_state.c
    msghub/msghub_topic.c
    msghub/msghub_pub.c
    msghub/msghub_sub.c
    topics/topics.c
    libs/pid/pid.c
    libs/crc/crc.c

    modules/blink/blink.c
    modules/led_controller/led_controller.c
    modules/button_detector/button_detector.c

    drivers/led.c
    drivers/button.c
)

target_compile_definitions(app PRIVATE ZEPHYR_ENV=1)