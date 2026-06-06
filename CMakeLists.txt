# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

if(NOT BUILD_TESTING)
    find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
endif()

project(CubeMot)

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
else()
    add_subdirectory(src)
endif()