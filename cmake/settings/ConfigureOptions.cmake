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
        MAREA_OPTION_STATIC_LIBS
        "Build every stage library as a static library (Marea-<Stage>), next to the shared one when both options are ON"
        ON
    )
    option(
        MAREA_OPTION_SHARED_LIBS
        "Build every stage library also as a shared library (Marea-<Stage>-Shared), next to the static one when both options are ON"
        OFF
    )
    if(NOT MAREA_OPTION_STATIC_LIBS AND NOT MAREA_OPTION_SHARED_LIBS)
        message(
            FATAL_ERROR
            "Both MAREA_OPTION_STATIC_LIBS and MAREA_OPTION_SHARED_LIBS are OFF, so no library would be built. Set at least one to ON, for example: -DMAREA_OPTION_STATIC_LIBS=ON"
        )
    endif()
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
    cmake_dependent_option(
        MAREA_OPTION_TARGET_BYTECODE
        "Build the Marea-Bytecode library"
        ON
        "MAREA_OPTION_TARGET_IR"
        OFF
    )
    cmake_dependent_option(
        MAREA_OPTION_TARGET_VM
        "Build the Marea-Vm library"
        ON
        "MAREA_OPTION_TARGET_BYTECODE"
        OFF
    )
    cmake_dependent_option(
        MAREA_OPTION_TARGET_CGEN
        "Build the Marea-Cgen library"
        ON
        "MAREA_OPTION_TARGET_IR"
        OFF
    )
endmacro()
