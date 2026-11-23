// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

    constexpr MAREA_Uinteger32 OUT_OF_RANGE{ MAREA_SEMA_DIAGNOSTIC_CODE_LITERAL_OUT_OF_RANGE };
    constexpr MAREA_Uinteger32 MISMATCH{ MAREA_SEMA_DIAGNOSTIC_CODE_TYPE_MISMATCH };

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

TEST(MAREASemaLiteralRange, CodeIs3033)
{
    GTEST_COUT << "Validating the diagnostic code of an integer literal that is out of range." << std::endl;

    EXPECT_EQ(OUT_OF_RANGE, 3033U) << "LITERAL_OUT_OF_RANGE must be 3033.";
}

TEST(MAREASemaLiteralRange, LocalInitializerOutOfRange)
{
    GTEST_COUT << "Validating a local initializer that does not fit its type." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f() local a: uinteger8 = 256; end"), Codes{ OUT_OF_RANGE }) << "256 does not fit uinteger8.";

    EXPECT_EQ(MAREA_codesOf("function f() local a: sinteger8 = 128; end"), Codes{ OUT_OF_RANGE }) << "128 does not fit sinteger8.";

    EXPECT_EQ(MAREA_codesOf("function f() local a: sinteger8 = -129; end"), Codes{ OUT_OF_RANGE }) << "-129 does not fit sinteger8.";

    EXPECT_EQ(MAREA_codesOf("function f() local a: sinteger32 = 2147483648; end"), Codes{ OUT_OF_RANGE }) << "2147483648 does not fit sinteger32.";

    EXPECT_EQ(MAREA_codesOf("function f() local a: uinteger32 = -1; end"), Codes{ OUT_OF_RANGE }) << "A negated literal does not fit an unsigned type.";

    EXPECT_EQ(MAREA_codesOf("function f() local a: uinteger64 = 99999999999999999999; end"), Codes{ OUT_OF_RANGE }) << "A literal too big for any type must raise.";

    EXPECT_EQ(MAREA_codesOf("function f() local a: uinteger8 = 0x100; end"), Codes{ OUT_OF_RANGE }) << "A hexadecimal literal is read by its value.";

    EXPECT_EQ(MAREA_codesOf("function f() local a: uinteger8 = 0b1_0000_0000; end"), Codes{ OUT_OF_RANGE }) << "A binary literal with underscores is read by its value.";

    EXPECT_EQ(MAREA_codesOf("function f() local a: sinteger8 = (-129); end"), Codes{ OUT_OF_RANGE }) << "A parenthesized literal is read by its value.";
}

TEST(MAREASemaLiteralRange, LiteralThatFitsIsSilent)
{
    GTEST_COUT << "Validating a literal that fits its type raises nothing." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f() local a: sinteger8 = -128; end"), Codes{}) << "-128 fits sinteger8.";

    EXPECT_EQ(MAREA_codesOf("function f() local a: sinteger8 = 127; end"), Codes{}) << "127 fits sinteger8.";

    EXPECT_EQ(MAREA_codesOf("function f() local a: uinteger8 = 255; end"), Codes{}) << "255 fits uinteger8.";

    EXPECT_EQ(MAREA_codesOf("function f() local a: uinteger8 = 0xFF; end"), Codes{}) << "0xFF fits uinteger8.";

    EXPECT_EQ(MAREA_codesOf("function f() local a: sinteger32 = 1_000; end"), Codes{}) << "1_000 fits sinteger32.";

    EXPECT_EQ(MAREA_codesOf("function f() local a: uinteger64 = 18446744073709551615; end"), Codes{}) << "The largest uinteger64 fits.";

    EXPECT_EQ(MAREA_codesOf("function f() local a: sinteger64 = -9223372036854775808; end"), Codes{}) << "The lowest sinteger64 fits.";

    EXPECT_EQ(MAREA_codesOf("function f() local a: uinteger8 = -0; end"), Codes{}) << "A negated zero fits an unsigned type.";

    EXPECT_EQ(MAREA_codesOf("function f() local a: float64 = 99999999999999999999; end"), Codes{}) << "A float target is never range checked.";

    EXPECT_EQ(MAREA_codesOf("function f() local a: float32 = 1.5; end"), Codes{}) << "A float literal is out of scope.";
}

TEST(MAREASemaLiteralRange, OtherSitesOutOfRange)
{
    GTEST_COUT << "Validating every site where a literal meets a type." << std::endl;

    EXPECT_EQ(MAREA_codesOf("constant BIG: uinteger8 = 300;"), Codes{ OUT_OF_RANGE }) << "A constant must raise.";

    EXPECT_EQ(MAREA_codesOf("function small(value: uinteger8 = 256): uinteger8 return value; end"), Codes{ OUT_OF_RANGE }) << "A parameter default must raise.";

    EXPECT_EQ(MAREA_codesOf("function f(): uinteger8 return 256; end"), Codes{ OUT_OF_RANGE }) << "A return value must raise.";

    EXPECT_EQ(MAREA_codesOf("function f() local a: uinteger8 = 1; a = 256; end"), Codes{ OUT_OF_RANGE }) << "An assignment must raise.";

    EXPECT_EQ(MAREA_codesOf("function small(value: uinteger8) end function f() small(300); end"), Codes{ OUT_OF_RANGE }) << "A call argument must raise.";

    EXPECT_EQ(MAREA_codesOf("function small(value: uinteger8) end function f() fork_join spawn small(300); end end"), Codes{ OUT_OF_RANGE }) << "A spawn argument must raise.";

    EXPECT_EQ(MAREA_codesOf("structure Pixel alpha: uinteger8; end function f(): Pixel return Pixel{ alpha = 256 }; end"), Codes{ OUT_OF_RANGE }) << "A field of a struct literal must raise.";
}

TEST(MAREASemaLiteralRange, OtherSitesThatFitAreSilent)
{
    GTEST_COUT << "Validating a struct literal field and a call argument that fit." << std::endl;

    EXPECT_EQ(MAREA_codesOf("structure Pixel alpha: uinteger8; end function f(): Pixel return Pixel{ alpha = 255 }; end"), Codes{}) << "A field that fits is silent.";

    EXPECT_EQ(MAREA_codesOf("function small(value: uinteger8) end function f() small(255); end"), Codes{}) << "An argument that fits is silent.";
}

TEST(MAREASemaLiteralRange, TypeMismatchIsNotReportedTwice)
{
    GTEST_COUT << "Validating that a node with TYPE_MISMATCH gets no second diagnostic." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f() local a: string = 300; end"), Codes{ MISMATCH }) << "A number for a string raises TYPE_MISMATCH only.";
}
