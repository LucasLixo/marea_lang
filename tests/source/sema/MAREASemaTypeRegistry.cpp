// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "marea/sema/MAREASemaTypedAst.h"

#include "tests/MAREASemaHelpers.hpp"

#include <cstring>
#include <string>
#include <vector>

namespace
{
    constexpr MAREA_Uinteger32 RESERVED_ROOT{ MAREA_SEMA_DIAGNOSTIC_CODE_RESERVED_NAMESPACE_ROOT };
    constexpr MAREA_Uinteger32 RAW_POINTER_OUTSIDE_UNSAFE{ MAREA_SEMA_DIAGNOSTIC_CODE_RAW_POINTER_OUTSIDE_UNSAFE };
    constexpr MAREA_Uinteger32 ELEMENT_WITHOUT_LAYOUT{ MAREA_SEMA_DIAGNOSTIC_CODE_ELEMENT_WITHOUT_MEASURED_LAYOUT };

    // The registry of a host: the root demo exists, with any namespace below it, exactly the neutral
    // fixture convention the sibling apply (sema-import-require-full-path) already uses.
    MAREA_SemaImportResult MAREA_demoRegistered(const std::string &text)
    {
        return ((text == "demo") || (text.rfind("demo.", 0) == 0)) ? MAREA_SEMA_IMPORT_RESULT_EXTERNAL : MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
    }

    MAREA_ResolveCallback MAREA_demoResolver()
    {
        return [](const std::string &text, const MAREA_AstNode **) { return MAREA_demoRegistered(text); };
    }

    MAREA_TypeDescriptor MAREA_descriptorOf(const char *namespaceText, const char *name, MAREA_TypeRole roles, MAREA_TypeOwnershipClass ownershipClass, MAREA_IntegerSize typeParameterCount, MAREA_Boolean hasIndexProtocol)
    {
        return MAREA_TypeDescriptor{
            .namespaceText = namespaceText,
            .namespaceLength = static_cast<MAREA_IntegerSize>(std::strlen(namespaceText)),
            .name = name,
            .nameLength = static_cast<MAREA_IntegerSize>(std::strlen(name)),
            .typeParameterCount = typeParameterCount,
            .roles = roles,
            .ownershipClass = ownershipClass,
            .hasIndexProtocol = hasIndexProtocol,
        };
    }

    // One registry with one fixture descriptor per role this apply defines: a raw pointer, an owner, a
    // view (also the partition part role, the way the old slice filled both at once) and a spawn
    // handle. None of the six removed built-in names appears anywhere in this file.
    struct MAREA_RoleFixtures
    {
        MAREA_Arena *arena{ nullptr };
        MAREA_TypeRegistry *registry{ nullptr };
        MAREA_TypeDescriptor rawPointer{ MAREA_descriptorOf("demo.memory", "Handle", MAREA_TYPE_ROLE_RAW_POINTER, MAREA_TYPE_OWNERSHIP_CLASS_COPY, MAREA_INTEGER_SIZE(1), MAREA_BOOLEAN_FALSE) };
        MAREA_TypeDescriptor owner{ MAREA_descriptorOf("demo.memory", "Box", MAREA_TYPE_ROLE_OWNER, MAREA_TYPE_OWNERSHIP_CLASS_MOVE, MAREA_INTEGER_SIZE(1), MAREA_BOOLEAN_FALSE) };
        MAREA_TypeDescriptor view{ MAREA_descriptorOf("demo.memory", "Window", (MAREA_TypeRole)(MAREA_TYPE_ROLE_VIEW | MAREA_TYPE_ROLE_PARTITION_PART), MAREA_TYPE_OWNERSHIP_CLASS_COPY, MAREA_INTEGER_SIZE(1), MAREA_BOOLEAN_TRUE) };
        MAREA_TypeDescriptor spawnHandle{ MAREA_descriptorOf("demo.threads", "Future", MAREA_TYPE_ROLE_SPAWN_HANDLE, MAREA_TYPE_OWNERSHIP_CLASS_MOVE, MAREA_INTEGER_SIZE(1), MAREA_BOOLEAN_FALSE) };

        MAREA_RoleFixtures()
        {
            MAREA_IGNORE(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(0), &arena));
            MAREA_IGNORE(MAREA_TypeRegistryCreate(arena, MAREA_LIBRARY_MASK_DEFAULT, &registry));
            MAREA_IGNORE(MAREA_TypeRegistryRegisterPublic(registry, &rawPointer));
            MAREA_IGNORE(MAREA_TypeRegistryRegisterPublic(registry, &owner));
            MAREA_IGNORE(MAREA_TypeRegistryRegisterPublic(registry, &view));
            MAREA_IGNORE(MAREA_TypeRegistryRegisterPublic(registry, &spawnHandle));
        }

        MAREA_RoleFixtures(const MAREA_RoleFixtures &) = delete;
        MAREA_RoleFixtures &operator=(const MAREA_RoleFixtures &) = delete;

