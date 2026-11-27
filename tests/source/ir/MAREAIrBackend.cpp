// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAIrHelpers.hpp"

#include "marea/internal/sema/MAREASemaInternal.h"

namespace
{
    // INFO: a file of marea.math cannot be typed through the project path yet (the reserved root is an error), so the test builds the typed tree of an ordinary file and sets the field that the build sets for a function of marea.math.
    void MAREA_setBackend(const MAREA_SemaTypedNode *function, const char *backend)
    {
        const_cast<MAREA_SemaTypedNode *>(function)->backend = backend;
        const_cast<MAREA_SemaTypedNode *>(function)->backendLength = std::string(backend).size();
    }

    const MAREA_SemaTypedNode *MAREA_typedChild(const MAREA_SemaTypedNode *node, MAREA_IntegerSize index)
    {
        const MAREA_SemaTypedNode *child{ nullptr };

        MAREA_IGNORE(MAREA_SemaTypedNodeGetChildAt(node, index, &child));

        return child;
    }

    std::string MAREA_dumpOf(MAREA_IrFixture &fixture)
    {
        const MAREA_Character8 *text{ nullptr };

        MAREA_IntegerSize length{ 0 };

        MAREA_IGNORE(MAREA_IrModuleDump(fixture.sema.parser.lexer.arena, fixture.module, &text, &length));

        return std::string(text, length);
    }
} // namespace

TEST(MAREAIrBackend, KindHasAStableName)
{
    GTEST_COUT << "Validating the new kind has its stable upper-case name." << std::endl;

    EXPECT_STREQ(MAREA_IrKindGetName(MAREA_IR_KIND_BACKEND_ATTRIBUTE), "BACKEND_ATTRIBUTE") << "The name must be stable.";

    EXPECT_STREQ(MAREA_IrKindGetName(MAREA_IR_KIND_COUNT), "UNKNOWN") << "The count is not a kind.";
}

TEST(MAREAIrBackend, FunctionWithAResolvedBackendCarriesItAsItsLastChild)
{
    GTEST_COUT << "Validating a function whose typed node holds a backend lowers with a BACKEND_ATTRIBUTE child after the body." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.sema.MAREA_sema("namespace main; function plain() end function pinned(a: sinteger32) end", "main.marea"), MAREA_SEMA_EXCEPT_SUCCESS) << "The unit must check.";

    ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.sema.context, fixture.sema.parser.root, &fixture.typedRoot), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The typed tree must build.";

    MAREA_setBackend(MAREA_typedChild(fixture.typedRoot, 2), "intrinsics");

    ASSERT_EQ(MAREA_IrLowerCompilationUnit(fixture.context, fixture.typedRoot, &fixture.module), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    EXPECT_EQ(MAREA_irRender(MAREA_irChild(fixture.module, 0)), "(FUNCTION_DECLARATION plain (PARAMETER_LIST) (BLOCK))") << "A function with no backend must keep its shape.";

    EXPECT_EQ(MAREA_irRender(MAREA_irChild(fixture.module, 1)), "(FUNCTION_DECLARATION pinned (PARAMETER_LIST (PARAMETER a :sinteger32)) (BLOCK) (BACKEND_ATTRIBUTE intrinsics))") << "The backend must come after the body, so the first two children keep their places.";
}

TEST(MAREAIrBackend, DumpShowsTheBackend)
{
    GTEST_COUT << "Validating the text dump shows the backend of a function on its own line." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.sema.MAREA_sema("namespace main; function pinned() end", "main.marea"), MAREA_SEMA_EXCEPT_SUCCESS) << "The unit must check.";

    ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.sema.context, fixture.sema.parser.root, &fixture.typedRoot), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The typed tree must build.";

    MAREA_setBackend(MAREA_typedChild(fixture.typedRoot, 1), "scalar");

    ASSERT_EQ(MAREA_IrLowerCompilationUnit(fixture.context, fixture.typedRoot, &fixture.module), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    EXPECT_NE(MAREA_dumpOf(fixture).find("    BLOCK\n    BACKEND_ATTRIBUTE \"scalar\"\n"), std::string::npos) << MAREA_dumpOf(fixture);
}

TEST(MAREAIrBackend, FunctionOfAnOrdinaryFileCarriesNoBackend)
{
    GTEST_COUT << "Validating a function of an ordinary file lowers with no BACKEND_ATTRIBUTE." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; function plain() end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    EXPECT_EQ(MAREA_dumpOf(fixture).find("BACKEND_ATTRIBUTE"), std::string::npos) << "No backend may appear.";
}

TEST(MAREAIrBackend, MethodOfAnImplementationCarriesItsBackendToo)
{
    GTEST_COUT << "Validating a function of an implementation block keeps its backend as well." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.sema.MAREA_sema("namespace main; structure Point x: float64; end implementation Point function norm(self: Point): float64 return self.x; end end", "main.marea"), MAREA_SEMA_EXCEPT_SUCCESS) << "The unit must check.";

    ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.sema.context, fixture.sema.parser.root, &fixture.typedRoot), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The typed tree must build.";

    const MAREA_SemaTypedNode *implementation{ MAREA_typedChild(fixture.typedRoot, 2) };

    MAREA_setBackend(MAREA_typedChild(implementation, 1), "vector_extension");

    ASSERT_EQ(MAREA_IrLowerCompilationUnit(fixture.context, fixture.typedRoot, &fixture.module), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    EXPECT_NE(MAREA_dumpOf(fixture).find("BACKEND_ATTRIBUTE \"vector_extension\""), std::string::npos) << MAREA_dumpOf(fixture);
}
