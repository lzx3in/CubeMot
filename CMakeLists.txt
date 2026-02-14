cmake_minimum_required(VERSION 3.28)

# Project configuration
project(CubeMot C ASM)
message(STATUS "Project: ${CMAKE_PROJECT_NAME}")

option(CMAKE_EXPORT_COMPILE_COMMANDS "generate compile_commands.json for IDE integration" ON)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Board configuration must come before output directory settings
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/board_config.cmake)

# Generate configuration headers from KConfig at configure time
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/kconfig.cmake)

enable_language(C ASM)
add_executable(${CMAKE_PROJECT_NAME})

# Add project modules
add_subdirectory(boards)
add_subdirectory(application)
add_subdirectory(drivers)

target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # User defined library search paths
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    # User defined include paths
)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # User defined symbols
)

# Remove incorrect libob.a dependency
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    drivers
    ${TOOLCHAIN_LINK_LIBRARIES}
)

# Generate binary, hex, and map files in target/ directory
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/post_build.cmake)

