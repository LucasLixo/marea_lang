// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_TESTS_MAREACATALOGUEHELPERS_HPP)
    #define MAREA_TESTS_MAREACATALOGUEHELPERS_HPP

    #include "tests/MAREASemaHelpers.hpp"

    #include <algorithm>
    #include <filesystem>
    #include <fstream>
    #include <sstream>
    #include <string>
    #include <vector>

// INFO: how the language catalogue reads an item: every test that reads one, the catalogue and the parity test, reads
// it with the same resolver and the same registry, so a program means the same in both.
struct MAREA_ItemFile
{
    std::string relativePath;
    std::string source;
};

inline MAREA_SemaImportResult MAREA_resolveLibrary(const std::string &text)
{
    const auto STARTS{ [&text](const std::string &prefix) { return text.compare(0, prefix.size(), prefix) == 0; } };

    const bool EXTERNAL{ STARTS("collections.") || STARTS("concurrency.") || STARTS("arenas.") || (text == "text") || STARTS("c.") || (text == "demo") || (text == "marea.array") || (text == "marea.memory") || (text == "marea.tasks") || STARTS("demo.") };

    return EXTERNAL ? MAREA_SEMA_IMPORT_RESULT_EXTERNAL : MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
}

inline std::string MAREA_readFile(const std::filesystem::path &path)
{
    std::ifstream stream(path, std::ios::binary);

    std::ostringstream buffer;

    buffer << stream.rdbuf();

    return buffer.str();
}

inline std::vector<MAREA_ItemFile> MAREA_collectItems()
{
    std::vector<MAREA_ItemFile> result;

    const std::filesystem::path ROOT{ MAREA_TESTS_LANGUAGE_DIRECTORY };

    for (const std::filesystem::directory_entry &entry : std::filesystem::recursive_directory_iterator(ROOT)) {
        if (entry.is_regular_file() && (entry.path().extension() == ".marea")) {
            result.push_back({ std::filesystem::relative(entry.path(), ROOT).generic_string(), MAREA_readFile(entry.path()) });
        }
    }

    std::sort(result.begin(), result.end(), [](const MAREA_ItemFile &left, const MAREA_ItemFile &right) { return left.relativePath < right.relativePath; });

    return result;
}

#endif
