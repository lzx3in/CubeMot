cmake_minimum_required(VERSION 3.28)

# Set default toolchain before project() as CMake loads toolchain file during project() command
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND NOT BUILD_TESTING)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/gcc_arm_none_eabi_toolchain.cmake"
        CACHE STRING "Toolchain file" FORCE)
    message(STATUS "Using default toolchain: ${CMAKE_TOOLCHAIN_FILE}")
endif()

# Project configuration
project(CubeMot)
message(STATUS "Project: ${CMAKE_PROJECT_NAME}")

# Global Language Standards
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(CMAKE_EXPORT_COMPILE_COMMANDS "generate compile_commands.json for IDE integration" ON)
option(BUILD_TESTING "Build unit tests" OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Board configuration (reads BOARD_NAME from Kconfig)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/board_config.cmake)

# Generate configuration header from Kconfig (after board config)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/kconfig.cmake)

# Add project modules
if(BUILD_TESTING)
    enable_language(C CXX)
    enable_testing()
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_C_EXTENSIONS ON)  # Testing build allows GNU extensions (required by gtest)
    add_library(freertos_kernel INTERFACE)
    target_compile_definitions(freertos_kernel INTERFACE BUILD_TESTING)
    add_subdirectory(tests)
else()
    enable_language(C ASM)
    set(CMAKE_C_EXTENSIONS OFF)  # Embedded strict mode
    add_subdirectory(src)
endif()
