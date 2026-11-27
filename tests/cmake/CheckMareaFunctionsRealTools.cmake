# Copyright (c) 2026 Paulo Lucas Alves Franca
# Licensed under the MIT License. See LICENSE.txt for full terms.

# CTest script: builds this repository's own real marea-cc/marea-bc in a first scratch folder, then
# proves FindExecutable resolves them (not a stand-in) and that each one actually runs end to end.
cmake_minimum_required(VERSION 3.20)

foreach(
    TEMP_REQUIRED
    IN
    ITEMS SOURCE TOOLS_BINARY FIXTURE_BINARY GENERATOR C_COMPILER CXX_COMPILER MAREA_ROOT FIXTURE_DIR
)
    if(NOT DEFINED CHECK_${TEMP_REQUIRED})
        message(FATAL_ERROR "CheckMareaFunctionsRealTools: missing required argument CHECK_${TEMP_REQUIRED}")
    endif()
endforeach()
unset(TEMP_REQUIRED)

file(REMOVE_RECURSE "${CHECK_TOOLS_BINARY}")
file(REMOVE_RECURSE "${CHECK_FIXTURE_BINARY}")

# Locate the build tool while PATH is still normal, exactly like CheckMareaFunctions.cmake.
find_program(TEMP_MAKE_PROGRAM ninja make)

# Step 1: build this repository's own marea-cc/marea-bc for real.
execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -S "${CHECK_SOURCE}" -B "${CHECK_TOOLS_BINARY}" -G "${CHECK_GENERATOR}"
        "-DCMAKE_MAKE_PROGRAM=${TEMP_MAKE_PROGRAM}" "-DCMAKE_C_COMPILER=${CHECK_C_COMPILER}"
        "-DCMAKE_CXX_COMPILER=${CHECK_CXX_COMPILER}" -DMAREA_OPTION_TARGET_TESTS=OFF -DMAREA_OPTION_TARGET_CC=ON
        -DMAREA_OPTION_TARGET_BC=ON
    RESULT_VARIABLE TEMP_RESULT
    OUTPUT_VARIABLE TEMP_OUTPUT
    ERROR_VARIABLE TEMP_ERROR
)
if(NOT TEMP_RESULT EQUAL 0)
    message(FATAL_ERROR "Configuring the real tools failed (${TEMP_RESULT}): ${TEMP_ERROR}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${CHECK_TOOLS_BINARY}" --target marea-cc --target marea-bc
    RESULT_VARIABLE TEMP_RESULT
    OUTPUT_VARIABLE TEMP_OUTPUT
    ERROR_VARIABLE TEMP_ERROR
)
if(NOT TEMP_RESULT EQUAL 0)
    message(FATAL_ERROR "Building the real marea-cc/marea-bc failed (${TEMP_RESULT}): ${TEMP_ERROR}")
endif()

file(GLOB_RECURSE TEMP_REAL_MAREA_CC "${CHECK_TOOLS_BINARY}/*/marea-cc")
file(GLOB_RECURSE TEMP_REAL_MAREA_BC "${CHECK_TOOLS_BINARY}/*/marea-bc")
if(NOT TEMP_REAL_MAREA_CC)
    message(FATAL_ERROR "The built marea-cc executable was not found under ${CHECK_TOOLS_BINARY}")
endif()
if(NOT TEMP_REAL_MAREA_BC)
    message(FATAL_ERROR "The built marea-bc executable was not found under ${CHECK_TOOLS_BINARY}")
endif()
list(GET TEMP_REAL_MAREA_CC 0 TEMP_REAL_MAREA_CC)
list(GET TEMP_REAL_MAREA_BC 0 TEMP_REAL_MAREA_BC)
get_filename_component(TEMP_REAL_MAREA_CC_DIR "${TEMP_REAL_MAREA_CC}" DIRECTORY)
get_filename_component(TEMP_REAL_MAREA_BC_DIR "${TEMP_REAL_MAREA_BC}" DIRECTORY)

