# Copyright (c) 2026 Paulo Lucas Alves Franca
# Licensed under the MIT License. See LICENSE.txt for full terms.

cmake_minimum_required(VERSION 3.20)

get_property(TEMP_GUARD_SET GLOBAL PROPERTY MareaAppsMareaBcGuard SET)
if(TEMP_GUARD_SET)
    message(FATAL_ERROR "apps/marea-bc/CMakeLists.txt was included twice")
endif()
set_property(GLOBAL PROPERTY MareaAppsMareaBcGuard ON)
unset(TEMP_GUARD_SET)

# marea-bc-driver: the testable internal pipeline (compile/assemble/disassemble/dump-*/list-settings),
# linked by the real executable below and by marea_tests through its own internal/ header.
file(GLOB_RECURSE MAREA_APPS_MAREA_BC_DRIVER_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/internal/*.c")

add_library(marea-bc-driver STATIC ${MAREA_APPS_MAREA_BC_DRIVER_SOURCES})

target_include_directories(marea-bc-driver PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/internal")

target_link_libraries(marea-bc-driver PUBLIC marea-shared)

# marea-bc: the real executable, a thin command-line argument parser over marea-bc-driver.
add_executable(marea-bc "${CMAKE_CURRENT_SOURCE_DIR}/source/MareaBcMain.c")

target_link_libraries(marea-bc PRIVATE marea-bc-driver marea-shared)
