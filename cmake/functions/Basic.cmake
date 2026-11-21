# Licensed under the MIT License. See LICENSE.txt for full terms.

cmake_minimum_required(VERSION 3.20)

get_property(TEMP_GUARD_SET GLOBAL PROPERTY BasicGuard SET)
if(TEMP_GUARD_SET)
    message(FATAL_ERROR "Basic.cmake was included twice")
endif()
set_property(GLOBAL PROPERTY BasicGuard ON)
unset(TEMP_GUARD_SET)

# Validate keyword arguments with cmake_parse_arguments and return them as <prefix>_<KEY>: BasicParseArguments(<owner> <prefix> "<flags>" "<one-value>" "<multi-value>" [REQUIRED <keys>] [TYPES <key>:<STRING|PATH|FILEPATH|BOOL|TARGET|LIST|NAME>[:EXISTS]] ARGS <args>)
function(
    BasicParseArguments
    TEMP_OWNER
    TEMP_PREFIX
    TEMP_FLAGS
    TEMP_ONE_VALUE
    TEMP_MULTI_VALUE
)
    # An empty value is kept (and rejected by its type) instead of silently dropped
    if(POLICY CMP0174)
        cmake_policy(SET CMP0174 NEW)
    endif()

    set(TEMP_SECTION "")
    set(TEMP_REQUIRED "")
    set(TEMP_TYPES "")
    set(TEMP_ARGS_START ${ARGC})
    if(ARGC GREATER 5)
        math(EXPR TEMP_LAST "${ARGC} - 1")
        foreach(TEMP_INDEX RANGE 5 ${TEMP_LAST})
            set(TEMP_TOKEN "${ARGV${TEMP_INDEX}}")
            if("${TEMP_TOKEN}" STREQUAL "ARGS")
                math(EXPR TEMP_ARGS_START "${TEMP_INDEX} + 1")
                break()
            elseif("${TEMP_TOKEN}" STREQUAL "REQUIRED" OR "${TEMP_TOKEN}" STREQUAL "TYPES")
                set(TEMP_SECTION "${TEMP_TOKEN}")
            elseif("${TEMP_SECTION}" STREQUAL "REQUIRED")
                list(APPEND TEMP_REQUIRED "${TEMP_TOKEN}")
            elseif("${TEMP_SECTION}" STREQUAL "TYPES")
                list(APPEND TEMP_TYPES "${TEMP_TOKEN}")
            endif()
        endforeach()
    endif()

    cmake_parse_arguments(
        PARSE_ARGV ${TEMP_ARGS_START}
        "${TEMP_PREFIX}"
        "${TEMP_FLAGS}"
        "${TEMP_ONE_VALUE}"
        "${TEMP_MULTI_VALUE}"
    )

    set(TEMP_MISSING_VALUES "${${TEMP_PREFIX}_KEYWORDS_MISSING_VALUES}")
    if(TEMP_MISSING_VALUES)
        list(GET TEMP_MISSING_VALUES 0 TEMP_KEY)
        message(FATAL_ERROR "${TEMP_OWNER}: keyword ${TEMP_KEY} requires a value")
    endif()

    set(TEMP_UNPARSED "${${TEMP_PREFIX}_UNPARSED_ARGUMENTS}")
    if(TEMP_UNPARSED)
        message(FATAL_ERROR "${TEMP_OWNER}: unexpected argument(s): ${TEMP_UNPARSED}")
    endif()

    foreach(TEMP_KEY IN LISTS TEMP_REQUIRED)
        if(NOT DEFINED ${TEMP_PREFIX}_${TEMP_KEY})
            message(FATAL_ERROR "${TEMP_OWNER}: missing required argument ${TEMP_KEY}")
        endif()
    endforeach()

    foreach(TEMP_SPEC IN LISTS TEMP_TYPES)
        string(REPLACE ":" ";" TEMP_PARTS "${TEMP_SPEC}")
        list(LENGTH TEMP_PARTS TEMP_PARTS_LENGTH)
        list(GET TEMP_PARTS 0 TEMP_KEY)
        list(GET TEMP_PARTS 1 TEMP_TYPE)
        set(TEMP_EXISTS FALSE)
        if(TEMP_PARTS_LENGTH GREATER 2)
            list(GET TEMP_PARTS 2 TEMP_MODIFIER)
            if(NOT "${TEMP_MODIFIER}" STREQUAL "EXISTS")
                message(FATAL_ERROR "${TEMP_OWNER}: unknown type modifier ${TEMP_MODIFIER} for ${TEMP_KEY}")
            endif()
            set(TEMP_EXISTS TRUE)
        endif()

        if(NOT DEFINED ${TEMP_PREFIX}_${TEMP_KEY})
            continue()
        endif()
        set(TEMP_VALUE "${${TEMP_PREFIX}_${TEMP_KEY}}")

        set(TEMP_EXPECTED "${TEMP_TYPE}")
        set(TEMP_VALID TRUE)
        if("${TEMP_TYPE}" STREQUAL "STRING" OR "${TEMP_TYPE}" STREQUAL "LIST")
            if("${TEMP_VALUE}" STREQUAL "")
                set(TEMP_VALID FALSE)
            endif()
        elseif("${TEMP_TYPE}" STREQUAL "PATH" OR "${TEMP_TYPE}" STREQUAL "FILEPATH")
            if("${TEMP_VALUE}" STREQUAL "")
                set(TEMP_VALID FALSE)
            elseif(TEMP_EXISTS AND NOT EXISTS "${TEMP_VALUE}")
                set(TEMP_VALID FALSE)
                set(TEMP_EXPECTED "existing ${TEMP_TYPE}")
            endif()
        elseif("${TEMP_TYPE}" STREQUAL "BOOL")
            string(TOUPPER "${TEMP_VALUE}" TEMP_UPPER)
            if(NOT "${TEMP_UPPER}" MATCHES "^(ON|OFF|TRUE|FALSE|YES|NO|1|0)$")
                set(TEMP_VALID FALSE)
            endif()
        elseif("${TEMP_TYPE}" STREQUAL "TARGET")
            if("${TEMP_VALUE}" STREQUAL "")
                set(TEMP_VALID FALSE)
            elseif(TEMP_EXISTS AND NOT TARGET "${TEMP_VALUE}")
                set(TEMP_VALID FALSE)
                set(TEMP_EXPECTED "existing ${TEMP_TYPE}")
            endif()
        elseif("${TEMP_TYPE}" STREQUAL "NAME")
            if(NOT "${TEMP_VALUE}" MATCHES "^[A-Za-z_][A-Za-z0-9_]*$")
                set(TEMP_VALID FALSE)
            endif()
        else()
            message(FATAL_ERROR "${TEMP_OWNER}: unknown type ${TEMP_TYPE} for ${TEMP_KEY}")
        endif()

        if(NOT TEMP_VALID)
            message(FATAL_ERROR "${TEMP_OWNER}: ${TEMP_KEY} expects ${TEMP_EXPECTED}, got '${TEMP_VALUE}'")
        endif()
    endforeach()

    foreach(TEMP_KEY IN LISTS TEMP_FLAGS TEMP_ONE_VALUE TEMP_MULTI_VALUE)
        if(DEFINED ${TEMP_PREFIX}_${TEMP_KEY})
            set(${TEMP_PREFIX}_${TEMP_KEY} "${${TEMP_PREFIX}_${TEMP_KEY}}" PARENT_SCOPE)
        else()
            unset(${TEMP_PREFIX}_${TEMP_KEY} PARENT_SCOPE)
        endif()
    endforeach()
