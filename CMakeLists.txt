# Copyright (c) 2026 Paulo Lucas Alves Franca
# Licensed under the MIT License. See LICENSE.txt for full terms.

cmake_minimum_required(VERSION 3.20)

project(marea_lang C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(marea STATIC
    lib/source/marea/MAREAVersion.c
)

target_include_directories(marea PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/include"
)
