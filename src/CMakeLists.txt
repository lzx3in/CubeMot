# Include post-build functions before subdirectories that need them
include(${CMAKE_SOURCE_DIR}/cmake/post_build.cmake)

add_subdirectory(boards)
add_subdirectory(drivers)
add_subdirectory(application)

# Remove incorrect libob.a dependency
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)
