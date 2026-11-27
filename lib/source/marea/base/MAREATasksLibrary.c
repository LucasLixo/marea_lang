// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/base/MAREATasksLibrary.h"

static const MAREA_TypeDescriptor MAREA_G_TASK = {
    .namespaceText = "marea.tasks",
    .namespaceLength = MAREA_INTEGER_SIZE(11),
    .name = "Task",
    .nameLength = MAREA_INTEGER_SIZE(4),
    .typeParameterCount = MAREA_INTEGER_SIZE(1),
    .roles = MAREA_TYPE_ROLE_SPAWN_HANDLE,
    .ownershipClass = MAREA_TYPE_OWNERSHIP_CLASS_MOVE,
    .hasIndexProtocol = MAREA_BOOLEAN_FALSE,
};

static const MAREA_TypeDescriptor MAREA_G_TASK_STATE = {
    .namespaceText = "marea.tasks",
    .namespaceLength = MAREA_INTEGER_SIZE(11),
    .name = "TaskState",
    .nameLength = MAREA_INTEGER_SIZE(9),
    .typeParameterCount = MAREA_INTEGER_SIZE(0),
    .roles = MAREA_TYPE_ROLE_NONE,
    .ownershipClass = MAREA_TYPE_OWNERSHIP_CLASS_COPY,
    .hasIndexProtocol = MAREA_BOOLEAN_FALSE,
};

static const MAREA_TypeDescriptor MAREA_G_POOL = {
    .namespaceText = "marea.tasks",
    .namespaceLength = MAREA_INTEGER_SIZE(11),
    .name = "Pool",
    .nameLength = MAREA_INTEGER_SIZE(4),
    .typeParameterCount = MAREA_INTEGER_SIZE(0),
    .roles = MAREA_TYPE_ROLE_NONE,
    .ownershipClass = MAREA_TYPE_OWNERSHIP_CLASS_MOVE,
    .hasIndexProtocol = MAREA_BOOLEAN_FALSE,
};

MAREA_TypeRegistryExcept MAREA_CALL MAREA_TasksLibraryRegister(MAREA_TypeRegistry *registry)
{
    // INFO: the pool and Task<T> need the raw thread, mutex and atomic primitives of c.threads to be on.
    MAREA_TypeRegistryExcept except = MAREA_TypeRegistryDeclareDependency(registry, MAREA_LIBRARY_FLAG_MAREA_TASKS, MAREA_LIBRARY_FLAG_C_THREADS);

    if (except == MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        except = MAREA_TypeRegistryRegisterInternal(registry, MAREA_LIBRARY_FLAG_MAREA_TASKS, &MAREA_G_TASK);
    }

    if (except == MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        except = MAREA_TypeRegistryRegisterInternal(registry, MAREA_LIBRARY_FLAG_MAREA_TASKS, &MAREA_G_TASK_STATE);
    }

    if (except == MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        except = MAREA_TypeRegistryRegisterInternal(registry, MAREA_LIBRARY_FLAG_MAREA_TASKS, &MAREA_G_POOL);
    }

    return except;
}
