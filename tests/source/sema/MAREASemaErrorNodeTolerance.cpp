// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>

namespace
{
    std::string MAREA_readSource(const std::filesystem::path &path)
    {
        std::ifstream stream(path, std::ios::binary);

        std::ostringstream buffer;

        buffer << stream.rdbuf();

        return buffer.str();
    }

    void MAREA_collectErrorRanges(const MAREA_AstNode *node, std::vector<std::pair<MAREA_Uinteger32, MAREA_Uinteger32>> &ranges)
    {
        if (MAREA_AstNodeGetKind(node) == MAREA_AST_KIND_ERROR_NODE) {
            const MAREA_SourcePosition POSITION{ MAREA_AstNodeGetPosition(node) };

            ranges.emplace_back(POSITION.offset, POSITION.offset + ((POSITION.length == 0) ? 1 : POSITION.length));

            return;
        }

        const MAREA_IntegerSize COUNT{ MAREA_AstNodeGetChildCount(node) };

        for (MAREA_IntegerSize index{ 0 }; index < COUNT; ++index) {
            const MAREA_AstNode *child{ nullptr };

            MAREA_IGNORE(MAREA_AstNodeGetChildAt(node, index, &child));

            MAREA_collectErrorRanges(child, ranges);
        }
    }

    ::testing::AssertionResult MAREA_survives(const std::string &source)
    {
        MAREA_SemaFixture fixture([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; });

        if (fixture.MAREA_sema(source) != MAREA_SEMA_EXCEPT_SUCCESS) {
            return ::testing::AssertionFailure() << "The check did not succeed for an input of " << source.size() << " bytes.";
        }

        std::vector<std::pair<MAREA_Uinteger32, MAREA_Uinteger32>> ranges;

        MAREA_collectErrorRanges(fixture.parser.root, ranges);

        const MAREA_IntegerSize COUNT{ MAREA_DiagnosticSinkGetCount(fixture.parser.lexer.sink) };

        for (MAREA_IntegerSize index{ 0 }; index < COUNT; ++index) {
            const MAREA_Diagnostic *diagnostic{ nullptr };

            MAREA_IGNORE(MAREA_DiagnosticSinkGetAt(fixture.parser.lexer.sink, index, &diagnostic));

            if (MAREA_DiagnosticGetCode(diagnostic) < 3000) {
                continue;
            }

            const MAREA_Uinteger32 OFFSET{ MAREA_DiagnosticGetPosition(diagnostic).offset };

            for (const auto &range : ranges) {
                if ((OFFSET >= range.first) && (OFFSET < range.second)) {
                    return ::testing::AssertionFailure() << "Sema diagnostic " << MAREA_DiagnosticGetCode(diagnostic) << " sits under an ERROR_NODE at offset " << OFFSET << ".";
                }
            }
        }

        return ::testing::AssertionSuccess();
    }
} // namespace

TEST(MAREASemaErrorNodeTolerance, DegenerateInputsSurvive)
{
    GTEST_COUT << "Validating degenerate inputs check without a crash and without a diagnostic under an ERROR_NODE." << std::endl;

    const std::vector<std::string> SOURCES{ "", "end", ";", ")", "}", "]", "#[", "#![", "namespace", "function", "constant", "if", std::string(1000, ';') };

    for (const std::string &source : SOURCES) {
        EXPECT_TRUE(MAREA_survives(source)) << "An input of " << source.size() << " bytes must survive.";
    }
}

TEST(MAREASemaErrorNodeTolerance, DeepNestingSurvives)
{
    GTEST_COUT << "Validating 300 nested ifs and 300 nested type arguments survive." << std::endl;

    std::string ifs{ "namespace main; function f() " };

    for (int index{ 0 }; index < 300; ++index) {
        ifs += "if a then ";
    }

    for (int index{ 0 }; index < 300; ++index) {
        ifs += "end end ";
    }

    ifs += "end";

    std::string types{ "namespace main; constant x: " };

    for (int index{ 0 }; index < 300; ++index) {
        types += "array<";
    }

    types += "integer32";

    for (int index{ 0 }; index < 300; ++index) {
        types += ">";
    }

    types += " = 1;";

    EXPECT_TRUE(MAREA_survives(ifs)) << "Nested ifs must survive.";

    EXPECT_TRUE(MAREA_survives(types)) << "Nested type arguments must survive.";
}

TEST(MAREASemaErrorNodeTolerance, TruncationAtEveryTokenSurvives)
{
    GTEST_COUT << "Validating every token prefix of examples/basics/functions.marea survives." << std::endl;

    const std::string SOURCE{ MAREA_readSource(std::filesystem::path(MAREA_TESTS_EXAMPLES_DIRECTORY) / "basics" / "functions.marea") };

    ASSERT_FALSE(SOURCE.empty()) << "The example must be readable.";

    MAREA_LexFixture lexer;

    ASSERT_EQ(lexer.MAREA_lex(SOURCE), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    for (const MAREA_SourcePosition &position : lexer.MAREA_positions()) {
        EXPECT_TRUE(MAREA_survives(SOURCE.substr(0, position.offset))) << "The prefix of " << position.offset << " bytes must survive.";
    }
}
