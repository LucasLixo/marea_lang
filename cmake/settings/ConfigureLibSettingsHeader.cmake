# Copyright (c) 2026 Paulo Lucas Alves Franca
# Licensed under the MIT License. See LICENSE.txt for full terms.

cmake_minimum_required(VERSION 3.20)

get_property(TEMP_GUARD_SET GLOBAL PROPERTY ConfigureLibSettingsHeaderGuard SET)
if(TEMP_GUARD_SET)
    message(FATAL_ERROR "ConfigureLibSettingsHeader.cmake was included twice")
endif()
set_property(GLOBAL PROPERTY ConfigureLibSettingsHeaderGuard ON)
unset(TEMP_GUARD_SET)

# Generate MAREASettingsHeader.h from metadata.yaml and the SETTING_* switches read by MAREAMacros.h through #cmakedefine
macro(ConfigureLibSettingsHeader)
    set(SETTING_USE_SHORT_LIFE_BLOCK OFF)
    set(SETTING_USE_STRUCTS_PACKED OFF)
    set(SETTING_BUILD_DEBUG OFF)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(SETTING_BUILD_DEBUG ON)
    endif()

    configure_file(
        "${MAREA_FOLDER_LIB_INCLUDE}/marea/base/MAREASettingsHeader.h.in"
        "${MAREA_FOLDER_CMAKE_BINARY_LIB_GENERATED}/marea/base/MAREASettingsHeader.h"
        @ONLY
    )

    unset(SETTING_USE_SHORT_LIFE_BLOCK)
    unset(SETTING_USE_STRUCTS_PACKED)
    unset(SETTING_BUILD_DEBUG)
endmacro()
