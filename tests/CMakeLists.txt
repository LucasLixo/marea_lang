# Copyright (c) 2026 Paulo Lucas Alves Franca
# Licensed under the MIT License. See LICENSE.txt for full terms.

cmake_minimum_required(VERSION 3.20)

add_subdirectory("${CMAKE_SOURCE_DIR}/3rdparty/GoogleTest" "${CMAKE_BINARY_DIR}/GoogleTest" EXCLUDE_FROM_ALL)

include(GoogleTest)

file(GLOB_RECURSE MAREA_TESTS_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")

if(NOT TARGET marea_base)
    list(FILTER MAREA_TESTS_SOURCES EXCLUDE REGEX "/source/base/")
endif()

if(NOT TARGET marea_lex)
    list(FILTER MAREA_TESTS_SOURCES EXCLUDE REGEX "/source/lex/")
endif()

add_executable(marea_tests ${MAREA_TESTS_SOURCES})

target_include_directories(marea_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(marea_tests PRIVATE marea GTest::gtest_main)

if(TARGET marea_base)
    target_link_libraries(marea_tests PRIVATE marea::base)
endif()

if(TARGET marea_lex)
    target_link_libraries(marea_tests PRIVATE marea::lex)
endif()

gtest_discover_tests(marea_tests)
