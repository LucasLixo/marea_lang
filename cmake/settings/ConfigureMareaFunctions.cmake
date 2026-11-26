# Copyright (c) 2026 Paulo Lucas Alves Franca
# Licensed under the MIT License. See LICENSE.txt for full terms.

cmake_minimum_required(VERSION 3.20)

get_property(TEMP_GUARD_SET GLOBAL PROPERTY ConfigureMareaFunctionsGuard SET)
if(TEMP_GUARD_SET)
    message(FATAL_ERROR "ConfigureMareaFunctions.cmake was included twice")
endif()
set_property(GLOBAL PROPERTY ConfigureMareaFunctionsGuard ON)
unset(TEMP_GUARD_SET)

# Locate marea-cc/marea-bc and record the compile plan shared by marea_add_library and marea_add_executable
function(MareaBuildMareaSources TEMP_TARGET_NAME)
    BasicParseArguments(
        MareaBuildMareaSources
        PARAM
        ""
        ""
        "SOURCES"
        REQUIRED
        SOURCES
        TYPES
        SOURCES:LIST
        ARGS
        ${ARGN}
    )

    FindExecutable(NAME "marea-cc" OUTPUT TEMP_MAREA_CC REQUIRED)
    FindExecutable(NAME "marea-bc" OUTPUT TEMP_MAREA_BC REQUIRED)

    message(STATUS "marea target ${TEMP_TARGET_NAME}: ${TEMP_MAREA_CC} ${TEMP_MAREA_BC} SOURCES ${PARAM_SOURCES}")

    unset(TEMP_MAREA_CC)
    unset(TEMP_MAREA_BC)
endfunction()

# Build a Marea library target from its sources: marea_add_library(<name> SOURCES <source>...)
function(marea_add_library TEMP_TARGET_NAME)
    MareaBuildMareaSources("${TEMP_TARGET_NAME}" ${ARGN})
endfunction()

# Build a Marea executable target from its sources: marea_add_executable(<name> SOURCES <source>...)
function(marea_add_executable TEMP_TARGET_NAME)
    MareaBuildMareaSources("${TEMP_TARGET_NAME}" ${ARGN})
endfunction()

# marea_add_library and marea_add_executable are declared above and ready to use once this macro runs; the SETTINGS/MAREA_SETTING_ forms and the include-bytes dependency table extend this macro in a later part
macro(ConfigureMareaFunctions)
endmacro()
