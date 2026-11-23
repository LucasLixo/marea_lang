# Copyright (c) 2026 Paulo Lucas Alves Franca
# Licensed under the MIT License. See LICENSE.txt for full terms.


cmake_minimum_required(VERSION 3.20)

get_property(TEMP_GUARD_SET GLOBAL PROPERTY MareaLibInternalGuard SET)
if(TEMP_GUARD_SET)
    message(FATAL_ERROR "lib/internal/CMakeLists.txt was included twice")
endif()
set_property(GLOBAL PROPERTY MareaLibInternalGuard ON)
unset(TEMP_GUARD_SET)

# marea_internal: INTERFACE target carrying the internal headers, linked PRIVATE by the stage libraries
file(GLOB_RECURSE MAREA_INTERNAL_HEADERS CONFIGURE_DEPENDS "*.h")

message(STATUS "Add target: marea_internal INTERFACE")
add_library(marea_internal INTERFACE EXCLUDE_FROM_ALL)
add_library(marea::internal ALIAS marea_internal)

# Link to the public headers interface (hierarchy)
target_link_libraries(marea_internal INTERFACE marea::include)

target_include_directories(marea_internal INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")

# Source group for IDE organization
source_group("internal" FILES ${MAREA_INTERNAL_HEADERS})
