# Copyright (c) 2026 Paulo Lucas Alves Franca
# Licensed under the MIT License. See LICENSE.txt for full terms.

unset(OFF)
unset(ON)
if(POLICY CMP0054)
    cmake_policy(SET CMP0054 NEW)
endif()

# Error guard; no cmake_minimum_required here, it would reset the policies set in the including scope
get_property(TEMP_GUARD_SET GLOBAL PROPERTY ConfigurePoliciesGuard SET)
if(TEMP_GUARD_SET)
    message(FATAL_ERROR "ConfigurePolicies.cmake was included twice")
endif()
set_property(GLOBAL PROPERTY ConfigurePoliciesGuard ON)
unset(TEMP_GUARD_SET)

# CMP0077: option() honors normal variables
if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()

# CMP0118: GENERATED source-file property is visible across directory scopes
set(CMAKE_POLICY_DEFAULT_CMP0118 NEW)
if(POLICY CMP0118)
    cmake_policy(SET CMP0118 NEW)
endif()
