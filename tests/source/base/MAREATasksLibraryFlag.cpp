// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "marea/base/MAREATypeRegistry.h"

TEST(MAREATasksLibraryFlag, MareaTasksBitExists)
{
    GTEST_COUT << "Validating a dedicated bit names marea.tasks in the library flag enum." << std::endl;

    EXPECT_NE(static_cast<MAREA_Uinteger32>(MAREA_LIBRARY_FLAG_MAREA_TASKS), static_cast<MAREA_Uinteger32>(MAREA_LIBRARY_FLAG_NONE)) << "The bit must be set.";
}

TEST(MAREATasksLibraryFlag, MareaTasksIsOnByDefault)
{
    GTEST_COUT << "Validating marea.tasks is part of the default library mask, alongside c.threads." << std::endl;

    const MAREA_LibraryMask MASK{ MAREA_LIBRARY_MASK_DEFAULT };

    EXPECT_NE((MASK & MAREA_LIBRARY_FLAG_MAREA_TASKS), MAREA_UINTEGER_32(0)) << "marea.tasks must be on by default.";

    EXPECT_NE((MASK & MAREA_LIBRARY_FLAG_C_THREADS), MAREA_UINTEGER_32(0)) << "c.threads, the library marea.tasks depends on, must also be on by default.";
}
