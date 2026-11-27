// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREACgenHelpers.hpp"

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include <unistd.h>

namespace
{
    // INFO: every compiler this machine has, found by cmake/CMakeLists.txt, each one a row of the acceptance
    // table ("compiles and runs with the same result under clang and gcc when both exist").
    std::vector<std::string> MAREA_availableCompilers()
    {
        std::vector<std::string> result;

#if defined(MAREA_TESTS_CGEN_CLANG)
        result.push_back(MAREA_TESTS_CGEN_CLANG);
#endif
#if defined(MAREA_TESTS_CGEN_GCC)
        result.push_back(MAREA_TESTS_CGEN_GCC);
#endif
        if (result.empty()) {
            result.push_back(MAREA_TESTS_CGEN_C_COMPILER);
        }

        return result;
    }

    std::string MAREA_mainCalling(const std::string &cType, const std::string &callArguments)
    {
        return "#include <stdio.h>\n#include <stdint.h>\n#include <string.h>\n"
               "int main(void){ marea_rt_trap trap = MAREA_RT_TRAP_NONE; " +
               cType + " r = " + callArguments + "; uint64_t bits = 0; memcpy(&bits, &r, sizeof(r)); printf(\"%llu\\n%d\\n\", (unsigned long long)bits, (int)trap); return 0; }\n";
    }

    struct MAREA_CgenRunOutcome
    {
        bool ok{ false };
        std::uint64_t bits{ 0 };
        int trap{ -1 };
        std::string diagnostics;
    };

    // INFO: runs the generated module with every available compiler and asserts they all agree, so a test
    // that calls this once covers the "same result under clang and gcc" row of the acceptance list by itself.
    MAREA_CgenRunOutcome MAREA_runEveryCompiler(const std::string &moduleText, const std::string &cType, const std::string &callArguments)
    {
        MAREA_CgenRunOutcome outcome;

        const std::string MAIN{ MAREA_mainCalling(cType, callArguments) };

        bool first{ true };

        for (const std::string &compiler : MAREA_availableCompilers()) {
            const MAREA_CgenRunResult RUN{ MAREA_cgenCompileAndRun(compiler, moduleText, MAIN) };

            if (!RUN.compiled) {
                outcome.diagnostics = "compile failed with " + compiler + ": " + RUN.output;

                return outcome;
            }

            if (!RUN.ran) {
                outcome.diagnostics = "run failed with " + compiler;

                return outcome;
            }

            std::istringstream stream(RUN.output);

            std::uint64_t bits{ 0 };
            int trap{ -1 };

            stream >> bits >> trap;

            if (first) {
                outcome.bits = bits;
                outcome.trap = trap;
                first = false;
            } else if ((bits != outcome.bits) || (trap != outcome.trap)) {
                outcome.diagnostics = "compilers disagree: " + compiler;

                return outcome;
            }
        }

        outcome.ok = true;

        return outcome;
    }

} // namespace

TEST(MAREACgen, GeneratesIntegerExpressionsLocalsAndReturn)
{
    GTEST_COUT << "Validating cgen writes C11 for integer expressions, a local and return, matching the vm's own result." << std::endl;

    MAREA_CgenFixture fixture;

    ASSERT_TRUE(fixture.MAREA_generate("namespace main; function f(a: sinteger32, b: sinteger32): sinteger32 local x: sinteger32 = a * 2; return x + b; end")) << "The module must generate.";

    const MAREA_CgenRunOutcome OUTCOME{ MAREA_runEveryCompiler(fixture.MAREA_text(), "int32_t", MAREA_cgenFunctionName("f") + "(3,4,&trap)") };

    ASSERT_TRUE(OUTCOME.ok) << OUTCOME.diagnostics;

    EXPECT_EQ(static_cast<std::int32_t>(OUTCOME.bits), 10) << "3 * 2 + 4 is 10.";

    EXPECT_EQ(OUTCOME.trap, 0) << "A plain arithmetic function never traps.";
}

TEST(MAREACgen, IntegerAddWrapsOnOverflowInsteadOfTrapping)
{
    GTEST_COUT << "Validating a signed 8-bit add wraps the C11 fixed-width way instead of being undefined behavior, the same outcome the vm gives." << std::endl;

    MAREA_CgenFixture fixture;

    ASSERT_TRUE(fixture.MAREA_generate("namespace main; function f(a: sinteger8, b: sinteger8): sinteger8 return a + b; end")) << "The module must generate.";

    const MAREA_CgenRunOutcome OUTCOME{ MAREA_runEveryCompiler(fixture.MAREA_text(), "int8_t", MAREA_cgenFunctionName("f") + "(100,100,&trap)") };

    ASSERT_TRUE(OUTCOME.ok) << OUTCOME.diagnostics;

    EXPECT_EQ(static_cast<std::int8_t>(OUTCOME.bits), static_cast<std::int8_t>(200)) << "100 + 100 wraps to -56 in sinteger8.";

    EXPECT_EQ(OUTCOME.trap, 0);
}

