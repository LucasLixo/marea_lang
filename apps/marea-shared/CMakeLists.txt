# Copyright (c) 2026 Paulo Lucas Alves Franca
# Licensed under the MIT License. See LICENSE.txt for full terms.

cmake_minimum_required(VERSION 3.20)

get_property(TEMP_GUARD_SET GLOBAL PROPERTY MareaAppsSharedGuard SET)
if(TEMP_GUARD_SET)
    message(FATAL_ERROR "apps/marea-shared/CMakeLists.txt was included twice")
endif()
set_property(GLOBAL PROPERTY MareaAppsSharedGuard ON)
unset(TEMP_GUARD_SET)

# marea-shared: the pieces marea-cc and marea-bc both link (diagnostic printer, --libraries/--settings
# parsers, target description, project-file import resolver). Internal to the two tools, not a stage
# library, so it carries no marea:: alias.
file(GLOB_RECURSE MAREA_APPS_SHARED_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/internal/*.c")

add_library(marea-shared STATIC ${MAREA_APPS_SHARED_SOURCES})

target_include_directories(marea-shared PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/internal")

target_link_libraries(marea-shared PUBLIC marea::include marea::base marea::lex marea::parse marea::sema marea::ir marea::bytecode)
