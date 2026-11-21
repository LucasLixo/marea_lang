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

if(NOT TARGET Marea-Base)
    list(FILTER MAREA_TESTS_SOURCES EXCLUDE REGEX "/source/base/")
endif()

if(NOT TARGET Marea-Lex)
    list(FILTER MAREA_TESTS_SOURCES EXCLUDE REGEX "/source/lex/")
endif()

if(NOT TARGET Marea-Parse)
    list(FILTER MAREA_TESTS_SOURCES EXCLUDE REGEX "/source/parse/")
endif()

if(NOT TARGET Marea-Sema)
    list(FILTER MAREA_TESTS_SOURCES EXCLUDE REGEX "/source/sema/")
endif()

add_executable(marea_tests ${MAREA_TESTS_SOURCES})

target_include_directories(marea_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(marea_tests PRIVATE marea::include GTest::gtest_main)

target_compile_definitions(marea_tests PRIVATE "MAREA_TESTS_EXAMPLES_DIRECTORY=\"${MAREA_FOLDER_EXAMPLES}\"")

if(TARGET Marea-Base)
    target_link_libraries(marea_tests PRIVATE marea::base)
endif()

if(TARGET Marea-Lex)
    target_link_libraries(marea_tests PRIVATE marea::lex)
endif()

if(TARGET Marea-Parse)
    target_link_libraries(marea_tests PRIVATE marea::parse)
endif()

if(TARGET Marea-Sema)
    target_link_libraries(marea_tests PRIVATE marea::sema)
endif()

gtest_discover_tests(marea_tests)