TEST(MAREACgen, GeneratesFloatExpressions)
{
    GTEST_COUT << "Validating cgen writes a plain C11 float64 expression with the expected IEEE 754 result." << std::endl;

    MAREA_CgenFixture fixture;

    ASSERT_TRUE(fixture.MAREA_generate("namespace main; function f(a: float64, b: float64): float64 return (a + b) * 2.0; end")) << "The module must generate.";

    const MAREA_CgenRunOutcome OUTCOME{ MAREA_runEveryCompiler(fixture.MAREA_text(), "double", MAREA_cgenFunctionName("f") + "(1.5,2.5,&trap)") };

    ASSERT_TRUE(OUTCOME.ok) << OUTCOME.diagnostics;

    double value{ 0.0 };

    std::memcpy(&value, &OUTCOME.bits, sizeof(value));

    EXPECT_DOUBLE_EQ(value, 8.0) << "(1.5 + 2.5) * 2.0 is 8.0.";
}

TEST(MAREACgen, GeneratesIfAndElse)
{
    GTEST_COUT << "Validating cgen's if/else picks the right branch." << std::endl;

    MAREA_CgenFixture fixture;

    ASSERT_TRUE(fixture.MAREA_generate("namespace main; function f(a: sinteger32): sinteger32 if a < 0 then return -1; else return 1; end end")) << "The module must generate.";

    const MAREA_CgenRunOutcome NEGATIVE{ MAREA_runEveryCompiler(fixture.MAREA_text(), "int32_t", MAREA_cgenFunctionName("f") + "(-5,&trap)") };

    ASSERT_TRUE(NEGATIVE.ok) << NEGATIVE.diagnostics;
    EXPECT_EQ(static_cast<std::int32_t>(NEGATIVE.bits), -1) << "A negative value takes the if branch.";

    const MAREA_CgenRunOutcome POSITIVE{ MAREA_runEveryCompiler(fixture.MAREA_text(), "int32_t", MAREA_cgenFunctionName("f") + "(5,&trap)") };

    ASSERT_TRUE(POSITIVE.ok) << POSITIVE.diagnostics;
    EXPECT_EQ(static_cast<std::int32_t>(POSITIVE.bits), 1) << "A positive value takes the else branch.";
}

TEST(MAREACgen, GeneratesAWhileLoopWithBreakAndContinue)
{
    GTEST_COUT << "Validating cgen's while, break and continue sum only the even values below n, the same program the vm already runs." << std::endl;

    MAREA_CgenFixture fixture;

    ASSERT_TRUE(fixture.MAREA_generate("namespace main; function f(n: sinteger32): sinteger32 local i: sinteger32 = 0; local sum: sinteger32 = 0; while true do if i >= n then break; end i += 1; if (i % 2) != 0 then continue; end sum += i; end return sum; end")) << "The module must generate.";

    const MAREA_CgenRunOutcome OUTCOME{ MAREA_runEveryCompiler(fixture.MAREA_text(), "int32_t", MAREA_cgenFunctionName("f") + "(7,&trap)") };

    ASSERT_TRUE(OUTCOME.ok) << OUTCOME.diagnostics;

    EXPECT_EQ(static_cast<std::int32_t>(OUTCOME.bits), 12) << "2 + 4 + 6 is 12.";
    EXPECT_EQ(OUTCOME.trap, 0);
}

TEST(MAREACgen, GeneratesADoWhileLoopAtLeastOnce)
{
    GTEST_COUT << "Validating the while-true-and-break the ir lowers a do-while to still runs its body at least once." << std::endl;

    MAREA_CgenFixture fixture;

    ASSERT_TRUE(fixture.MAREA_generate("namespace main; function f(n: sinteger32): sinteger32 local i: sinteger32 = 0; do i += 1; while i < n end return i; end")) << "The module must generate.";

    const MAREA_CgenRunOutcome OUTCOME{ MAREA_runEveryCompiler(fixture.MAREA_text(), "int32_t", MAREA_cgenFunctionName("f") + "(0,&trap)") };

    ASSERT_TRUE(OUTCOME.ok) << OUTCOME.diagnostics;

    EXPECT_EQ(static_cast<std::int32_t>(OUTCOME.bits), 1) << "n is 0, but the body still runs once before the test.";
}

