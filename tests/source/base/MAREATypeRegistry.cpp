// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include <cstring>

namespace
{
    struct MAREA_RegistryFixture
    {
        MAREA_Arena *arena{ nullptr };
        MAREA_TypeRegistry *registry{ nullptr };

        explicit MAREA_RegistryFixture(MAREA_LibraryMask mask = MAREA_LIBRARY_MASK_DEFAULT)
        {
            MAREA_IGNORE(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(0), &arena));
            MAREA_IGNORE(MAREA_TypeRegistryCreate(arena, mask, &registry));
        }

        MAREA_RegistryFixture(const MAREA_RegistryFixture &) = delete;
        MAREA_RegistryFixture &operator=(const MAREA_RegistryFixture &) = delete;

        ~MAREA_RegistryFixture()
        {
            MAREA_ArenaDestroy(arena);
        }
    };

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

    void MAREA_hostFunction()
    {
    }

} // namespace

TEST(MAREATypeRegistry, CreateAndGetEnabledMaskRoundTrip)
{
    GTEST_COUT << "Validating MAREA_TypeRegistryCreate stores the enabled mask it is given." << std::endl;

    MAREA_RegistryFixture fixture(MAREA_LIBRARY_FLAG_MAREA_ARRAY | MAREA_LIBRARY_FLAG_C_MEMORY);

    EXPECT_EQ(MAREA_TypeRegistryGetEnabledMask(fixture.registry), (MAREA_LibraryMask)(MAREA_LIBRARY_FLAG_MAREA_ARRAY | MAREA_LIBRARY_FLAG_C_MEMORY)) << "The mask given to create must round trip.";
}

TEST(MAREATypeRegistry, CreateRejectsNull)
{
    GTEST_COUT << "Validating MAREA_TypeRegistryCreate rejects null arguments." << std::endl;

    MAREA_Arena *arena{ nullptr };

    ASSERT_EQ(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(0), &arena), MAREA_ARENA_EXCEPT_SUCCESS) << "MAREA_ArenaCreate must succeed.";

    MAREA_TypeRegistry *registry{ nullptr };

    EXPECT_EQ(MAREA_TypeRegistryCreate(nullptr, MAREA_LIBRARY_MASK_DEFAULT, &registry), MAREA_TYPE_REGISTRY_EXCEPT_ERROR_NULL_POINTER) << "A null arena must be rejected.";

    EXPECT_EQ(MAREA_TypeRegistryCreate(arena, MAREA_LIBRARY_MASK_DEFAULT, nullptr), MAREA_TYPE_REGISTRY_EXCEPT_ERROR_NULL_POINTER) << "A null outRegistry must be rejected.";

    MAREA_ArenaDestroy(arena);
}

TEST(MAREATypeRegistry, InternalEntryRegistersAndFindsUnderAReservedRoot)
{
    GTEST_COUT << "Validating a shipped library registers a type under c or marea through the internal entry." << std::endl;

    MAREA_RegistryFixture fixture;

    const MAREA_TypeDescriptor DESCRIPTOR = MAREA_descriptorOf("c.memory", "Pointer", MAREA_TYPE_ROLE_RAW_POINTER, MAREA_TYPE_OWNERSHIP_CLASS_COPY, MAREA_INTEGER_SIZE(1), MAREA_BOOLEAN_FALSE);

    ASSERT_EQ(MAREA_TypeRegistryRegisterInternal(fixture.registry, MAREA_LIBRARY_FLAG_C_MEMORY, &DESCRIPTOR), MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) << "A shipped library must register through the internal entry.";

    const MAREA_TypeDescriptor *found = MAREA_TypeRegistryFind(fixture.registry, "c.memory", MAREA_INTEGER_SIZE(8), "Pointer", MAREA_INTEGER_SIZE(7));

    ASSERT_NE(found, nullptr) << "A registered type must be found by its namespace and its name.";

    EXPECT_EQ(found->roles, MAREA_TYPE_ROLE_RAW_POINTER) << "The found descriptor must carry the role it was registered with.";
}

