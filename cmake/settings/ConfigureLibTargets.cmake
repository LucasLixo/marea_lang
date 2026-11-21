# Copyright (c) 2026 Paulo Lucas Alves Franca
# Licensed under the MIT License. See LICENSE.txt for full terms.

cmake_minimum_required(VERSION 3.20)

get_property(TEMP_GUARD_SET GLOBAL PROPERTY ConfigureLibTargetsGuard SET)
if(TEMP_GUARD_SET)
    message(FATAL_ERROR "ConfigureLibTargets.cmake was included twice")
endif()
set_property(GLOBAL PROPERTY ConfigureLibTargetsGuard ON)
unset(TEMP_GUARD_SET)

# One library target per compiler stage, with its alias in the marea:: namespace
macro(ConfigureLibTargets)
    if(MAREA_OPTION_SHARED_LIBS)
        set(MAREA_LIBRARY_TYPE SHARED)
    else()
        set(MAREA_LIBRARY_TYPE STATIC)
    endif()

    if(MAREA_OPTION_TARGET_BASE)
        message(STATUS "Add target: Marea-Base")

        file(GLOB MAREA_BASE_SOURCES CONFIGURE_DEPENDS "${MAREA_FOLDER_LIB_SOURCE}/marea/base/*.c")

        add_library(Marea-Base ${MAREA_LIBRARY_TYPE} ${MAREA_BASE_SOURCES})
        add_library(marea::base ALIAS Marea-Base)

        target_link_libraries(Marea-Base PUBLIC marea::include PRIVATE marea::internal)

        target_compile_options(Marea-Base PRIVATE "$<$<C_COMPILER_ID:Clang,AppleClang,GNU>:-Wall;-Wextra;-Wpedantic>")

        if(MAREA_OPTION_SHARED_LIBS)
            set_target_properties(Marea-Base PROPERTIES C_VISIBILITY_PRESET hidden)
            target_compile_definitions(Marea-Base PRIVATE MAREA_ENABLE_EXPORT INTERFACE MAREA_ENABLE_IMPORT)
        endif()
    endif()

    if(MAREA_OPTION_TARGET_LEX)
        message(STATUS "Add target: Marea-Lex")

        file(GLOB MAREA_LEX_SOURCES CONFIGURE_DEPENDS "${MAREA_FOLDER_LIB_SOURCE}/marea/lex/*.c")

        add_library(Marea-Lex ${MAREA_LIBRARY_TYPE} ${MAREA_LEX_SOURCES})
        add_library(marea::lex ALIAS Marea-Lex)

        target_link_libraries(Marea-Lex PUBLIC marea::include PRIVATE marea::internal)

        target_compile_options(Marea-Lex PRIVATE "$<$<C_COMPILER_ID:Clang,AppleClang,GNU>:-Wall;-Wextra;-Wpedantic>")

        target_link_libraries(Marea-Lex PUBLIC marea::base)

        if(MAREA_OPTION_SHARED_LIBS)
            set_target_properties(Marea-Lex PROPERTIES C_VISIBILITY_PRESET hidden)
            target_compile_definitions(Marea-Lex PRIVATE MAREA_ENABLE_EXPORT INTERFACE MAREA_ENABLE_IMPORT)
        endif()
    endif()
endmacro()
