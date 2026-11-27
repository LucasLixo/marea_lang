# Copyright (c) 2026 Paulo Lucas Alves Franca
# Licensed under the MIT License. See LICENSE.txt for full terms.

cmake_minimum_required(VERSION 3.20)

get_property(TEMP_GUARD_SET GLOBAL PROPERTY MareaRootGuard SET)
if(TEMP_GUARD_SET)
    message(FATAL_ERROR "The root CMakeLists.txt was included twice")
endif()
set_property(GLOBAL PROPERTY MareaRootGuard ON)
unset(TEMP_GUARD_SET)

# Policies before the project, so every later file sees them
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/settings/ConfigurePolicies.cmake" NO_POLICY_SCOPE)

# CMake built-in modules used by the root project
include(CMakeDependentOption)

# Bootstrap folders let the root load the functions and the folder settings
set(MAREA_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}" CACHE PATH "Path to the Marea current source directory")
set(MAREA_FOLDER_CMAKE "${MAREA_FOLDER}/cmake" CACHE PATH "Path to the custom CMake modules directory")
set(MAREA_FOLDER_CMAKE_FUNCTIONS "${MAREA_FOLDER_CMAKE}/functions" CACHE PATH "Path to the custom CMake functions")

# Functions and macros, Basic first: the others parse their arguments with it
include("${MAREA_FOLDER_CMAKE_FUNCTIONS}/Basic.cmake")
include("${MAREA_FOLDER_CMAKE_FUNCTIONS}/FindExecutable.cmake")
include("${MAREA_FOLDER_CMAKE_FUNCTIONS}/YamlParser.cmake")

include("${MAREA_FOLDER_CMAKE}/settings/ConfigureFolders.cmake")
configurefolders()

# Project metadata and version (major.minor.patch.build) come from metadata.yaml, not a hand-written header.
YamlParser(FILE "${MAREA_FOLDER}/metadata.yaml" PREFIX "MAREA_METADATA")

project(
    marea_lang
    VERSION
        "${MAREA_METADATA_version_major}.${MAREA_METADATA_version_minor}.${MAREA_METADATA_version_patch}.${MAREA_METADATA_version_build}"
    LANGUAGES C CXX
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# C++ is used only for the GoogleTest suite under tests/ (see cpp-testing) - never for production code.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include("${MAREA_FOLDER_CMAKE_SETTINGS}/ConfigureCompilerID.cmake")
configurecompilerid()

include("${MAREA_FOLDER_CMAKE_SETTINGS}/ConfigureBuildType.cmake")
configurebuildtype(CURRENT MAREA_MODE_BUILD_CURRENT AVAILABLE MAREA_MODE_BUILD_AVAILABLE)

include("${MAREA_FOLDER_CMAKE_SETTINGS}/ConfigureOptions.cmake")
configureoptions()

# After the options, whose values decide which tools are required
include("${MAREA_FOLDER_CMAKE_SETTINGS}/ConfigureMiseTools.cmake")
configuremisetools()

include("${MAREA_FOLDER_CMAKE_SETTINGS}/ConfigureThirdpartyProjects.cmake")
configurethirdpartyprojects()

add_subdirectory("${MAREA_FOLDER_LIB}" "${MAREA_FOLDER_CMAKE_BINARY}/Marea-Lib")

if(MAREA_OPTION_TARGET_CC OR MAREA_OPTION_TARGET_BC)
    add_subdirectory("${MAREA_FOLDER_APPS}" "${MAREA_FOLDER_CMAKE_BINARY}/Marea-Apps")
endif()

if(MAREA_OPTION_TARGET_TESTS)
    enable_testing()
    add_subdirectory("${MAREA_FOLDER_TESTS}" "${MAREA_FOLDER_CMAKE_BINARY}/Marea-Tests")
else()
    message(STATUS "Not add targets of: ${MAREA_FOLDER_TESTS}")
endif()
