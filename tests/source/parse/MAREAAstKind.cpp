// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include <set>
#include <string>

TEST(MAREAAstKind, EveryKindHasAUniqueName)
{
    GTEST_COUT << "Validating every AST kind has a non-empty unique name that is not UNKNOWN." << std::endl;

    std::set<std::string> names;

    for (MAREA_Sinteger32 value{ 0 }; value < static_cast<MAREA_Sinteger32>(MAREA_AST_KIND_COUNT); ++value) {
        const MAREA_Character8 *name{ MAREA_AstKindGetName(static_cast<MAREA_AstKind>(value)) };

        ASSERT_NE(name, nullptr) << "Kind " << value << " must have a name.";

        EXPECT_STRNE(name, "") << "Kind " << value << " must have a non-empty name.";

        EXPECT_STRNE(name, "UNKNOWN") << "Kind " << value << " must not be named UNKNOWN.";

        EXPECT_TRUE(names.insert(name).second) << "Kind " << value << " repeats the name " << name << ".";
    }

    EXPECT_EQ(names.size(), static_cast<size_t>(MAREA_AST_KIND_COUNT)) << "Every kind must add one distinct name.";
}

TEST(MAREAAstKind, SpotCheckNames)
{
    GTEST_COUT << "Validating the names of a few kinds." << std::endl;

    EXPECT_STREQ(MAREA_AstKindGetName(MAREA_AST_KIND_COMPILATION_UNIT), "COMPILATION_UNIT") << "The first kind must be COMPILATION_UNIT.";

    EXPECT_STREQ(MAREA_AstKindGetName(MAREA_AST_KIND_BINARY_EXPRESSION), "BINARY_EXPRESSION") << "BINARY_EXPRESSION must keep its name.";

    EXPECT_STREQ(MAREA_AstKindGetName(MAREA_AST_KIND_ERROR_NODE), "ERROR_NODE") << "ERROR_NODE must keep its name.";

    EXPECT_STREQ(MAREA_AstKindGetName(MAREA_AST_KIND_IMPORT_MEMBER), "IMPORT_MEMBER") << "IMPORT_MEMBER must be named.";

    EXPECT_STREQ(MAREA_AstKindGetName(MAREA_AST_KIND_CHARACTER_LITERAL), "CHARACTER_LITERAL") << "CHARACTER_LITERAL must be named.";
}

TEST(MAREAAstKind, NewKindsFollowImportMemberAndEndTheList)
{
    GTEST_COUT << "Validating IMPORT_MEMBER keeps its place after ERROR_NODE and the new kinds follow it in order, right before COUNT." << std::endl;

    EXPECT_EQ(static_cast<MAREA_Sinteger32>(MAREA_AST_KIND_IMPORT_MEMBER), static_cast<MAREA_Sinteger32>(MAREA_AST_KIND_ERROR_NODE) + 1) << "IMPORT_MEMBER must follow ERROR_NODE.";

    const MAREA_AstKind NEW_KINDS[]{ MAREA_AST_KIND_REQUIRE_DECLARATION, MAREA_AST_KIND_DO_WHILE_STATEMENT, MAREA_AST_KIND_WHEN_ITEM, MAREA_AST_KIND_WHEN_COMPARISON, MAREA_AST_KIND_WHEN_COMBINATOR };

    MAREA_Sinteger32 expected{ static_cast<MAREA_Sinteger32>(MAREA_AST_KIND_IMPORT_MEMBER) + 1 };

    for (const MAREA_AstKind KIND : NEW_KINDS) {
        EXPECT_EQ(static_cast<MAREA_Sinteger32>(KIND), expected) << "Each new kind must follow the one before it.";

        expected += 1;
    }

    EXPECT_EQ(expected, static_cast<MAREA_Sinteger32>(MAREA_AST_KIND_COUNT)) << "The last new kind must come right before COUNT.";

    EXPECT_STREQ(MAREA_AstKindGetName(MAREA_AST_KIND_REQUIRE_DECLARATION), "REQUIRE_DECLARATION") << "REQUIRE_DECLARATION must be named.";

    EXPECT_STREQ(MAREA_AstKindGetName(MAREA_AST_KIND_DO_WHILE_STATEMENT), "DO_WHILE_STATEMENT") << "DO_WHILE_STATEMENT must be named.";

    EXPECT_STREQ(MAREA_AstKindGetName(MAREA_AST_KIND_WHEN_ITEM), "WHEN_ITEM") << "WHEN_ITEM must be named.";

    EXPECT_STREQ(MAREA_AstKindGetName(MAREA_AST_KIND_WHEN_COMPARISON), "WHEN_COMPARISON") << "WHEN_COMPARISON must be named.";

    EXPECT_STREQ(MAREA_AstKindGetName(MAREA_AST_KIND_WHEN_COMBINATOR), "WHEN_COMBINATOR") << "WHEN_COMBINATOR must be named.";
}

TEST(MAREAAstKind, WildcardFlagIsDistinct)
{
    GTEST_COUT << "Validating the wildcard flag is 64 and shares no bit with another flag." << std::endl;

    EXPECT_EQ(static_cast<MAREA_Uinteger32>(MAREA_AST_FLAG_WILDCARD), 64U) << "The wildcard flag must be the next free bit.";

    const MAREA_Uinteger32 OTHERS{ static_cast<MAREA_Uinteger32>(MAREA_AST_FLAG_MUTABLE) | static_cast<MAREA_Uinteger32>(MAREA_AST_FLAG_CONSTANT) | static_cast<MAREA_Uinteger32>(MAREA_AST_FLAG_HAS_ALIAS) | static_cast<MAREA_Uinteger32>(MAREA_AST_FLAG_FILE) | static_cast<MAREA_Uinteger32>(MAREA_AST_FLAG_HAS_MEMBERS) };

    EXPECT_EQ(OTHERS & static_cast<MAREA_Uinteger32>(MAREA_AST_FLAG_WILDCARD), 0U) << "The wildcard bit must be free.";
}

TEST(MAREAAstKind, OutOfRangeIsUnknown)
{
    GTEST_COUT << "Validating an out of range kind is named UNKNOWN." << std::endl;

    EXPECT_STREQ(MAREA_AstKindGetName(MAREA_AST_KIND_COUNT), "UNKNOWN") << "The count itself is not a kind.";

    EXPECT_STREQ(MAREA_AstKindGetName(static_cast<MAREA_AstKind>(-1)), "UNKNOWN") << "A negative value is not a kind.";
}
