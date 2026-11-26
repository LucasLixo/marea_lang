# Copyright (c) 2026 Paulo Lucas Alves Franca
# Licensed under the MIT License. See LICENSE.txt for full terms.

# CTest script: configures the MareaSettings fixture and checks SETTINGS/MAREA_SETTING_ normalization, override
# order and the include-bytes dependency table of marea_add_library/marea_add_executable
cmake_minimum_required(VERSION 3.20)

foreach(
    TEMP_REQUIRED
    IN
    ITEMS SOURCE BINARY GENERATOR C_COMPILER CXX_COMPILER MAREA_ROOT EXPECT
)
    if(NOT DEFINED CHECK_${TEMP_REQUIRED})
        message(FATAL_ERROR "CheckMareaSettings: missing required argument CHECK_${TEMP_REQUIRED}")
    endif()
endforeach()
unset(TEMP_REQUIRED)

file(REMOVE_RECURSE "${CHECK_BINARY}")
find_program(TEMP_MAKE_PROGRAM ninja make)

set(TEMP_CONFIGURE_ARGS
    "${CMAKE_COMMAND}"
    -S
    "${CHECK_SOURCE}"
    -B
    "${CHECK_BINARY}"
    -G
    "${CHECK_GENERATOR}"
    "-DCMAKE_MAKE_PROGRAM=${TEMP_MAKE_PROGRAM}"
    "-DCMAKE_C_COMPILER=${CHECK_C_COMPILER}"
    "-DCMAKE_CXX_COMPILER=${CHECK_CXX_COMPILER}"
    "-DCHECK_MAREA_ROOT=${CHECK_MAREA_ROOT}"
)

if(DEFINED CHECK_SETTINGS)
    list(APPEND TEMP_CONFIGURE_ARGS "-DCHECK_SETTINGS=${CHECK_SETTINGS}")
endif()
if(DEFINED CHECK_LIBRARIES)
    list(APPEND TEMP_CONFIGURE_ARGS "-DCHECK_LIBRARIES=${CHECK_LIBRARIES}")
endif()
if(DEFINED CHECK_MACROS)
    list(APPEND TEMP_CONFIGURE_ARGS "-DCHECK_MACROS=${CHECK_MACROS}")
endif()
if(DEFINED CHECK_KNOWN)
    list(APPEND TEMP_CONFIGURE_ARGS "-DCHECK_KNOWN=${CHECK_KNOWN}")
endif()

# A global MAREA_SETTING_<NAME> cache variable, one per CHECK_GLOBAL_SETTINGS entry (NAME=value, joined with '|')
if(DEFINED CHECK_GLOBAL_SETTINGS)
    string(REPLACE "|" ";" TEMP_GLOBAL_LIST "${CHECK_GLOBAL_SETTINGS}")
    foreach(TEMP_PAIR IN LISTS TEMP_GLOBAL_LIST)
        if(TEMP_PAIR MATCHES "^([^=]+)=(.*)$")
            list(APPEND TEMP_CONFIGURE_ARGS "-DMAREA_SETTING_${CMAKE_MATCH_1}=${CMAKE_MATCH_2}")
        endif()
    endforeach()
endif()

# Keep the stand-in marea-cc/marea-bc reachable: isolate PATH only when a stub directory is given
if(DEFINED CHECK_STUB_DIR)
    set(ENV{PATH} "${CHECK_STUB_DIR}")
endif()

execute_process(
    COMMAND ${TEMP_CONFIGURE_ARGS}
    RESULT_VARIABLE TEMP_RESULT
    OUTPUT_VARIABLE TEMP_OUTPUT
    ERROR_VARIABLE TEMP_ERROR
)

if(CHECK_EXPECT STREQUAL "FAIL")
    if(TEMP_RESULT EQUAL 0)
        message(FATAL_ERROR "Configure must fail, but it succeeded. Output: ${TEMP_OUTPUT}")
    endif()
    if(DEFINED CHECK_EXPECT_FAIL_CONTAINS)
        string(REPLACE "|" ";" TEMP_EXPECT_LIST "${CHECK_EXPECT_FAIL_CONTAINS}")
        foreach(TEMP_EXPECT IN LISTS TEMP_EXPECT_LIST)
            if(NOT TEMP_ERROR MATCHES "${TEMP_EXPECT}")
                message(FATAL_ERROR "The failure message must contain '${TEMP_EXPECT}', got: ${TEMP_ERROR}")
            endif()
        endforeach()
    endif()
    return()
endif()

if(NOT TEMP_RESULT EQUAL 0)
    message(FATAL_ERROR "Configure failed (${TEMP_RESULT}): ${TEMP_ERROR}")
endif()

# message(STATUS ...) lands on stdout, message(WARNING ...) on stderr: a check may need either
set(TEMP_ALL_OUTPUT "${TEMP_OUTPUT}${TEMP_ERROR}")

if(DEFINED CHECK_EXPECT_CONTAINS)
    string(REPLACE "|" ";" TEMP_EXPECT_LIST "${CHECK_EXPECT_CONTAINS}")
    foreach(TEMP_EXPECT IN LISTS TEMP_EXPECT_LIST)
        if(NOT TEMP_ALL_OUTPUT MATCHES "${TEMP_EXPECT}")
            message(FATAL_ERROR "The configure output must contain '${TEMP_EXPECT}', got: ${TEMP_ALL_OUTPUT}")
        endif()
    endforeach()
endif()

if(DEFINED CHECK_EXPECT_NOT_CONTAINS)
    string(REPLACE "|" ";" TEMP_NOT_EXPECT_LIST "${CHECK_EXPECT_NOT_CONTAINS}")
    foreach(TEMP_NOT_EXPECT IN LISTS TEMP_NOT_EXPECT_LIST)
        if(TEMP_ALL_OUTPUT MATCHES "${TEMP_NOT_EXPECT}")
            message(FATAL_ERROR "The configure output must not contain '${TEMP_NOT_EXPECT}', got: ${TEMP_ALL_OUTPUT}")
        endif()
    endforeach()
endif()