# Step 2: configure the MareaFunctions fixture with that output directory on CMAKE_PROGRAM_PATH, and
# PATH isolated to nothing (the build tool is already located above), so a real binary on this
# machine's own PATH never masks what CMAKE_PROGRAM_PATH alone resolves.
set(ENV{PATH} "")
execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -S "${CHECK_SOURCE}/tests/cmake/fixtures/MareaFunctions" -B "${CHECK_FIXTURE_BINARY}" -G
        "${CHECK_GENERATOR}" "-DCMAKE_MAKE_PROGRAM=${TEMP_MAKE_PROGRAM}" "-DCMAKE_C_COMPILER=${CHECK_C_COMPILER}"
        "-DCMAKE_CXX_COMPILER=${CHECK_CXX_COMPILER}" "-DCHECK_MAREA_ROOT=${CHECK_MAREA_ROOT}"
        "-DCMAKE_PROGRAM_PATH=${TEMP_REAL_MAREA_CC_DIR};${TEMP_REAL_MAREA_BC_DIR}"
    RESULT_VARIABLE TEMP_RESULT
    OUTPUT_VARIABLE TEMP_OUTPUT
    ERROR_VARIABLE TEMP_ERROR
)
if(NOT TEMP_RESULT EQUAL 0)
    message(FATAL_ERROR "Configuring the fixture against the real tools failed (${TEMP_RESULT}): ${TEMP_ERROR}")
endif()

if(NOT TEMP_OUTPUT MATCHES "marea-cc found: ${TEMP_REAL_MAREA_CC}")
    message(FATAL_ERROR "FindExecutable must resolve the real, freshly built marea-cc, got: ${TEMP_OUTPUT}")
endif()
if(NOT TEMP_OUTPUT MATCHES "marea-bc found: ${TEMP_REAL_MAREA_BC}")
    message(FATAL_ERROR "FindExecutable must resolve the real, freshly built marea-bc, got: ${TEMP_OUTPUT}")
endif()

# Step 3: marea_add_library/marea_add_executable only record the compile plan at configure time (see
# ConfigureMareaFunctions.cmake, owned by the sibling apply) - they never invoke marea-cc/marea-bc as
# a build step. So this proves the real binary FindExecutable just resolved actually works, by running
# it directly against a real ".marea" fixture, end to end.
# demo.marea's own namespace is "demo" (one segment); sema's namespace-matches-file-path check
# compares the whole given path against the dotted namespace, so the input must be exactly
# "demo.marea" with the fixture directory as the working directory (the same shape
# MareaBcDriver.cpp's own test uses for examples/basics/operators.marea).
execute_process(
    COMMAND "${TEMP_REAL_MAREA_CC}" "demo.marea" -o "${CHECK_FIXTURE_BINARY}/demo.c" --emit c
    WORKING_DIRECTORY "${CHECK_FIXTURE_DIR}"
    RESULT_VARIABLE TEMP_RESULT
    OUTPUT_VARIABLE TEMP_OUTPUT
    ERROR_VARIABLE TEMP_ERROR
)
if(NOT TEMP_RESULT EQUAL 0)
    message(FATAL_ERROR "The real marea-cc failed on demo.marea (${TEMP_RESULT}): ${TEMP_ERROR}")
endif()
if(NOT EXISTS "${CHECK_FIXTURE_BINARY}/demo.c")
    message(FATAL_ERROR "The real marea-cc did not write demo.c")
endif()

execute_process(
    COMMAND "${TEMP_REAL_MAREA_BC}" compile "demo.marea" -o "${CHECK_FIXTURE_BINARY}/demo.mbc"
    WORKING_DIRECTORY "${CHECK_FIXTURE_DIR}"
    RESULT_VARIABLE TEMP_RESULT
    OUTPUT_VARIABLE TEMP_OUTPUT
    ERROR_VARIABLE TEMP_ERROR
)
if(NOT TEMP_RESULT EQUAL 0)
    message(FATAL_ERROR "The real marea-bc failed on demo.marea (${TEMP_RESULT}): ${TEMP_ERROR}")
endif()
if(NOT EXISTS "${CHECK_FIXTURE_BINARY}/demo.mbc")
    message(FATAL_ERROR "The real marea-bc did not write demo.mbc")
endif()
