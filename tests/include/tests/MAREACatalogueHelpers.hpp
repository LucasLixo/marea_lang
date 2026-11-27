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

// INFO: the one second file of the catalogue: a namespace that the resolver finds, so an item can use a name of
// another namespace. It is parsed once and every check only reads its tree.
inline const MAREA_AstNode *MAREA_fixtureShapes()
{
    static MAREA_ParseFixture parser;

    static const MAREA_ParseExcept PARSED{ parser.MAREA_parse(MAREA_lines({
        "namespace fixture.shapes;",
        "#[visibility = public]",
        "structure Box",
        "    open: sinteger32;",
        "    #[visibility = private]",
        "    secret: sinteger32;",
        "end",
        "#[visibility = private]",
        "structure Hidden",
        "end",
        "implementation Box",
        "    #[visibility = private]",
        "    function conceal(self: Box)",
        "    end",
        "    function reveal(self: Box)",
        "    end",
        "end",
        "function make(): Box",
        "    return Box{ open = 1, secret = 2 };",
        "end",
        "opaque structure Texture;",
        "enum Color",
        "    Red;",
        "    Green;",
        "end",
    })) };

    return (PARSED == MAREA_PARSE_EXCEPT_SUCCESS) ? parser.root : nullptr;
}

inline std::string MAREA_readFile(const std::filesystem::path &path)
{
    std::ifstream stream(path, std::ios::binary);

    std::ostringstream buffer;

    buffer << stream.rdbuf();

    return buffer.str();
}

// INFO: the shipped source of marea.math, so an item uses the types that the language ships. It is parsed once.
inline const MAREA_AstNode *MAREA_shippedMath()
{
    static MAREA_ParseFixture parser;

    static const std::string SOURCE{ MAREA_readFile(MAREA_mathSourcePath()) };

    static const bool PARSED{ !SOURCE.empty() && (parser.MAREA_parse(SOURCE) == MAREA_PARSE_EXCEPT_SUCCESS) };

    return PARSED ? parser.root : nullptr;
}

// INFO: an item whose second line is '-- shipped: <path>' is a source file that the language ships: it is checked with
// that path and with the shipped library source input of sema. Any other item gives an empty path.
inline std::string MAREA_shippedPathOf(const std::string &source)
{
    const std::string MARK{ "-- shipped: " };

    const std::size_t SECOND{ source.find('\n') };

    if ((SECOND == std::string::npos) || (source.compare(SECOND + 1, MARK.size(), MARK) != 0)) {
        return "";
    }

    const std::size_t BEGIN{ SECOND + 1 + MARK.size() };

    return source.substr(BEGIN, source.find('\n', BEGIN) - BEGIN);
}

inline MAREA_SemaImportResult MAREA_resolveLibrary(const std::string &text, const MAREA_AstNode **outRoot)
{
    if ((text == "marea.math") && (MAREA_shippedMath() != nullptr)) {
        *outRoot = MAREA_shippedMath();

        return MAREA_SEMA_IMPORT_RESULT_FOUND;
    }

    if (text == "fixture.shapes") {
        *outRoot = MAREA_fixtureShapes();

        return MAREA_SEMA_IMPORT_RESULT_FOUND;
    }

    const auto STARTS{ [&text](const std::string &prefix) { return text.compare(0, prefix.size(), prefix) == 0; } };

    const bool EXTERNAL{ STARTS("collections.") || STARTS("concurrency.") || STARTS("arenas.") || (text == "text") || STARTS("c.") || (text == "demo") || (text == "marea.array") || (text == "marea.vector") || (text == "marea.tuple") || (text == "marea.memory") || (text == "marea.tasks") || STARTS("demo.") };

    return EXTERNAL ? MAREA_SEMA_IMPORT_RESULT_EXTERNAL : MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
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
