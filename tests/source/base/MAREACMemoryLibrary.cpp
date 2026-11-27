// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAShippedLibraries.hpp"

#include <string>

TEST(MAREACMemoryLibrary, CMemoryLibraryRegisterGivesARawPointerHeldByCopy)
{
    GTEST_COUT << "Validating the library registers c.memory.Pointer with the raw pointer role." << std::endl;

    const MAREA_ShippedRegistry SHIPPED;

    ASSERT_TRUE(MAREA_registerCMemoryLibrary(SHIPPED.registry)) << "The registration must succeed.";

    const MAREA_TypeDescriptor *DESCRIPTOR{ SHIPPED.MAREA_find("c.memory", "Pointer") };

    ASSERT_NE(DESCRIPTOR, nullptr) << "The type must be found by its namespace and name.";

    EXPECT_EQ(DESCRIPTOR->typeParameterCount, MAREA_INTEGER_SIZE(1)) << "The pointee type.";

    EXPECT_EQ(DESCRIPTOR->roles, MAREA_TYPE_ROLE_RAW_POINTER) << "Legal only inside unsafe.";

    EXPECT_EQ(DESCRIPTOR->ownershipClass, MAREA_TYPE_OWNERSHIP_CLASS_COPY) << "An address is copied and never owns.";

    EXPECT_EQ(DESCRIPTOR->hasIndexProtocol, MAREA_BOOLEAN_FALSE) << "No element is reached by index.";

    EXPECT_EQ(SHIPPED.MAREA_find("c.memory", "Owned"), nullptr) << "The owner belongs to marea.memory.";

    EXPECT_EQ(SHIPPED.MAREA_find("c.memory", "Slice"), nullptr) << "The view belongs to marea.memory.";
}

TEST(MAREACMemoryLibrary, CMemoryLibraryRegisterGivesTheFunctionsOfTheRuntime)
{
    GTEST_COUT << "Validating each function of c.memory is registered for the runtime." << std::endl;

    const MAREA_ShippedRegistry SHIPPED;

    ASSERT_TRUE(MAREA_registerCMemoryLibrary(SHIPPED.registry)) << "The registration must succeed.";

    for (const std::string &name : { std::string("alloc"), std::string("free"), std::string("copy_bytes"), std::string("fill_bytes"), std::string("offset"), std::string("is_null") }) {
        EXPECT_NE(MAREA_TypeRegistryFindFunction(SHIPPED.registry, "c.memory", 8, name.c_str(), name.size()), nullptr) << "The function " << name;
    }
}

TEST(MAREACMemoryLibrary, CMemoryLibraryRegisterRegistersNothingWhenItsBitIsOff)
{
    GTEST_COUT << "Validating a registry with the c.memory bit off holds no type and no function of it." << std::endl;

    const MAREA_ShippedRegistry SHIPPED{ static_cast<MAREA_LibraryMask>(MAREA_LIBRARY_MASK_DEFAULT & ~static_cast<MAREA_LibraryMask>(MAREA_LIBRARY_FLAG_C_MEMORY)) };

    ASSERT_TRUE(MAREA_registerCMemoryLibrary(SHIPPED.registry)) << "A library that is off is a success that registers nothing.";

    EXPECT_EQ(SHIPPED.MAREA_find("c.memory", "Pointer"), nullptr) << "The type of a library that is off is unknown.";

    EXPECT_EQ(MAREA_TypeRegistryFindFunction(SHIPPED.registry, "c.memory", 8, "alloc", 5), nullptr) << "So is its function.";
}
