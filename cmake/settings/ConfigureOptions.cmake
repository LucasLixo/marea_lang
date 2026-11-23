# Copyright (c) 2026 Paulo Lucas Alves Franca
# Licensed under the MIT License. See LICENSE.txt for full terms.

cmake_minimum_required(VERSION 3.20)

get_property(TEMP_GUARD_SET GLOBAL PROPERTY ConfigureOptionsGuard SET)
if(TEMP_GUARD_SET)
    message(FATAL_ERROR "ConfigureOptions.cmake was included twice")
endif()
set_property(GLOBAL PROPERTY ConfigureOptionsGuard ON)
unset(TEMP_GUARD_SET)

# Every MAREA_OPTION_* of the project, declared in one block
macro(ConfigureOptions)
    option(MAREA_OPTION_TARGET_TESTS "Build the GoogleTest suite under tests/" ON)
    option(
        MAREA_OPTION_SHARED_LIBS
        "Build Marea-Base, Marea-Lex, Marea-Parse, Marea-Sema and Marea-Ir as shared libraries instead of static"
        OFF
    )
    option(MAREA_OPTION_PRECOMPILE_HEADER "Precompile the most basic headers in every library target" ON)
    option(MAREA_OPTION_TARGET_BASE "Build the Marea-Base library" ON)
    cmake_dependent_option(
        MAREA_OPTION_TARGET_LEX
        "Build the Marea-Lex library"
        ON
        "MAREA_OPTION_TARGET_BASE"
        OFF
    )
    cmake_dependent_option(
        MAREA_OPTION_TARGET_PARSE
        "Build the Marea-Parse library"
        ON
        "MAREA_OPTION_TARGET_LEX"
        OFF
    )
    cmake_dependent_option(
        MAREA_OPTION_TARGET_SEMA
        "Build the Marea-Sema library"
        ON
        "MAREA_OPTION_TARGET_PARSE"
        OFF
    )
    cmake_dependent_option(
        MAREA_OPTION_TARGET_IR
        "Build the Marea-Ir library"
        ON
        "MAREA_OPTION_TARGET_SEMA"
        OFF
    )
endmacro()
