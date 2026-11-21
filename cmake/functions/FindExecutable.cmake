# Copyright (c) 2026 Paulo Lucas Alves Franca
# Licensed under the MIT License. See LICENSE.txt for full terms.

cmake_minimum_required(VERSION 3.20)

get_property(TEMP_GUARD_SET GLOBAL PROPERTY FindExecutableGuard SET)
if(TEMP_GUARD_SET)
    message(FATAL_ERROR "FindExecutable.cmake was included twice")
endif()
set_property(GLOBAL PROPERTY FindExecutableGuard ON)
unset(TEMP_GUARD_SET)

# Prepare and get executable
function(FindExecutable)
    basicparsearguments(
        FindExecutable
        PARAM
        "REQUIRED"
        "NAME;OUTPUT"
        ""
        REQUIRED
        NAME
        OUTPUT
        TYPES
        NAME:STRING
        OUTPUT:NAME
        ARGS
        ${ARGN}
    )

    # Create unique cache variable name for each executable
    string(TOUPPER "${PARAM_NAME}" TEMP_EXECUTABLE_UPPER)
    string(REPLACE "-" "_" TEMP_EXECUTABLE_UPPER "${TEMP_EXECUTABLE_UPPER}")
    set(TEMP_CACHE_VAR_NAME "CACHE_FIND_PROGRAM_${TEMP_EXECUTABLE_UPPER}")

    # Clear any previous result to force fresh search
    unset(${TEMP_CACHE_VAR_NAME} CACHE)

    # find_program caches results so names are unique; HINTS adds mise's shim directory for callers that never sourced the shell profile (Android Studio's Gradle daemon).
    find_program(${TEMP_CACHE_VAR_NAME} NAMES ${PARAM_NAME} HINTS "$ENV{HOME}/.local/share/mise/shims")

    # If the executable is found, set the title and return
    if(${TEMP_CACHE_VAR_NAME})
        message(STATUS "${PARAM_NAME} found: ${${TEMP_CACHE_VAR_NAME}}")
        set(${PARAM_OUTPUT} "${${TEMP_CACHE_VAR_NAME}}" PARENT_SCOPE)
        return()
    endif()

    # If the executable is not found but it's not necessary, we can skip it
    if(NOT PARAM_REQUIRED)
        message(STATUS "${PARAM_NAME} not found (optional). Skipping.")
        set(${PARAM_OUTPUT} "" PARENT_SCOPE)
        return()
    endif()

    # If we reach here, the executable is necessary but not found
    message(STATUS "${PARAM_NAME} not found in system PATH.")
    message(FATAL_ERROR "Install ${PARAM_NAME} on your system")
endfunction()