TEST(MAREATypeRegistry, FindByRoleReturnsTheFirstTypeThatHoldsTheRole)
{
    GTEST_COUT << "Validating MAREA_TypeRegistryFindByRole gives the registered type that holds a role, and null for a role no type holds." << std::endl;

    MAREA_RegistryFixture fixture;

    const MAREA_TypeDescriptor TASK = MAREA_descriptorOf("c.threads", "Handle", MAREA_TYPE_ROLE_SPAWN_HANDLE, MAREA_TYPE_OWNERSHIP_CLASS_MOVE, MAREA_INTEGER_SIZE(1), MAREA_BOOLEAN_FALSE);

    ASSERT_EQ(MAREA_TypeRegistryRegisterInternal(fixture.registry, MAREA_LIBRARY_FLAG_C_THREADS, &TASK), MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) << "The internal entry must register the fixture.";

    EXPECT_EQ(MAREA_TypeRegistryFindByRole(fixture.registry, MAREA_TYPE_ROLE_SPAWN_HANDLE), &TASK) << "The spawn handle role must find the registered type.";

    EXPECT_EQ(MAREA_TypeRegistryFindByRole(fixture.registry, MAREA_TYPE_ROLE_OWNER), nullptr) << "A role no type holds must find nothing.";

    EXPECT_EQ(MAREA_TypeRegistryFindByRole(nullptr, MAREA_TYPE_ROLE_OWNER), nullptr) << "A null registry must find nothing.";
}

TEST(MAREATypeRegistry, FindReturnsNullForAnUnregisteredType)
{
    GTEST_COUT << "Validating MAREA_TypeRegistryFind returns null for a name nothing registered." << std::endl;

    MAREA_RegistryFixture fixture;

    EXPECT_EQ(MAREA_TypeRegistryFind(fixture.registry, "demo.memory", MAREA_INTEGER_SIZE(11), "Nothing", MAREA_INTEGER_SIZE(7)), nullptr) << "An unregistered name must answer null.";
}

TEST(MAREATypeRegistry, PublicEntryRegistersUnderAnOrdinaryNamespace)
{
    GTEST_COUT << "Validating a user or a host registers a type under an ordinary root through the public entry." << std::endl;

    MAREA_RegistryFixture fixture;

    const MAREA_TypeDescriptor DESCRIPTOR = MAREA_descriptorOf("host.graphics", "Texture", MAREA_TYPE_ROLE_NONE, MAREA_TYPE_OWNERSHIP_CLASS_MOVE, MAREA_INTEGER_SIZE(0), MAREA_BOOLEAN_FALSE);

    ASSERT_EQ(MAREA_TypeRegistryRegisterPublic(fixture.registry, &DESCRIPTOR), MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) << "A public registration under an ordinary root must succeed.";

    EXPECT_NE(MAREA_TypeRegistryFind(fixture.registry, "host.graphics", MAREA_INTEGER_SIZE(13), "Texture", MAREA_INTEGER_SIZE(7)), nullptr) << "The registered type must be found.";
}

TEST(MAREATypeRegistry, PublicEntryRejectsTheReservedRootC)
{
    GTEST_COUT << "Validating the public entry rejects a registration whose namespace starts with c." << std::endl;

    MAREA_RegistryFixture fixture;

    const MAREA_TypeDescriptor DESCRIPTOR = MAREA_descriptorOf("c.graphics", "Texture", MAREA_TYPE_ROLE_NONE, MAREA_TYPE_OWNERSHIP_CLASS_MOVE, MAREA_INTEGER_SIZE(0), MAREA_BOOLEAN_FALSE);

    EXPECT_EQ(MAREA_TypeRegistryRegisterPublic(fixture.registry, &DESCRIPTOR), MAREA_TYPE_REGISTRY_EXCEPT_ERROR_RESERVED_NAMESPACE_ROOT) << "A public registration under c must be rejected.";
}

TEST(MAREATypeRegistry, PublicEntryRejectsTheReservedRootMarea)
{
    GTEST_COUT << "Validating the public entry rejects a registration whose namespace starts with marea." << std::endl;

    MAREA_RegistryFixture fixture;

    const MAREA_TypeDescriptor DESCRIPTOR = MAREA_descriptorOf("marea.graphics", "Texture", MAREA_TYPE_ROLE_NONE, MAREA_TYPE_OWNERSHIP_CLASS_MOVE, MAREA_INTEGER_SIZE(0), MAREA_BOOLEAN_FALSE);

    EXPECT_EQ(MAREA_TypeRegistryRegisterPublic(fixture.registry, &DESCRIPTOR), MAREA_TYPE_REGISTRY_EXCEPT_ERROR_RESERVED_NAMESPACE_ROOT) << "A public registration under marea must be rejected.";
}

