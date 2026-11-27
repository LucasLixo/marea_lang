// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

    constexpr MAREA_Uinteger32 CYCLIC{ MAREA_SEMA_DIAGNOSTIC_CODE_CYCLIC_CONSTANT };

    Codes MAREA_codesOfProgram(const std::string &program)
    {
        MAREA_SemaFixture fixture([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; });

        if (fixture.MAREA_sema(program) != MAREA_SEMA_EXCEPT_SUCCESS) {
            return { 0 };
        }

        return fixture.MAREA_semaCodes();
    }

    std::string MAREA_chain(int length, bool closeCycle)
    {
        std::string program{ "namespace main; " };

        for (int index = 0; index < length; ++index) {
            const bool LAST{ index == (length - 1) };

            program += "constant C" + std::to_string(index) + ": sinteger32 = ";
            program += LAST ? (closeCycle ? "C0" : "1") : ("C" + std::to_string(index + 1));
            program += "; ";
        }

        return program;
    }
} // namespace

TEST(MAREASemaConstantCycle, SelfReferenceIsReportedOnce)
{
    GTEST_COUT << "Validating that a constant that is its own initial value raises one CYCLIC_CONSTANT." << std::endl;

    EXPECT_EQ(MAREA_codesOfProgram("namespace main; constant A: sinteger32 = A;"), Codes{ CYCLIC });
}

TEST(MAREASemaConstantCycle, TwoConstantCycleIsReportedOnceAtTheFirst)
{
    GTEST_COUT << "Validating that A = B and B = A raise exactly one CYCLIC_CONSTANT, at A." << std::endl;

    MAREA_SemaFixture fixture([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; });

    ASSERT_EQ(fixture.MAREA_sema("namespace main;\nconstant A: sinteger32 = B;\nconstant B: sinteger32 = A;\n"), MAREA_SEMA_EXCEPT_SUCCESS);

    EXPECT_EQ(fixture.MAREA_semaCodes(), Codes{ CYCLIC });
}

TEST(MAREASemaConstantCycle, ThreeConstantCycleIsReportedOnce)
{
    GTEST_COUT << "Validating that A = B, B = C and C = A raise exactly one CYCLIC_CONSTANT." << std::endl;

    EXPECT_EQ(MAREA_codesOfProgram("namespace main; constant A: sinteger32 = B; constant B: sinteger32 = C; constant C: sinteger32 = A;"), Codes{ CYCLIC });
}

TEST(MAREASemaConstantCycle, CycleThroughBinaryAndParenthesizedExpressionIsReported)
{
    GTEST_COUT << "Validating that a cycle through a binary and a parenthesized expression is found." << std::endl;

    EXPECT_EQ(MAREA_codesOfProgram("namespace main; constant A: sinteger32 = (B + 1) * 2; constant B: sinteger32 = 3 + (A);"), Codes{ CYCLIC });
}

TEST(MAREASemaConstantCycle, ConstantThatOnlyUsesACycleIsNotReported)
{
    GTEST_COUT << "Validating that a constant that reaches a cycle without being on it raises nothing extra." << std::endl;

    EXPECT_EQ(MAREA_codesOfProgram("namespace main; constant U: sinteger32 = A; constant A: sinteger32 = B; constant B: sinteger32 = A;"), Codes{ CYCLIC });
}

TEST(MAREASemaConstantCycle, TwoSeparateCyclesAreReportedTwice)
{
    GTEST_COUT << "Validating that each cycle raises its own CYCLIC_CONSTANT." << std::endl;

    EXPECT_EQ(MAREA_codesOfProgram("namespace main; constant A: sinteger32 = A; constant B: sinteger32 = C; constant C: sinteger32 = B;"), (Codes{ CYCLIC, CYCLIC }));
}

TEST(MAREASemaConstantCycle, AcyclicUseIsAccepted)
{
    GTEST_COUT << "Validating that a use of a later constant without a cycle raises nothing." << std::endl;

    EXPECT_EQ(MAREA_codesOfProgram("namespace main; constant A: sinteger32 = B + 1; constant B: sinteger32 = 2;"), Codes{});
    EXPECT_EQ(MAREA_codesOfProgram("namespace main; constant A: sinteger32 = B + B; constant B: sinteger32 = 2;"), Codes{});
}

