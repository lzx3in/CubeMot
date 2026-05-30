cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(cubemot)

target_sources(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/led_driver.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/button_driver.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common_time.c

    ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/blink.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/led_controller.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/button_detector.c

    ${CMAKE_SOURCE_DIR}/../src/msghub/msghub_state.c
    ${CMAKE_SOURCE_DIR}/../src/msghub/msghub_topic.c
    ${CMAKE_SOURCE_DIR}/../src/msghub/msghub_pub.c
    ${CMAKE_SOURCE_DIR}/../src/msghub/msghub_sub.c

    ${CMAKE_SOURCE_DIR}/../src/topics/topics.c
    ${CMAKE_SOURCE_DIR}/../src/app/app_init.c
    ${CMAKE_SOURCE_DIR}/../src/libs/pid/pid.c
    ${CMAKE_SOURCE_DIR}/../src/libs/crc/crc.c
)

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/../src/msghub/include
    ${CMAKE_SOURCE_DIR}/../src/topics/include
    ${CMAKE_SOURCE_DIR}/../src/common/include
    ${CMAKE_SOURCE_DIR}/../src/app
    ${CMAKE_SOURCE_DIR}/../src/libs/pid
    ${CMAKE_SOURCE_DIR}/../src/libs/crc
)
