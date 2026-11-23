// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREALexHelpers.hpp"

#include <algorithm>

TEST(MAREALexerBasics, EmptySourceGivesOnlyEndOfFile)
{
    GTEST_COUT << "Validating an empty source lexes to only an END_OF_FILE token." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex(""), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing an empty source must succeed.";

    const std::vector<MAREA_TokenKind> KINDS{ fixture.MAREA_kinds() };

    EXPECT_EQ(KINDS, std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_END_OF_FILE }) << "An empty source must produce only END_OF_FILE.";
}

TEST(MAREALexerBasics, WhitespaceOnlyGivesOnlyEndOfFile)
{
    GTEST_COUT << "Validating a source made only of whitespace lexes to only an END_OF_FILE token." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("  \t\r\n  \t"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing whitespace-only source must succeed.";

    const std::vector<MAREA_TokenKind> KINDS{ fixture.MAREA_kinds() };

    EXPECT_EQ(KINDS, std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_END_OF_FILE }) << "Whitespace-only source must produce only END_OF_FILE.";
}

TEST(MAREALexerBasics, CommentOnlyGivesOnlyEndOfFile)
{
    GTEST_COUT << "Validating a source made only of a comment lexes to only an END_OF_FILE token." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("-- a whole line comment"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing comment-only source must succeed.";

    const std::vector<MAREA_TokenKind> KINDS{ fixture.MAREA_kinds() };

    EXPECT_EQ(KINDS, std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_END_OF_FILE }) << "Comment-only source must produce only END_OF_FILE.";
}

TEST(MAREALexerBasics, CommentEndsAtNewline)
{
    GTEST_COUT << "Validating a comment ends at the line end and lexing resumes after it." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("-- comment\nabc"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    const std::vector<MAREA_TokenKind> KINDS{ fixture.MAREA_kinds() };

    EXPECT_EQ(KINDS, (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_IDENTIFIER, MAREA_TOKEN_KIND_END_OF_FILE })) << "Lexing must resume with the identifier after the comment's line.";
}

TEST(MAREALexerBasics, CommentAtEndWithoutNewline)
{
    GTEST_COUT << "Validating a comment with no trailing newline still ends cleanly at end of input." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("abc -- trailing comment"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    const std::vector<MAREA_TokenKind> KINDS{ fixture.MAREA_kinds() };

    EXPECT_EQ(KINDS, (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_IDENTIFIER, MAREA_TOKEN_KIND_END_OF_FILE })) << "A comment at end of input without a newline must still be skipped entirely.";
}

TEST(MAREALexerBasics, MinusMinusIsAlwaysComment)
{
    GTEST_COUT << "Validating two hyphens always start a comment, even adjacent to identifiers." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("a--b"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    const std::vector<MAREA_TokenKind> KINDS{ fixture.MAREA_kinds() };

    EXPECT_EQ(KINDS, (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_IDENTIFIER, MAREA_TOKEN_KIND_END_OF_FILE })) << "a--b must lex as the identifier a followed by a comment, never as a, MINUS, MINUS, b.";
}

