// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

    constexpr MAREA_Uinteger32 ALREADY_DECLARED{ MAREA_SEMA_DIAGNOSTIC_CODE_NAME_ALREADY_DECLARED };
    constexpr MAREA_Uinteger32 DUPLICATE_MEMBER{ MAREA_SEMA_DIAGNOSTIC_CODE_DUPLICATE_MEMBER };

    Codes MAREA_codesOf(const std::string &body)
    {
        MAREA_SemaFixture fixture([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; });

        if (fixture.MAREA_sema("namespace main; " + body) != MAREA_SEMA_EXCEPT_SUCCESS) {
            return { 0 };
        }

        for (MAREA_Uinteger32 code : fixture.parser.MAREA_codes()) {
            EXPECT_GE(code, 3000U) << "The program must lex and parse without a diagnostic: " << body;
        }

        return fixture.MAREA_semaCodes();
    }
} // namespace

TEST(MAREASemaDuplicateNames, MemberOfADeclarationMustBeUnique)
{
    GTEST_COUT << "Validating a repeated field, variant or contract function raises DUPLICATE_MEMBER." << std::endl;

    EXPECT_EQ(MAREA_codesOf("structure Item value: sinteger32; value: sinteger32; end"), Codes{ DUPLICATE_MEMBER }) << "A repeated field must raise DUPLICATE_MEMBER once.";

    EXPECT_EQ(MAREA_codesOf("enum Color Red; Red; end"), Codes{ DUPLICATE_MEMBER }) << "A repeated variant must raise DUPLICATE_MEMBER once.";

    EXPECT_EQ(MAREA_codesOf("contract Twice function run(self: Self); function run(self: Self); end"), Codes{ DUPLICATE_MEMBER }) << "A repeated contract function must raise DUPLICATE_MEMBER once.";

    EXPECT_EQ(MAREA_codesOf("structure Item value: sinteger32; other: sinteger32; end"), Codes{}) << "Distinct fields must raise nothing.";

    EXPECT_EQ(MAREA_codesOf("enum Color Red; Green; end"), Codes{}) << "Distinct variants must raise nothing.";

    EXPECT_EQ(MAREA_codesOf("contract Twice function run(self: Self); function stop(self: Self); end"), Codes{}) << "Distinct contract functions must raise nothing.";
}

TEST(MAREASemaDuplicateNames, GenericParameterMustBeUnique)
{
    GTEST_COUT << "Validating a repeated generic parameter, or one named as a declaration, raises NAME_ALREADY_DECLARED." << std::endl;

    EXPECT_EQ(MAREA_codesOf("structure Pair<Item, Item> first: Item; end"), Codes{ ALREADY_DECLARED }) << "A repeated structure generic parameter must raise NAME_ALREADY_DECLARED.";

    EXPECT_EQ(MAREA_codesOf("function pick<Item, Item>(left: Item) return; end"), Codes{ ALREADY_DECLARED }) << "A repeated function generic parameter must raise NAME_ALREADY_DECLARED.";

    EXPECT_EQ(MAREA_codesOf("structure Item value: sinteger32; end function same<Item>(value: Item): Item where Item: Copyable return copy value; end"), Codes{ ALREADY_DECLARED }) << "A generic parameter named as a declaration must raise NAME_ALREADY_DECLARED.";

    EXPECT_EQ(MAREA_codesOf("structure Pair<First, Second> first: First; end"), Codes{}) << "Distinct generic parameters must raise nothing.";
}

TEST(MAREASemaDuplicateNames, ParameterAndLocalMustBeUniqueInOneScope)
{
    GTEST_COUT << "Validating a repeated parameter or local in one scope raises NAME_ALREADY_DECLARED." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function run(value: sinteger32, value: sinteger32): sinteger32 return value; end"), Codes{ ALREADY_DECLARED }) << "A repeated parameter must raise NAME_ALREADY_DECLARED.";

    EXPECT_EQ(MAREA_codesOf("function run(value: sinteger32): sinteger32 local value: sinteger32 = 1; return value; end"), Codes{ ALREADY_DECLARED }) << "A body local that repeats a parameter must raise NAME_ALREADY_DECLARED.";

    EXPECT_EQ(MAREA_codesOf("function check(): sinteger32 local value: sinteger32 = 1; local value: sinteger32 = 2; return value; end"), Codes{ ALREADY_DECLARED }) << "A local declared twice in one block must raise NAME_ALREADY_DECLARED.";

    EXPECT_EQ(MAREA_codesOf("function run(first: sinteger32, second: sinteger32): sinteger32 local third: sinteger32 = 1; return third; end"), Codes{}) << "Distinct names must raise nothing.";

    EXPECT_EQ(MAREA_codesOf("function check(): sinteger32 local value: sinteger32 = 1; do local value: sinteger32 = 2; end return value; end"), Codes{}) << "A local of an inner block that hides an outer one must raise nothing.";

    EXPECT_EQ(MAREA_codesOf("function check(): sinteger32 do local value: sinteger32 = 1; end do local value: sinteger32 = 2; end return 0; end"), Codes{}) << "Two sibling blocks that each declare one name must raise nothing.";
}

