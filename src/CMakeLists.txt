enable_language(C ASM)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Include post-build functions before subdirectories that need them
include(${CMAKE_SOURCE_DIR}/cmake/post_build.cmake)

add_subdirectory(application)
add_subdirectory(drivers)
add_subdirectory(boards)

# Remove incorrect libob.a dependency
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)
