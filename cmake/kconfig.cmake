# KConfig integration
# Generates configuration headers from .config using gen_config.py

set(KCONFIG_ROOT ${CMAKE_SOURCE_DIR}/Kconfig)
set(KCONFIG_CONFIG ${CMAKE_SOURCE_DIR}/.config)

# Generate .config from defconfig if not exists
if(NOT EXISTS ${KCONFIG_CONFIG})

    # Search for defconfig in priority order: board-specific first, then global
    set(DEFCONFIG_PATH "")
    if(DEFINED BOARD)
        set(BOARD_DEFCONFIG "${CMAKE_SOURCE_DIR}/boards/${BOARD}/defconfig")
        if(EXISTS ${BOARD_DEFCONFIG})
            set(DEFCONFIG_PATH ${BOARD_DEFCONFIG})
            message(STATUS "KConfig: Found board defconfig: ${BOARD_DEFCONFIG}")
        endif()
    endif()

    if(NOT DEFCONFIG_PATH AND EXISTS ${CMAKE_SOURCE_DIR}/boards/defconfig)
        set(DEFCONFIG_PATH ${CMAKE_SOURCE_DIR}/boards/defconfig)
        message(STATUS "KConfig: Found global defconfig: ${CMAKE_SOURCE_DIR}/boards/defconfig")
    endif()

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

    # Generate .config from defconfig or defaults
    if(DEFCONFIG_PATH)
        message(STATUS "KConfig: Generating .config from ${DEFCONFIG_PATH}...")
        execute_process(
            COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/gen_config.py
                    generate-config ${KCONFIG_ROOT} ${KCONFIG_CONFIG} ${DEFCONFIG_PATH}
            RESULT_VARIABLE GENERATE_RESULT
            OUTPUT_VARIABLE GENERATE_OUTPUT
            ERROR_VARIABLE GENERATE_ERROR
        )
    else()
        message(STATUS "KConfig: No defconfig found, generating .config from Kconfig defaults...")
        execute_process(
            COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/gen_config.py
                    generate-config ${KCONFIG_ROOT} ${KCONFIG_CONFIG}
            RESULT_VARIABLE GENERATE_RESULT
            OUTPUT_VARIABLE GENERATE_OUTPUT
            ERROR_VARIABLE GENERATE_ERROR
        )
    endif()

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
    ${CMAKE_SOURCE_DIR}/boards/board_config.h
    ${CMAKE_SOURCE_DIR}/boards/system_config.h
    ${CMAKE_SOURCE_DIR}/src/drivers/driver_config.h
    ${CMAKE_SOURCE_DIR}/application/app_config.h
)

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
