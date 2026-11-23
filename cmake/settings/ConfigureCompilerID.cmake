# Copyright (c) 2026 Paulo Lucas Alves Franca
# Licensed under the MIT License. See LICENSE.txt for full terms.

cmake_minimum_required(VERSION 3.20)

get_property(TEMP_GUARD_SET GLOBAL PROPERTY ConfigureCompilerIDGuard SET)
if(TEMP_GUARD_SET)
    message(FATAL_ERROR "ConfigureCompilerID.cmake was included twice")
endif()
set_property(GLOBAL PROPERTY ConfigureCompilerIDGuard ON)
unset(TEMP_GUARD_SET)

# Compiler family flags; the toolchain is clang/LLVM, so GCC-like is the expected one
macro(ConfigureCompilerID)
    set(MAREA_COMPILER_IS_GCC_LIKE OFF)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang|IntelLLVM")
        set(MAREA_COMPILER_IS_GCC_LIKE ON)
    endif()

    set(MAREA_COMPILER_IS_MSVC_LIKE OFF)
    if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
        set(MAREA_COMPILER_IS_MSVC_LIKE ON)
    endif()

    message(STATUS "Compiler Is GCC Like: ${MAREA_COMPILER_IS_GCC_LIKE}")
    message(STATUS "Compiler Is MSVC Like: ${MAREA_COMPILER_IS_MSVC_LIKE}")
endmacro()
