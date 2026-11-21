# Copyright (c) 2026 Paulo Lucas Alves Franca
# Licensed under the MIT License. See LICENSE.txt for full terms.

# Check up front that every tool pinned in mise.toml provides the executables the build needs and report all missing ones at once; call after the options and before any add_subdirectory(); it only checks, FindExecutable still caches the real path of an optional tool.

cmake_minimum_required(VERSION 3.20)

get_property(TEMP_GUARD_SET GLOBAL PROPERTY ConfigureMiseToolsGuard SET)
if(TEMP_GUARD_SET)
    message(FATAL_ERROR "ConfigureMiseTools.cmake was included twice")
endif()
set_property(GLOBAL PROPERTY ConfigureMiseToolsGuard ON)
unset(TEMP_GUARD_SET)

# Macro
macro(ConfigureMiseTools)
    set(TEMP_MISE_TOML "${MAREA_FOLDER}/mise.toml")

    if(NOT EXISTS "${TEMP_MISE_TOML}")
        message(STATUS "ConfigureMiseTools: no mise.toml at '${TEMP_MISE_TOML}', tool check skipped")
    else()
        # Reconfigure when the pinned tools change
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${TEMP_MISE_TOML}")

        # Read the file as a list of lines, with the characters that would split a CMake list replaced by markers
        file(READ "${TEMP_MISE_TOML}" TEMP_MISE_CONTENT)
        string(REPLACE "\r" "" TEMP_MISE_CONTENT "${TEMP_MISE_CONTENT}")
        string(REPLACE ";" "<SC>" TEMP_MISE_CONTENT "${TEMP_MISE_CONTENT}")
        string(REPLACE "[" "<LB>" TEMP_MISE_CONTENT "${TEMP_MISE_CONTENT}")
        string(REPLACE "]" "<RB>" TEMP_MISE_CONTENT "${TEMP_MISE_CONTENT}")
        string(REPLACE "\n" ";" TEMP_MISE_LINES "${TEMP_MISE_CONTENT}")

        # Keys of the [tools] table: bare or quoted name at depth 0, until the next table header
        set(TEMP_MISE_KEYS "")
        set(TEMP_MISE_IN_TOOLS OFF)
        set(TEMP_MISE_DEPTH 0)
        foreach(TEMP_LINE IN LISTS TEMP_MISE_LINES)
            string(STRIP "${TEMP_LINE}" TEMP_LINE)

            if(TEMP_MISE_DEPTH EQUAL 0 AND TEMP_LINE MATCHES "^<LB>")
                if(TEMP_LINE MATCHES "^<LB>tools<RB>")
                    set(TEMP_MISE_IN_TOOLS ON)
                else()
                    set(TEMP_MISE_IN_TOOLS OFF)
                endif()
                continue()
            endif()

            if(NOT TEMP_MISE_IN_TOOLS OR TEMP_LINE MATCHES "^#")
                continue()
            endif()

            if(TEMP_MISE_DEPTH EQUAL 0)
                if(TEMP_LINE MATCHES "^\"([^\"]+)\"[ \t]*=")
                    list(APPEND TEMP_MISE_KEYS "${CMAKE_MATCH_1}")
                elseif(TEMP_LINE MATCHES "^([A-Za-z0-9_.-]+)[ \t]*=")
                    list(APPEND TEMP_MISE_KEYS "${CMAKE_MATCH_1}")
                endif()
            endif()

            # Track inline table and array nesting so continuation lines are never read as keys
            string(REGEX REPLACE "\"[^\"]*\"" "" TEMP_LINE "${TEMP_LINE}")
            string(REGEX REPLACE "#.*$" "" TEMP_LINE "${TEMP_LINE}")
            string(REGEX MATCHALL "{|<LB>" TEMP_OPENS "${TEMP_LINE}")
            string(REGEX MATCHALL "}|<RB>" TEMP_CLOSES "${TEMP_LINE}")
            list(LENGTH TEMP_OPENS TEMP_OPEN_COUNT)
            list(LENGTH TEMP_CLOSES TEMP_CLOSE_COUNT)
            math(EXPR TEMP_MISE_DEPTH "${TEMP_MISE_DEPTH} + ${TEMP_OPEN_COUNT} - ${TEMP_CLOSE_COUNT}")
        endforeach()

        if(NOT TEMP_MISE_KEYS)
            message(FATAL_ERROR "ConfigureMiseTools: no keys found in the [tools] table of '${TEMP_MISE_TOML}'")
        endif()

        # Executables each tool must provide under the current options and platform; empty means nothing to verify here
        set(TEMP_MISE_VERIFIED "")
        set(TEMP_MISE_SKIPPED "")
        set(TEMP_MISE_MISSING "")
        set(TEMP_MISE_UNKNOWN "")
        foreach(TEMP_KEY IN LISTS TEMP_MISE_KEYS)
            set(TEMP_EXECUTABLES "")

            if(TEMP_KEY STREQUAL "conda:llvm")
                # A metapackage that installs no executable; its archiver comes from conda:llvm-tools
            elseif(TEMP_KEY STREQUAL "conda:llvm-tools")
                set(TEMP_EXECUTABLES llvm-ar llvm-ranlib)
            elseif(TEMP_KEY STREQUAL "conda:clang")
                set(TEMP_EXECUTABLES clang)
            elseif(TEMP_KEY STREQUAL "conda:clangxx")
                set(TEMP_EXECUTABLES clang++)
            elseif(TEMP_KEY STREQUAL "conda:lld")
                if(CMAKE_HOST_WIN32)
                    set(TEMP_EXECUTABLES lld-link)
                elseif(CMAKE_HOST_APPLE)
                    set(TEMP_EXECUTABLES ld64.lld)
                else()
                    set(TEMP_EXECUTABLES ld.lld)
                endif()
            elseif(TEMP_KEY STREQUAL "ninja")
                if(CMAKE_GENERATOR MATCHES "Ninja")
                    set(TEMP_EXECUTABLES ninja)
                endif()
            elseif(
                TEMP_KEY STREQUAL "cmake"
                OR TEMP_KEY STREQUAL "conda:libclang"
                OR TEMP_KEY STREQUAL "ccache"
                OR TEMP_KEY STREQUAL "stylua"
                OR TEMP_KEY STREQUAL "uv"
                OR TEMP_KEY STREQUAL "pipx:gersemi"
                OR TEMP_KEY STREQUAL "rust"
                OR TEMP_KEY STREQUAL "lua"
            )
                # cmake is running, ccache is optional, and libclang, stylua, uv, gersemi, rust and lua are only used by mise tasks
            else()
                list(APPEND TEMP_MISE_UNKNOWN "  unknown tool '${TEMP_KEY}' in mise.toml: add it to ConfigureMiseTools")
                continue()
            endif()

            if(NOT TEMP_EXECUTABLES)
                list(APPEND TEMP_MISE_SKIPPED "${TEMP_KEY}")
                continue()
            endif()

            # HINTS falls back to mise's shim directory, like FindExecutable, for callers that never sourced the shell profile
            set(TEMP_KEY_MISSING OFF)
            foreach(TEMP_EXECUTABLE IN LISTS TEMP_EXECUTABLES)
                unset(TEMP_MISE_FOUND)
                unset(TEMP_MISE_FOUND CACHE)
                find_program(
                    TEMP_MISE_FOUND
                    NAMES "${TEMP_EXECUTABLE}"
                    HINTS "$ENV{HOME}/.local/share/mise/shims"
                    NO_CMAKE_FIND_ROOT_PATH
                )
                if(NOT TEMP_MISE_FOUND)
                    set(TEMP_KEY_MISSING ON)
                    list(APPEND TEMP_MISE_MISSING "  missing '${TEMP_KEY}': executable '${TEMP_EXECUTABLE}' not found")
                endif()
                unset(TEMP_MISE_FOUND CACHE)
            endforeach()

            if(NOT TEMP_KEY_MISSING)
                list(APPEND TEMP_MISE_VERIFIED "${TEMP_KEY}")
            endif()
        endforeach()

        # One error for everything found; the install hint only when a tool is missing
        if(TEMP_MISE_MISSING OR TEMP_MISE_UNKNOWN)
            set(TEMP_MISE_REPORT ${TEMP_MISE_UNKNOWN} ${TEMP_MISE_MISSING})
            string(REPLACE ";" "\n" TEMP_MISE_REPORT "${TEMP_MISE_REPORT}")
            if(TEMP_MISE_MISSING)
                set(TEMP_MISE_REPORT "${TEMP_MISE_REPORT}\nrun: mise install")
            endif()
            message(FATAL_ERROR "ConfigureMiseTools: the mise tools are not ready:\n${TEMP_MISE_REPORT}")
        endif()

        list(LENGTH TEMP_MISE_VERIFIED TEMP_MISE_VERIFIED_COUNT)
        list(LENGTH TEMP_MISE_KEYS TEMP_MISE_KEYS_COUNT)
        string(REPLACE ";" ", " TEMP_MISE_VERIFIED_TEXT "${TEMP_MISE_VERIFIED}")
        string(REPLACE ";" ", " TEMP_MISE_SKIPPED_TEXT "${TEMP_MISE_SKIPPED}")
        message(
            STATUS
            "ConfigureMiseTools: verified ${TEMP_MISE_VERIFIED_COUNT} of ${TEMP_MISE_KEYS_COUNT} mise tools (${TEMP_MISE_VERIFIED_TEXT}); not required here: ${TEMP_MISE_SKIPPED_TEXT}"
        )
    endif()

    # Unset
    unset(TEMP_MISE_TOML)
    unset(TEMP_MISE_CONTENT)
    unset(TEMP_MISE_LINES)
    unset(TEMP_MISE_KEYS)
    unset(TEMP_MISE_IN_TOOLS)
    unset(TEMP_MISE_DEPTH)
    unset(TEMP_LINE)
    unset(TEMP_OPENS)
    unset(TEMP_CLOSES)
    unset(TEMP_OPEN_COUNT)
    unset(TEMP_CLOSE_COUNT)
    unset(TEMP_MISE_VERIFIED)
    unset(TEMP_MISE_SKIPPED)
    unset(TEMP_MISE_MISSING)
    unset(TEMP_MISE_UNKNOWN)
    unset(TEMP_MISE_REPORT)
    unset(TEMP_KEY)
    unset(TEMP_EXECUTABLES)
    unset(TEMP_EXECUTABLE)
    unset(TEMP_KEY_MISSING)
    unset(TEMP_MISE_FOUND)
    unset(TEMP_MISE_VERIFIED_COUNT)
    unset(TEMP_MISE_KEYS_COUNT)
    unset(TEMP_MISE_VERIFIED_TEXT)
    unset(TEMP_MISE_SKIPPED_TEXT)
endmacro()
