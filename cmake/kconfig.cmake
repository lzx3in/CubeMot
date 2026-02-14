# KConfig integration
# Generates configuration headers from .config using gen_config.py

set(KCONFIG_ROOT ${CMAKE_SOURCE_DIR}/Kconfig)
set(KCONFIG_CONFIG ${CMAKE_SOURCE_DIR}/.config)

# Find Python interpreter (prefer virtual environment if available)
if(EXISTS ${CMAKE_SOURCE_DIR}/.venv/bin/python)
    set(PYTHON_EXECUTABLE ${CMAKE_SOURCE_DIR}/.venv/bin/python)
    message(STATUS "KConfig: Using virtual environment Python")
else()
    find_package(Python3 REQUIRED)
    set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})
endif()

# Check if gen_config.py exists
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/tools/gen_config.py)
    message(FATAL_ERROR "gen_config.py not found in tools/ directory")
endif()

# Generate .config from defconfig if not exists
if(NOT EXISTS ${KCONFIG_CONFIG})
    message(STATUS "KConfig: Generating .config...")

    # Build command arguments
    set(GEN_CONFIG_ARGS
        generate-config
        ${KCONFIG_ROOT}
        ${KCONFIG_CONFIG}
        --search-path ${CMAKE_SOURCE_DIR}/src/boards
    )
    if(DEFINED BOARD)
        list(APPEND GEN_CONFIG_ARGS --board ${BOARD})
    endif()

    execute_process(
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/gen_config.py
                ${GEN_CONFIG_ARGS}
        RESULT_VARIABLE GENERATE_RESULT
        OUTPUT_VARIABLE GENERATE_OUTPUT
        ERROR_VARIABLE GENERATE_ERROR
    )

    if(GENERATE_RESULT EQUAL 0)
        message(STATUS "KConfig: Successfully generated ${KCONFIG_CONFIG}")
        if(GENERATE_OUTPUT)
            message(STATUS "KConfig: ${GENERATE_OUTPUT}")
        endif()
    else()
        message(FATAL_ERROR "KConfig: Failed to generate .config: ${GENERATE_ERROR}")
    endif()
else()
    message(STATUS "KConfig: Using existing .config")
endif()

# Output configuration header files
set(CONFIG_HEADERS
    ${CMAKE_SOURCE_DIR}/src/boards/board_config.h
    ${CMAKE_SOURCE_DIR}/src/boards/system_config.h
    ${CMAKE_SOURCE_DIR}/src/drivers/driver_config.h
    ${CMAKE_SOURCE_DIR}/src/application/app_config.h
    ${CMAKE_SOURCE_DIR}/middlewares/middleware_config.h
)

# Generate configuration headers from .config at configure time
# Ensures headers exist before compilation starts
message(STATUS "KConfig: Generating configuration headers...")

execute_process(
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/gen_config.py
            generate-headers ${KCONFIG_ROOT} ${KCONFIG_CONFIG} ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE GENERATE_RESULT
    OUTPUT_VARIABLE GENERATE_OUTPUT
    ERROR_VARIABLE GENERATE_ERROR
)

if(GENERATE_RESULT EQUAL 0)
    message(STATUS "KConfig: Successfully generated configuration headers")
    if(GENERATE_OUTPUT)
        message(STATUS "KConfig: ${GENERATE_OUTPUT}")
    endif()
else()
    message(FATAL_ERROR "KConfig: Failed to generate configuration headers: ${GENERATE_ERROR}")
endif()

# Make config headers available globally
foreach(header ${CONFIG_HEADERS})
    get_filename_component(header_dir ${header} DIRECTORY)
    include_directories(${header_dir})
endforeach()

message(STATUS "KConfig: Headers: ${CONFIG_HEADERS}")
