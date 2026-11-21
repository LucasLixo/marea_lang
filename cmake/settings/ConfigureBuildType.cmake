# Copyright (c) 2026 Paulo Lucas Alves Franca
# Licensed under the MIT License. See LICENSE.txt for full terms.

cmake_minimum_required(VERSION 3.20)

get_property(TEMP_GUARD_SET GLOBAL PROPERTY ConfigureBuildTypeGuard SET)
if(TEMP_GUARD_SET)
    message(FATAL_ERROR "ConfigureBuildType.cmake was included twice")
endif()
set_property(GLOBAL PROPERTY ConfigureBuildTypeGuard ON)
unset(TEMP_GUARD_SET)

# Macro
macro(ConfigureBuildType)
    BasicParseArguments(
        ConfigureBuildType
        PARAM
        ""
        "CURRENT;AVAILABLE"
        ""
        REQUIRED
        CURRENT
        AVAILABLE
        TYPES
        CURRENT:NAME
        AVAILABLE:NAME
        ARGS
        ${ARGN}
    )

    # Check if the current generator supports multiple configurations
    get_property(TEMP_IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

    if(TEMP_IS_MULTI_CONFIG)
        # Multi-config generators (Visual Studio, Xcode, Ninja Multi-Config): the types come from CMAKE_CONFIGURATION_TYPES.
        set(${PARAM_AVAILABLE} "${CMAKE_CONFIGURATION_TYPES}")

        # The current type is unknown until build time, so return the generator expression that resolves to it.
        set(${PARAM_CURRENT} "$<CONFIG>")
    else()
        # Single-config generators (Makefiles, Ninja): no list is defined, so provide the standard CMake defaults.
        set(${PARAM_AVAILABLE} "Debug;Release;RelWithDebInfo;MinSizeRel")

        # The current build type is defined during the 'mount' (configuration) step
        if(CMAKE_BUILD_TYPE)
            set(${PARAM_CURRENT} "${CMAKE_BUILD_TYPE}")
        else()
            # Default to Debug if the user didn't specify one
            set(${PARAM_CURRENT} "Debug")
        endif()
    endif()

    # Clean up internal variable
    unset(TEMP_IS_MULTI_CONFIG)

    # Summary
    message(STATUS "Mode build current config: ${${PARAM_CURRENT}}")
    message(STATUS "Mode build available configs: ${${PARAM_AVAILABLE}}")

    unset(PARAM_CURRENT)
    unset(PARAM_AVAILABLE)
endmacro()
