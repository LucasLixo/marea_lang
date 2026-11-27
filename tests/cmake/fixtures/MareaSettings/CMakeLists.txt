# Copyright (c) 2026 Paulo Lucas Alves Franca
# Licensed under the MIT License. See LICENSE.txt for full terms.

# Tiny fixture project: proves SETTINGS/MAREA_SETTING_ and the include-bytes dependency table without the real tools
cmake_minimum_required(VERSION 3.20)
project(MareaSettingsFixture LANGUAGES C)

if(NOT DEFINED CHECK_MAREA_ROOT)
    message(FATAL_ERROR "MareaSettingsFixture: missing required -DCHECK_MAREA_ROOT")
endif()

include("${CHECK_MAREA_ROOT}/cmake/functions/Basic.cmake")
include("${CHECK_MAREA_ROOT}/cmake/functions/FindExecutable.cmake")
include("${CHECK_MAREA_ROOT}/cmake/settings/ConfigureMareaFunctions.cmake")
configuremareafunctions()

# The outer check passes lists joined with '|' (a CMake list of ';' cannot survive one -D argument), split here
set(TEMP_CALL_ARGS "")
if(DEFINED CHECK_SETTINGS)
    string(REPLACE "|" ";" TEMP_SETTINGS_LIST "${CHECK_SETTINGS}")
    list(APPEND TEMP_CALL_ARGS SETTINGS ${TEMP_SETTINGS_LIST})
endif()
if(DEFINED CHECK_LIBRARIES)
    string(REPLACE "|" ";" TEMP_LIBRARIES_LIST "${CHECK_LIBRARIES}")
    list(APPEND TEMP_CALL_ARGS LIBRARIES ${TEMP_LIBRARIES_LIST})
endif()
if(DEFINED CHECK_MACROS)
    string(REPLACE "|" ";" TEMP_MACROS_LIST "${CHECK_MACROS}")
    list(APPEND TEMP_CALL_ARGS MACROS ${TEMP_MACROS_LIST})
endif()

marea_add_library(demo SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/demo.marea" ${TEMP_CALL_ARGS})

get_target_property(TEMP_DEMO_DEFINITIONS demo INTERFACE_COMPILE_DEFINITIONS)
message(STATUS "demo compile definitions: ${TEMP_DEMO_DEFINITIONS}")