TEST(MAREALexerBasics, IdentifierForms)
{
    GTEST_COUT << "Validating every accepted identifier form lexes as a single IDENTIFIER token." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("a _a a1 A_b_2 _ __x"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    const std::vector<std::string> TEXTS{ fixture.MAREA_texts() };

    const std::vector<std::string> EXPECTED{ "a", "_a", "a1", "A_b_2", "_", "__x", "" };

    ASSERT_EQ(TEXTS.size(), EXPECTED.size()) << "There must be exactly 6 identifiers plus END_OF_FILE.";

    for (std::size_t index{ 0 }; index < EXPECTED.size(); ++index) {
        EXPECT_EQ(TEXTS[index], EXPECTED[index]) << "Token " << index << " text must match.";
    }

    for (const MAREA_TokenKind KIND : fixture.MAREA_kinds()) {
        EXPECT_TRUE((KIND == MAREA_TOKEN_KIND_IDENTIFIER) || (KIND == MAREA_TOKEN_KIND_END_OF_FILE)) << "Every token must be an identifier or END_OF_FILE.";
    }
}

TEST(MAREALexerBasics, IdentifierStopsAtPunctuation)
{
    GTEST_COUT << "Validating an identifier stops scanning at the first punctuation byte." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("abc;"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    const std::vector<std::string> TEXTS{ fixture.MAREA_texts() };

    ASSERT_EQ(TEXTS.size(), 3U) << "There must be an identifier, a semicolon and END_OF_FILE.";

    EXPECT_EQ(TEXTS[0], "abc") << "The identifier must stop before the semicolon.";

    const std::vector<MAREA_TokenKind> KINDS{ fixture.MAREA_kinds() };

    EXPECT_EQ(KINDS[1], MAREA_TOKEN_KIND_SEMICOLON) << "The second token must be the semicolon.";
}

TEST(MAREALexerBasics, IdentifierNamesThatLookLikeKeywords)
{
    GTEST_COUT << "Validating predeclared and look-alike names lex as plain identifiers, never keywords." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("sinteger32 Self self public private c boolean string forkjoin"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    for (const MAREA_TokenKind KIND : fixture.MAREA_kinds()) {
        EXPECT_TRUE((KIND == MAREA_TOKEN_KIND_IDENTIFIER) || (KIND == MAREA_TOKEN_KIND_END_OF_FILE)) << "Every name here must lex as a plain identifier.";
    }
}

TEST(MAREALexerBasics, ExactlyOneEndOfFile)
{
    GTEST_COUT << "Validating lexing any source ends with exactly one END_OF_FILE token." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("a b c ; : ."), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    const std::vector<MAREA_TokenKind> KINDS{ fixture.MAREA_kinds() };

    ASSERT_FALSE(KINDS.empty()) << "The token list must not be empty.";

    EXPECT_EQ(KINDS.back(), MAREA_TOKEN_KIND_END_OF_FILE) << "The last token must be END_OF_FILE.";

    EXPECT_EQ(std::count(KINDS.begin(), KINDS.end(), MAREA_TOKEN_KIND_END_OF_FILE), 1) << "There must be exactly one END_OF_FILE token.";
}

TEST(MAREALexerBasics, RejectsNullArguments)
{
    GTEST_COUT << "Validating MAREA_LexTokenize rejects null context, a null source with a non-zero length and a null outList." << std::endl;

    MAREA_LexFixture fixture;

    MAREA_TokenList *list{ nullptr };

    EXPECT_EQ(MAREA_LexTokenize(nullptr, "a", MAREA_INTEGER_SIZE(1), &list), MAREA_LEX_EXCEPT_ERROR_NULL_POINTER) << "A null context must be rejected.";

    EXPECT_EQ(MAREA_LexTokenize(fixture.context, nullptr, MAREA_INTEGER_SIZE(1), &list), MAREA_LEX_EXCEPT_ERROR_NULL_POINTER) << "A null source with a non-zero length must be rejected.";

    EXPECT_EQ(MAREA_LexTokenize(fixture.context, "a", MAREA_INTEGER_SIZE(1), nullptr), MAREA_LEX_EXCEPT_ERROR_NULL_POINTER) << "A null outList must be rejected.";
}

TEST(MAREALexerBasics, SameInputSameOutput)
{
    GTEST_COUT << "Validating lexing the same source twice gives the same kinds and texts." << std::endl;

    MAREA_LexFixture first;

    ASSERT_EQ(first.MAREA_lex("function main() end"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    MAREA_LexFixture second;

    ASSERT_EQ(second.MAREA_lex("function main() end"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(first.MAREA_kinds(), second.MAREA_kinds()) << "The same source must produce the same kinds every time.";

    EXPECT_EQ(first.MAREA_texts(), second.MAREA_texts()) << "The same source must produce the same texts every time.";
}