        ~MAREA_RoleFixtures()
        {
            MAREA_ArenaDestroy(arena);
        }
    };

    bool MAREA_contains(const std::vector<MAREA_Uinteger32> &codes, MAREA_Uinteger32 code)
    {
        for (MAREA_Uinteger32 entry : codes) {
            if (entry == code) {
                return true;
            }
        }

        return false;
    }

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

    // One fixture of one checked file, with the role registry attached to its context.
    struct MAREA_RoleFixture
    {
        MAREA_RoleFixtures roles;
        MAREA_SemaFixture sema;

        MAREA_RoleFixture()
            : sema(MAREA_demoResolver())
        {
            MAREA_IGNORE(MAREA_SemaContextSetTypeRegistry(sema.context, roles.registry));
        }
    };

} // namespace

TEST(MAREASemaTypeRegistry, ARawPointerRoleLocalOutsideUnsafeIsRejected)
{
    GTEST_COUT << "Validating a registered type with the raw pointer role is rejected outside unsafe, exactly as the old pointer<T> was." << std::endl;

    MAREA_RoleFixture fixture;

    ASSERT_EQ(fixture.sema.MAREA_sema("namespace main; require demo.memory; function f(): sinteger32 local p: demo.memory.Handle<sinteger32>; return 0; end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must run.";

    EXPECT_TRUE(MAREA_contains(fixture.sema.MAREA_semaCodes(), RAW_POINTER_OUTSIDE_UNSAFE)) << "A raw-pointer-role local outside unsafe must raise RAW_POINTER_OUTSIDE_UNSAFE, the same as the old pointer<T> local.";
}

TEST(MAREASemaTypeRegistry, ARawPointerRoleLocalInsideUnsafeIsAccepted)
{
    GTEST_COUT << "Validating a registered type with the raw pointer role is accepted inside unsafe." << std::endl;

    MAREA_RoleFixture fixture;

    ASSERT_EQ(fixture.sema.MAREA_sema("namespace main; require demo.memory; function f(): sinteger32 unsafe local p: demo.memory.Handle<sinteger32>; end return 0; end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must run.";

    EXPECT_FALSE(MAREA_contains(fixture.sema.MAREA_semaCodes(), RAW_POINTER_OUTSIDE_UNSAFE)) << "A raw-pointer-role local inside unsafe must raise nothing.";
}

TEST(MAREASemaTypeRegistry, AnOwnerRoleTypeClassifiesAsMoveBuiltin)
{
    GTEST_COUT << "Validating a registered type with the owner role classifies MOVE_BUILTIN by its role, never by its name." << std::endl;

    MAREA_RoleFixture fixture;

    ASSERT_EQ(fixture.sema.MAREA_sema("namespace main; require demo.memory; function f(n: demo.memory.Box<sinteger32>): demo.memory.Box<sinteger32> return move n; end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must run.";

    const MAREA_SemaTypedNode *outRoot{ nullptr };

    ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.sema.context, fixture.sema.parser.root, &outRoot), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The typed tree must build.";

    const MAREA_SemaTypedNode *moveExpression{ MAREA_findTypedNodeByKind(outRoot, MAREA_AST_KIND_UNARY_EXPRESSION) };

    ASSERT_NE(moveExpression, nullptr) << "The typed tree must carry the move expression node.";

    const MAREA_SemaTypedNode *returned{ nullptr };

    ASSERT_EQ(MAREA_SemaTypedNodeGetChildAt(moveExpression, 0, &returned), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The moved identifier must be reachable.";

    EXPECT_EQ(MAREA_SemaTypedNodeGetTypeClass(returned), MAREA_SEMA_TYPE_CLASS_MOVE_BUILTIN) << "A registered owner-role parameter must classify MOVE_BUILTIN, the same as the old owned<T>.";
}

TEST(MAREASemaTypeRegistry, AViewRoleTypeClassifiesAsCopyBuiltin)
{
    GTEST_COUT << "Validating a registered type with the view and the partition part roles classifies COPY_BUILTIN by its role, never by its name." << std::endl;

    MAREA_RoleFixture fixture;

    ASSERT_EQ(fixture.sema.MAREA_sema("namespace main; require demo.memory; function f(n: demo.memory.Window<sinteger32>): demo.memory.Window<sinteger32> return copy n; end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must run.";

    const MAREA_SemaTypedNode *outRoot{ nullptr };

    ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.sema.context, fixture.sema.parser.root, &outRoot), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The typed tree must build.";

    const MAREA_SemaTypedNode *copyExpression{ MAREA_findTypedNodeByKind(outRoot, MAREA_AST_KIND_UNARY_EXPRESSION) };

    ASSERT_NE(copyExpression, nullptr) << "The typed tree must carry the copy expression node.";

    const MAREA_SemaTypedNode *returned{ nullptr };

    ASSERT_EQ(MAREA_SemaTypedNodeGetChildAt(copyExpression, 0, &returned), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The copied identifier must be reachable.";

    EXPECT_EQ(MAREA_SemaTypedNodeGetTypeClass(returned), MAREA_SEMA_TYPE_CLASS_COPY_BUILTIN) << "A registered view-role parameter must classify COPY_BUILTIN, the same as the old slice<T>.";
}

TEST(MAREASemaTypeRegistry, ASpawnHandleRoleTypeClassifiesAsMoveBuiltin)
{
    GTEST_COUT << "Validating a registered type with the spawn handle role classifies MOVE_BUILTIN by its role, never by its name." << std::endl;

    MAREA_RoleFixture fixture;

    ASSERT_EQ(fixture.sema.MAREA_sema("namespace main; require demo.threads; function f(n: demo.threads.Future<sinteger32>): demo.threads.Future<sinteger32> return move n; end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must run.";

    const MAREA_SemaTypedNode *outRoot{ nullptr };

    ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.sema.context, fixture.sema.parser.root, &outRoot), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The typed tree must build.";

    const MAREA_SemaTypedNode *moveExpression{ MAREA_findTypedNodeByKind(outRoot, MAREA_AST_KIND_UNARY_EXPRESSION) };

    ASSERT_NE(moveExpression, nullptr) << "The typed tree must carry the move expression node.";

    const MAREA_SemaTypedNode *returned{ nullptr };

    ASSERT_EQ(MAREA_SemaTypedNodeGetChildAt(moveExpression, 0, &returned), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The moved identifier must be reachable.";

    EXPECT_EQ(MAREA_SemaTypedNodeGetTypeClass(returned), MAREA_SEMA_TYPE_CLASS_MOVE_BUILTIN) << "A registered spawn-handle-role parameter must classify MOVE_BUILTIN, the same as the old task<T>.";
}

TEST(MAREASemaTypeRegistry, AnElementHeldByValueWithNoMeasuredLayoutIsRejected)
{
    GTEST_COUT << "Validating an element type with no measured layout held by value is rejected (decision 28, file 14_UB_C_BORDER.md)." << std::endl;

    MAREA_RoleFixtures roles;

    const MAREA_TypeDescriptor INLINE_HOLDER = MAREA_descriptorOf("demo.memory", "Inline", MAREA_TYPE_ROLE_NONE, MAREA_TYPE_OWNERSHIP_CLASS_COPY, MAREA_INTEGER_SIZE(1), MAREA_BOOLEAN_TRUE);

    ASSERT_EQ(MAREA_TypeRegistryRegisterPublic(roles.registry, &INLINE_HOLDER), MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) << "Registering the fixture must succeed.";

    MAREA_SemaFixture sema(MAREA_demoResolver());

    MAREA_IGNORE(MAREA_SemaContextSetTypeRegistry(sema.context, roles.registry));

    // INFO: a function type has no measured layout (MAREA_SEMA_MEASURE_REASON_FUNCTION_TYPE), so holding
    // it by value inside an element-by-value registered type must be rejected.
    ASSERT_EQ(sema.MAREA_sema("namespace main; require demo.memory; function f(n: demo.memory.Inline<function(): sinteger32>): sinteger32 return 0; end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must run.";

    EXPECT_TRUE(MAREA_contains(sema.MAREA_semaCodes(), ELEMENT_WITHOUT_LAYOUT)) << "An element with no measured layout held by value must raise ELEMENT_WITHOUT_MEASURED_LAYOUT.";
}

TEST(MAREASemaTypeRegistry, APublicRegistrationUsedByAProjectFileStillRaisesReservedNamespaceRoot)
{
    GTEST_COUT << "Validating RESERVED_NAMESPACE_ROOT fires unconditionally for a project file, even when the registry knows the name (the 2026-11-25 decision update, replacing the old conditional reading)." << std::endl;

    MAREA_SemaFixture fixture(MAREA_demoResolver());

    // INFO: "demo" is not reserved; "marea" is. The registry knowing "marea.math" must make no
    // difference: only the shipped libraries, through the internal entry, ever reach that root.
    MAREA_SemaImportResolver *registryResolver{ nullptr };

    const auto REGISTERED_RESOLVE = [](const std::string &text, const MAREA_AstNode **) {
        return ((text == "marea") || (text.rfind("marea.", 0) == 0)) ? MAREA_SEMA_IMPORT_RESULT_EXTERNAL : MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
    };

    MAREA_ResolveCallback registeredCallback{ REGISTERED_RESOLVE };

    ASSERT_EQ(MAREA_SemaImportResolverCreate(fixture.parser.lexer.arena, &MAREA_resolveThunk, &registeredCallback, &registryResolver), MAREA_SEMA_IMPORT_RESOLVER_EXCEPT_SUCCESS) << "Creating the registry resolver must succeed.";

    ASSERT_EQ(MAREA_SemaContextSetRequireResolver(fixture.context, registryResolver), MAREA_SEMA_EXCEPT_SUCCESS) << "Setting the require resolver must succeed.";

    ASSERT_EQ(fixture.MAREA_sema("namespace marea.math;", "marea/math.marea"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must run.";

    EXPECT_TRUE(MAREA_contains(fixture.MAREA_semaCodes(), RESERVED_ROOT)) << "A project file under marea must raise RESERVED_NAMESPACE_ROOT even when the registry knows the name.";
}
