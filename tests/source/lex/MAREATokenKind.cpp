// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include <cstring>
#include <set>
#include <string>

namespace
{

    struct MAREA_KeywordRow
    {
        const char *text;
        MAREA_TokenKind kind;
    };

    const MAREA_KeywordRow MAREA_G_KEYWORD_TABLE[]{
        { "and", MAREA_TOKEN_KIND_KEYWORD_AND },
        { "as", MAREA_TOKEN_KIND_KEYWORD_AS },
        { "break", MAREA_TOKEN_KIND_KEYWORD_BREAK },
        { "case", MAREA_TOKEN_KIND_KEYWORD_CASE },
        { "constant", MAREA_TOKEN_KIND_KEYWORD_CONSTANT },
        { "continue", MAREA_TOKEN_KIND_KEYWORD_CONTINUE },
        { "contract", MAREA_TOKEN_KIND_KEYWORD_CONTRACT },
        { "copy", MAREA_TOKEN_KIND_KEYWORD_COPY },
        { "default", MAREA_TOKEN_KIND_KEYWORD_DEFAULT },
        { "do", MAREA_TOKEN_KIND_KEYWORD_DO },
        { "each", MAREA_TOKEN_KIND_KEYWORD_EACH },
        { "else", MAREA_TOKEN_KIND_KEYWORD_ELSE },
        { "elseif", MAREA_TOKEN_KIND_KEYWORD_ELSEIF },
        { "end", MAREA_TOKEN_KIND_KEYWORD_END },
        { "enum", MAREA_TOKEN_KIND_KEYWORD_ENUM },
        { "false", MAREA_TOKEN_KIND_KEYWORD_FALSE },
        { "for", MAREA_TOKEN_KIND_KEYWORD_FOR },
        { "fork_join", MAREA_TOKEN_KIND_KEYWORD_FORK_JOIN },
        { "function", MAREA_TOKEN_KIND_KEYWORD_FUNCTION },
        { "if", MAREA_TOKEN_KIND_KEYWORD_IF },
        { "implementation", MAREA_TOKEN_KIND_KEYWORD_IMPLEMENTATION },
        { "import", MAREA_TOKEN_KIND_KEYWORD_IMPORT },
        { "in", MAREA_TOKEN_KIND_KEYWORD_IN },
        { "into", MAREA_TOKEN_KIND_KEYWORD_INTO },
        { "local", MAREA_TOKEN_KIND_KEYWORD_LOCAL },
        { "move", MAREA_TOKEN_KIND_KEYWORD_MOVE },
        { "mutable", MAREA_TOKEN_KIND_KEYWORD_MUTABLE },
        { "namespace", MAREA_TOKEN_KIND_KEYWORD_NAMESPACE },
        { "not", MAREA_TOKEN_KIND_KEYWORD_NOT },
        { "opaque", MAREA_TOKEN_KIND_KEYWORD_OPAQUE },
        { "or", MAREA_TOKEN_KIND_KEYWORD_OR },
        { "partition", MAREA_TOKEN_KIND_KEYWORD_PARTITION },
        { "require", MAREA_TOKEN_KIND_KEYWORD_REQUIRE },
        { "return", MAREA_TOKEN_KIND_KEYWORD_RETURN },
        { "spawn", MAREA_TOKEN_KIND_KEYWORD_SPAWN },
        { "structure", MAREA_TOKEN_KIND_KEYWORD_STRUCTURE },
        { "switch", MAREA_TOKEN_KIND_KEYWORD_SWITCH },
        { "then", MAREA_TOKEN_KIND_KEYWORD_THEN },
        { "true", MAREA_TOKEN_KIND_KEYWORD_TRUE },
        { "type", MAREA_TOKEN_KIND_KEYWORD_TYPE },
        { "unsafe", MAREA_TOKEN_KIND_KEYWORD_UNSAFE },
        { "where", MAREA_TOKEN_KIND_KEYWORD_WHERE },
        { "while", MAREA_TOKEN_KIND_KEYWORD_WHILE },
        { "with", MAREA_TOKEN_KIND_KEYWORD_WITH },
    };

} // namespace

