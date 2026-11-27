# Copyright (c) 2026 Paulo Lucas Alves Franca
# Licensed under the MIT License. See LICENSE.txt for full terms.

# CTest script: configures the MareaFunctions fixture project and checks marea_add_library/marea_add_executable tool detection
cmake_minimum_required(VERSION 3.20)

foreach(
    TEMP_REQUIRED
    IN
    ITEMS SOURCE BINARY GENERATOR C_COMPILER CXX_COMPILER MAREA_ROOT STUB_DIR EXPECT
)
    if(NOT DEFINED CHECK_${TEMP_REQUIRED})
        message(FATAL_ERROR "CheckMareaFunctions: missing required argument CHECK_${TEMP_REQUIRED}")
    endif()
endforeach()
unset(TEMP_REQUIRED)

file(REMOVE_RECURSE "${CHECK_BINARY}")

# Locate the build tool while PATH is still normal: the generator must keep working once PATH is isolated below
find_program(TEMP_MAKE_PROGRAM ninja make)

# Isolate PATH to only the stand-in directory (when one is given), so a real marea-cc/marea-bc on this machine never masks the check
set(ENV{PATH} "${CHECK_STUB_DIR}")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -S "${CHECK_SOURCE}" -B "${CHECK_BINARY}" -G "${CHECK_GENERATOR}"
        "-DCMAKE_MAKE_PROGRAM=${TEMP_MAKE_PROGRAM}" "-DCMAKE_C_COMPILER=${CHECK_C_COMPILER}"
        "-DCMAKE_CXX_COMPILER=${CHECK_CXX_COMPILER}" "-DCHECK_MAREA_ROOT=${CHECK_MAREA_ROOT}"
    RESULT_VARIABLE TEMP_RESULT
    OUTPUT_VARIABLE TEMP_OUTPUT
    ERROR_VARIABLE TEMP_ERROR
)

if(CHECK_EXPECT STREQUAL "FAIL")
    if(TEMP_RESULT EQUAL 0)
        message(FATAL_ERROR "Configure must fail when marea-cc/marea-bc are not on PATH, but it succeeded")
    endif()
    if(NOT TEMP_ERROR MATCHES "marea-cc" AND NOT TEMP_ERROR MATCHES "marea-bc")
        message(FATAL_ERROR "The failure message must name the missing tool, got: ${TEMP_ERROR}")
    endif()
    return()
endif()

if(NOT TEMP_RESULT EQUAL 0)
    message(FATAL_ERROR "Configure failed (${TEMP_RESULT}): ${TEMP_ERROR}")
endif()

if(NOT TEMP_OUTPUT MATCHES "demo[.]marea")
    message(FATAL_ERROR "The recorded compile plan must name the given source, got: ${TEMP_OUTPUT}")
endif()

if(NOT TEMP_OUTPUT MATCHES "demo_tool")
    message(FATAL_ERROR "The recorded compile plan must name the executable target, got: ${TEMP_OUTPUT}")
endif()
