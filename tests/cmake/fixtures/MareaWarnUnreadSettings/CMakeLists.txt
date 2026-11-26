# Copyright (c) 2026 Paulo Lucas Alves Franca
# Licensed under the MIT License. See LICENSE.txt for full terms.

# Tiny fixture project: calls MareaWarnUnreadSettings directly with an explicit KNOWN list, to prove the typo-guard
# comparison itself works, even though no real tool feeds it that list end to end yet (marea-cc-and-bc-tools)
cmake_minimum_required(VERSION 3.20)
project(MareaWarnUnreadSettingsFixture LANGUAGES C)

if(NOT DEFINED CHECK_MAREA_ROOT)
    message(FATAL_ERROR "MareaWarnUnreadSettingsFixture: missing required -DCHECK_MAREA_ROOT")
endif()

include("${CHECK_MAREA_ROOT}/cmake/functions/Basic.cmake")
include("${CHECK_MAREA_ROOT}/cmake/settings/ConfigureMareaFunctions.cmake")
configuremareafunctions()

string(REPLACE "|" ";" TEMP_SETTINGS_LIST "${CHECK_SETTINGS}")
string(REPLACE "|" ";" TEMP_KNOWN_LIST "${CHECK_KNOWN}")
mareawarnunreadsettings("demo" "${TEMP_SETTINGS_LIST}" "${TEMP_KNOWN_LIST}")
