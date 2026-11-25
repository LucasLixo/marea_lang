// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaCallerHelpers.hpp"

#include "marea/internal/sema/MAREASemaInternal.h"

namespace
{
    MAREA_CallerInputs MAREA_target(const std::string &architecture, const std::vector<std::string> &features)
    {
        MAREA_CallerInputs inputs;

        inputs.hasTarget = true;
        inputs.architecture = architecture;
        inputs.operatingSystem = "linux";
        inputs.compiler = "gcc";
        inputs.endianness = "little";
        inputs.features = features;
        inputs.pointerWidth = 64;

        return inputs;
    }

    // INFO: the resolved backend of the first function of the program, "<none>" when the file has none, or "<no function>". A file of marea.math raises the reserved root code, which this check does not care about.
    std::string MAREA_backendOf(const std::string &program, const MAREA_CallerInputs &inputs = MAREA_CallerInputs{})
    {
        MAREA_CallerRun run(program, inputs);

        const MAREA_AstNode *root{ run.fixture.parser.root };

        for (MAREA_IntegerSize index{ 0 }; index < MAREA_AstNodeGetChildCount(root); ++index) {
            const MAREA_AstNode *child{ nullptr };

            MAREA_IGNORE(MAREA_AstNodeGetChildAt(root, index, &child));

            if (MAREA_AstNodeGetKind(child) != MAREA_AST_KIND_FUNCTION_DECLARATION) {
                continue;
            }

            const MAREA_Character8 *text{ nullptr };

            MAREA_IntegerSize length{ 0 };

            if (MAREA_SemaResolveBackend(run.fixture.context, child, &text, &length) != MAREA_SEMA_EXCEPT_SUCCESS) {
                return "<failed>";
            }

            return (text == nullptr) ? "<none>" : std::string(text, length);
        }

        return "<no function>";
    }

    const std::string MATH{ "namespace marea.math;\n" };
} // namespace

TEST(MAREASemaBackendResolve, AFileOutsideMareaMathHasNoBackend)
{
    GTEST_COUT << "Validating a function of a file that is not in marea.math resolves to no backend." << std::endl;

    EXPECT_EQ(MAREA_backendOf("namespace main;\nfunction run()\nend\n"), "<none>") << "The backend key stands only in marea.math, so another file has none.";
}

TEST(MAREASemaBackendResolve, NoKeyMeansAutoAndAutoWithNoTargetIsScalar)
{
    GTEST_COUT << "Validating a function with no backend key resolves auto, and no target gives scalar." << std::endl;

    EXPECT_EQ(MAREA_backendOf(MATH + "function run()\nend\n"), "scalar") << "Auto with no target description must be scalar.";
}

TEST(MAREASemaBackendResolve, AutoTakesTheBestBackendTheTargetGives)
{
    GTEST_COUT << "Validating auto resolves to intrinsics where the target gives them, and to vector_extension where only that is given." << std::endl;

    EXPECT_EQ(MAREA_backendOf(MATH + "function run()\nend\n", MAREA_target("x86_64", { "sse2" })), "intrinsics") << "Auto on x86_64 with sse2 must be intrinsics.";

    EXPECT_EQ(MAREA_backendOf(MATH + "function run()\nend\n", MAREA_target("aarch64", {})), "vector_extension") << "Auto on aarch64 with no neon feature must be vector_extension.";

    EXPECT_EQ(MAREA_backendOf(MATH + "function run()\nend\n", MAREA_target("riscv64", {})), "scalar") << "Auto on a target with no vector backend must be scalar.";
}

TEST(MAREASemaBackendResolve, AnExplicitBackendIsKept)
{
    GTEST_COUT << "Validating an explicit file backend is the backend of every function of the file." << std::endl;

    EXPECT_EQ(MAREA_backendOf(MATH + "#![backend = scalar]\nfunction run()\nend\n", MAREA_target("x86_64", { "sse2" })), "scalar") << "An explicit scalar must not become intrinsics.";
}

TEST(MAREASemaBackendResolve, AFunctionAttributeWinsOverTheFileAttribute)
{
    GTEST_COUT << "Validating the attribute of the function wins over the attribute of the file." << std::endl;

    EXPECT_EQ(MAREA_backendOf(MATH + "#![backend = scalar]\n#[backend = vector_extension]\nfunction run()\nend\n", MAREA_target("x86_64", { "sse2" })), "vector_extension") << "The function attribute must win.";
}

TEST(MAREASemaBackendResolve, AWhenThatHoldsGivesTheBackend)
{
    GTEST_COUT << "Validating a when item whose predicate holds replaces the plain backend, and one that does not hold does not." << std::endl;

    const std::string PROGRAM{ MATH + "#![backend = scalar]\n#![when(architecture = \"x86_64\", backend = intrinsics)]\nfunction run()\nend\n" };

    EXPECT_EQ(MAREA_backendOf(PROGRAM, MAREA_target("x86_64", { "sse2" })), "intrinsics") << "The holding when must win over the plain value.";

    EXPECT_EQ(MAREA_backendOf(PROGRAM, MAREA_target("aarch64", { "neon" })), "scalar") << "A when that does not hold must leave the plain value.";
}

TEST(MAREASemaBackendResolve, TheTypedFunctionAnswersNoBackendOutsideMareaMath)
{
    GTEST_COUT << "Validating the typed node of a function in an ordinary file has no backend, and a null node is rejected." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_EQ(fixture.MAREA_sema("namespace main; function run() end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The unit must check.";

    const MAREA_SemaTypedNode *root{ nullptr };

    ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &root), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The typed tree must build.";

    const MAREA_SemaTypedNode *function{ nullptr };

    ASSERT_EQ(MAREA_SemaTypedNodeGetChildAt(root, 1, &function), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The function must be the second child.";

    const MAREA_Character8 *text{ nullptr };

    MAREA_IntegerSize length{ 0 };

    EXPECT_EQ(MAREA_SemaTypedNodeGetBackend(function, &text, &length), MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NO_BACKEND) << "A function of an ordinary file has no backend.";

    EXPECT_EQ(MAREA_SemaTypedNodeGetBackend(nullptr, &text, &length), MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER) << "A null node must be rejected.";

    EXPECT_EQ(MAREA_SemaTypedNodeGetBackend(function, nullptr, &length), MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER) << "A null out text must be rejected.";
}

TEST(MAREASemaBackendResolve, TheTypedFunctionAnswersTheBackendItHolds)
{
    GTEST_COUT << "Validating the accessor answers the text that the build stored in the typed node." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_EQ(fixture.MAREA_sema("namespace main; function run() end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The unit must check.";

    const MAREA_SemaTypedNode *root{ nullptr };

    ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &root), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The typed tree must build.";

    const MAREA_SemaTypedNode *function{ nullptr };

    ASSERT_EQ(MAREA_SemaTypedNodeGetChildAt(root, 1, &function), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The function must be the second child.";

    // INFO: a file of marea.math cannot be typed through the project path yet (the reserved root is an error), so the test sets the field that the build would set.
    const_cast<MAREA_SemaTypedNode *>(function)->backend = "intrinsics";
    const_cast<MAREA_SemaTypedNode *>(function)->backendLength = 10;

    const MAREA_Character8 *text{ nullptr };

    MAREA_IntegerSize length{ 0 };

    ASSERT_EQ(MAREA_SemaTypedNodeGetBackend(function, &text, &length), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "A held backend must be answered.";

    EXPECT_EQ(std::string(text, length), "intrinsics") << "The text must be the stored one.";
}
