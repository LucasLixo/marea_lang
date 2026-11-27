# Copyright (c) 2026 Paulo Lucas Alves Franca
# Licensed under the MIT License. See LICENSE.txt for full terms.

cmake_minimum_required(VERSION 3.20)

get_property(TEMP_GUARD_SET GLOBAL PROPERTY MareaAppsGuard SET)
if(TEMP_GUARD_SET)
    message(FATAL_ERROR "apps/CMakeLists.txt was included twice")
endif()
set_property(GLOBAL PROPERTY MareaAppsGuard ON)
unset(TEMP_GUARD_SET)

# marea-shared comes first: both tool executables link it.
add_subdirectory("${MAREA_FOLDER_APPS}/marea-shared" "${CMAKE_CURRENT_BINARY_DIR}/Marea-Shared")

if(MAREA_OPTION_TARGET_CC)
    add_subdirectory("${MAREA_FOLDER_APPS}/marea-cc" "${CMAKE_CURRENT_BINARY_DIR}/Marea-Cc")
endif()

if(MAREA_OPTION_TARGET_BC)
    add_subdirectory("${MAREA_FOLDER_APPS}/marea-bc" "${CMAKE_CURRENT_BINARY_DIR}/Marea-Bc")
endif()
