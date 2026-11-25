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

add_executable(marea_tests ${MAREA_TESTS_SOURCES})

target_include_directories(marea_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(marea_tests PRIVATE marea::include GTest::gtest_main)

# INFO: tests may include internal headers (see the cpp-testing skill).
target_include_directories(marea_tests PRIVATE "${MAREA_FOLDER_LIB_INTERNAL}")

target_compile_definitions(marea_tests PRIVATE "MAREA_TESTS_EXAMPLES_DIRECTORY=\"${MAREA_FOLDER_EXAMPLES}\"")

target_compile_definitions(marea_tests PRIVATE "MAREA_TESTS_SEMA_SOURCE_DIRECTORY=\"${MAREA_FOLDER_LIB_SOURCE}/marea/sema\"")

target_compile_definitions(marea_tests PRIVATE "MAREA_TESTS_LANGUAGE_DIRECTORY=\"${MAREA_FOLDER_TESTS}/language\"")

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
