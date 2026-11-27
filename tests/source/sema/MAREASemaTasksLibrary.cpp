// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "marea/sema/MAREASemaTypedAst.h"

#include "tests/MAREAContainerHelpers.hpp"
#include "tests/MAREASemaHelpers.hpp"
#include "tests/MAREAShippedLibraries.hpp"

#include "marea/base/MAREACThreadsLibrary.h"
#include "marea/base/MAREATasksLibrary.h"

namespace
{
    const MAREA_SemaTypedNode *MAREA_findTypedNodeByKind(const MAREA_SemaTypedNode *node, MAREA_AstKind kind)
    {
        if (node == nullptr) {
            return nullptr;
        }

        if (MAREA_AstNodeGetKind(MAREA_SemaTypedNodeGetAstNode(node)) == kind) {
            return node;
        }

        const MAREA_IntegerSize COUNT{ MAREA_SemaTypedNodeGetChildCount(node) };

        for (MAREA_IntegerSize index{ 0 }; index < COUNT; ++index) {
            const MAREA_SemaTypedNode *child{ nullptr };

            if (MAREA_SemaTypedNodeGetChildAt(node, index, &child) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
                continue;
            }

            const MAREA_SemaTypedNode *found{ MAREA_findTypedNodeByKind(child, kind) };

            if (found != nullptr) {
                return found;
            }
        }

        return nullptr;
    }
} // namespace

TEST(MAREASemaTasksLibrary, SpawnCallResolvesToTheRealMareaTasksTask)
{
    GTEST_COUT << "Validating a spawn call's resolved type becomes marea.tasks.Task<T> once marea.tasks is "
                  "registered with the spawn handle role - the real shipped library, not a throwaway fixture."
               << std::endl;

    const MAREA_ShippedRegistry SHIPPED;

    ASSERT_EQ(MAREA_CThreadsLibraryRegister(SHIPPED.registry), MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) << "c.threads must register first, since marea.tasks depends on it.";

    ASSERT_EQ(MAREA_TasksLibraryRegister(SHIPPED.registry), MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) << "marea.tasks must register.";

    MAREA_SemaFixture fixture(MAREA_containerResolver());

    ASSERT_EQ(MAREA_SemaContextSetTypeRegistry(fixture.context, SHIPPED.registry), MAREA_SEMA_EXCEPT_SUCCESS);

    const std::string SOURCE{ "namespace main; require c.threads; require marea.tasks; function work(a: float32): float32 return a; end function f(a: float32) fork_join local t: marea.tasks.Task<float32> = spawn work(a); end end" };

    ASSERT_EQ(fixture.MAREA_sema(SOURCE), MAREA_SEMA_EXCEPT_SUCCESS) << "The source must check with no error.";

    ASSERT_EQ(fixture.MAREA_errorCount(), MAREA_INTEGER_SIZE(0)) << "No diagnostic is expected.";

    const MAREA_SemaTypedNode *root{ nullptr };

    ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &root), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS);

    const MAREA_SemaTypedNode *spawn{ MAREA_findTypedNodeByKind(root, MAREA_AST_KIND_UNARY_EXPRESSION) };

    ASSERT_NE(spawn, nullptr) << "The spawn expression must be in the typed tree.";

    const MAREA_Character8 *text{ nullptr };

    MAREA_IntegerSize length{ 0 };

    ASSERT_EQ(MAREA_SemaTypedNodeGetResolvedType(spawn, &text, &length), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS);

    EXPECT_EQ(std::string(text, length), "marea.tasks.Task<float32>") << "The spawn handle role now points at the real marea.tasks.Task, not the pre-decision c.threads.Task.";
}
