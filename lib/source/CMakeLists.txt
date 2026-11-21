# Copyright (c) 2026 Paulo Lucas Alves Franca
# Licensed under the MIT License. See LICENSE.txt for full terms.


cmake_minimum_required(VERSION 3.20)

get_property(TEMP_GUARD_SET GLOBAL PROPERTY MareaLibSourceGuard SET)
if(TEMP_GUARD_SET)
    message(FATAL_ERROR "lib/source/CMakeLists.txt was included twice")
endif()
set_property(GLOBAL PROPERTY MareaLibSourceGuard ON)
unset(TEMP_GUARD_SET)

# The generated settings header, the stage libraries and their precompiled header
include("${MAREA_FOLDER_CMAKE_SETTINGS}/ConfigureLibSettingsHeader.cmake")
ConfigureLibSettingsHeader()

include("${MAREA_FOLDER_CMAKE_SETTINGS}/ConfigureLibTargets.cmake")
ConfigureLibTargets()

include("${MAREA_FOLDER_CMAKE_SETTINGS}/ConfigureLibPrecompiledHeaders.cmake")
ConfigureLibPrecompiledHeaders()
