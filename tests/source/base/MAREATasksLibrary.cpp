// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAShippedLibraries.hpp"

#include "marea/base/MAREATasksLibrary.h"

TEST(MAREATasksLibrary, TasksLibraryRegisterGivesATaskHeldByMoveWithTheSpawnHandleRole)
{
    GTEST_COUT << "Validating the library registers marea.tasks.Task with the spawn handle role." << std::endl;

    const MAREA_ShippedRegistry SHIPPED;

    ASSERT_EQ(MAREA_TasksLibraryRegister(SHIPPED.registry), MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) << "The registration must succeed.";

    const MAREA_TypeDescriptor *DESCRIPTOR{ SHIPPED.MAREA_find("marea.tasks", "Task") };

    ASSERT_NE(DESCRIPTOR, nullptr) << "The type must be found by its namespace and name.";

    EXPECT_EQ(DESCRIPTOR->typeParameterCount, MAREA_INTEGER_SIZE(1)) << "The result type.";

    EXPECT_EQ(DESCRIPTOR->roles, MAREA_TYPE_ROLE_SPAWN_HANDLE) << "sema resolves a spawn call's type through this role.";

    EXPECT_EQ(DESCRIPTOR->ownershipClass, MAREA_TYPE_OWNERSHIP_CLASS_MOVE) << "A task has one owner.";
}

TEST(MAREATasksLibrary, TasksLibraryRegisterAlsoGivesTheStateEnumAndThePool)
{
    GTEST_COUT << "Validating the library also registers marea.tasks.TaskState and marea.tasks.Pool." << std::endl;

    const MAREA_ShippedRegistry SHIPPED;

    ASSERT_EQ(MAREA_TasksLibraryRegister(SHIPPED.registry), MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS);

    EXPECT_NE(SHIPPED.MAREA_find("marea.tasks", "TaskState"), nullptr) << "TaskState (Pending, Running, Completed, Cancelled) must be registered.";

    EXPECT_NE(SHIPPED.MAREA_find("marea.tasks", "Pool"), nullptr) << "The pool type must be registered.";
}

TEST(MAREATasksLibrary, TasksLibraryRegisterNeedsCThreadsOn)
{
    GTEST_COUT << "Validating marea.tasks declares its dependency on c.threads: off, registration must fail." << std::endl;

    MAREA_Arena *arena{ nullptr };

    ASSERT_EQ(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(0), &arena), MAREA_ARENA_EXCEPT_SUCCESS);

    MAREA_TypeRegistry *registry{ nullptr };

    const MAREA_LibraryMask MASK{ static_cast<MAREA_LibraryMask>(MAREA_LIBRARY_MASK_DEFAULT & ~static_cast<MAREA_LibraryMask>(MAREA_LIBRARY_FLAG_C_THREADS)) };

    ASSERT_EQ(MAREA_TypeRegistryCreate(arena, MASK, &registry), MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS);

    EXPECT_EQ(MAREA_TasksLibraryRegister(registry), MAREA_TYPE_REGISTRY_EXCEPT_ERROR_MISSING_DEPENDENCY) << "marea.tasks is on but c.threads, its declared dependency, is off.";

    MAREA_ArenaDestroy(arena);
}

TEST(MAREATasksLibrary, TasksLibraryRegisterRegistersNothingWhenItsOwnBitIsOff)
{
    GTEST_COUT << "Validating a registry with the marea.tasks bit off holds no type of it." << std::endl;

    const MAREA_ShippedRegistry SHIPPED{ static_cast<MAREA_LibraryMask>(MAREA_LIBRARY_MASK_DEFAULT & ~static_cast<MAREA_LibraryMask>(MAREA_LIBRARY_FLAG_MAREA_TASKS)) };

    ASSERT_EQ(MAREA_TasksLibraryRegister(SHIPPED.registry), MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) << "A library that is off is a success that registers nothing.";

    EXPECT_EQ(SHIPPED.MAREA_find("marea.tasks", "Task"), nullptr) << "The type of a library that is off is unknown.";
}
