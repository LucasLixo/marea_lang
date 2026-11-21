# Copyright (c) 2026 Paulo Lucas Alves Franca
# Licensed under the MIT License. See LICENSE.txt for full terms.


cmake_minimum_required(VERSION 3.20)

get_property(TEMP_GUARD_SET GLOBAL PROPERTY MareaLibIncludeGuard SET)
if(TEMP_GUARD_SET)
    message(FATAL_ERROR "lib/include/CMakeLists.txt was included twice")
endif()
set_property(GLOBAL PROPERTY MareaLibIncludeGuard ON)
unset(TEMP_GUARD_SET)

# marea_include: INTERFACE target carrying the public headers
file(GLOB_RECURSE MAREA_PUBLIC_HEADERS CONFIGURE_DEPENDS "*.h")

message(STATUS "Add target: marea_include INTERFACE")
add_library(marea_include INTERFACE EXCLUDE_FROM_ALL)
add_library(marea::include ALIAS marea_include)

target_include_directories(marea_include INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")

# Source group for IDE organization
source_group("include" FILES ${MAREA_PUBLIC_HEADERS})

# Generate the settings header from its template and force-include it into every consumer
include("${CMAKE_CURRENT_SOURCE_DIR}/marea/base/MAREASettingsHeader.cmake")
