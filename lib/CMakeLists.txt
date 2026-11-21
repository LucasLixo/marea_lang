# Copyright (c) 2026 Paulo Lucas Alves Franca
# Licensed under the MIT License. See LICENSE.txt for full terms.

cmake_minimum_required(VERSION 3.20)

get_property(MAREA_LIB_GUARD_SET GLOBAL PROPERTY MareaLibGuard SET)
if(MAREA_LIB_GUARD_SET)
    message(FATAL_ERROR "lib/CMakeLists.txt was included twice")
endif()
set_property(GLOBAL PROPERTY MareaLibGuard ON)
unset(MAREA_LIB_GUARD_SET)

if(MAREA_OPTION_SHARED_LIBS)
    set(MAREA_LIBRARY_TYPE SHARED)
else()
    set(MAREA_LIBRARY_TYPE STATIC)
endif()

if(MAREA_OPTION_TARGET_BASE)
    message(STATUS "Add target: marea_base")

    file(GLOB MAREA_BASE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/marea/base/*.c")

    add_library(marea_base ${MAREA_LIBRARY_TYPE} ${MAREA_BASE_SOURCES})
    add_library(marea::base ALIAS marea_base)

    target_include_directories(
        marea_base
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/internal"
    )

    target_compile_options(marea_base PRIVATE "$<$<C_COMPILER_ID:Clang,AppleClang,GNU>:-Wall;-Wextra;-Wpedantic>")

    if(MAREA_OPTION_SHARED_LIBS)
        set_target_properties(marea_base PROPERTIES C_VISIBILITY_PRESET hidden)
        target_compile_definitions(marea_base PRIVATE MAREA_ENABLE_EXPORT INTERFACE MAREA_ENABLE_IMPORT)
    endif()
endif()

if(MAREA_OPTION_TARGET_LEX)
    message(STATUS "Add target: marea_lex")

    file(GLOB MAREA_LEX_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/marea/lex/*.c")

    add_library(marea_lex ${MAREA_LIBRARY_TYPE} ${MAREA_LEX_SOURCES})
    add_library(marea::lex ALIAS marea_lex)

    target_include_directories(
        marea_lex
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/internal"
    )

    target_compile_options(marea_lex PRIVATE "$<$<C_COMPILER_ID:Clang,AppleClang,GNU>:-Wall;-Wextra;-Wpedantic>")

    target_link_libraries(marea_lex PUBLIC marea::base)

    if(MAREA_OPTION_SHARED_LIBS)
        set_target_properties(marea_lex PROPERTIES C_VISIBILITY_PRESET hidden)
        target_compile_definitions(marea_lex PRIVATE MAREA_ENABLE_EXPORT INTERFACE MAREA_ENABLE_IMPORT)
    endif()
endif()

if(MAREA_OPTION_PRECOMPILE_HEADER)
    foreach(MAREA_STAGE_TARGET IN ITEMS marea_base marea_lex)
        if(TARGET ${MAREA_STAGE_TARGET})
            # PRIVATE: nothing links the precompiled header; the C gate keeps it out of C++ units.
            target_precompile_headers(
                ${MAREA_STAGE_TARGET}
                PRIVATE "$<$<COMPILE_LANGUAGE:C>:${CMAKE_CURRENT_SOURCE_DIR}/internal/marea/internal/PrecompileHeader.h>"
            )
            target_compile_definitions(${MAREA_STAGE_TARGET} PRIVATE SETTING_PRECOMPILE_HEADER)
        endif()
    endforeach()
    unset(MAREA_STAGE_TARGET)
endif()
