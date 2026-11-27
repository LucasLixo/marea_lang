# Copyright (c) 2026 Paulo Lucas Alves Franca
# Licensed under the MIT License. See LICENSE.txt for full terms.

cmake_minimum_required(VERSION 3.20)

get_property(TEMP_GUARD_SET GLOBAL PROPERTY ConfigureMareaFunctionsGuard SET)
if(TEMP_GUARD_SET)
    message(FATAL_ERROR "ConfigureMareaFunctions.cmake was included twice")
endif()
set_property(GLOBAL PROPERTY ConfigureMareaFunctionsGuard ON)
unset(TEMP_GUARD_SET)

# One name per MAREA_LibraryFlag bit of MAREA_TypeRegistry.h, the set marea_add_library/marea_add_executable LIBRARIES may pick from
set(MAREA_KNOWN_LIBRARIES
    "marea.array"
    "marea.vector"
    "marea.tuple"
    "c.memory"
    "c.threads"
    "marea.memory"
)

# The shipped-macro to library dependency table (DECISIONS row 53): today only include_bytes! needs marea.array
set(MAREA_MACRO_LIBRARY_DEPENDENCIES "include_bytes!=marea.array")

# Normalize one CMake boolean spelling (ON, OFF, TRUE, FALSE, 1, 0) to the true/false text build_value! reads; any other text is kept as given
function(MareaNormalizeSettingValue TEMP_VALUE TEMP_OUTPUT)
    string(TOUPPER "${TEMP_VALUE}" TEMP_UPPER)
    if(TEMP_UPPER MATCHES "^(ON|TRUE|1)$")
        set(${TEMP_OUTPUT} "true" PARENT_SCOPE)
    elseif(TEMP_UPPER MATCHES "^(OFF|FALSE|0)$")
        set(${TEMP_OUTPUT} "false" PARENT_SCOPE)
    else()
        set(${TEMP_OUTPUT} "${TEMP_VALUE}" PARENT_SCOPE)
    endif()
endfunction()

# Insert or replace NAME=value in a NAMES/VALUES pair of lists, the later call always winning: MareaUpsertSetting(NAMES <var> VALUES <var> NAME <name> VALUE <value>)
function(MareaUpsertSetting)
    BasicParseArguments(
        MareaUpsertSetting
        PARAM
        ""
        "NAMES;VALUES;NAME;VALUE"
        ""
        REQUIRED
        NAMES
        VALUES
        NAME
        VALUE
        TYPES
        NAMES:NAME
        VALUES:NAME
        NAME:STRING
        VALUE:STRING
        ARGS
        ${ARGN}
    )

    set(TEMP_NAMES "${${PARAM_NAMES}}")
    set(TEMP_VALUES "${${PARAM_VALUES}}")
    list(FIND TEMP_NAMES "${PARAM_NAME}" TEMP_INDEX)
    if(TEMP_INDEX EQUAL -1)
        list(APPEND TEMP_NAMES "${PARAM_NAME}")
        list(APPEND TEMP_VALUES "${PARAM_VALUE}")
    else()
        list(REMOVE_AT TEMP_VALUES ${TEMP_INDEX})
        list(INSERT TEMP_VALUES ${TEMP_INDEX} "${PARAM_VALUE}")
    endif()
    set(${PARAM_NAMES} "${TEMP_NAMES}" PARENT_SCOPE)
    set(${PARAM_VALUES} "${TEMP_VALUES}" PARENT_SCOPE)
endfunction()

