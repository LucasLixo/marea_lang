// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_TESTS_MAREASCRATCHDIRECTORY_HPP)
    #define MAREA_TESTS_MAREASCRATCHDIRECTORY_HPP

    #include <gtest/gtest.h>

    #include <filesystem>
    #include <string>
    #include <system_error>

    #include <stdlib.h>

// INFO: ctest -j starts one process per test, so only the system can promise a name that no other process takes.
struct MAREA_ScratchDirectory
{
    std::filesystem::path path;

    MAREA_ScratchDirectory()
    {
        std::string pattern{ std::string(::testing::TempDir()) + "marea_test_XXXXXX" };

        if (mkdtemp(pattern.data()) == nullptr) {
            ADD_FAILURE() << "A scratch directory must be created from " << pattern;

            return;
        }

        path = pattern;
    }

    ~MAREA_ScratchDirectory()
    {
        std::error_code ignored;

        std::filesystem::remove_all(path, ignored);
    }

    MAREA_ScratchDirectory(const MAREA_ScratchDirectory &) = delete;
    MAREA_ScratchDirectory &operator=(const MAREA_ScratchDirectory &) = delete;

    std::string MAREA_file(const std::string &name) const
    {
        return (path / name).string();
    }
};

#endif