endfunction()

# Set NAME to VALUE, also in the parent scope when given, and log it: BasicSet(NAME <var> VALUE <value> [PARENT_SCOPE])
macro(BasicSet)
    basicparsearguments(
        BasicSet
        PARAM
        "PARENT_SCOPE"
        "NAME;VALUE"
        ""
        REQUIRED
        NAME
        VALUE
        TYPES
        NAME:NAME
        VALUE:STRING
        ARGS
        ${ARGN}
    )

    set(${PARAM_NAME} ${PARAM_VALUE})
    if(PARAM_PARENT_SCOPE)
        set(${PARAM_NAME} ${PARAM_VALUE} PARENT_SCOPE)
    endif()

    message(STATUS "set(${PARAM_NAME} ${PARAM_VALUE})")

    # Unset
    unset(PARAM_NAME)
    unset(PARAM_VALUE)
    unset(PARAM_PARENT_SCOPE)
endmacro()

# Execute function
function(BasicExecuteCommand)
    basicparsearguments(
        BasicExecuteCommand
        PARAM
        ""
        "ERROR_MESSAGE"
        "COMMAND"
        REQUIRED
        COMMAND
        ERROR_MESSAGE
        TYPES
        COMMAND:LIST
        ERROR_MESSAGE:STRING
        ARGS
        ${ARGN}
    )

    execute_process(
        COMMAND ${PARAM_COMMAND}
        RESULT_VARIABLE TEMP_COMMAND_RESULT
        OUTPUT_VARIABLE TEMP_COMMAND_OUTPUT
        ERROR_VARIABLE TEMP_COMMAND_ERROR
    )

    if(NOT TEMP_COMMAND_RESULT EQUAL 0)
        message(FATAL_ERROR "${PARAM_ERROR_MESSAGE}: ${TEMP_COMMAND_ERROR}")
    endif()

    if(TEMP_COMMAND_OUTPUT)
        message(STATUS "${TEMP_COMMAND_OUTPUT}")
    endif()

    # Unset
    unset(TEMP_COMMAND_RESULT)
    unset(TEMP_COMMAND_OUTPUT)
    unset(TEMP_COMMAND_ERROR)