TEST(MAREATypeRegistry, LibraryFlagsCombineWithBitwiseOr)
{
    GTEST_COUT << "Validating every MAREA_LibraryFlag bit is distinct and combines with bitwise or." << std::endl;

    const MAREA_LibraryMask COMBINED = (MAREA_LibraryMask)(MAREA_LIBRARY_FLAG_MAREA_ARRAY | MAREA_LIBRARY_FLAG_C_THREADS);

    EXPECT_NE((COMBINED & MAREA_LIBRARY_FLAG_MAREA_ARRAY), MAREA_UINTEGER_32(0)) << "The combined mask must keep the first bit.";

    EXPECT_NE((COMBINED & MAREA_LIBRARY_FLAG_C_THREADS), MAREA_UINTEGER_32(0)) << "The combined mask must keep the second bit.";

    EXPECT_EQ((COMBINED & MAREA_LIBRARY_FLAG_MAREA_VECTOR), MAREA_UINTEGER_32(0)) << "A bit that was not combined must stay off.";
}

TEST(MAREATypeRegistry, DefaultMaskTurnsOnEveryShippedLibrary)
{
    GTEST_COUT << "Validating MAREA_LIBRARY_MASK_DEFAULT turns on every shipped library." << std::endl;

    MAREA_RegistryFixture fixture;

    const MAREA_LibraryMask MASK = MAREA_TypeRegistryGetEnabledMask(fixture.registry);

    EXPECT_NE((MASK & MAREA_LIBRARY_FLAG_MAREA_ARRAY), MAREA_UINTEGER_32(0)) << "marea.array must be on by default.";
    EXPECT_NE((MASK & MAREA_LIBRARY_FLAG_MAREA_VECTOR), MAREA_UINTEGER_32(0)) << "marea.vector must be on by default.";
    EXPECT_NE((MASK & MAREA_LIBRARY_FLAG_MAREA_TUPLE), MAREA_UINTEGER_32(0)) << "marea.tuple must be on by default.";
    EXPECT_NE((MASK & MAREA_LIBRARY_FLAG_C_MEMORY), MAREA_UINTEGER_32(0)) << "c.memory must be on by default.";
    EXPECT_NE((MASK & MAREA_LIBRARY_FLAG_C_THREADS), MAREA_UINTEGER_32(0)) << "c.threads must be on by default.";
}

TEST(MAREATypeRegistry, ALibraryThatIsOffRegistersNothing)
{
    GTEST_COUT << "Validating a library whose bit is off registers no type, so its name stays unknown." << std::endl;

    MAREA_RegistryFixture fixture(MAREA_LIBRARY_FLAG_C_MEMORY);

    const MAREA_TypeDescriptor DESCRIPTOR = MAREA_descriptorOf("marea.array", "Array", MAREA_TYPE_ROLE_NONE, MAREA_TYPE_OWNERSHIP_CLASS_COPY, MAREA_INTEGER_SIZE(2), MAREA_BOOLEAN_TRUE);

    ASSERT_EQ(MAREA_TypeRegistryRegisterInternal(fixture.registry, MAREA_LIBRARY_FLAG_MAREA_ARRAY, &DESCRIPTOR), MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) << "Registering an off library must still succeed (a no-op), never an error.";

    EXPECT_EQ(MAREA_TypeRegistryFind(fixture.registry, "marea.array", MAREA_INTEGER_SIZE(11), "Array", MAREA_INTEGER_SIZE(5)), nullptr) << "An off library must register nothing, so its type stays unknown.";
}

