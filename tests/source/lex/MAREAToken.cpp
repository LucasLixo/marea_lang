#include "tests/dependencies.hpp"

#include "tests/MAREALexHelpers.hpp"

#include <type_traits>

namespace
{

    template <typename T, typename = void>
    struct MAREA_IsComplete : std::false_type
    {
    };

    template <typename T>
    struct MAREA_IsComplete<T, std::void_t<decltype(sizeof(T))>> : std::true_type
    {
    };

} // namespace

TEST(MAREAToken, TypesAreOpaque)
{
    GTEST_COUT << "Validating MAREA_Token, MAREA_TokenList and MAREA_LexContext stay opaque to callers." << std::endl;

    EXPECT_FALSE(MAREA_IsComplete<MAREA_Token>::value) << "MAREA_Token must not be a complete type outside its implementation.";

    EXPECT_FALSE(MAREA_IsComplete<MAREA_TokenList>::value) << "MAREA_TokenList must not be a complete type outside its implementation.";

    EXPECT_FALSE(MAREA_IsComplete<MAREA_LexContext>::value) << "MAREA_LexContext must not be a complete type outside its implementation.";
}

TEST(MAREAToken, AccessorsOnNullReturnDefaults)
{
    GTEST_COUT << "Validating every token accessor returns a safe default for a null token." << std::endl;

    EXPECT_EQ(MAREA_TokenGetKind(nullptr), MAREA_TOKEN_KIND_ERROR) << "MAREA_TokenGetKind must return ERROR for null.";

    EXPECT_EQ(MAREA_TokenGetText(nullptr), nullptr) << "MAREA_TokenGetText must return null for null.";

    EXPECT_EQ(MAREA_TokenGetLength(nullptr), MAREA_UINTEGER_32(0)) << "MAREA_TokenGetLength must return 0 for null.";

    EXPECT_EQ(MAREA_TokenGetPosition(nullptr).offset, MAREA_UINTEGER_32(0)) << "MAREA_TokenGetPosition must return a zeroed position for null.";

    EXPECT_EQ(MAREA_TokenListGetCount(nullptr), MAREA_INTEGER_SIZE(0)) << "MAREA_TokenListGetCount must return 0 for null.";
}

TEST(MAREAToken, ListGetAtRejectsOutOfRange)
{
    GTEST_COUT << "Validating MAREA_TokenListGetAt rejects an out of range index." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("a"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    const MAREA_IntegerSize COUNT{ MAREA_TokenListGetCount(fixture.list) };

    const MAREA_Token *token{ nullptr };

    EXPECT_EQ(MAREA_TokenListGetAt(fixture.list, COUNT, &token), MAREA_LEX_EXCEPT_ERROR_OUT_OF_RANGE) << "An index equal to the count must be rejected.";
}

TEST(MAREAToken, ListGetAtRejectsNull)
{
    GTEST_COUT << "Validating MAREA_TokenListGetAt rejects a null list or output." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("a"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    const MAREA_Token *token{ nullptr };

    EXPECT_EQ(MAREA_TokenListGetAt(nullptr, MAREA_INTEGER_SIZE(0), &token), MAREA_LEX_EXCEPT_ERROR_NULL_POINTER) << "A null list must be rejected.";

    EXPECT_EQ(MAREA_TokenListGetAt(fixture.list, MAREA_INTEGER_SIZE(0), nullptr), MAREA_LEX_EXCEPT_ERROR_NULL_POINTER) << "A null out pointer must be rejected.";
}

TEST(MAREAToken, TokenAccessorsMatchSource)
{
    GTEST_COUT << "Validating a token's accessors match the lexed source." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("abc"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    const MAREA_Token *token{ nullptr };

    ASSERT_EQ(MAREA_TokenListGetAt(fixture.list, MAREA_INTEGER_SIZE(0), &token), MAREA_LEX_EXCEPT_SUCCESS) << "MAREA_TokenListGetAt must succeed for index 0.";

    EXPECT_EQ(MAREA_TokenGetKind(token), MAREA_TOKEN_KIND_IDENTIFIER) << "The first token must be the identifier abc.";

    EXPECT_EQ(MAREA_TokenGetLength(token), MAREA_UINTEGER_32(3)) << "The token's length must be 3.";

    EXPECT_EQ(std::string(MAREA_TokenGetText(token), MAREA_TokenGetLength(token)), "abc") << "The token's text must match the source.";
}

TEST(MAREAToken, ListEndsWithEndOfFile)
{
    GTEST_COUT << "Validating the token list always ends with exactly one END_OF_FILE token." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("abc"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    const std::vector<MAREA_TokenKind> KINDS{ fixture.MAREA_kinds() };

    ASSERT_FALSE(KINDS.empty()) << "The token list must not be empty.";

    EXPECT_EQ(KINDS.back(), MAREA_TOKEN_KIND_END_OF_FILE) << "The last token must be END_OF_FILE.";

    int endOfFileCount{ 0 };

    for (const MAREA_TokenKind KIND : KINDS) {
        if (KIND == MAREA_TOKEN_KIND_END_OF_FILE) {
            ++endOfFileCount;
        }
    }

    EXPECT_EQ(endOfFileCount, 1) << "There must be exactly one END_OF_FILE token.";
}
