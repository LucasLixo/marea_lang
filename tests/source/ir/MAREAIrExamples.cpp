// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAIrHelpers.hpp"

#include <set>

namespace
{
    constexpr MAREA_Uinteger32 FORK_JOIN{ MAREA_IR_DIAGNOSTIC_CODE_UNSUPPORTED_FORK_JOIN };
    constexpr MAREA_Uinteger32 SWITCH{ MAREA_IR_DIAGNOSTIC_CODE_UNSUPPORTED_SWITCH };
    constexpr MAREA_Uinteger32 FOR_EACH{ MAREA_IR_DIAGNOSTIC_CODE_UNSUPPORTED_FOR_EACH };
    constexpr MAREA_Uinteger32 GENERIC{ MAREA_IR_DIAGNOSTIC_CODE_UNINSTANTIATED_GENERIC };
    constexpr MAREA_Uinteger32 MACRO{ MAREA_IR_DIAGNOSTIC_CODE_UNSUPPORTED_MACRO };

    using Bucket = std::set<MAREA_Uinteger32>;

    const std::map<std::string, Bucket> EXPECTED{
        { "attributes/inline_and_build.marea", {} },
        { "attributes/layout.marea", { GENERIC } },
        { "attributes/simd.marea", {} },
        { "attributes/visibility.marea", {} },
        { "basics/blocks.marea", { GENERIC } },
        { "basics/control_flow.marea", { SWITCH, FOR_EACH } },
        { "basics/functions.marea", {} },
        { "basics/operators.marea", {} },
        { "basics/ownership.marea", {} },
        { "basics/primitive_types.marea", { GENERIC } },
        { "interop/c_library.marea", { MACRO } },
        { "interop/gpu_handles.marea", { FORK_JOIN, GENERIC } },
        { "macros/builtin.marea", { MACRO } },
        { "main.marea", {} },
        { "memory/regions.marea", {} },
        { "namespaces/consumer.marea", {} },
        { "namespaces/library.marea", {} },
        { "parallel/scaling.marea", { FORK_JOIN, FOR_EACH } },
        { "types/collections.marea", {} },
        { "types/enum_methods.marea", { SWITCH } },
        { "types/generics.marea", { GENERIC } },
        { "types/methods.marea", {} },
        { "types/structures.marea", {} },
    };

    std::string MAREA_describe(const Bucket &bucket)
    {
        std::string text{ "{" };

        for (MAREA_Uinteger32 code : bucket) {
            text += (text.size() > 1 ? "," : "") + std::to_string(code);
        }

        return text + "}";
    }
} // namespace

TEST(MAREAIrExamples, TwentyThreeExamplesExist)
{
    GTEST_COUT << "Validating the examples directory holds 23 programs and the table covers each one." << std::endl;

    const MAREA_IrExampleSet EXAMPLES;

    EXPECT_EQ(EXAMPLES.files.size(), 23U) << "The examples directory must hold 23 .marea files.";

    EXPECT_EQ(EXPECTED.size(), 23U) << "The expected table must hold one row per example.";

    for (const MAREA_IrExample &file : EXAMPLES.files) {
        EXPECT_EQ(EXPECTED.count(file.relativePath), 1U) << "The example " << file.relativePath << " must have a row in the table.";
    }
}