TEST(MAREASemaConstantCycle, FunctionAndLocalNamesAreNotEdges)
{
    GTEST_COUT << "Validating that a name of a function does not make an edge." << std::endl;

    EXPECT_EQ(MAREA_codesOfProgram("namespace main; function f(): sinteger32 return 1; end constant A: sinteger32 = f(); function g(): sinteger32 local A: sinteger32 = A; return A; end"), Codes{});
}

TEST(MAREASemaConstantCycle, LongChainWithoutCycleIsAccepted)
{
    GTEST_COUT << "Validating that a chain of 70 constants raises nothing." << std::endl;

    EXPECT_EQ(MAREA_codesOfProgram(MAREA_chain(70, false)), Codes{});
}

TEST(MAREASemaConstantCycle, LongChainThatClosesIsReportedOnce)
{
    GTEST_COUT << "Validating that a ring of 70 constants raises one CYCLIC_CONSTANT." << std::endl;

    EXPECT_EQ(MAREA_codesOfProgram(MAREA_chain(70, true)), Codes{ CYCLIC });
}

TEST(MAREASemaConstantCycle, ImplementationConstantsAreInTheSearch)
{
    GTEST_COUT << "Validating that a cycle through a constant of an implementation block raises one CYCLIC_CONSTANT." << std::endl;

    const std::string HEAD{ "namespace main; structure P x: sinteger32; end " };

    EXPECT_EQ(MAREA_codesOfProgram(HEAD + "implementation P constant A: sinteger32 = B; constant B: sinteger32 = A; end"), Codes{ CYCLIC }) << "Two constants of one block.";

    EXPECT_EQ(MAREA_codesOfProgram(HEAD + "constant C: sinteger32 = P.B; implementation P constant B: sinteger32 = C; end"), Codes{ CYCLIC }) << "A cycle through Type.NAME.";

    EXPECT_EQ(MAREA_codesOfProgram(HEAD + "implementation P constant A: sinteger32 = Self.A; end"), Codes{ CYCLIC }) << "A constant that reads itself through Self.";

    EXPECT_EQ(MAREA_codesOfProgram(HEAD + "implementation P constant A: sinteger32 = 1; constant B: sinteger32 = A + 1; end constant C: sinteger32 = P.B;"), Codes{}) << "A use with no cycle.";
}

namespace
{
    Codes MAREA_codesWithOther(const std::string &other, const std::string &program)
    {
        MAREA_SemaFixture library;

        MAREA_IGNORE(library.parser.MAREA_parse(other));

        const MAREA_AstNode *ROOT{ library.parser.root };

        MAREA_SemaFixture fixture([ROOT](const std::string &text, const MAREA_AstNode **outRoot) {
            if (text == "other") {
                *outRoot = ROOT;

                return MAREA_SEMA_IMPORT_RESULT_FOUND;
            }

            return MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
        });

        if (fixture.MAREA_sema(program) != MAREA_SEMA_EXCEPT_SUCCESS) {
            return { 0 };
        }

        return fixture.MAREA_semaCodes();
    }
} // namespace

TEST(MAREASemaConstantCycle, QualifiedNamesAreFollowedThroughTheResolver)
{
    GTEST_COUT << "Validating that a cycle through a constant of another namespace raises one CYCLIC_CONSTANT." << std::endl;

    EXPECT_EQ(MAREA_codesWithOther("namespace other; import main; constant B: sinteger32 = main.A;", "namespace main; import other; constant A: sinteger32 = other.B;"), Codes{ CYCLIC }) << "Full paths on both sides.";

    EXPECT_EQ(MAREA_codesWithOther("namespace other; import main as m; constant B: sinteger32 = m.A + 1;", "namespace main; import other as o; constant A: sinteger32 = o.B;"), Codes{ CYCLIC }) << "Aliases on both sides.";

    EXPECT_EQ(MAREA_codesWithOther("namespace other; import main; constant B: sinteger32 = C; constant C: sinteger32 = main.A;", "namespace main; import other { B as D }; constant A: sinteger32 = D;"), Codes{ CYCLIC }) << "An imported constant with a second step in its file.";

    EXPECT_EQ(MAREA_codesWithOther("namespace other; constant B: sinteger32 = 2;", "namespace main; import other; constant A: sinteger32 = other.B;"), Codes{}) << "A qualified name with no cycle.";

    EXPECT_EQ(MAREA_codesWithOther("namespace other; constant B: sinteger32 = B;", "namespace main; import other; constant A: sinteger32 = other.B;"), Codes{}) << "A cycle of another file is not reported here.";
}
