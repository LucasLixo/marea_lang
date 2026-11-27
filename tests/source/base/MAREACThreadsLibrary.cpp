// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAShippedLibraries.hpp"

#include "marea/base/MAREACThreadsLibrary.h"

#include <string>

TEST(MAREACThreadsLibrary, CThreadsLibraryRegisterGivesTheFunctionsOfTheRuntime)
{
    GTEST_COUT << "Validating each raw function of c.threads is registered for the runtime." << std::endl;

    const MAREA_ShippedRegistry SHIPPED;

    ASSERT_EQ(MAREA_CThreadsLibraryRegister(SHIPPED.registry), MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) << "The registration must succeed.";

    for (const std::string &name : { std::string("thread_create"), std::string("thread_join"), std::string("mutex_init"), std::string("mutex_lock"), std::string("mutex_unlock"), std::string("atomic32_load"), std::string("atomic32_store"), std::string("atomic32_add") }) {
        EXPECT_NE(MAREA_TypeRegistryFindFunction(SHIPPED.registry, "c.threads", 9, name.c_str(), name.size()), nullptr) << "The function " << name;
    }
}

TEST(MAREACThreadsLibrary, CThreadsLibraryRegisterRegistersNothingWhenItsBitIsOff)
{
    GTEST_COUT << "Validating a registry with the c.threads bit off holds no function of it." << std::endl;

    const MAREA_ShippedRegistry SHIPPED{ static_cast<MAREA_LibraryMask>(MAREA_LIBRARY_MASK_DEFAULT & ~static_cast<MAREA_LibraryMask>(MAREA_LIBRARY_FLAG_C_THREADS)) };

    ASSERT_EQ(MAREA_CThreadsLibraryRegister(SHIPPED.registry), MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) << "A library that is off is a success that registers nothing.";

    EXPECT_EQ(MAREA_TypeRegistryFindFunction(SHIPPED.registry, "c.threads", 9, "thread_create", 13), nullptr) << "Its function is unknown.";
}

TEST(MAREACThreadsLibrary, CThreadsLibraryRegisterRejectsANullRegistry)
{
    GTEST_COUT << "Validating a null registry is rejected, never silently accepted." << std::endl;

    EXPECT_EQ(MAREA_CThreadsLibraryRegister(nullptr), MAREA_TYPE_REGISTRY_EXCEPT_ERROR_NULL_POINTER) << "A null registry must be rejected.";
}
