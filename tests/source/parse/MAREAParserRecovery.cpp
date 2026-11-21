// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAParseHelpers.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
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

    bool MAREA_parsesAndDumps(const std::string &source)
    {
        MAREA_ParseFixture fixture;

        if (fixture.MAREA_parse(source) != MAREA_PARSE_EXCEPT_SUCCESS) {
            return false;
        }

        return (fixture.root != nullptr) && (fixture.MAREA_dump().find("COMPILATION_UNIT\n") == 0);
    }

} // namespace

TEST(MAREAParserRecovery, DegenerateInputsTerminate)
{
    GTEST_COUT << "Validating degenerate inputs end with a root and a diagnostic." << std::endl;

    const std::vector<std::string> SOURCES{ "", "end", ";", ")", "}", "]", "#[", "#![", "namespace", "function", "constant", "if", std::string(1000, ';') };

    for (const std::string &source : SOURCES) {
        MAREA_ParseFixture fixture;

        ASSERT_EQ(fixture.MAREA_parse(source), MAREA_PARSE_EXCEPT_SUCCESS) << "Parsing of an input of " << source.size() << " bytes must succeed.";

        EXPECT_NE(fixture.root, nullptr) << "A root must be returned.";

        EXPECT_FALSE(fixture.MAREA_codes().empty()) << "An input of " << source.size() << " bytes must give a diagnostic.";
    }
}

TEST(MAREAParserRecovery, DeepNestingOfBlocksAndTypes)
{
    GTEST_COUT << "Validating 300 nested ifs and 300 nested type arguments give RECURSION_TOO_DEEP." << std::endl;

    std::string ifs{ "namespace main; function f() " };

    for (int index{ 0 }; index < 300; ++index) {
        ifs += "if a then ";
    }

    for (int index{ 0 }; index < 300; ++index) {
        ifs += "end ";
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

    for (const std::string &source : { ifs, types }) {
        MAREA_ParseFixture fixture;

        ASSERT_EQ(fixture.MAREA_parse(source), MAREA_PARSE_EXCEPT_SUCCESS) << "Parsing must succeed.";

        const std::vector<MAREA_Uinteger32> CODES{ fixture.MAREA_codes() };

        EXPECT_NE(std::find(CODES.begin(), CODES.end(), static_cast<MAREA_Uinteger32>(MAREA_PARSE_DIAGNOSTIC_CODE_RECURSION_TOO_DEEP)), CODES.end()) << "RECURSION_TOO_DEEP must be reported.";
    }
}

TEST(MAREAParserRecovery, LongFlatChainsDoNotCrash)
{
    GTEST_COUT << "Validating a chain of 5000 additions and a block of 5000 statements parse and dump." << std::endl;

    std::string chain{ "a" };

    for (int index{ 0 }; index < 5000; ++index) {
        chain += " + a";
    }

    MAREA_ParseFixture chainFixture;

    EXPECT_NE(chainFixture.MAREA_expression(chain).find("binary:PLUS"), std::string::npos) << "The chain must parse.";

    EXPECT_TRUE(chainFixture.MAREA_codes().empty()) << "A long chain must give no diagnostic.";

    std::string statements;

    for (int index{ 0 }; index < 5000; ++index) {
        statements += "f(); ";
    }

    MAREA_ParseFixture blockFixture;

    EXPECT_NE(blockFixture.MAREA_statements(statements).find("expression_statement"), std::string::npos) << "The block must parse.";

    EXPECT_TRUE(blockFixture.MAREA_codes().empty()) << "A long block must give no diagnostic.";
}

TEST(MAREAParserRecovery, TruncationAtEveryTokenOfOneExample)
{
    GTEST_COUT << "Validating every token prefix of examples/basics/functions.marea parses without a crash or a hang." << std::endl;

    const std::string SOURCE{ MAREA_readSource(std::filesystem::path(MAREA_TESTS_EXAMPLES_DIRECTORY) / "basics" / "functions.marea") };

    ASSERT_FALSE(SOURCE.empty()) << "The example must be readable.";

    MAREA_LexFixture lexer;

    ASSERT_EQ(lexer.MAREA_lex(SOURCE), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    const std::vector<MAREA_SourcePosition> POSITIONS{ lexer.MAREA_positions() };

    ASSERT_GT(POSITIONS.size(), 100U) << "The example must hold many tokens.";

    for (const MAREA_SourcePosition &position : POSITIONS) {
        EXPECT_TRUE(MAREA_parsesAndDumps(SOURCE.substr(0, position.offset))) << "The prefix of " << position.offset << " bytes must parse and dump.";
    }

    MAREA_ParseFixture whole;

    ASSERT_EQ(whole.MAREA_parse(SOURCE), MAREA_PARSE_EXCEPT_SUCCESS) << "Parsing must succeed.";

    EXPECT_TRUE(whole.MAREA_codes().empty()) << "The whole example must give no diagnostic.";
}

TEST(MAREAParserRecovery, DeletingEveryTokenOfOneExample)
{
    GTEST_COUT << "Validating an example with any single token blanked out still parses without a crash or a hang." << std::endl;

    const std::string SOURCE{ MAREA_readSource(std::filesystem::path(MAREA_TESTS_EXAMPLES_DIRECTORY) / "types" / "generics.marea") };

    ASSERT_FALSE(SOURCE.empty()) << "The example must be readable.";

    MAREA_LexFixture lexer;

    ASSERT_EQ(lexer.MAREA_lex(SOURCE), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    for (const MAREA_SourcePosition &position : lexer.MAREA_positions()) {
        std::string blanked{ SOURCE };

        for (MAREA_Uinteger32 offset{ position.offset }; offset < (position.offset + position.length); ++offset) {
            blanked[offset] = ' ';
        }

        EXPECT_TRUE(MAREA_parsesAndDumps(blanked)) << "The example without the token at " << position.offset << " must parse and dump.";
    }
}

TEST(MAREAParserRecovery, SampledTruncationOfEveryExample)
{
    GTEST_COUT << "Validating every example cut at 40 even byte offsets parses without a crash or a hang." << std::endl;

    for (const std::filesystem::directory_entry &entry : std::filesystem::recursive_directory_iterator(MAREA_TESTS_EXAMPLES_DIRECTORY)) {
        if (!entry.is_regular_file() || (entry.path().extension() != ".marea")) {
            continue;
        }

        const std::string SOURCE{ MAREA_readSource(entry.path()) };

        for (size_t step{ 0 }; step <= 40; ++step) {
            EXPECT_TRUE(MAREA_parsesAndDumps(SOURCE.substr(0, (SOURCE.size() * step) / 40))) << entry.path() << " cut at step " << step << " must parse and dump.";
        }
    }
}
