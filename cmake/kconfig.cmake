# KConfig Integration
# Generates cubemot_config.h from Kconfig files using gen_config.py

set(KCONFIG_ROOT ${CMAKE_SOURCE_DIR}/Kconfig)
set(KCONFIG_USRCONFIG ${CMAKE_SOURCE_DIR}/.config)
set(KCONFIG_DEFCONFIG ${CMAKE_SOURCE_DIR}/defconfig)
set(KCONFIG_HEADER ${CMAKE_SOURCE_DIR}/cubemot_config.h)

# Python interpreter: prefer venv, fallback to system Python3
if(EXISTS ${CMAKE_SOURCE_DIR}/.venv/bin/python)
    set(PYTHON_EXECUTABLE ${CMAKE_SOURCE_DIR}/.venv/bin/python)
    message(STATUS "KConfig: Using virtual environment Python")
else()
    find_package(Python3 REQUIRED)
    set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})
endif()

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/tools/gen_config.py)
    message(FATAL_ERROR "gen_config.py not found in tools/ directory")
endif()

message(STATUS "KConfig: Generating configuration header...")

execute_process(
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/gen_config.py
            ${KCONFIG_ROOT}
            --output ${KCONFIG_HEADER}
            --usrconfig ${KCONFIG_USRCONFIG}
            --defconfig ${KCONFIG_DEFCONFIG}
    RESULT_VARIABLE GENERATE_RESULT
    OUTPUT_VARIABLE GENERATE_OUTPUT
    ERROR_VARIABLE GENERATE_ERROR
)

if(GENERATE_RESULT EQUAL 0)
    message(STATUS "KConfig: Successfully generated configuration header")
    if(GENERATE_OUTPUT)
        string(REPLACE "\n" ";" OUTPUT_LINES ${GENERATE_OUTPUT})
        foreach(line ${OUTPUT_LINES})
            string(STRIP ${line} stripped_line)
            if(stripped_line)
                message(STATUS "  ${stripped_line}")
            endif()
        endforeach()
    endif()
else()
    message(FATAL_ERROR "KConfig: Failed to generate configuration: ${GENERATE_ERROR}")
endif()

# Expose generated header globally
include_directories(${CMAKE_SOURCE_DIR})

message(STATUS "KConfig: Header: ${KCONFIG_HEADER}")
