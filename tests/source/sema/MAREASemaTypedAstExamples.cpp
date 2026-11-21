// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "marea/sema/MAREASemaTypedAst.h"

#include "tests/MAREASemaHelpers.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>

namespace
{
    struct MAREA_ExampleFile
    {
        std::string relativePath;
        std::string source;
        std::string namespaceText;
    };

    std::string MAREA_readFile(const std::filesystem::path &path)
    {
        std::ifstream stream(path, std::ios::binary);

        std::ostringstream buffer;

        buffer << stream.rdbuf();

        return buffer.str();
    }

    std::vector<MAREA_ExampleFile> MAREA_collectExamples()
    {
        std::vector<MAREA_ExampleFile> result;

        const std::filesystem::path ROOT{ MAREA_TESTS_EXAMPLES_DIRECTORY };

        for (const std::filesystem::directory_entry &entry : std::filesystem::recursive_directory_iterator(ROOT)) {
            if (!entry.is_regular_file() || (entry.path().extension() != ".marea")) {
                continue;
            }

            std::string relative{ std::filesystem::relative(entry.path(), ROOT).generic_string() };

            std::string namespaceText{ relative.substr(0, relative.size() - std::string(".marea").size()) };

            std::replace(namespaceText.begin(), namespaceText.end(), '/', '.');

            result.push_back({ relative, MAREA_readFile(entry.path()), namespaceText });
        }

        std::sort(result.begin(), result.end(), [](const MAREA_ExampleFile &left, const MAREA_ExampleFile &right) { return left.relativePath < right.relativePath; });

        return result;
    }

    bool MAREA_startsWith(const std::string &text, const std::string &prefix)
    {
        return text.compare(0, prefix.size(), prefix) == 0;
    }
} // namespace

TEST(MAREASemaTypedAstExamples, EveryExampleBuildsATypedTreeWithoutCrashing)
{
    GTEST_COUT << "Validating every example checks clean and then builds a typed shadow tree without crashing." << std::endl;

    const std::vector<MAREA_ExampleFile> FILES{ MAREA_collectExamples() };

    std::map<std::string, const MAREA_AstNode *> roots;

    std::vector<std::unique_ptr<MAREA_ParseFixture>> parsed;

    std::vector<std::string> paths;

    for (const MAREA_ExampleFile &file : FILES) {
        paths.push_back(file.relativePath);

        auto fixture{ std::make_unique<MAREA_ParseFixture>() };

        ASSERT_EQ(MAREA_LexContextSetFileName(fixture->lexer.context, paths.back().c_str()), MAREA_LEX_EXCEPT_SUCCESS) << "The file name of " << file.relativePath << " must be set.";

        ASSERT_EQ(fixture->MAREA_parse(file.source), MAREA_PARSE_EXCEPT_SUCCESS) << "The example " << file.relativePath << " must parse.";

        roots[file.namespaceText] = fixture->root;

        parsed.push_back(std::move(fixture));
    }

    const MAREA_ResolveCallback RESOLVE{ [&roots](const std::string &text, const MAREA_AstNode **outRoot) {
        const auto FOUND{ roots.find(text) };

        if (FOUND != roots.end()) {
            *outRoot = FOUND->second;

            return MAREA_SEMA_IMPORT_RESULT_FOUND;
        }

        const bool EXTERNAL{ MAREA_startsWith(text, "collections.") || MAREA_startsWith(text, "concurrency.") || MAREA_startsWith(text, "arenas.") || (text == "text") || MAREA_startsWith(text, "c.") };

        return EXTERNAL ? MAREA_SEMA_IMPORT_RESULT_EXTERNAL : MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
    } };

    for (const MAREA_ExampleFile &file : FILES) {
        MAREA_SemaFixture fixture(RESOLVE);

        ASSERT_EQ(fixture.MAREA_sema(file.source, file.relativePath), MAREA_SEMA_EXCEPT_SUCCESS) << "The check of " << file.relativePath << " must succeed.";

        ASSERT_EQ(fixture.MAREA_errorCount(), MAREA_INTEGER_SIZE(0)) << "The example " << file.relativePath << " must check with zero errors.";

        const MAREA_SemaTypedNode *root{ nullptr };

        EXPECT_EQ(MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &root), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The example " << file.relativePath << " must build a typed tree.";

        EXPECT_NE(root, nullptr) << "The example " << file.relativePath << " must produce a typed root.";
    }
}