# Build the NAME=value settings pairs of one target: the implicit defaults (MAREA_DEFAULT_INLINE, the build mode), then every MAREA_SETTING_<NAME> cache variable, then the per-call SETTINGS - each later source wins over the earlier one for the same name
function(MareaCollectSettings TEMP_TARGET_NAME TEMP_CALL_SETTINGS TEMP_OUTPUT)
    set(TEMP_NAMES "")
    set(TEMP_VALUES "")

    # Implicit defaults: MAREA_DEFAULT_INLINE (cmake-style option naming is MAREA_OPTION_DEFAULT_INLINE; the setting pair
    # keeps the design doc's own name, MAREA_DEFAULT_INLINE, so a build_value! call matches it unchanged) and the current
    # build mode, under the name ConfigureBuildType already gives it (MAREA_MODE_BUILD_CURRENT)
    if(DEFINED MAREA_OPTION_DEFAULT_INLINE)
        mareanormalizesettingvalue("${MAREA_OPTION_DEFAULT_INLINE}" TEMP_INLINE_VALUE)
        mareaupsertsetting(NAMES TEMP_NAMES VALUES TEMP_VALUES NAME "MAREA_DEFAULT_INLINE" VALUE "${TEMP_INLINE_VALUE}")
    endif()
    if(DEFINED MAREA_MODE_BUILD_CURRENT)
        mareaupsertsetting(
            NAMES
            TEMP_NAMES
            VALUES
            TEMP_VALUES
            NAME
            "MAREA_MODE_BUILD_CURRENT"
            VALUE
            "${MAREA_MODE_BUILD_CURRENT}"
        )
    endif()

    # Global form: every cache variable named MAREA_SETTING_<NAME>, mirroring how MAREA_OPTION_* is declared
    get_cmake_property(TEMP_CACHE_VARIABLES CACHE_VARIABLES)
    foreach(TEMP_CACHE_VAR IN LISTS TEMP_CACHE_VARIABLES)
        if(TEMP_CACHE_VAR MATCHES "^MAREA_SETTING_(.+)$")
            mareanormalizesettingvalue("${${TEMP_CACHE_VAR}}" TEMP_GLOBAL_VALUE)
            mareaupsertsetting(
                NAMES
                TEMP_NAMES
                VALUES
                TEMP_VALUES
                NAME
                "${CMAKE_MATCH_1}"
                VALUE
                "${TEMP_GLOBAL_VALUE}"
            )
        endif()
    endforeach()

    # Per-call SETTINGS NAME=value pairs win over the global and the implicit ones
    foreach(TEMP_PAIR IN LISTS TEMP_CALL_SETTINGS)
        if(NOT TEMP_PAIR MATCHES "^([^=]+)=(.*)$")
            message(FATAL_ERROR "${TEMP_TARGET_NAME}: SETTINGS entry '${TEMP_PAIR}' is not of the form NAME=value")
        endif()
        mareanormalizesettingvalue("${CMAKE_MATCH_2}" TEMP_CALL_VALUE)
        mareaupsertsetting(
            NAMES
            TEMP_NAMES
            VALUES
            TEMP_VALUES
            NAME
            "${CMAKE_MATCH_1}"
            VALUE
            "${TEMP_CALL_VALUE}"
        )
    endforeach()

    # Rebuild the NAME=value text pairs in their final, already-overridden order
    set(TEMP_RESULT "")
    list(LENGTH TEMP_NAMES TEMP_COUNT)
    if(TEMP_COUNT GREATER 0)
        math(EXPR TEMP_LAST "${TEMP_COUNT} - 1")
        foreach(TEMP_INDEX RANGE 0 ${TEMP_LAST})
            list(GET TEMP_NAMES ${TEMP_INDEX} TEMP_NAME)
            list(GET TEMP_VALUES ${TEMP_INDEX} TEMP_VALUE)
            list(APPEND TEMP_RESULT "${TEMP_NAME}=${TEMP_VALUE}")
        endforeach()
    endif()
    set(${TEMP_OUTPUT} "${TEMP_RESULT}" PARENT_SCOPE)
endfunction()

# Fail configure when a macro this target lists in MACROS needs a library that LIBRARIES does not enable, naming both (mirrors MAREA_TYPE_REGISTRY_EXCEPT_ERROR_MISSING_DEPENDENCY)
function(MareaCheckMacroLibraryDependencies TEMP_TARGET_NAME TEMP_MACROS TEMP_ENABLED_LIBRARIES)
    foreach(TEMP_MACRO IN LISTS TEMP_MACROS)
        foreach(TEMP_ENTRY IN LISTS MAREA_MACRO_LIBRARY_DEPENDENCIES)
            if(TEMP_ENTRY MATCHES "^([^=]+)=(.+)$")
                set(TEMP_ENTRY_MACRO "${CMAKE_MATCH_1}")
                set(TEMP_ENTRY_LIBRARY "${CMAKE_MATCH_2}")
                if(TEMP_ENTRY_MACRO STREQUAL TEMP_MACRO AND NOT TEMP_ENTRY_LIBRARY IN_LIST TEMP_ENABLED_LIBRARIES)
                    message(
                        FATAL_ERROR
                        "${TEMP_TARGET_NAME}: macro ${TEMP_MACRO} needs library ${TEMP_ENTRY_LIBRARY}, which is not in LIBRARIES (${TEMP_ENABLED_LIBRARIES})"
                    )
                endif()
            endif()
        endforeach()
    endforeach()
endfunction()

