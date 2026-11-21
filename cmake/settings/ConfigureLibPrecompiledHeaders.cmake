# Copyright (c) 2026 Paulo Lucas Alves Franca
# Licensed under the MIT License. See LICENSE.txt for full terms.

cmake_minimum_required(VERSION 3.20)

get_property(TEMP_GUARD_SET GLOBAL PROPERTY ConfigureLibPrecompiledHeadersGuard SET)
if(TEMP_GUARD_SET)
    message(FATAL_ERROR "ConfigureLibPrecompiledHeaders.cmake was included twice")
endif()
set_property(GLOBAL PROPERTY ConfigureLibPrecompiledHeadersGuard ON)
unset(TEMP_GUARD_SET)

# Precompiled header of every stage target (PRIVATE, as nothing links it), gated to C so C++ units never get it
macro(ConfigureLibPrecompiledHeaders)
    if(MAREA_OPTION_PRECOMPILE_HEADER)
        foreach(TEMP_STAGE_TARGET IN ITEMS Marea-Base Marea-Lex Marea-Parse Marea-Sema)
            if(TARGET ${TEMP_STAGE_TARGET})
                target_precompile_headers(
                    ${TEMP_STAGE_TARGET}
                    PRIVATE "$<$<COMPILE_LANGUAGE:C>:${MAREA_FOLDER_LIB_INTERNAL}/marea/internal/PrecompileHeader.h>"
                )
                target_compile_definitions(${TEMP_STAGE_TARGET} PRIVATE SETTING_PRECOMPILE_HEADER)
            endif()
        endforeach()
        unset(TEMP_STAGE_TARGET)
    endif()
endmacro()
