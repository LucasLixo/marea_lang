# Copyright (c) 2026 Paulo Lucas Alves Franca
# Licensed under the MIT License. See LICENSE.txt for full terms.

# Tiny fixture project: proves marea_add_library/marea_add_executable and marea-cc/marea-bc detection without the real tools
cmake_minimum_required(VERSION 3.20)
project(MareaFunctionsFixture LANGUAGES C)

if(NOT DEFINED CHECK_MAREA_ROOT)
    message(FATAL_ERROR "MareaFunctionsFixture: missing required -DCHECK_MAREA_ROOT")
endif()

include("${CHECK_MAREA_ROOT}/cmake/functions/Basic.cmake")
include("${CHECK_MAREA_ROOT}/cmake/functions/FindExecutable.cmake")
include("${CHECK_MAREA_ROOT}/cmake/settings/ConfigureMareaFunctions.cmake")
ConfigureMareaFunctions()

marea_add_library(demo SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/demo.marea")
marea_add_executable(demo_tool SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/demo.marea")
