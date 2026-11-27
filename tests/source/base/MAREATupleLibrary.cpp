// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAShippedLibraries.hpp"

TEST(MAREATupleLibrary, TupleLibraryRegisterGivesTheTupleType)
{
    GTEST_COUT << "Validating the tuple library registers marea.tuple.Tuple with no role." << std::endl;

    const MAREA_ShippedRegistry SHIPPED;

    ASSERT_TRUE(MAREA_registerTupleLibrary(SHIPPED.registry)) << "marea/base/MAREATupleLibrary.h must declare the registration of the tuple library.";

    const MAREA_TypeDescriptor *DESCRIPTOR{ SHIPPED.MAREA_find("marea.tuple", "Tuple") };

    ASSERT_NE(DESCRIPTOR, nullptr) << "The type must be found by its namespace and name.";

    EXPECT_EQ(DESCRIPTOR->roles, MAREA_TYPE_ROLE_NONE) << "A tuple has no role.";
}

TEST(MAREATupleLibrary, TupleLibraryRegisterRegistersNothingWhenItsBitIsOff)
{
    GTEST_COUT << "Validating a registry with the tuple bit off holds no tuple type." << std::endl;

    const MAREA_ShippedRegistry SHIPPED{ static_cast<MAREA_LibraryMask>(MAREA_LIBRARY_MASK_DEFAULT & ~static_cast<MAREA_LibraryMask>(MAREA_LIBRARY_FLAG_MAREA_TUPLE)) };

    ASSERT_TRUE(MAREA_registerTupleLibrary(SHIPPED.registry)) << "A library that is off is a success that registers nothing.";

    EXPECT_EQ(SHIPPED.MAREA_find("marea.tuple", "Tuple"), nullptr) << "The type of a library that is off is unknown.";
}
