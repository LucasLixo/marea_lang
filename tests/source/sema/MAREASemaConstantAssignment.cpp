// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

#include <algorithm>

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

    constexpr MAREA_Uinteger32 TO_CONSTANT{ MAREA_SEMA_DIAGNOSTIC_CODE_ASSIGNMENT_TO_CONSTANT };

    const std::string PRELUDE{ "namespace main; constant LIMIT: sinteger32 = 10; structure Point x: sinteger32; y: sinteger32; end constant ORIGIN: Point = Point{ x = 0, y = 0 }; " };

    Codes MAREA_codesOfProgram(const std::string &program)
    {
        MAREA_SemaFixture fixture([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; });

        if (fixture.MAREA_sema(program) != MAREA_SEMA_EXCEPT_SUCCESS) {
            return { 0 };
        }

        for (MAREA_Uinteger32 code : fixture.parser.MAREA_codes()) {
            EXPECT_GE(code, 3000U) << "The program must lex and parse without a diagnostic: " << program;
        }

        return fixture.MAREA_semaCodes();
    }

    Codes MAREA_codesOfFunction(const std::string &parameters, const std::string &statements)
    {
        return MAREA_codesOfProgram(PRELUDE + "function f(" + parameters + ") " + statements + " end");
    }
} // namespace

TEST(MAREASemaConstantAssignment, PlainAssignmentToConstantIsReported)
{
    GTEST_COUT << "Validating that = on a file constant raises ASSIGNMENT_TO_CONSTANT." << std::endl;

    EXPECT_EQ(MAREA_codesOfFunction("", "LIMIT = 11;"), Codes{ TO_CONSTANT });
}

TEST(MAREASemaConstantAssignment, CompoundAssignmentToConstantIsReported)
{
    GTEST_COUT << "Validating that a compound operator of each family on a file constant raises ASSIGNMENT_TO_CONSTANT." << std::endl;

    for (const char *statement : { "LIMIT += 1;", "LIMIT <<= 1;", "LIMIT &= 1;" }) {
        EXPECT_EQ(MAREA_codesOfFunction("", statement), Codes{ TO_CONSTANT }) << statement;
    }
}

TEST(MAREASemaConstantAssignment, FieldPathRootedInConstantIsReported)
{
    GTEST_COUT << "Validating that a field path rooted in a file constant raises ASSIGNMENT_TO_CONSTANT." << std::endl;

    EXPECT_EQ(MAREA_codesOfFunction("", "ORIGIN.x = 1;"), Codes{ TO_CONSTANT });
    EXPECT_EQ(MAREA_codesOfFunction("", "ORIGIN.x += 1;"), Codes{ TO_CONSTANT });
}

TEST(MAREASemaConstantAssignment, ElementPathRootedInConstantIsReported)
{
    GTEST_COUT << "Validating that an element path rooted in a file constant raises ASSIGNMENT_TO_CONSTANT." << std::endl;

    const Codes CODES{ MAREA_codesOfProgram("namespace main; require c.array; constant TABLE: c.array<sinteger32> = c.array<sinteger32>{ 1, 2 }; function f() TABLE[0] = 1; end") };

    EXPECT_NE(std::find(CODES.begin(), CODES.end(), TO_CONSTANT), CODES.end()) << "An index path rooted in a constant must raise.";
}

TEST(MAREASemaConstantAssignment, HidingNamesAreAssignable)
{
    GTEST_COUT << "Validating that a local, a parameter or a for variable named like a constant is assignable." << std::endl;

    EXPECT_EQ(MAREA_codesOfFunction("", "local LIMIT: sinteger32 = 1; LIMIT = 2; LIMIT += 1;"), Codes{});
    EXPECT_EQ(MAREA_codesOfFunction("LIMIT: sinteger32", "LIMIT = 2;"), Codes{});
    EXPECT_EQ(MAREA_codesOfFunction("", "for LIMIT in 0 .. 3 do LIMIT = 1; end"), Codes{});
}

TEST(MAREASemaConstantAssignment, ReadingConstantRaisesNothing)
{
    GTEST_COUT << "Validating that reading a file constant raises nothing." << std::endl;

    EXPECT_EQ(MAREA_codesOfFunction("", "local a: sinteger32 = LIMIT; local b: sinteger32 = ORIGIN.x; a = b;"), Codes{});
}

TEST(MAREASemaConstantAssignment, ImplementationConstantIsReported)
{
    GTEST_COUT << "Validating that a write to a constant of an implementation block raises ASSIGNMENT_TO_CONSTANT, by its bare name, its type and Self." << std::endl;

    const std::string HEAD{ "namespace main; structure Point x: sinteger32; end implementation Point constant ORIGIN: Point = Point{ x = 0 }; constant LIMIT: sinteger32 = 4; " };

    EXPECT_EQ(MAREA_codesOfProgram(HEAD + "function f() LIMIT = 5; end end"), Codes{ TO_CONSTANT }) << "A bare name inside the block.";

    EXPECT_EQ(MAREA_codesOfProgram(HEAD + "function f() Self.LIMIT = 5; end end"), Codes{ TO_CONSTANT }) << "A name of Self inside the block.";

    EXPECT_EQ(MAREA_codesOfProgram(HEAD + "end function f() Point.LIMIT = 5; end"), Codes{ TO_CONSTANT }) << "A name of the type outside the block.";

    EXPECT_EQ(MAREA_codesOfProgram(HEAD + "end function f() Point.ORIGIN.x += 5; end"), Codes{ TO_CONSTANT }) << "A field path rooted in a constant of the type.";

    EXPECT_EQ(MAREA_codesOfProgram(HEAD + "function f() local LIMIT: sinteger32 = 0; LIMIT = 5; end end"), Codes{}) << "A local hides the constant of the block.";

    EXPECT_EQ(MAREA_codesOfProgram(HEAD + "end function f() local p: Point = Point{ x = 0 }; p.x = 5; end"), Codes{}) << "A field of a local is writable.";
}
