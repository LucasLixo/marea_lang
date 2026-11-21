# Copyright (c) 2026 Paulo Lucas Alves Franca
# Licensed under the MIT License. See LICENSE.txt for full terms.

cmake_minimum_required(VERSION 3.20)

include(CMakeDependentOption)

project(marea_lang C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# C++ is used only for the GoogleTest suite under tests/ (see cpp-testing) - never for production code.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(marea STATIC lib/source/marea/MAREAVersion.c)

target_include_directories(marea PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/lib/include")

option(MAREA_OPTION_TARGET_TESTS "Build the GoogleTest suite under tests/" ON)
option(MAREA_OPTION_SHARED_LIBS "Build marea_base and marea_lex as shared libraries instead of static" OFF)
option(MAREA_OPTION_PRECOMPILE_HEADER "Precompile the most basic headers in every library target" ON)
option(MAREA_OPTION_TARGET_BASE "Build the marea_base library" ON)
cmake_dependent_option(
    MAREA_OPTION_TARGET_LEX
    "Build the marea_lex library"
    ON
    "MAREA_OPTION_TARGET_BASE"
    OFF
)

if(MAREA_OPTION_TARGET_TESTS AND NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/GoogleTest/CMakeLists.txt")
    message(FATAL_ERROR "3rdparty/GoogleTest is missing - run: mise run setup:git-modules")
endif()

add_subdirectory(lib)

if(MAREA_OPTION_TARGET_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
