# Copyright (c) 2026 Paulo Lucas Alves Franca
# Licensed under the MIT License. See LICENSE.txt for full terms.

cmake_minimum_required(VERSION 3.20)

get_property(TEMP_GUARD_SET GLOBAL PROPERTY ConfigureFoldersGuard SET)
if(TEMP_GUARD_SET)
    message(FATAL_ERROR "ConfigureFolders.cmake was included twice")
endif()
set_property(GLOBAL PROPERTY ConfigureFoldersGuard ON)
unset(TEMP_GUARD_SET)

# Every folder of the repository is defined here once; the other files read these variables
macro(ConfigureFolders)
    # Source folders
    set(MAREA_FOLDER_PARENTAL "${CMAKE_SOURCE_DIR}" CACHE PATH "Path to the root CMake source directory")
    set(MAREA_FOLDER_CMAKE_BINARY "${CMAKE_BINARY_DIR}" CACHE PATH "Path to the root CMake build directory")
    set(MAREA_FOLDER_CMAKE_SETTINGS "${MAREA_FOLDER_CMAKE}/settings" CACHE PATH "Path to the CMake settings modules")
    set(MAREA_FOLDER_3RDPARTY "${MAREA_FOLDER}/3rdparty" CACHE PATH "Source directory for 3rdparty dependencies")
    set(MAREA_FOLDER_DOCS "${MAREA_FOLDER}/docs" CACHE PATH "Source directory for the documentation")
    set(MAREA_FOLDER_EXAMPLES "${MAREA_FOLDER}/examples" CACHE PATH "Source directory for the example programs")
    set(MAREA_FOLDER_LIB "${MAREA_FOLDER}/lib" CACHE PATH "Source directory for the compiler libraries")
    set(MAREA_FOLDER_APPS
        "${MAREA_FOLDER}/apps"
        CACHE PATH
        "Source directory for the marea-cc/marea-bc command-line tools"
    )
    set(MAREA_FOLDER_TESTS "${MAREA_FOLDER}/tests" CACHE PATH "Source directory for the test suites")
    set(MAREA_FOLDER_TOOLS "${MAREA_FOLDER}/tools" CACHE PATH "Source directory for the build-time tools")

    # Derived from MAREA_FOLDER and not cached, so a build directory never keeps a stale path
    BasicSet(NAME MAREA_FOLDER_LIB_INCLUDE VALUE "${MAREA_FOLDER_LIB}/include")
    BasicSet(NAME MAREA_FOLDER_LIB_INTERNAL VALUE "${MAREA_FOLDER_LIB}/internal")
    BasicSet(NAME MAREA_FOLDER_LIB_SOURCE VALUE "${MAREA_FOLDER_LIB}/source")
    BasicSet(
        NAME
        MAREA_FOLDER_CMAKE_BINARY_LIB_GENERATED
        VALUE
        "${MAREA_FOLDER_CMAKE_BINARY}/Marea-Lib/Generated/include"
    )
    BasicSet(NAME MAREA_FOLDER_3RDPARTY_GOOGLE_TEST VALUE "${MAREA_FOLDER_3RDPARTY}/GoogleTest")
    BasicSet(NAME MAREA_FOLDER_CMAKE_BINARY_3RDPARTY_GOOGLE_TEST VALUE "${MAREA_FOLDER_CMAKE_BINARY}/GoogleTest")
endmacro()
