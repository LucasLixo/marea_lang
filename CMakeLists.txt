# Copyright (c) 2026 Paulo Lucas Alves Franca
# Licensed under the MIT License. See LICENSE.txt for full terms.

cmake_minimum_required(VERSION 3.20)

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
target_include_directories(marea PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/lib/internal")

option(MAREA_OPTION_TARGET_TESTS "Build the GoogleTest suite under tests/" ON)

if(MAREA_OPTION_TARGET_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