endfunction()

# Resolve a target's private Case-Case directory under the build root without changing its target name: BasicGetDirectoryTarget(OUTPUT <variable> TARGET <target>)
macro(BasicGetDirectoryTarget)
    basicparsearguments(
        BasicGetDirectoryTarget
        PARAM
        ""
        "OUTPUT;TARGET"
        ""
        REQUIRED
        OUTPUT
        TARGET
        TYPES
        OUTPUT:NAME
        TARGET:STRING
        ARGS
        ${ARGN}
    )

    string(REGEX REPLACE "([A-Z]+)([A-Z][a-z])" "\\1-\\2" TEMP_DIRECTORY_NAME "${PARAM_TARGET}")
    string(REGEX REPLACE "([a-z0-9])([A-Z])" "\\1-\\2" TEMP_DIRECTORY_NAME "${TEMP_DIRECTORY_NAME}")
    basicset(NAME ${PARAM_OUTPUT} VALUE "${MAREA_FOLDER_CMAKE_BINARY}/${TEMP_DIRECTORY_NAME}")

    # Unset
    unset(PARAM_OUTPUT)
    unset(PARAM_TARGET)
    unset(TEMP_DIRECTORY_NAME)
endmacro()

# Set every artifact output property to its target-private Case-Case directory: BasicSetTargetOutputDirectory(TARGET <target>)
macro(BasicSetTargetOutputDirectory)
    basicparsearguments(
        BasicSetTargetOutputDirectory
        PARAM
        ""
        "TARGET"
        ""
        REQUIRED
        TARGET
        TYPES
        TARGET:TARGET:EXISTS
        ARGS
        ${ARGN}
    )

    set(TEMP_OUTPUT_TARGET "${PARAM_TARGET}")
    basicgetdirectorytarget(OUTPUT TEMP_TARGET_OUTPUT_DIRECTORY TARGET "${PARAM_TARGET}")
    set_target_properties(
        ${TEMP_OUTPUT_TARGET}
        PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${TEMP_TARGET_OUTPUT_DIRECTORY}"
            ARCHIVE_OUTPUT_DIRECTORY "${TEMP_TARGET_OUTPUT_DIRECTORY}"
            LIBRARY_OUTPUT_DIRECTORY "${TEMP_TARGET_OUTPUT_DIRECTORY}"
    )

    # Unset
    unset(PARAM_TARGET)
    unset(TEMP_OUTPUT_TARGET)
    unset(TEMP_TARGET_OUTPUT_DIRECTORY)
