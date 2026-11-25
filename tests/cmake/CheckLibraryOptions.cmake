# Copyright (c) 2026 Paulo Lucas Alves Franca
# Licensed under the MIT License. See LICENSE.txt for full terms.

# CTest script: configures the project in a scratch folder with one combination of the library options and checks the result
cmake_minimum_required(VERSION 3.20)

foreach(
    TEMP_REQUIRED
    IN
    ITEMS SOURCE BINARY GENERATOR C_COMPILER CXX_COMPILER STATIC SHARED EXPECT
)
    if(NOT DEFINED CHECK_${TEMP_REQUIRED})
        message(FATAL_ERROR "CheckLibraryOptions: missing required argument CHECK_${TEMP_REQUIRED}")
    endif()
endforeach()
unset(TEMP_REQUIRED)

file(REMOVE_RECURSE "${CHECK_BINARY}")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -S "${CHECK_SOURCE}" -B "${CHECK_BINARY}" -G "${CHECK_GENERATOR}"
        "-DCMAKE_C_COMPILER=${CHECK_C_COMPILER}" "-DCMAKE_CXX_COMPILER=${CHECK_CXX_COMPILER}"
        -DMAREA_OPTION_TARGET_TESTS=OFF "-DMAREA_OPTION_STATIC_LIBS=${CHECK_STATIC}"
        "-DMAREA_OPTION_SHARED_LIBS=${CHECK_SHARED}"
    RESULT_VARIABLE TEMP_RESULT
    OUTPUT_VARIABLE TEMP_OUTPUT
    ERROR_VARIABLE TEMP_ERROR
)

if(CHECK_EXPECT STREQUAL "FAIL")
    if(TEMP_RESULT EQUAL 0)
        message(FATAL_ERROR "Configure must fail when both library options are OFF, but it succeeded")
    endif()
    if(NOT TEMP_ERROR MATCHES "MAREA_OPTION_STATIC_LIBS")
        message(FATAL_ERROR "The failure message must name MAREA_OPTION_STATIC_LIBS, got: ${TEMP_ERROR}")
    endif()
    return()
endif()

if(NOT TEMP_RESULT EQUAL 0)
    message(FATAL_ERROR "Configure failed (${TEMP_RESULT}): ${TEMP_ERROR}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${CHECK_BINARY}" --target help
    RESULT_VARIABLE TEMP_RESULT
    OUTPUT_VARIABLE TEMP_TARGETS
)
if(NOT TEMP_RESULT EQUAL 0)
    message(FATAL_ERROR "Listing the targets failed (${TEMP_RESULT})")
endif()

foreach(
    TEMP_STAGE
    IN
    ITEMS Base Lex Parse Sema Ir
)
    foreach(TEMP_KIND IN ITEMS STATIC SHARED)
        if(TEMP_KIND STREQUAL "STATIC")
            set(TEMP_NAME "Marea-${TEMP_STAGE}")
        else()
            set(TEMP_NAME "Marea-${TEMP_STAGE}-Shared")
        endif()
        if(TEMP_TARGETS MATCHES "(^|\n)${TEMP_NAME}: " OR TEMP_TARGETS MATCHES "\n[.][.][.] ${TEMP_NAME}\n")
            set(TEMP_PRESENT TRUE)
        else()
            set(TEMP_PRESENT FALSE)
        endif()
        if(CHECK_${TEMP_KIND} AND NOT TEMP_PRESENT)
            message(FATAL_ERROR "The target ${TEMP_NAME} must exist with ${TEMP_KIND} ON")
        endif()
        if(NOT CHECK_${TEMP_KIND} AND TEMP_PRESENT)
            message(FATAL_ERROR "The target ${TEMP_NAME} must not exist with ${TEMP_KIND} OFF")
        endif()
    endforeach()
endforeach()
