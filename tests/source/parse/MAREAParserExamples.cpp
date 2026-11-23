// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAParseHelpers.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace
{

    std::vector<std::filesystem::path> MAREA_collectExampleFiles()
    {
        std::vector<std::filesystem::path> result;

        for (const std::filesystem::directory_entry &entry : std::filesystem::recursive_directory_iterator(MAREA_TESTS_EXAMPLES_DIRECTORY)) {
            if (entry.is_regular_file() && (entry.path().extension() == ".marea")) {
                result.push_back(entry.path());
            }
        }

        return result;
    }

    std::string MAREA_readFile(const std::filesystem::path &path)
    {
        std::ifstream stream(path, std::ios::binary);

        std::ostringstream buffer;

        buffer << stream.rdbuf();

        return buffer.str();
    }

    bool MAREA_hasErrorNode(const MAREA_AstNode *node)
    {
        if (MAREA_AstNodeGetKind(node) == MAREA_AST_KIND_ERROR_NODE) {
            return true;
        }

        const MAREA_IntegerSize COUNT{ MAREA_AstNodeGetChildCount(node) };

        for (MAREA_IntegerSize index{ 0 }; index < COUNT; ++index) {
            const MAREA_AstNode *child{ nullptr };

            MAREA_IGNORE(MAREA_AstNodeGetChildAt(node, index, &child));

            if (MAREA_hasErrorNode(child)) {
                return true;
            }
        }

        return false;
    }

    std::string MAREA_firstDiagnostics(const MAREA_ParseFixture &fixture, const std::filesystem::path &path)
    {
        std::ostringstream text;

        const MAREA_IntegerSize COUNT{ MAREA_DiagnosticSinkGetCount(fixture.lexer.sink) };

        for (MAREA_IntegerSize index{ 0 }; (index < COUNT) && (index < 5); ++index) {
            const MAREA_Diagnostic *diagnostic{ nullptr };

            MAREA_IGNORE(MAREA_DiagnosticSinkGetAt(fixture.lexer.sink, index, &diagnostic));

            const MAREA_SourcePosition POSITION{ MAREA_DiagnosticGetPosition(diagnostic) };

            text << "\n  " << path.string() << ":" << POSITION.line << ":" << POSITION.column << ": [" << MAREA_DiagnosticGetCode(diagnostic) << "] " << MAREA_DiagnosticGetMessage(diagnostic);
        }

        return text.str();
    }

} // namespace

TEST(MAREAParserExamples, ExampleCorpusHasExpectedFileCount)
{
    GTEST_COUT << "Validating the examples corpus has exactly 23 .marea files, so an empty directory cannot pass." << std::endl;

    EXPECT_EQ(MAREA_collectExampleFiles().size(), 23U) << "There must be exactly 23 example files under " << MAREA_TESTS_EXAMPLES_DIRECTORY << ".";
}

TEST(MAREAParserExamples, EveryExampleParsesWithZeroDiagnostics)
{
    GTEST_COUT << "Validating every example parses with zero diagnostics and no ERROR_NODE." << std::endl;

    for (const std::filesystem::path &path : MAREA_collectExampleFiles()) {
        MAREA_ParseFixture fixture;

        ASSERT_EQ(fixture.MAREA_parse(MAREA_readFile(path)), MAREA_PARSE_EXCEPT_SUCCESS) << "Parsing " << path << " must succeed.";

        EXPECT_EQ(MAREA_DiagnosticSinkGetCount(fixture.lexer.sink), MAREA_INTEGER_SIZE(0)) << path << " must give no diagnostic." << MAREA_firstDiagnostics(fixture, path);

        EXPECT_FALSE(MAREA_hasErrorNode(fixture.root)) << path << " must not hold an ERROR_NODE.";
    }
}

TEST(MAREAParserExamples, ExampleRootsStartWithNamespace)
{
    GTEST_COUT << "Validating every example gives a COMPILATION_UNIT that starts with a NAMESPACE_DECLARATION." << std::endl;

    for (const std::filesystem::path &path : MAREA_collectExampleFiles()) {
        MAREA_ParseFixture fixture;

        ASSERT_EQ(fixture.MAREA_parse(MAREA_readFile(path)), MAREA_PARSE_EXCEPT_SUCCESS) << "Parsing " << path << " must succeed.";

        EXPECT_EQ(MAREA_AstNodeGetKind(fixture.root), MAREA_AST_KIND_COMPILATION_UNIT) << path << " must give a COMPILATION_UNIT.";

        const MAREA_AstNode *first{ nullptr };

        ASSERT_EQ(MAREA_AstNodeGetChildAt(fixture.root, 0, &first), MAREA_AST_EXCEPT_SUCCESS) << path << " must have a first child.";

        EXPECT_EQ(MAREA_AstNodeGetKind(first), MAREA_AST_KIND_NAMESPACE_DECLARATION) << path << " must start with the namespace.";
    }
}

TEST(MAREAParserExamples, ExampleDumpsAreDeterministic)
{
    GTEST_COUT << "Validating parsing an example twice gives byte identical dumps." << std::endl;

    for (const std::filesystem::path &path : MAREA_collectExampleFiles()) {
        const std::string SOURCE{ MAREA_readFile(path) };

        MAREA_ParseFixture first;

        MAREA_ParseFixture second;

        ASSERT_EQ(first.MAREA_parse(SOURCE), MAREA_PARSE_EXCEPT_SUCCESS) << "Parsing " << path << " must succeed.";

        ASSERT_EQ(second.MAREA_parse(SOURCE), MAREA_PARSE_EXCEPT_SUCCESS) << "Parsing " << path << " must succeed.";

        EXPECT_EQ(first.MAREA_dump(), second.MAREA_dump()) << path << " must dump the same text twice.";
    }
}
