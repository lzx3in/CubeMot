# CubeMot Zephyr Application
#
# Architecture:
#   Core modules are always compiled. Optional modules are Kconfig-gated
#   (CONFIG_MODULE_*_ENABLE), keeping the build modular while respecting
#   Zephyr's requirement that app target receives direct target_sources().
#
# msghub integration (framework glue lives here, not in the msghub repo):
#   - portable core sources from ${MSGHUB_DIR}/src (excluding msghub_port.c)
#   - Zephyr adapter: src/msghub_port_zephyr.c (strong msghub_port_* impls)
#   - capacity: CONFIG_MSGHUB_MAX_* (Kconfig) mapped to MSGHUB_MAX_* macros

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ── Include paths ────────────────────────────────────────

target_include_directories(app PRIVATE
    common/include
    ${MSGHUB_DIR}/include
    ${MSGHUB_DIR}/src
    topics/include
    drivers/foc
    libs/pid
    libs/crc
    libs/math
    libs/foc
    app
)

# ── Core sources (always compiled) ───────────────────────

target_sources(app PRIVATE
    # App entry & lifecycle
    app/main.c
    app/app_main.c
    app/app_init.c
    # Common utilities
    common/common_time.c
    # Message hub (pub/sub bus) — portable core + Zephyr adapter
    ${MSGHUB_DIR}/src/msghub_state.c
    ${MSGHUB_DIR}/src/msghub_topic.c
    ${MSGHUB_DIR}/src/msghub_pub.c
    ${MSGHUB_DIR}/src/msghub_sub.c
    msghub_port_zephyr.c
    # Topic definitions
    topics/topics.c
    # Math / data processing
    libs/pid/pid.c
    libs/crc/crc.c
    # FOC library
    libs/foc/foc_core.c
    libs/foc/observer.c
    # FOC drivers (always compiled for now)
    drivers/foc/foc_pwm.c
    drivers/foc/foc_adc.c
    drivers/foc/foc_isr.c
)

# ── Optional modules (Kconfig-gated) ─────────────────────

# Shell commands (development/verification interface)
if(CONFIG_SHELL)
    target_sources(app PRIVATE
        app/shell_cmds.c
        app/scope.c
    )
endif()

if(CONFIG_MODULE_BLINK_ENABLE)
    target_include_directories(app PRIVATE
        modules/blink/include
    )
    target_sources(app PRIVATE
        modules/blink/blink.c
    )
endif()

if(CONFIG_MODULE_BUTTON_DETECTOR_ENABLE)
    target_include_directories(app PRIVATE
        modules/button_detector/include
    )
    target_sources(app PRIVATE
        modules/button_detector/button_detector.c
    )
endif()

if(CONFIG_MODULE_LED_CONTROLLER_ENABLE)
    target_include_directories(app PRIVATE
        modules/led_controller/include
    )
    target_sources(app PRIVATE
        modules/led_controller/led_controller.c
    )
endif()

if(CONFIG_MODULE_MOTOR_CTRL_ENABLE)
    target_include_directories(app PRIVATE
        modules/motor_ctrl/include
    )
    target_sources(app PRIVATE
        modules/motor_ctrl/motor_ctrl.c
    )
endif()



if(CONFIG_MODULE_SERIAL_CMD_ENABLE)
    target_include_directories(app PRIVATE
        comm/serial_cmd/include
        libs/serial_protocol
    )
    target_sources(app PRIVATE
        comm/serial_cmd/serial_cmd.c
        libs/serial_protocol/serial_protocol.c
    )
endif()


# ── msghub capacity: map Zephyr Kconfig to msghub compile-time macros ─────────

if(CONFIG_MSGHUB_MAX_TOPICS)
    target_compile_definitions(app PRIVATE MSGHUB_MAX_TOPICS=${CONFIG_MSGHUB_MAX_TOPICS})
endif()
if(CONFIG_MSGHUB_MAX_INSTANCES)
    target_compile_definitions(app PRIVATE MSGHUB_MAX_INSTANCES=${CONFIG_MSGHUB_MAX_INSTANCES})
endif()
if(CONFIG_MSGHUB_MAX_SUBSCRIBERS)
    target_compile_definitions(app PRIVATE MSGHUB_MAX_SUBSCRIBERS=${CONFIG_MSGHUB_MAX_SUBSCRIBERS})
endif()
if(CONFIG_MSGHUB_MAX_MSG_SIZE)
    target_compile_definitions(app PRIVATE MSGHUB_MAX_MSG_SIZE=${CONFIG_MSGHUB_MAX_MSG_SIZE})
endif()