TEST(MAREATypeRegistry, EnablingALibraryWithoutItsDependencyIsAConfigurationError)
{
    GTEST_COUT << "Validating a library enabled without a library it depends on is a configuration error, never a silent pass." << std::endl;

    MAREA_RegistryFixture fixture(MAREA_LIBRARY_FLAG_MAREA_VECTOR);

    ASSERT_EQ(MAREA_TypeRegistryDeclareDependency(fixture.registry, MAREA_LIBRARY_FLAG_MAREA_VECTOR, MAREA_LIBRARY_FLAG_MAREA_ARRAY), MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) << "Declaring a dependency must succeed.";

    const MAREA_TypeDescriptor DESCRIPTOR = MAREA_descriptorOf("marea.vector", "Vector", MAREA_TYPE_ROLE_NONE, MAREA_TYPE_OWNERSHIP_CLASS_MOVE, MAREA_INTEGER_SIZE(1), MAREA_BOOLEAN_TRUE);

    EXPECT_EQ(MAREA_TypeRegistryRegisterInternal(fixture.registry, MAREA_LIBRARY_FLAG_MAREA_VECTOR, &DESCRIPTOR), MAREA_TYPE_REGISTRY_EXCEPT_ERROR_MISSING_DEPENDENCY) << "marea.vector is on but marea.array, its dependency, is off: this must be a configuration error.";
}

TEST(MAREATypeRegistry, RegisterFunctionMacroRegistersThroughTheInternalEntry)
{
    GTEST_COUT << "Validating MAREA_REGISTER_FUNCTION registers a C function through the internal entry when owner names a shipped library." << std::endl;

    MAREA_RegistryFixture fixture;

    ASSERT_EQ(MAREA_REGISTER_FUNCTION(fixture.registry, MAREA_LIBRARY_FLAG_C_THREADS, "c.threads", MAREA_INTEGER_SIZE(9), "yield", MAREA_INTEGER_SIZE(5), &MAREA_hostFunction), MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) << "The macro must register the function.";

    EXPECT_EQ(MAREA_TypeRegistryFindFunction(fixture.registry, "c.threads", MAREA_INTEGER_SIZE(9), "yield", MAREA_INTEGER_SIZE(5)), reinterpret_cast<MAREA_Null>(&MAREA_hostFunction)) << "The registered function must be found.";
}

TEST(MAREATypeRegistry, RegisterFunctionMacroRejectsAReservedRootForAUserOrAHost)
{
    GTEST_COUT << "Validating MAREA_REGISTER_FUNCTION rejects a user or a host registration under a reserved root." << std::endl;

    MAREA_RegistryFixture fixture;

    EXPECT_EQ(MAREA_REGISTER_FUNCTION(fixture.registry, MAREA_LIBRARY_FLAG_NONE, "c.threads", MAREA_INTEGER_SIZE(9), "yield", MAREA_INTEGER_SIZE(5), &MAREA_hostFunction), MAREA_TYPE_REGISTRY_EXCEPT_ERROR_RESERVED_NAMESPACE_ROOT) << "owner NONE means a user or a host; it must not register under c.threads.";
}

TEST(MAREATypeRegistry, AHostMayReplaceARuntimeFunction)
{
    GTEST_COUT << "Validating a host may replace a runtime function such as alloc." << std::endl;

    MAREA_RegistryFixture fixture;

    ASSERT_EQ(MAREA_TypeRegistryReplaceRuntimeFunction(fixture.registry, "alloc", MAREA_INTEGER_SIZE(5), reinterpret_cast<MAREA_Null>(&MAREA_hostFunction)), MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) << "Replacing alloc must succeed.";

    EXPECT_EQ(MAREA_TypeRegistryFindRuntimeFunction(fixture.registry, "alloc", MAREA_INTEGER_SIZE(5)), reinterpret_cast<MAREA_Null>(&MAREA_hostFunction)) << "The replacement must be found back.";
}

TEST(MAREATypeRegistry, ReplacingAnUnknownRuntimeFunctionIsRejected)
{
    GTEST_COUT << "Validating a host cannot replace a name that is not a runtime hook the registry declares." << std::endl;

    MAREA_RegistryFixture fixture;

    EXPECT_EQ(MAREA_TypeRegistryReplaceRuntimeFunction(fixture.registry, "not_a_hook", MAREA_INTEGER_SIZE(10), reinterpret_cast<MAREA_Null>(&MAREA_hostFunction)), MAREA_TYPE_REGISTRY_EXCEPT_ERROR_UNKNOWN_RUNTIME_FUNCTION) << "A name that is not a declared runtime hook must be rejected.";
}
