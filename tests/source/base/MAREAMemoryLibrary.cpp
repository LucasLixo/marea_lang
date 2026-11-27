// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAShippedLibraries.hpp"

#include <string>

TEST(MAREAMemoryLibrary, DefaultMaskTurnsOnMareaMemory)
{
    GTEST_COUT << "Validating marea.memory has a bit of its own that is on by default." << std::endl;

    EXPECT_NE(MAREA_LIBRARY_MASK_DEFAULT & static_cast<MAREA_LibraryMask>(MAREA_LIBRARY_FLAG_MAREA_MEMORY), 0U) << "marea.memory is on by default.";

    EXPECT_NE(MAREA_LIBRARY_FLAG_MAREA_MEMORY, MAREA_LIBRARY_FLAG_C_MEMORY) << "The two libraries have two bits.";
}

TEST(MAREAMemoryLibrary, MemoryLibraryRegisterGivesAnOwnerAViewAndAnArena)
{
    GTEST_COUT << "Validating the roles, the ownership class and the index protocol of the three types." << std::endl;

    const MAREA_ShippedRegistry SHIPPED;

    ASSERT_TRUE(MAREA_registerCMemoryLibrary(SHIPPED.registry) && MAREA_registerMemoryLibrary(SHIPPED.registry)) << "Both registrations must succeed.";

    const MAREA_TypeDescriptor *OWNED{ SHIPPED.MAREA_find("marea.memory", "Owned") };

    const MAREA_TypeDescriptor *SLICE{ SHIPPED.MAREA_find("marea.memory", "Slice") };

    const MAREA_TypeDescriptor *ARENA{ SHIPPED.MAREA_find("marea.memory", "Arena") };

    ASSERT_TRUE((OWNED != nullptr) && (SLICE != nullptr) && (ARENA != nullptr)) << "The three types must be found.";

    EXPECT_EQ(OWNED->typeParameterCount, MAREA_INTEGER_SIZE(1)) << "The owned type.";

    EXPECT_EQ(OWNED->roles, MAREA_TYPE_ROLE_OWNER) << "An owner is peeled to reach its value.";

    EXPECT_EQ(OWNED->ownershipClass, MAREA_TYPE_OWNERSHIP_CLASS_MOVE) << "One owner.";

    EXPECT_EQ(OWNED->hasIndexProtocol, MAREA_BOOLEAN_FALSE) << "One value, no index.";

    EXPECT_EQ(SLICE->typeParameterCount, MAREA_INTEGER_SIZE(1)) << "The element type.";

    EXPECT_NE(SLICE->roles & MAREA_TYPE_ROLE_VIEW, 0) << "A view.";

    EXPECT_EQ(SLICE->roles & (MAREA_TYPE_ROLE_OWNER | MAREA_TYPE_ROLE_RAW_POINTER), 0) << "A view owns nothing and is safe.";

    EXPECT_EQ(SLICE->ownershipClass, MAREA_TYPE_OWNERSHIP_CLASS_COPY) << "A borrow passes by value.";

    EXPECT_EQ(SLICE->hasIndexProtocol, MAREA_BOOLEAN_TRUE) << "An element is reached by its index.";

    EXPECT_EQ(ARENA->typeParameterCount, MAREA_INTEGER_SIZE(0)) << "An arena holds bytes, not one type.";

    EXPECT_EQ(ARENA->roles, MAREA_TYPE_ROLE_NONE) << "An arena has no role.";

    EXPECT_EQ(ARENA->ownershipClass, MAREA_TYPE_OWNERSHIP_CLASS_MOVE) << "One owner of the block.";

    EXPECT_EQ(ARENA->hasIndexProtocol, MAREA_BOOLEAN_FALSE) << "No index.";
}

TEST(MAREAMemoryLibrary, MemoryLibraryRegisterGivesTheFunctionsOfTheRuntime)
{
    GTEST_COUT << "Validating each runtime function of marea.memory is registered." << std::endl;

    const MAREA_ShippedRegistry SHIPPED;

    ASSERT_TRUE(MAREA_registerCMemoryLibrary(SHIPPED.registry) && MAREA_registerMemoryLibrary(SHIPPED.registry)) << "Both registrations must succeed.";

    for (const std::string &name : { std::string("runtime_owned_create"), std::string("runtime_owned_destroy"), std::string("runtime_slice_element"), std::string("runtime_arena_create"), std::string("runtime_arena_allocate"), std::string("runtime_arena_destroy") }) {
        EXPECT_NE(MAREA_TypeRegistryFindFunction(SHIPPED.registry, "marea.memory", 12, name.c_str(), name.size()), nullptr) << "The function " << name;
    }
}

TEST(MAREAMemoryLibrary, MemoryLibraryRegisterNeedsCMemory)
{
    GTEST_COUT << "Validating marea.memory on with c.memory off is a configuration error." << std::endl;

    const MAREA_ShippedRegistry SHIPPED{ static_cast<MAREA_LibraryMask>(MAREA_LIBRARY_MASK_DEFAULT & ~static_cast<MAREA_LibraryMask>(MAREA_LIBRARY_FLAG_C_MEMORY)) };

    EXPECT_EQ(MAREA_MemoryLibraryRegister(SHIPPED.registry), MAREA_TYPE_REGISTRY_EXCEPT_ERROR_MISSING_DEPENDENCY) << "The dependency is off.";

    EXPECT_EQ(SHIPPED.MAREA_find("marea.memory", "Owned"), nullptr) << "Nothing was registered.";
}

TEST(MAREAMemoryLibrary, MemoryLibraryRegisterRegistersNothingWhenItsBitIsOff)
{
    GTEST_COUT << "Validating a registry with the marea.memory bit off holds no type of it." << std::endl;

    const MAREA_ShippedRegistry SHIPPED{ static_cast<MAREA_LibraryMask>(MAREA_LIBRARY_MASK_DEFAULT & ~static_cast<MAREA_LibraryMask>(MAREA_LIBRARY_FLAG_MAREA_MEMORY)) };

    ASSERT_TRUE(MAREA_registerCMemoryLibrary(SHIPPED.registry) && MAREA_registerMemoryLibrary(SHIPPED.registry)) << "A library that is off is a success that registers nothing.";

    EXPECT_EQ(SHIPPED.MAREA_find("marea.memory", "Owned"), nullptr) << "The owner is unknown.";

    EXPECT_EQ(SHIPPED.MAREA_find("marea.memory", "Slice"), nullptr) << "The view is unknown.";

    EXPECT_EQ(SHIPPED.MAREA_find("marea.memory", "Arena"), nullptr) << "The arena is unknown.";
}