TEST(MAREATokenKind, NamesAreUniqueAndNonEmpty)
{
    GTEST_COUT << "Validating every token kind name is non-null, non-empty, unique and only A-Z0-9_." << std::endl;

    ASSERT_EQ(static_cast<int>(MAREA_TOKEN_KIND_COUNT), 101) << "MAREA_TOKEN_KIND_COUNT must equal 101.";

    std::set<std::string> seenNames;

    for (int kind{ 0 }; kind < static_cast<int>(MAREA_TOKEN_KIND_COUNT); ++kind) {
        const MAREA_Character8 *name{ MAREA_TokenKindGetName(static_cast<MAREA_TokenKind>(kind)) };

        ASSERT_NE(name, nullptr) << "Kind " << kind << " must have a non-null name.";

        ASSERT_GT(std::strlen(name), 0U) << "Kind " << kind << " must have a non-empty name.";

        for (const char CHARACTER : std::string(name)) {
            EXPECT_TRUE(((CHARACTER >= 'A') && (CHARACTER <= 'Z')) || ((CHARACTER >= '0') && (CHARACTER <= '9')) || (CHARACTER == '_')) << "Kind " << kind << " name '" << name << "' must only use A-Z0-9_.";
        }

        EXPECT_TRUE(seenNames.insert(name).second) << "Kind " << kind << " name '" << name << "' must be unique.";
    }
}

TEST(MAREATokenKind, NameOfOutOfRangeKindIsUnknown)
{
    GTEST_COUT << "Validating MAREA_TokenKindGetName returns UNKNOWN for an out of range kind." << std::endl;

    EXPECT_STREQ(MAREA_TokenKindGetName(static_cast<MAREA_TokenKind>(-1)), "UNKNOWN") << "Kind -1 must report UNKNOWN.";

    EXPECT_STREQ(MAREA_TokenKindGetName(MAREA_TOKEN_KIND_COUNT), "UNKNOWN") << "MAREA_TOKEN_KIND_COUNT must report UNKNOWN.";
}

TEST(MAREATokenKind, KeywordLookupFindsAll44)
{
    GTEST_COUT << "Validating MAREA_TokenKindLookupKeyword finds every one of the 44 keywords." << std::endl;

    ASSERT_EQ(sizeof(MAREA_G_KEYWORD_TABLE) / sizeof(MAREA_G_KEYWORD_TABLE[0]), 44U) << "The keyword table must have 44 rows.";

    for (const MAREA_KeywordRow &row : MAREA_G_KEYWORD_TABLE) {
        const MAREA_TokenKind KIND{ MAREA_TokenKindLookupKeyword(row.text, static_cast<MAREA_IntegerSize>(std::strlen(row.text))) };

        EXPECT_EQ(KIND, row.kind) << "Keyword '" << row.text << "' must map to its own kind.";
    }
}

TEST(MAREATokenKind, KeywordLookupRejectsNonKeywords)
{
    GTEST_COUT << "Validating MAREA_TokenKindLookupKeyword treats every non-keyword as an identifier." << std::endl;

    const char *nonKeywords[]{ "Self", "self", "public", "private", "c", "sinteger32", "boolean", "string", "vector", "pointer", "an", "ands", "", "fork", "reference" };

    for (const char *text : nonKeywords) {
        const MAREA_TokenKind KIND{ MAREA_TokenKindLookupKeyword(text, static_cast<MAREA_IntegerSize>(std::strlen(text))) };

        EXPECT_EQ(KIND, MAREA_TOKEN_KIND_IDENTIFIER) << "'" << text << "' must not be recognized as a keyword.";
    }
}

TEST(MAREATokenKind, KeywordLookupIsCaseSensitive)
{
    GTEST_COUT << "Validating MAREA_TokenKindLookupKeyword is case sensitive." << std::endl;

    const char *wrongCase[]{ "End", "FUNCTION", "True" };

    for (const char *text : wrongCase) {
        const MAREA_TokenKind KIND{ MAREA_TokenKindLookupKeyword(text, static_cast<MAREA_IntegerSize>(std::strlen(text))) };

        EXPECT_EQ(KIND, MAREA_TOKEN_KIND_IDENTIFIER) << "'" << text << "' must not match any keyword case-insensitively.";
    }
}
