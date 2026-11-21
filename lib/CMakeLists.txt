# Copyright (c) 2026 Paulo Lucas Alves Franca
# Licensed under the MIT License. See LICENSE.txt for full terms.

cmake_minimum_required(VERSION 3.20)

get_property(TEMP_GUARD_SET GLOBAL PROPERTY MareaLibGuard SET)
if(TEMP_GUARD_SET)
    message(FATAL_ERROR "lib/CMakeLists.txt was included twice")
endif()
set_property(GLOBAL PROPERTY MareaLibGuard ON)
unset(TEMP_GUARD_SET)

# Include and internal come first: the stage libraries in source link them
add_subdirectory("${MAREA_FOLDER_LIB_INCLUDE}" "${CMAKE_CURRENT_BINARY_DIR}/Include")
add_subdirectory("${MAREA_FOLDER_LIB_INTERNAL}" "${CMAKE_CURRENT_BINARY_DIR}/Internal")
add_subdirectory("${MAREA_FOLDER_LIB_SOURCE}" "${CMAKE_CURRENT_BINARY_DIR}/Source")
