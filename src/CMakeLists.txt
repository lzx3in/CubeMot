# Include post-build functions before subdirectories that need them
include(${CMAKE_SOURCE_DIR}/cmake/post_build.cmake)

# FreeRTOS configuration
# Note: freertos_config target is defined in boards/<BOARD>/freertos_config/CMakeLists.txt
set(FREERTOS_PORT GCC_ARM_CM4F CACHE STRING "FreeRTOS port" FORCE)
set(FREERTOS_HEAP 4 CACHE STRING "FreeRTOS heap implementation" FORCE)

add_subdirectory(lib)
add_subdirectory(boards)
add_subdirectory(freertos)
add_subdirectory(drivers)
add_subdirectory(msghub)
add_subdirectory(application)

# Remove incorrect libob.a dependency
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)
