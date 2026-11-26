# Copyright (c) 2026 Paulo Lucas Alves Franca
# Licensed under the MIT License. See LICENSE.txt for full terms.

cmake_minimum_required(VERSION 3.20)

add_subdirectory(
    "${MAREA_FOLDER_3RDPARTY_GOOGLE_TEST}"
    "${MAREA_FOLDER_CMAKE_BINARY_3RDPARTY_GOOGLE_TEST}"
    EXCLUDE_FROM_ALL
)

include(GoogleTest)

file(GLOB_RECURSE MAREA_TESTS_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")

if(NOT TARGET marea::base)
    list(FILTER MAREA_TESTS_SOURCES EXCLUDE REGEX "/source/base/")
endif()

if(NOT TARGET marea::lex)
    list(FILTER MAREA_TESTS_SOURCES EXCLUDE REGEX "/source/lex/")
endif()

if(NOT TARGET marea::parse)
    list(FILTER MAREA_TESTS_SOURCES EXCLUDE REGEX "/source/parse/")
endif()

if(NOT TARGET marea::sema)
    list(FILTER MAREA_TESTS_SOURCES EXCLUDE REGEX "/source/sema/")
endif()

if(NOT TARGET marea::ir)
    list(FILTER MAREA_TESTS_SOURCES EXCLUDE REGEX "/source/ir/")
endif()

if(NOT TARGET marea::bytecode)
    list(FILTER MAREA_TESTS_SOURCES EXCLUDE REGEX "/source/bytecode/")
endif()

if(NOT TARGET marea::vm)
    list(FILTER MAREA_TESTS_SOURCES EXCLUDE REGEX "/source/vm/")
endif()

# INFO: cgen is a separate backend of the ir (never bytecode or vm), so its test filter is its own region.
if(NOT TARGET marea::cgen)
    list(FILTER MAREA_TESTS_SOURCES EXCLUDE REGEX "/source/cgen/")
endif()

# INFO: the parity tests need both backends, so they and the cgen tests that compare with the vm need both targets.
if(NOT TARGET marea::cgen OR NOT TARGET marea::vm)
    list(FILTER MAREA_TESTS_SOURCES EXCLUDE REGEX "/source/cgen/MAREACgen[A-Za-z]+\\.cpp$")
    list(FILTER MAREA_TESTS_SOURCES EXCLUDE REGEX "/source/language/MAREALanguageParity\\.cpp$")
endif()

add_executable(marea_tests ${MAREA_TESTS_SOURCES})

target_include_directories(marea_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(marea_tests PRIVATE marea::include GTest::gtest_main)

# INFO: tests may include internal headers (see the cpp-testing skill).
target_include_directories(marea_tests PRIVATE "${MAREA_FOLDER_LIB_INTERNAL}")

target_compile_definitions(marea_tests PRIVATE "MAREA_TESTS_EXAMPLES_DIRECTORY=\"${MAREA_FOLDER_EXAMPLES}\"")

target_compile_definitions(marea_tests PRIVATE "MAREA_TESTS_SEMA_SOURCE_DIRECTORY=\"${MAREA_FOLDER_LIB_SOURCE}/marea/sema\"")

target_compile_definitions(marea_tests PRIVATE "MAREA_TESTS_LANGUAGE_DIRECTORY=\"${MAREA_FOLDER_TESTS}/language\"")

target_compile_definitions(marea_tests PRIVATE "MAREA_TESTS_LIB_INCLUDE_DIRECTORY=\"${MAREA_FOLDER_LIB_INCLUDE}\"")

if(TARGET marea::base)
    target_link_libraries(marea_tests PRIVATE marea::base)
endif()

if(TARGET marea::lex)
    target_link_libraries(marea_tests PRIVATE marea::lex)
endif()

if(TARGET marea::parse)
    target_link_libraries(marea_tests PRIVATE marea::parse)
endif()

if(TARGET marea::sema)
    target_link_libraries(marea_tests PRIVATE marea::sema)
endif()

if(TARGET marea::ir)
    target_link_libraries(marea_tests PRIVATE marea::ir)
endif()

if(TARGET marea::bytecode)
    target_link_libraries(marea_tests PRIVATE marea::bytecode)
endif()

if(TARGET marea::vm)
    target_link_libraries(marea_tests PRIVATE marea::vm)
endif()

# INFO: cgen's own link block, separate from the bytecode/vm region above so a parallel worker never collides.
if(TARGET marea::cgen)
    target_link_libraries(marea_tests PRIVATE marea::cgen)
endif()

target_compile_definitions(marea_tests PRIVATE "MAREA_TESTS_CGEN_C_COMPILER=\"${CMAKE_C_COMPILER}\"")

# INFO: the generated C programs of the parity tests include the host header of tests/include and link the base library.
target_compile_definitions(marea_tests PRIVATE "MAREA_TESTS_SUPPORT_DIRECTORY=\"${CMAKE_CURRENT_SOURCE_DIR}/include\"")

if(TARGET marea::base)
    target_compile_definitions(marea_tests PRIVATE "MAREA_TESTS_BASE_LIBRARY=\"$<TARGET_FILE:marea::base>\"")
endif()

# INFO: the cgen tests also exercise every other C11 compiler this machine has, beyond CMAKE_C_COMPILER.
find_program(MAREA_TESTS_CGEN_CLANG "clang")
find_program(MAREA_TESTS_CGEN_GCC "gcc")

if(MAREA_TESTS_CGEN_CLANG)
    target_compile_definitions(marea_tests PRIVATE "MAREA_TESTS_CGEN_CLANG=\"${MAREA_TESTS_CGEN_CLANG}\"")
endif()

if(MAREA_TESTS_CGEN_GCC)
    target_compile_definitions(marea_tests PRIVATE "MAREA_TESTS_CGEN_GCC=\"${MAREA_TESTS_CGEN_GCC}\"")
endif()

gtest_discover_tests(marea_tests)

# One CTest entry per combination of the library options, each configuring the project in a scratch folder
foreach(
    TEMP_CASE
    IN
    ITEMS
        "StaticOnly;ON;OFF;PASS"
        "SharedOnly;OFF;ON;PASS"
        "StaticAndShared;ON;ON;PASS"
        "BothOff;OFF;OFF;FAIL"
)
    list(GET TEMP_CASE 0 TEMP_CASE_NAME)
    list(GET TEMP_CASE 1 TEMP_CASE_STATIC)
    list(GET TEMP_CASE 2 TEMP_CASE_SHARED)
    list(GET TEMP_CASE 3 TEMP_CASE_EXPECT)
    add_test(
        NAME MareaLibraryOptions.${TEMP_CASE_NAME}
        COMMAND
            "${CMAKE_COMMAND}" "-DCHECK_SOURCE=${MAREA_FOLDER}"
            "-DCHECK_BINARY=${CMAKE_CURRENT_BINARY_DIR}/OptionCheck/${TEMP_CASE_NAME}"
            "-DCHECK_GENERATOR=${CMAKE_GENERATOR}" "-DCHECK_C_COMPILER=${CMAKE_C_COMPILER}"
            "-DCHECK_CXX_COMPILER=${CMAKE_CXX_COMPILER}" "-DCHECK_STATIC=${TEMP_CASE_STATIC}"
            "-DCHECK_SHARED=${TEMP_CASE_SHARED}" "-DCHECK_EXPECT=${TEMP_CASE_EXPECT}" -P
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CheckLibraryOptions.cmake"
    )
endforeach()
unset(TEMP_CASE)
unset(TEMP_CASE_NAME)
unset(TEMP_CASE_STATIC)
unset(TEMP_CASE_SHARED)
unset(TEMP_CASE_EXPECT)

# marea_add_library/marea_add_executable tool detection, with and without a marea-cc/marea-bc stand-in on PATH
foreach(TEMP_CASE IN ITEMS "NoTool;;FAIL" "StandIn;${CMAKE_CURRENT_SOURCE_DIR}/cmake/fixtures/MareaStandIns;PASS")
    list(GET TEMP_CASE 0 TEMP_CASE_NAME)
    list(GET TEMP_CASE 1 TEMP_CASE_STUB_DIR)
    list(GET TEMP_CASE 2 TEMP_CASE_EXPECT)
    add_test(
        NAME MareaFunctions.${TEMP_CASE_NAME}
        COMMAND
            "${CMAKE_COMMAND}" "-DCHECK_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/cmake/fixtures/MareaFunctions"
            "-DCHECK_BINARY=${CMAKE_CURRENT_BINARY_DIR}/MareaFunctionsCheck/${TEMP_CASE_NAME}"
            "-DCHECK_GENERATOR=${CMAKE_GENERATOR}" "-DCHECK_C_COMPILER=${CMAKE_C_COMPILER}"
            "-DCHECK_CXX_COMPILER=${CMAKE_CXX_COMPILER}" "-DCHECK_MAREA_ROOT=${MAREA_FOLDER}"
            "-DCHECK_STUB_DIR=${TEMP_CASE_STUB_DIR}" "-DCHECK_EXPECT=${TEMP_CASE_EXPECT}" -P
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CheckMareaFunctions.cmake"
    )
endforeach()
unset(TEMP_CASE)
unset(TEMP_CASE_NAME)
unset(TEMP_CASE_STUB_DIR)
unset(TEMP_CASE_EXPECT)
