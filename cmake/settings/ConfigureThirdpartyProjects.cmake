# Copyright (c) 2026 Paulo Lucas Alves Franca
# Licensed under the MIT License. See LICENSE.txt for full terms.

cmake_minimum_required(VERSION 3.20)

get_property(TEMP_GUARD_SET GLOBAL PROPERTY ConfigureThirdpartyProjectsGuard SET)
if(TEMP_GUARD_SET)
    message(FATAL_ERROR "ConfigureThirdpartyProjects.cmake was included twice")
endif()
set_property(GLOBAL PROPERTY ConfigureThirdpartyProjectsGuard ON)
unset(TEMP_GUARD_SET)

# Early check of the vendored projects the enabled options need
macro(ConfigureThirdpartyProjects)
    if(MAREA_OPTION_TARGET_TESTS AND NOT EXISTS "${MAREA_FOLDER_3RDPARTY_GOOGLE_TEST}/CMakeLists.txt")
        message(FATAL_ERROR "3rdparty/GoogleTest is missing - run: mise run setup:git-modules")
    endif()
endmacro()
