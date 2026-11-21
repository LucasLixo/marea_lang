#include "tests/dependencies.hpp"

#include "tests/MAREALexHelpers.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace
{

    std::vector<std::filesystem::path> MAREA_CollectExampleFiles()
    {
        std::vector<std::filesystem::path> result;

        for (const std::filesystem::directory_entry &ENTRY : std::filesystem::recursive_directory_iterator(MAREA_TESTS_EXAMPLES_DIRECTORY)) {
            if (ENTRY.is_regular_file() && (ENTRY.path().extension() == ".marea")) {
                result.push_back(ENTRY.path());
            }
        }

        return result;
    }

    std::string MAREA_ReadFile(const std::filesystem::path &path)
    {
        std::ifstream stream(path, std::ios::binary);

        std::ostringstream buffer;

        buffer << stream.rdbuf();

        return buffer.str();
    }

} // namespace

TEST(MAREALexerExamples, ExampleCorpusHasExpectedFileCount)
{
    GTEST_COUT << "Validating the examples corpus has exactly 23 .marea files, so an empty directory cannot pass." << std::endl;

    const std::vector<std::filesystem::path> FILES{ MAREA_CollectExampleFiles() };

    EXPECT_EQ(FILES.size(), 23U) << "There must be exactly 23 example files under " << MAREA_TESTS_EXAMPLES_DIRECTORY << ".";
}

TEST(MAREALexerExamples, EveryExampleFileLexesWithZeroDiagnostics)
{
    GTEST_COUT << "Validating every example file lexes with zero diagnostics." << std::endl;

    for (const std::filesystem::path &path : MAREA_CollectExampleFiles()) {
        MAREA_LexFixture fixture;

        ASSERT_EQ(fixture.MAREA_lex(MAREA_ReadFile(path)), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing " << path << " must succeed.";

        const MAREA_IntegerSize COUNT{ MAREA_DiagnosticSinkGetCount(fixture.sink) };

        if (COUNT > MAREA_INTEGER_SIZE(0)) {
            const MAREA_Diagnostic *diagnostic{ nullptr };

            MAREA_IGNORE(MAREA_DiagnosticSinkGetAt(fixture.sink, MAREA_INTEGER_SIZE(0), &diagnostic));

            const MAREA_SourcePosition POSITION{ MAREA_DiagnosticGetPosition(diagnostic) };

            ADD_FAILURE() << path << ":" << POSITION.line << ":" << POSITION.column << ": " << MAREA_DiagnosticGetMessage(diagnostic);
        }
    }
}

TEST(MAREALexerExamples, ExampleTokensStayInsideSource)
{
    GTEST_COUT << "Validating every example's tokens have strictly increasing offsets, lie inside the source and end with one END_OF_FILE." << std::endl;

    for (const std::filesystem::path &path : MAREA_CollectExampleFiles()) {
        const std::string SOURCE{ MAREA_ReadFile(path) };

        MAREA_LexFixture fixture;

        ASSERT_EQ(fixture.MAREA_lex(SOURCE), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing " << path << " must succeed.";

        const std::vector<MAREA_SourcePosition> POSITIONS{ fixture.MAREA_positions() };

        ASSERT_FALSE(POSITIONS.empty()) << path << " must lex to at least one token.";

        for (std::size_t index{ 1 }; index < POSITIONS.size(); ++index) {
            EXPECT_GT(POSITIONS[index].offset, POSITIONS[index - 1].offset) << path << ": token offsets must strictly increase at index " << index << ".";
        }

        for (const MAREA_SourcePosition &position : POSITIONS) {
            EXPECT_LE((static_cast<std::size_t>(position.offset) + position.length), SOURCE.size()) << path << ": every token must lie inside the source.";
        }

        EXPECT_EQ(fixture.MAREA_kinds().back(), MAREA_TOKEN_KIND_END_OF_FILE) << path << " must end with exactly one END_OF_FILE token.";
    }
}

TEST(MAREALexerExamples, ExamplesContainNoErrorTokens)
{
    GTEST_COUT << "Validating no example produces an ERROR token." << std::endl;

    for (const std::filesystem::path &path : MAREA_CollectExampleFiles()) {
        MAREA_LexFixture fixture;

        ASSERT_EQ(fixture.MAREA_lex(MAREA_ReadFile(path)), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing " << path << " must succeed.";

        for (const MAREA_TokenKind KIND : fixture.MAREA_kinds()) {
            EXPECT_NE(KIND, MAREA_TOKEN_KIND_ERROR) << path << " must not produce an ERROR token.";
        }
    }
}

TEST(MAREALexerExamples, ExamplesStartWithNamespace)
{
    GTEST_COUT << "Validating every example starts with the namespace declaration." << std::endl;

    for (const std::filesystem::path &path : MAREA_CollectExampleFiles()) {
        MAREA_LexFixture fixture;

        ASSERT_EQ(fixture.MAREA_lex(MAREA_ReadFile(path)), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing " << path << " must succeed.";

        const std::vector<MAREA_TokenKind> KINDS{ fixture.MAREA_kinds() };

        ASSERT_GE(KINDS.size(), 2U) << path << " must have at least the namespace keyword and an identifier.";

        EXPECT_EQ(KINDS[0], MAREA_TOKEN_KIND_KEYWORD_NAMESPACE) << path << " must start with the namespace keyword.";

        EXPECT_EQ(KINDS[1], MAREA_TOKEN_KIND_IDENTIFIER) << path << " must follow the namespace keyword with an identifier.";
    }
}
