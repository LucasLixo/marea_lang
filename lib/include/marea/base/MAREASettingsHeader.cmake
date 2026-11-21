# Copyright (c) 2026 Paulo Lucas Alves Franca
# Licensed under the MIT License. See LICENSE.txt for full terms.

cmake_minimum_required(VERSION 3.20)

get_property(TEMP_GUARD_SET GLOBAL PROPERTY SettingsHeaderGuard SET)
if(TEMP_GUARD_SET)
    message(FATAL_ERROR "MAREASettingsHeader.cmake was included twice")
endif()
set_property(GLOBAL PROPERTY SettingsHeaderGuard ON)
unset(TEMP_GUARD_SET)

# Values the template reads with @VAR@, from metadata.yaml
set(MAREA_TITLE "${MAREA_METADATA_title}")
set(MAREA_LABEL "${MAREA_METADATA_label}")
set(MAREA_PACKAGE "${MAREA_METADATA_package}")
set(MAREA_VERSION_MAJOR ${MAREA_METADATA_version_major})
set(MAREA_VERSION_MINOR ${MAREA_METADATA_version_minor})
set(MAREA_VERSION_PATCH ${MAREA_METADATA_version_patch})
set(MAREA_VERSION_BUILD ${MAREA_METADATA_version_build})
set(MAREA_VERSION
    "${MAREA_METADATA_version_major}.${MAREA_METADATA_version_minor}.${MAREA_METADATA_version_patch}.${MAREA_METADATA_version_build}"
)

# Use for short life block, which is a block that is executed only once and then discarded.
set(SETTING_USE_SHORT_LIFE_BLOCK OFF)

# Use for packing structs, if enabled, pack structs with 1 byte alignment, if disabled, use default alignment, only structs specified.
set(SETTING_USE_STRUCTS_PACKED OFF)

# Whether this configure produces a debug build, mirroring CMAKE_BUILD_TYPE (a multi-config generator has it empty at configure time, so it stays OFF there).
set(SETTING_BUILD_DEBUG OFF)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(SETTING_BUILD_DEBUG ON)
endif()

configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/MAREASettingsHeader.h.in"
    "${MAREA_FOLDER_CMAKE_BINARY_LIB_GENERATED}/marea/base/MAREASettingsHeader.h"
    @ONLY
)

# Force-included on the command line, never through a #include, and INTERFACE so every target linking the public headers gets it
BasicForceIncludeHeader(
    TARGET
    marea_include
    VISIBILITY
    INTERFACE
    HEADER
    "${MAREA_FOLDER_CMAKE_BINARY_LIB_GENERATED}/marea/base/MAREASettingsHeader.h"
)

# Unset
unset(MAREA_TITLE)
unset(MAREA_LABEL)
unset(MAREA_PACKAGE)
unset(MAREA_VERSION_MAJOR)
unset(MAREA_VERSION_MINOR)
unset(MAREA_VERSION_PATCH)
unset(MAREA_VERSION_BUILD)
unset(MAREA_VERSION)
unset(SETTING_USE_SHORT_LIFE_BLOCK)
unset(SETTING_USE_STRUCTS_PACKED)
unset(SETTING_BUILD_DEBUG)