TEST(MAREACgen, TrapsOnDivisionByZeroInsteadOfUndefinedBehavior)
{
    GTEST_COUT << "Validating an integer division by zero sets the generated trap flag instead of crashing or invoking undefined behavior." << std::endl;

    MAREA_CgenFixture fixture;

    ASSERT_TRUE(fixture.MAREA_generate("namespace main; function f(a: sinteger32, b: sinteger32): sinteger32 return a / b; end")) << "The module must generate.";

    const MAREA_CgenRunOutcome OUTCOME{ MAREA_runEveryCompiler(fixture.MAREA_text(), "int32_t", MAREA_cgenFunctionName("f") + "(10,0,&trap)") };

    ASSERT_TRUE(OUTCOME.ok) << OUTCOME.diagnostics;

    EXPECT_EQ(OUTCOME.trap, 1) << "MAREA_RT_TRAP_DIVISION_BY_ZERO is 1.";
}

TEST(MAREACgen, DivisionOfTheSignedMinimumByMinusOneNeverOverflows)
{
    GTEST_COUT << "Validating sinteger32 minimum divided by -1 wraps back to the minimum, computed through unsigned negation, never the one C expression that would overflow." << std::endl;

    MAREA_CgenFixture fixture;

    ASSERT_TRUE(fixture.MAREA_generate("namespace main; function f(a: sinteger32, b: sinteger32): sinteger32 return a / b; end")) << "The module must generate.";

    const std::string ARGS{ std::to_string(static_cast<std::int64_t>(std::int32_t{ -2147483647 - 1 })) + ",-1,&trap" };

    const MAREA_CgenRunOutcome OUTCOME{ MAREA_runEveryCompiler(fixture.MAREA_text(), "int32_t", MAREA_cgenFunctionName("f") + "(" + ARGS + ")") };

    ASSERT_TRUE(OUTCOME.ok) << OUTCOME.diagnostics;

    EXPECT_EQ(OUTCOME.trap, 0) << "This is a defined wraparound, never a trap.";
    EXPECT_EQ(static_cast<std::int32_t>(OUTCOME.bits), (-2147483647 - 1)) << "The minimum divided by -1 wraps back to the minimum.";
}

TEST(MAREACgen, TrapsOnAShiftPastTheWidth)
{
    GTEST_COUT << "Validating a shift whose count is not less than the width of the kind sets the generated trap flag instead of invoking undefined behavior." << std::endl;

    MAREA_CgenFixture fixture;

    ASSERT_TRUE(fixture.MAREA_generate("namespace main; function f(a: uinteger32, b: uinteger32): uinteger32 return a << b; end")) << "The module must generate.";

    const MAREA_CgenRunOutcome OUTCOME{ MAREA_runEveryCompiler(fixture.MAREA_text(), "uint32_t", MAREA_cgenFunctionName("f") + "(1,32,&trap)") };

    ASSERT_TRUE(OUTCOME.ok) << OUTCOME.diagnostics;

    EXPECT_EQ(OUTCOME.trap, 2) << "MAREA_RT_TRAP_SHIFT_OUT_OF_RANGE is 2.";
}

TEST(MAREACgen, CallsAnotherFunctionOfTheSameModule)
{
    GTEST_COUT << "Validating a plain call of another function of the same unit generates a C11 call of its mangled name." << std::endl;

    MAREA_CgenFixture fixture;

    ASSERT_TRUE(fixture.MAREA_generate("namespace main; function double_it(x: sinteger32): sinteger32 return x * 2; end function f(a: sinteger32, b: sinteger32): sinteger32 return double_it(a) + b; end")) << "The module must generate.";

    const MAREA_CgenRunOutcome OUTCOME{ MAREA_runEveryCompiler(fixture.MAREA_text(), "int32_t", MAREA_cgenFunctionName("f") + "(3,4,&trap)") };

    ASSERT_TRUE(OUTCOME.ok) << OUTCOME.diagnostics;

    EXPECT_EQ(static_cast<std::int32_t>(OUTCOME.bits), 10) << "double_it(3) + 4 is 10.";
    EXPECT_EQ(OUTCOME.trap, 0);
}

TEST(MAREACgen, SameIrGivesTheSameTextTwice)
{
    GTEST_COUT << "Validating cgen is deterministic: the same ir module generated twice gives the exact same C11 text." << std::endl;

    MAREA_CgenFixture fixture;

    ASSERT_TRUE(fixture.MAREA_generate("namespace main; function f(a: sinteger32, b: sinteger32): sinteger32 return a + b; end")) << "The module must generate.";

    const std::string FIRST{ fixture.MAREA_text() };

    const MAREA_Character8 *secondText{ nullptr };
    MAREA_IntegerSize secondLength{ 0 };

    ASSERT_EQ(MAREA_CgenGenerateModule(fixture.context, fixture.ir.module, &secondText, &secondLength), MAREA_CGEN_EXCEPT_SUCCESS) << "A second generation of the same module must succeed.";

    const std::string SECOND(secondText, static_cast<std::size_t>(secondLength));

    EXPECT_EQ(FIRST, SECOND) << "The same ir module must generate the exact same text every time.";
}