# Warn about a SETTINGS/MAREA_SETTING_ pair no build_value! call reads. TEMP_KNOWN_NAMES is the name list a "list
# settings" tool mode would give; that mode does not exist before marea-cc-and-bc-tools, so every caller today passes
# an empty list and this never warns - see temp/_LOG_APPLY_ERROR.md for the open end-to-end gap
function(MareaWarnUnreadSettings TEMP_TARGET_NAME TEMP_SETTINGS TEMP_KNOWN_NAMES)
    if(NOT TEMP_KNOWN_NAMES)
        return()
    endif()
    foreach(TEMP_PAIR IN LISTS TEMP_SETTINGS)
        if(TEMP_PAIR MATCHES "^([^=]+)=")
            set(TEMP_NAME "${CMAKE_MATCH_1}")
            if(NOT TEMP_NAME IN_LIST TEMP_KNOWN_NAMES)
                message(
                    WARNING
                    "${TEMP_TARGET_NAME}: SETTINGS/MAREA_SETTING_ pair '${TEMP_NAME}' is not read by any build_value! call"
                )
            endif()
        endif()
    endforeach()
endfunction()

# Locate marea-cc/marea-bc, build the settings pipeline and the compile plan shared by marea_add_library and marea_add_executable
function(MareaBuildMareaSources TEMP_TARGET_NAME)
    BasicParseArguments(
        MareaBuildMareaSources
        PARAM
        ""
        ""
        "SOURCES;SETTINGS;LIBRARIES;MACROS"
        REQUIRED
        SOURCES
        TYPES
        SOURCES:LIST
        SETTINGS:LIST
        LIBRARIES:LIST
        MACROS:LIST
        ARGS
        ${ARGN}
    )

    FindExecutable(
        NAME
        "marea-cc"
        OUTPUT
        TEMP_MAREA_CC
        REQUIRED
    )
    FindExecutable(
        NAME
        "marea-bc"
        OUTPUT
        TEMP_MAREA_BC
        REQUIRED
    )

    # The enabled library set: every known library by default, or the caller's own subset
    if(PARAM_LIBRARIES)
        foreach(TEMP_LIBRARY IN LISTS PARAM_LIBRARIES)
            if(NOT TEMP_LIBRARY IN_LIST MAREA_KNOWN_LIBRARIES)
                message(
                    FATAL_ERROR
                    "${TEMP_TARGET_NAME}: LIBRARIES names an unknown library '${TEMP_LIBRARY}' (known: ${MAREA_KNOWN_LIBRARIES})"
                )
            endif()
        endforeach()
        set(TEMP_ENABLED_LIBRARIES "${PARAM_LIBRARIES}")
    else()
        set(TEMP_ENABLED_LIBRARIES "${MAREA_KNOWN_LIBRARIES}")
    endif()

    mareacheckmacrolibrarydependencies("${TEMP_TARGET_NAME}" "${PARAM_MACROS}" "${TEMP_ENABLED_LIBRARIES}")

    mareacollectsettings("${TEMP_TARGET_NAME}" "${PARAM_SETTINGS}" TEMP_ALL_SETTINGS)
    mareawarnunreadsettings("${TEMP_TARGET_NAME}" "${TEMP_ALL_SETTINGS}" "")

    # A placeholder target: no real marea-cc/marea-bc invocation exists yet (marea-cc-and-bc-tools), so this carries
    # only the C11-side compile definitions until a real compiled target replaces it
    if(NOT TARGET ${TEMP_TARGET_NAME})
        add_library(${TEMP_TARGET_NAME} INTERFACE)
    endif()
    if(TEMP_ALL_SETTINGS)
        target_compile_definitions(${TEMP_TARGET_NAME} INTERFACE ${TEMP_ALL_SETTINGS})
    endif()

    message(
        STATUS
        "marea target ${TEMP_TARGET_NAME}: ${TEMP_MAREA_CC} ${TEMP_MAREA_BC} SOURCES ${PARAM_SOURCES} SETTINGS ${TEMP_ALL_SETTINGS} LIBRARIES ${TEMP_ENABLED_LIBRARIES}"
    )

    unset(TEMP_MAREA_CC)
    unset(TEMP_MAREA_BC)
    unset(TEMP_ENABLED_LIBRARIES)
    unset(TEMP_ALL_SETTINGS)
endfunction()

# Build a Marea library target from its sources: marea_add_library(<name> SOURCES <source>... [SETTINGS <NAME=value>...] [LIBRARIES <name>...] [MACROS <name>...])
function(marea_add_library TEMP_TARGET_NAME)
    mareabuildmareasources("${TEMP_TARGET_NAME}" ${ARGN})
endfunction()

# Build a Marea executable target from its sources: marea_add_executable(<name> SOURCES <source>... [SETTINGS <NAME=value>...] [LIBRARIES <name>...] [MACROS <name>...])
function(marea_add_executable TEMP_TARGET_NAME)
    mareabuildmareasources("${TEMP_TARGET_NAME}" ${ARGN})
endfunction()

# marea_add_library and marea_add_executable are declared above and ready to use once this macro runs
macro(ConfigureMareaFunctions)
endmacro()
