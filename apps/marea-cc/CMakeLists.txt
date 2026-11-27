# Copyright (c) 2026 Paulo Lucas Alves Franca
# Licensed under the MIT License. See LICENSE.txt for full terms.

cmake_minimum_required(VERSION 3.20)

get_property(TEMP_GUARD_SET GLOBAL PROPERTY MareaAppsMareaCcGuard SET)
if(TEMP_GUARD_SET)
    message(FATAL_ERROR "apps/marea-cc/CMakeLists.txt was included twice")
endif()
set_property(GLOBAL PROPERTY MareaAppsMareaCcGuard ON)
unset(TEMP_GUARD_SET)

# marea-cc-driver: the testable internal pipeline (generate-c/compile/list-settings), linked by the
# real executable below and by marea_tests through its own internal/ header.
file(GLOB_RECURSE MAREA_APPS_MAREA_CC_DRIVER_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/internal/*.c")

add_library(marea-cc-driver STATIC ${MAREA_APPS_MAREA_CC_DRIVER_SOURCES})

target_include_directories(marea-cc-driver PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/internal")

target_link_libraries(marea-cc-driver PUBLIC marea-shared marea::cgen)

# marea-cc: the real executable, a thin command-line argument parser over marea-cc-driver.
add_executable(marea-cc "${CMAKE_CURRENT_SOURCE_DIR}/source/MareaCcMain.c")

target_link_libraries(marea-cc PRIVATE marea-cc-driver marea-shared)
