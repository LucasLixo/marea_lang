// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

    constexpr MAREA_Uinteger32 RAW_POINTER{ MAREA_SEMA_DIAGNOSTIC_CODE_RAW_POINTER_OUTSIDE_UNSAFE };

    Codes MAREA_codesOf(const std::string &body)
    {
        MAREA_SemaFixture fixture([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; });

        if (fixture.MAREA_sema("namespace main; import c.lib as lib; " + body) != MAREA_SEMA_EXCEPT_SUCCESS) {
            return { 0 };
        }

        for (MAREA_Uinteger32 code : fixture.parser.MAREA_codes()) {
            EXPECT_GE(code, 3000U) << "The program must lex and parse without a diagnostic: " << body;
        }

        return fixture.MAREA_semaCodes();
    }

    std::string MAREA_function(const std::string &statement)
    {
        return "function g(flag: boolean) " + statement + " end";
    }
} // namespace

TEST(MAREASemaRawPointer, PointerLocalOutsideUnsafeIsRejected)
{
    GTEST_COUT << "Validating a pointer local outside unsafe raises RAW_POINTER_OUTSIDE_UNSAFE." << std::endl;

    EXPECT_EQ(MAREA_codesOf(MAREA_function("local raw: pointer<sinteger32> = lib.f();")), Codes{ RAW_POINTER }) << "A pointer local outside unsafe must raise 3041.";
}

TEST(MAREASemaRawPointer, PointerLocalInsideUnsafeIsSilent)
{
    GTEST_COUT << "Validating a pointer local inside unsafe, also in a nested block, raises nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf(MAREA_function("unsafe local raw: pointer<sinteger32> = lib.f(); end")).empty()) << "A pointer local directly in unsafe must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(MAREA_function("unsafe while flag do local raw: pointer<sinteger32> = lib.f(); end end")).empty()) << "A pointer local in a block nested in unsafe must raise nothing.";
}

TEST(MAREASemaRawPointer, PointerLocalAfterUnsafeIsRejected)
{
    GTEST_COUT << "Validating a pointer local after an unsafe block has ended raises 3041." << std::endl;

    EXPECT_EQ(MAREA_codesOf(MAREA_function("unsafe local a: sinteger32 = 1; end local raw: pointer<sinteger32> = lib.f();")), Codes{ RAW_POINTER }) << "A pointer local after unsafe must raise 3041.";
}

TEST(MAREASemaRawPointer, UnsafeInNestedBlockDoesNotLeak)
{
    GTEST_COUT << "Validating an unsafe block inside a nested block does not cover a later local." << std::endl;

    EXPECT_EQ(MAREA_codesOf(MAREA_function("while flag do unsafe local a: sinteger32 = 1; end end local raw: pointer<sinteger32> = lib.f();")), Codes{ RAW_POINTER }) << "A pointer local after a nested unsafe must raise 3041.";
}

TEST(MAREASemaRawPointer, OtherLocalTypesAreSilent)
{
    GTEST_COUT << "Validating a local of another type outside unsafe raises nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf(MAREA_function("local a: sinteger32 = 1; local b: float64 = 1.5;")).empty()) << "A primitive local must raise nothing.";
}