TEST(MAREASemaDuplicateNames, LoopNameMustNotRepeatALocalOfItsBlock)
{
    GTEST_COUT << "Validating a for, for each, partition or with name that repeats a local of the same block raises NAME_ALREADY_DECLARED, in both directions." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f() local i: sinteger32 = 0; for i in 0 .. 10 do end end"), Codes{ ALREADY_DECLARED }) << "A for name that repeats a local must raise once.";

    EXPECT_EQ(MAREA_codesOf("function f() for i in 0 .. 10 do local i: sinteger32 = 0; end end"), Codes{ ALREADY_DECLARED }) << "A body local that repeats the for name must raise once.";

    EXPECT_EQ(MAREA_codesOf("require vulkan; function f(items: vulkan.List) local v: sinteger32 = 0; for each v in items do end end"), Codes{ ALREADY_DECLARED }) << "A for each name that repeats a local must raise once.";

    EXPECT_EQ(MAREA_codesOf("require vulkan; function f(items: vulkan.List) for each v in items do local v: sinteger32 = 0; end end"), Codes{ ALREADY_DECLARED }) << "A body local that repeats the for each name must raise once.";

    EXPECT_EQ(MAREA_codesOf("require vulkan; function f(items: vulkan.List) for each v, v in items do end end"), Codes{ ALREADY_DECLARED }) << "Two loop names of one for each must be different.";

    EXPECT_EQ(MAREA_codesOf("require vulkan; function f(items: vulkan.List) local view: sinteger32 = 0; with items as view do end end"), Codes{ ALREADY_DECLARED }) << "A with name that repeats a local must raise once.";

    EXPECT_EQ(MAREA_codesOf("require vulkan; function f(items: vulkan.List) with items as view do local view: sinteger32 = 0; end end"), Codes{ ALREADY_DECLARED }) << "A body local that repeats the with name must raise once.";

    EXPECT_EQ(MAREA_codesOf("require vulkan; function f(items: vulkan.List) fork_join local part: sinteger32 = 0; partition items into 2 as part do end end end"), Codes{ ALREADY_DECLARED }) << "A partition name that repeats a local must raise once.";

    EXPECT_EQ(MAREA_codesOf("require vulkan; function f(items: vulkan.List) fork_join partition items into 2 as part do local part: sinteger32 = 0; end end end"), Codes{ ALREADY_DECLARED }) << "A body local that repeats the partition name must raise once.";

    EXPECT_EQ(MAREA_codesOf("function f() local i: sinteger32 = 4; do for i in 0 .. i do end end end"), Codes{}) << "A loop name may hide a local of an outer block, and the header reads the outer one.";

    EXPECT_EQ(MAREA_codesOf("function f() for i in 0 .. 4 do for i in 0 .. i do end end end"), Codes{}) << "A nested statement may hide the name of the statement around it.";

    EXPECT_EQ(MAREA_codesOf("function f() for i in 0 .. 4 do end for i in 0 .. 4 do end local i: sinteger32 = 0; end"), Codes{}) << "The name of a loop ends with the loop.";
}

TEST(MAREASemaDuplicateNames, LoopNameIsNotInScopeInItsOwnHeader)
{
    GTEST_COUT << "Validating the header of a for, for each, partition or with statement cannot read the name that the statement declares." << std::endl;

    constexpr MAREA_Uinteger32 UNDECLARED{ MAREA_SEMA_DIAGNOSTIC_CODE_UNDECLARED_NAME };

    EXPECT_EQ(MAREA_codesOf("function f() for i in 0 .. i do end end"), Codes{ UNDECLARED }) << "A for bound must not read its own name.";

    EXPECT_EQ(MAREA_codesOf("function f() for each v in v do end end"), Codes{ UNDECLARED }) << "A for each collection must not read its own name.";

    EXPECT_EQ(MAREA_codesOf("function f() with view as view do end end"), Codes{ UNDECLARED }) << "A with source must not read its own name.";

    EXPECT_EQ(MAREA_codesOf("function f() fork_join partition part into 2 as part do end end end"), Codes{ UNDECLARED }) << "A partition source must not read its own name.";
}