TEST(MAREACgen, GenerateModuleRejectsNullArgumentsAndABadRoot)
{
    GTEST_COUT << "Validating MAREA_CgenGenerateModule rejects a null argument and a root that is not a MODULE." << std::endl;

    MAREA_CgenFixture fixture;

    ASSERT_TRUE(fixture.MAREA_generate("namespace main; function f(): sinteger32 return 1; end")) << "The module must generate once, to have a context and a module to probe with.";

    const MAREA_Character8 *outText{ nullptr };
    MAREA_IntegerSize outLength{ 0 };

    EXPECT_EQ(MAREA_CgenGenerateModule(nullptr, fixture.ir.module, &outText, &outLength), MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER);
    EXPECT_EQ(MAREA_CgenGenerateModule(fixture.context, nullptr, &outText, &outLength), MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER);
    EXPECT_EQ(MAREA_CgenGenerateModule(fixture.context, fixture.ir.module, nullptr, &outLength), MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER);
    EXPECT_EQ(MAREA_CgenGenerateModule(fixture.context, fixture.ir.module, &outText, nullptr), MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER);

    const MAREA_IrNode *notAModule{ nullptr };

    MAREA_IGNORE(MAREA_IrNodeGetChildAt(fixture.ir.module, 0, &notAModule));

    EXPECT_EQ(MAREA_CgenGenerateModule(fixture.context, notAModule, &outText, &outLength), MAREA_CGEN_EXCEPT_ERROR_BAD_ROOT);
}

TEST(MAREACgen, TwoTestProcessesNeverShareTheFilesOfACompile)
{
    GTEST_COUT << "Validating two test processes, as ctest -j starts them, never name the same scratch directory for a compile." << std::endl;

    int channel[2]{ -1, -1 };

    ASSERT_EQ(pipe(channel), 0) << "The pipe must open.";

    const pid_t CHILD{ fork() };

    ASSERT_NE(CHILD, -1) << "The second process must start.";

    if (CHILD == 0) {
        const MAREA_ScratchDirectory SCRATCH;

        const std::string NAME{ SCRATCH.path.string() };

        const bool SENT{ write(channel[1], NAME.data(), NAME.size()) == static_cast<ssize_t>(NAME.size()) };

        _exit(SENT ? 0 : 1);
    }

    close(channel[1]);

    const MAREA_ScratchDirectory SCRATCH;

    std::string other;

    char chunk[256];

    for (ssize_t read{ ::read(channel[0], chunk, sizeof(chunk)) }; read > 0; read = ::read(channel[0], chunk, sizeof(chunk))) {
        other.append(chunk, static_cast<std::size_t>(read));
    }

    close(channel[0]);

    int status{ -1 };

    ASSERT_EQ(waitpid(CHILD, &status, 0), CHILD) << "The second process must end.";

    EXPECT_TRUE(WIFEXITED(status) && (WEXITSTATUS(status) == 0)) << "The second process must send its name.";

    EXPECT_FALSE(other.empty()) << "The name of the second process must arrive.";

    EXPECT_NE(other, SCRATCH.path.string()) << "Two processes must never share one directory.";

    std::error_code ignored;

    std::filesystem::remove_all(other, ignored);
}

TEST(MAREACgen, ContextCreateRejectsNullArguments)
{
    GTEST_COUT << "Validating MAREA_CgenContextCreate rejects a null arena and a null sink." << std::endl;

    MAREA_Arena *arena{ nullptr };

    ASSERT_EQ(MAREA_ArenaCreate(static_cast<MAREA_IntegerSize>(4096), &arena), MAREA_ARENA_EXCEPT_SUCCESS);

    MAREA_DiagnosticSink *sink{ nullptr };

    ASSERT_EQ(MAREA_DiagnosticSinkCreate(arena, &sink), MAREA_DIAGNOSTIC_EXCEPT_SUCCESS);

    MAREA_CgenContext *context{ nullptr };

    EXPECT_EQ(MAREA_CgenContextCreate(nullptr, sink, &context), MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER);
    EXPECT_EQ(MAREA_CgenContextCreate(arena, nullptr, &context), MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER);
    EXPECT_EQ(MAREA_CgenContextCreate(arena, sink, nullptr), MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER);

    ASSERT_EQ(MAREA_CgenContextCreate(arena, sink, &context), MAREA_CGEN_EXCEPT_SUCCESS);

    EXPECT_EQ(MAREA_CgenContextGetArena(context), arena);
    EXPECT_EQ(MAREA_CgenContextGetDiagnostics(context), sink);
    EXPECT_EQ(MAREA_CgenContextGetArena(nullptr), nullptr);
    EXPECT_EQ(MAREA_CgenContextGetDiagnostics(nullptr), nullptr);
}