endmacro()

# Create a target-private generated directory: BasicGetDirectoryGenerate(OUTPUT <variable> TARGET <target> [SUFFIX <path>])
macro(BasicGetDirectoryGenerate)
    basicparsearguments(
        BasicGetDirectoryGenerate
        PARAM
        ""
        "OUTPUT;TARGET;SUFFIX"
        ""
        REQUIRED
        OUTPUT
        TARGET
        TYPES
        OUTPUT:NAME
        TARGET:STRING
        SUFFIX:STRING
        ARGS
        ${ARGN}
    )

    set(TEMP_GENERATED_OUTPUT "${PARAM_OUTPUT}")
    set(TEMP_GENERATED_SUFFIX "${PARAM_SUFFIX}")
    basicgetdirectorytarget(OUTPUT TEMP_TARGET_DIRECTORY TARGET "${PARAM_TARGET}")
    set(${TEMP_GENERATED_OUTPUT} "${TEMP_TARGET_DIRECTORY}/Generated${TEMP_GENERATED_SUFFIX}")

    if(NOT EXISTS "${${TEMP_GENERATED_OUTPUT}}")
        file(MAKE_DIRECTORY "${${TEMP_GENERATED_OUTPUT}}")
    endif()

    # Unset
    unset(PARAM_OUTPUT)
    unset(PARAM_TARGET)
    unset(PARAM_SUFFIX)
    unset(TEMP_TARGET_DIRECTORY)
    unset(TEMP_GENERATED_OUTPUT)
    unset(TEMP_GENERATED_SUFFIX)
endmacro()

# Function to force include header for entire target
macro(BasicForceIncludeHeader)
    basicparsearguments(
        BasicForceIncludeHeader
        PARAM
        ""
        "TARGET;VISIBILITY;HEADER"
        ""
        REQUIRED
        TARGET
        VISIBILITY
        HEADER
        TYPES
        TARGET:TARGET:EXISTS
        VISIBILITY:STRING
        HEADER:PATH
        ARGS
        ${ARGN}
    )

    string(STRIP "${PARAM_VISIBILITY}" PARAM_VISIBILITY)

    # Validate visibility keyword
    if(PARAM_VISIBILITY STREQUAL "INTERFACE")
        set(TEMP_VISIBILITY INTERFACE)
    elseif(PARAM_VISIBILITY STREQUAL "PUBLIC")
        set(TEMP_VISIBILITY PUBLIC)
    elseif(PARAM_VISIBILITY STREQUAL "PRIVATE")
        set(TEMP_VISIBILITY PRIVATE)
    else()
        message(FATAL_ERROR "Invalid visibility: ${PARAM_VISIBILITY}. Must be INTERFACE, PUBLIC, or PRIVATE.")
    endif()

    # INTERFACE libraries only accept INTERFACE keyword - override if needed
    get_property(TEMP_TYPE TARGET "${PARAM_TARGET}" PROPERTY TYPE)
    if(TEMP_TYPE STREQUAL "INTERFACE_LIBRARY")
        set(TEMP_VISIBILITY INTERFACE)
    endif()

    # Apply force-include flag per compiler
    if(MAREA_COMPILER_IS_GCC_LIKE)
        target_compile_options("${PARAM_TARGET}" ${TEMP_VISIBILITY} "SHELL:-include \"${PARAM_HEADER}\"")
    elseif(MAREA_COMPILER_IS_MSVC_LIKE)
        target_compile_options("${PARAM_TARGET}" ${TEMP_VISIBILITY} "SHELL:/FI \"${PARAM_HEADER}\"")
    else()
        message(FATAL_ERROR "BasicForceIncludeHeader: the compiler is neither GCC-like nor MSVC-like")
    endif()

    # Cleanup local variables
    unset(TEMP_VISIBILITY)
    unset(TEMP_TYPE)
    unset(PARAM_TARGET)
    unset(PARAM_VISIBILITY)
    unset(PARAM_HEADER)
endmacro()
