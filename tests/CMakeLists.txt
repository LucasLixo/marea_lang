# Copyright (c) 2026 Paulo Lucas Alves Franca
# Licensed under the MIT License. See LICENSE.txt for full terms.

cmake_minimum_required(VERSION 3.20)

add_subdirectory("${CMAKE_SOURCE_DIR}/3rdparty/GoogleTest" "${CMAKE_BINARY_DIR}/GoogleTest" EXCLUDE_FROM_ALL)

include(GoogleTest)

file(GLOB_RECURSE MAREA_TESTS_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")

add_executable(marea_tests ${MAREA_TESTS_SOURCES})

target_include_directories(marea_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(marea_tests PRIVATE marea GTest::gtest_main)

gtest_discover_tests(marea_tests)
