if(NOT DEFINED BOARD)
    message(FATAL_ERROR "BOARD variable not set. Use -DBOARD=<board_name>")
endif()

add_library(boards STATIC)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/${BOARD})
