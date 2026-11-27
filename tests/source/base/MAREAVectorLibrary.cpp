// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAShippedLibraries.hpp"

TEST(MAREAVectorLibrary, VectorLibraryRegisterGivesAnOwnedIndexableRunOfOneParameter)
{
    GTEST_COUT << "Validating the vector library registers marea.vector.Vector as an owned, indexable run of one element type." << std::endl;

    const MAREA_ShippedRegistry SHIPPED;

    ASSERT_TRUE(MAREA_registerVectorLibrary(SHIPPED.registry)) << "marea/base/MAREAVectorLibrary.h must declare the registration of the vector library.";

    const MAREA_TypeDescriptor *DESCRIPTOR{ SHIPPED.MAREA_find("marea.vector", "Vector") };

    ASSERT_NE(DESCRIPTOR, nullptr) << "The type must be found by its namespace and name.";

    EXPECT_EQ(DESCRIPTOR->typeParameterCount, MAREA_INTEGER_SIZE(1)) << "An element type and no count.";

    EXPECT_EQ(DESCRIPTOR->roles, MAREA_TYPE_ROLE_NONE) << "A vector has no role.";

    EXPECT_EQ(DESCRIPTOR->ownershipClass, MAREA_TYPE_OWNERSHIP_CLASS_MOVE) << "A vector has one owner.";

    EXPECT_EQ(DESCRIPTOR->hasIndexProtocol, MAREA_BOOLEAN_TRUE) << "An element is reached by its index.";
}

TEST(MAREAVectorLibrary, VectorLibraryRegisterRegistersNothingWhenItsBitIsOff)
{
    GTEST_COUT << "Validating a registry with the vector bit off holds no vector type." << std::endl;

    const MAREA_ShippedRegistry SHIPPED{ static_cast<MAREA_LibraryMask>(MAREA_LIBRARY_MASK_DEFAULT & ~static_cast<MAREA_LibraryMask>(MAREA_LIBRARY_FLAG_MAREA_VECTOR)) };

    ASSERT_TRUE(MAREA_registerVectorLibrary(SHIPPED.registry)) << "A library that is off is a success that registers nothing.";

    EXPECT_EQ(SHIPPED.MAREA_find("marea.vector", "Vector"), nullptr) << "The type of a library that is off is unknown.";
}
