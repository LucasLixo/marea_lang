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

# INFO: marea-cc's own subdirectory and the MAREA_OPTION_TARGET_CC/MAREA_OPTION_TARGET_BC options are
# added by the marea-cc-and-bc-tools apply's own part 3 (marea-cc-and-cmake-wiring); marea-bc builds
# unconditionally here until that part gates both behind their options.
add_subdirectory("${MAREA_FOLDER_APPS}/marea-bc" "${CMAKE_CURRENT_BINARY_DIR}/Marea-Bc")