TEST(MAREAIrExamples, EveryExampleLowersIntoItsBucket)
{
    GTEST_COUT << "Validating every example lowers to a MODULE and raises exactly the gap codes of its row." << std::endl;

    const MAREA_IrExampleSet EXAMPLES;

    std::string mismatches;

    for (const MAREA_IrExample &file : EXAMPLES.files) {
        MAREA_IrFixture fixture(EXAMPLES.MAREA_resolver());

        ASSERT_EQ(fixture.sema.MAREA_sema(file.source, file.relativePath), MAREA_SEMA_EXCEPT_SUCCESS) << "The check of " << file.relativePath << " must succeed.";

        ASSERT_EQ(fixture.sema.MAREA_errorCount(), 0U) << "The example " << file.relativePath << " must check with zero errors.";

        ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.sema.context, fixture.sema.parser.root, &fixture.typedRoot), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The typed tree of " << file.relativePath << " must build.";

        ASSERT_EQ(MAREA_IrLowerCompilationUnit(fixture.context, fixture.typedRoot, &fixture.module), MAREA_IR_EXCEPT_SUCCESS) << "The example " << file.relativePath << " must lower.";

        ASSERT_EQ(MAREA_IrNodeGetKind(fixture.module), MAREA_IR_KIND_MODULE) << "The root of " << file.relativePath << " must be a MODULE.";

        const std::vector<MAREA_Uinteger32> CODES{ fixture.MAREA_irCodes() };

        const Bucket ACTUAL{ CODES.begin(), CODES.end() };

        EXPECT_EQ(MAREA_DiagnosticSinkGetCount(fixture.sema.parser.lexer.sink), CODES.size()) << "The example " << file.relativePath << " may only raise ir diagnostics.";

        const auto ROW{ EXPECTED.find(file.relativePath) };

        const Bucket WANTED{ (ROW == EXPECTED.end()) ? Bucket{} : ROW->second };

        if (ACTUAL != WANTED) {
            mismatches += file.relativePath + " expected " + MAREA_describe(WANTED) + " got " + MAREA_describe(ACTUAL) + "\n";
        }
    }

    EXPECT_TRUE(mismatches.empty()) << "Every example must match its row:\n"
                                    << mismatches;
}

TEST(MAREAIrExamples, EveryExampleDumpsAsText)
{
    GTEST_COUT << "Validating every lowered example dumps to a non-empty text that starts with MODULE." << std::endl;

    const MAREA_IrExampleSet EXAMPLES;

    for (const MAREA_IrExample &file : EXAMPLES.files) {
        MAREA_IrFixture fixture(EXAMPLES.MAREA_resolver());

        ASSERT_EQ(fixture.MAREA_lower(file.source, file.relativePath), MAREA_IR_EXCEPT_SUCCESS) << "The example " << file.relativePath << " must lower.";

        const MAREA_Character8 *text{ nullptr };

        MAREA_IntegerSize length{ 0 };

        ASSERT_EQ(MAREA_IrModuleDump(fixture.sema.parser.lexer.arena, fixture.module, &text, &length), MAREA_IR_EXCEPT_SUCCESS) << "The dump of " << file.relativePath << " must succeed.";

        EXPECT_EQ(std::string(text, length).compare(0, 7, "MODULE\n"), 0) << "The dump of " << file.relativePath << " must start with MODULE.";
    }
}

TEST(MAREAIrExamples, LoweringTwiceGivesTheSameDump)
{
    GTEST_COUT << "Validating lowering the same example twice gives byte-identical dumps." << std::endl;

    const MAREA_IrExampleSet EXAMPLES;

    for (const MAREA_IrExample &file : EXAMPLES.files) {
        std::string dumps[2];

        for (std::string &dump : dumps) {
            MAREA_IrFixture fixture(EXAMPLES.MAREA_resolver());

            ASSERT_EQ(fixture.MAREA_lower(file.source, file.relativePath), MAREA_IR_EXCEPT_SUCCESS) << "The example " << file.relativePath << " must lower.";

            const MAREA_Character8 *text{ nullptr };

            MAREA_IntegerSize length{ 0 };

            ASSERT_EQ(MAREA_IrModuleDump(fixture.sema.parser.lexer.arena, fixture.module, &text, &length), MAREA_IR_EXCEPT_SUCCESS) << "The dump must succeed.";

            dump.assign(text, length);
        }

        EXPECT_EQ(dumps[0], dumps[1]) << "The dump of " << file.relativePath << " must be stable.";
    }
}
