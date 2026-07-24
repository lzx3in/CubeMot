# CubeMot Zephyr Application
# ZEPHYR_ENV is defined so app code knows it's on Zephyr
#
# Architecture:
#   Core modules are always compiled. Optional modules are Kconfig-gated
#   (CONFIG_MODULE_*_ENABLE), keeping the build modular while respecting
#   Zephyr's requirement that app target receives direct target_sources().

target_compile_definitions(app PRIVATE ZEPHYR_ENV=1)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ── Include paths ────────────────────────────────────────

target_include_directories(app PRIVATE
    common/include
    msghub/include
    topics/include
    drivers/include
    drivers/foc
    libs/pid
    libs/crc
    libs/math
    libs/foc
)

# ── Core sources (always compiled) ───────────────────────

target_sources(app PRIVATE
    # App entry & lifecycle
    app/main.c
    app/app_main.c
    app/app_init.c
    # Common utilities
    common/common_time.c
    # Message hub (pub/sub bus)
    msghub/msghub_state.c
    msghub/msghub_topic.c
    msghub/msghub_pub.c
    msghub/msghub_sub.c
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

if(CONFIG_MODULE_COMMANDER_ENABLE)
    target_include_directories(app PRIVATE
        modules/commander/include
    )
    target_sources(app PRIVATE
        modules/commander/commander.c
    )
endif()

if(CONFIG_MODULE_VEHICLE_ENABLE)
    target_include_directories(app PRIVATE
        modules/vehicle/include
    )
    target_sources(app PRIVATE
        modules/vehicle/vehicle.c
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

if(CONFIG_MODULE_SERVO_CTRL)
    target_include_directories(app PRIVATE
        modules/servo_ctrl
    )
    target_sources(app PRIVATE
        modules/servo_ctrl/servo_ctrl.c
    )
endif()
