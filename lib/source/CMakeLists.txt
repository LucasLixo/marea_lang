# Copyright (c) 2026 Paulo Lucas Alves Franca
# Licensed under the MIT License. See LICENSE.txt for full terms.


cmake_minimum_required(VERSION 3.20)

get_property(TEMP_GUARD_SET GLOBAL PROPERTY MareaLibSourceGuard SET)
if(TEMP_GUARD_SET)
    message(FATAL_ERROR "lib/source/CMakeLists.txt was included twice")
endif()
set_property(GLOBAL PROPERTY MareaLibSourceGuard ON)
unset(TEMP_GUARD_SET)

# The stage libraries and their precompiled header
include("${MAREA_FOLDER_CMAKE_SETTINGS}/ConfigureLibTargets.cmake")
ConfigureLibTargets()

# marea_add_library/marea_add_executable: the keyword functions a consumer of this build uses to compile Marea sources
include("${MAREA_FOLDER_CMAKE_SETTINGS}/ConfigureMareaFunctions.cmake")
ConfigureMareaFunctions()

include("${MAREA_FOLDER_CMAKE_SETTINGS}/ConfigureLibPrecompiledHeaders.cmake")
ConfigureLibPrecompiledHeaders()
