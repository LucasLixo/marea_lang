// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// INFO: sema knows no library: a library type is read by the role and the ownership class its descriptor
// gives, never by its name. This test records that no source file of sema holds the name of a removed
// built-in as a string literal.

TEST(MAREASemaLibraryFree, NoSemaSourceNamesARemovedBuiltInType)
{
    GTEST_COUT << "Validating no source of sema holds array, vector, pointer, owned, slice or task as a string literal." << std::endl;

    const std::vector<std::string> NAMES{ "\"array\"", "\"vector\"", "\"pointer\"", "\"owned\"", "\"slice\"", "\"task\"" };

    std::string offenders;

    for (const std::filesystem::directory_entry &entry : std::filesystem::directory_iterator(MAREA_TESTS_SEMA_SOURCE_DIRECTORY)) {
        if (entry.path().extension() != ".c") {
            continue;
        }

        std::ifstream stream(entry.path(), std::ios::binary);

        std::ostringstream buffer;

        buffer << stream.rdbuf();

        const std::string TEXT{ buffer.str() };

        for (const std::string &name : NAMES) {
            if (TEXT.find(name) != std::string::npos) {
                offenders += entry.path().filename().string() + " names " + name + "\n";
            }
        }
    }

    EXPECT_TRUE(offenders.empty()) << "Sema must read descriptors, never a library type name:\n"
                                   << offenders;
}
