# Copyright (c) 2026 Paulo Lucas Alves Franca
# Licensed under the MIT License. See LICENSE.txt for full terms.

cmake_minimum_required(VERSION 3.20)

get_property(MAREA_YAML_PARSER_GUARD_SET GLOBAL PROPERTY MareaYamlParserGuard SET)
if(MAREA_YAML_PARSER_GUARD_SET)
    message(FATAL_ERROR "YamlParser.cmake was included twice")
endif()
set_property(GLOBAL PROPERTY MareaYamlParserGuard ON)
unset(MAREA_YAML_PARSER_GUARD_SET)

# Parses a YAML file of scalar and nested-mapping keys into PARENT_SCOPE variables named
# <PREFIX>_<key> (a nested mapping prefixes every descendant key with its own name, so
# "version: {major: 0}" under prefix MAREA_METADATA becomes MAREA_METADATA_version_major).
function(YamlParser)
    cmake_parse_arguments(PARSE_ARGV 0 PARAM "" "FILE;PREFIX" "")

    if(NOT PARAM_FILE)
        message(FATAL_ERROR "YamlParser: missing required argument FILE")
    endif()
    if(NOT PARAM_PREFIX)
        message(FATAL_ERROR "YamlParser: missing required argument PREFIX")
    endif()
    if(NOT EXISTS "${PARAM_FILE}")
        message(FATAL_ERROR "YamlParser: file not found -> ${PARAM_FILE}")
    endif()

    file(STRINGS "${PARAM_FILE}" TEMP_YAML_LINES)

    # Indent stack: the indent length of each open parent node and the prefix it opened.
    set(TEMP_STACK_INDENTS "")
    set(TEMP_STACK_PREFIXES "")

    foreach(TEMP_LINE IN LISTS TEMP_YAML_LINES)
        if(TEMP_LINE MATCHES "^[ \t]*#" OR TEMP_LINE MATCHES "^[ \t]*$")
            continue()
        endif()

        if(TEMP_LINE MATCHES "^([ \t]*)([a-zA-Z0-9_\\.-]+):[ \t]+(.+)$")
            set(TEMP_INDENT "${CMAKE_MATCH_1}")
            set(TEMP_KEY "${CMAKE_MATCH_2}")
            set(TEMP_VALUE "${CMAKE_MATCH_3}")
            set(TEMP_IS_PARENT FALSE)
        elseif(TEMP_LINE MATCHES "^([ \t]*)([a-zA-Z0-9_\\.-]+):[ \t]*$")
            # A parent (nested-mapping) key, such as "version:", opens a deeper indent level.
            set(TEMP_INDENT "${CMAKE_MATCH_1}")
            set(TEMP_KEY "${CMAKE_MATCH_2}")
            set(TEMP_IS_PARENT TRUE)
        else()
            continue()
        endif()

        # Leave every open node that is not shallower than this line.
        string(LENGTH "${TEMP_INDENT}" TEMP_INDENT_LENGTH)
        list(LENGTH TEMP_STACK_INDENTS TEMP_STACK_SIZE)
        while(TEMP_STACK_SIZE GREATER 0)
            list(GET TEMP_STACK_INDENTS -1 TEMP_TOP_INDENT)
            if(TEMP_TOP_INDENT LESS ${TEMP_INDENT_LENGTH})
                break()
            endif()
            list(REMOVE_AT TEMP_STACK_INDENTS -1)
            list(REMOVE_AT TEMP_STACK_PREFIXES -1)
            list(LENGTH TEMP_STACK_INDENTS TEMP_STACK_SIZE)
        endwhile()
        set(TEMP_CURRENT_PREFIX "${PARAM_PREFIX}")
        if(TEMP_STACK_SIZE GREATER 0)
            list(GET TEMP_STACK_PREFIXES -1 TEMP_CURRENT_PREFIX)
        endif()

        if(TEMP_IS_PARENT)
            list(APPEND TEMP_STACK_INDENTS ${TEMP_INDENT_LENGTH})
            list(APPEND TEMP_STACK_PREFIXES "${TEMP_CURRENT_PREFIX}_${TEMP_KEY}")
            continue()
        endif()

        # A quoted value may carry a trailing comment after the closing quote; an unquoted
        # value has any trailing comment stripped outright.
        if(TEMP_VALUE MATCHES "^\"([^\"]*)\"[ \t]*(#.*)?$")
            set(TEMP_VALUE "${CMAKE_MATCH_1}")
        elseif(TEMP_VALUE MATCHES "^'([^']*)'[ \t]*(#.*)?$")
            set(TEMP_VALUE "${CMAKE_MATCH_1}")
        else()
            string(REGEX REPLACE "[ \t]*#.*$" "" TEMP_VALUE "${TEMP_VALUE}")
        endif()

        set("${TEMP_CURRENT_PREFIX}_${TEMP_KEY}" "${TEMP_VALUE}" PARENT_SCOPE)
    endforeach()
endfunction()
