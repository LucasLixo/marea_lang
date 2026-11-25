# Copyright (c) 2026 Paulo Lucas Alves Franca
# Licensed under the MIT License. See LICENSE.txt for full terms.

cmake_minimum_required(VERSION 3.20)

get_property(TEMP_GUARD_SET GLOBAL PROPERTY ConfigureLibTargetsGuard SET)
if(TEMP_GUARD_SET)
    message(FATAL_ERROR "ConfigureLibTargets.cmake was included twice")
endif()
set_property(GLOBAL PROPERTY ConfigureLibTargetsGuard ON)
unset(TEMP_GUARD_SET)

# One stage library: a static target (Marea-<NAME>), a shared target (Marea-<NAME>-Shared), or both, as the options say
function(MareaAddStageLibrary)
    BasicParseArguments(
        MareaAddStageLibrary
        PARAM
        ""
        "STAGE;NAME"
        "LINK"
        REQUIRED
        STAGE
        NAME
        TYPES
        STAGE:STRING
        NAME:STRING
        ARGS
        ${ARGN}
    )

    file(GLOB TEMP_SOURCES CONFIGURE_DEPENDS "${MAREA_FOLDER_LIB_SOURCE}/marea/${PARAM_STAGE}/*.c")

    foreach(TEMP_KIND IN ITEMS STATIC SHARED)
        if(NOT MAREA_OPTION_${TEMP_KIND}_LIBS)
            continue()
        endif()

        string(TOLOWER "${TEMP_KIND}" TEMP_KIND_LOWER)

        if(TEMP_KIND STREQUAL "STATIC")
            set(TEMP_TARGET "Marea-${PARAM_NAME}")
        else()
            set(TEMP_TARGET "Marea-${PARAM_NAME}-Shared")
        endif()

        message(STATUS "Add target: ${TEMP_TARGET}")

        add_library(${TEMP_TARGET} ${TEMP_KIND} ${TEMP_SOURCES})
        add_library(marea::${PARAM_STAGE}_${TEMP_KIND_LOWER} ALIAS ${TEMP_TARGET})

        # The static and the shared library of one stage share the same base file name
        set_target_properties(${TEMP_TARGET} PROPERTIES OUTPUT_NAME "Marea-${PARAM_NAME}")

        target_link_libraries(${TEMP_TARGET} PUBLIC marea::include PRIVATE marea::internal)

        target_compile_options(
            ${TEMP_TARGET}
            PRIVATE "$<$<C_COMPILER_ID:Clang,AppleClang,GNU>:-Wall;-Wextra;-Wpedantic>"
        )

        foreach(TEMP_LINK IN LISTS PARAM_LINK)
            target_link_libraries(${TEMP_TARGET} PUBLIC marea::${TEMP_LINK}_${TEMP_KIND_LOWER})
        endforeach()

        if(TEMP_KIND STREQUAL "SHARED")
            set_target_properties(${TEMP_TARGET} PROPERTIES C_VISIBILITY_PRESET hidden)
            target_compile_definitions(${TEMP_TARGET} PRIVATE MAREA_ENABLE_EXPORT INTERFACE MAREA_ENABLE_IMPORT)
        endif()
    endforeach()

    # The plain alias marea::<stage> names the static library when it exists, and the shared one otherwise
    if(MAREA_OPTION_STATIC_LIBS)
        add_library(marea::${PARAM_STAGE} ALIAS Marea-${PARAM_NAME})
    else()
        add_library(marea::${PARAM_STAGE} ALIAS Marea-${PARAM_NAME}-Shared)
    endif()
endfunction()

# Every stage library, each one linking the stage before it
macro(ConfigureLibTargets)
    if(MAREA_OPTION_TARGET_BASE)
        mareaaddstagelibrary(STAGE base NAME Base)
    endif()

    if(MAREA_OPTION_TARGET_LEX)
        mareaaddstagelibrary(STAGE lex NAME Lex LINK base)
    endif()

    if(MAREA_OPTION_TARGET_PARSE)
        mareaaddstagelibrary(STAGE parse NAME Parse LINK lex)
    endif()

    if(MAREA_OPTION_TARGET_SEMA)
        mareaaddstagelibrary(STAGE sema NAME Sema LINK parse)
    endif()

    if(MAREA_OPTION_TARGET_IR)
        mareaaddstagelibrary(STAGE ir NAME Ir LINK sema)
    endif()
endmacro()
