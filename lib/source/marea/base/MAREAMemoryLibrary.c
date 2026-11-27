// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/base/MAREAMemoryLibrary.h"

#include "marea/base/MAREAHostFunction.h"
#include "marea/base/MAREAMemoryRuntime.h"

#include <string.h>

MAREA_HOST_FUNCTION(MAREA_MemoryOwnedCreate, MAREA_HOST_RETURNS_NOTHING, MAREA_HOST_CONTEXT, MAREA_HOST_BORROW_MUTABLE(MAREA_MemoryOwnedHeader), MAREA_HOST_VALUE(uinteger64), MAREA_HOST_VALUE(uinteger64));
MAREA_HOST_FUNCTION(MAREA_MemoryOwnedDestroy, MAREA_HOST_RETURNS_NOTHING, MAREA_HOST_CONTEXT, MAREA_HOST_BORROW_MUTABLE(MAREA_MemoryOwnedHeader));
MAREA_HOST_FUNCTION(MAREA_MemorySliceElement, MAREA_HOST_RETURNS(uinteger64), MAREA_HOST_BORROW(MAREA_MemorySliceHeader), MAREA_HOST_VALUE(uinteger64), MAREA_HOST_VALUE(uinteger64));
MAREA_HOST_FUNCTION(MAREA_MemoryArenaCreate, MAREA_HOST_RETURNS_NOTHING, MAREA_HOST_CONTEXT, MAREA_HOST_BORROW_MUTABLE(MAREA_MemoryArenaHeader), MAREA_HOST_VALUE(uinteger64));
MAREA_HOST_FUNCTION(MAREA_MemoryArenaAllocate, MAREA_HOST_RETURNS(uinteger64), MAREA_HOST_CONTEXT, MAREA_HOST_BORROW_MUTABLE(MAREA_MemoryArenaHeader), MAREA_HOST_VALUE(uinteger64), MAREA_HOST_VALUE(uinteger64));
MAREA_HOST_FUNCTION(MAREA_MemoryArenaDestroy, MAREA_HOST_RETURNS_NOTHING, MAREA_HOST_CONTEXT, MAREA_HOST_BORROW_MUTABLE(MAREA_MemoryArenaHeader));

static const MAREA_TypeDescriptor MAREA_G_OWNED = {
    .namespaceText = "marea.memory",
    .namespaceLength = MAREA_INTEGER_SIZE(12),
    .name = "Owned",
    .nameLength = MAREA_INTEGER_SIZE(5),
    .typeParameterCount = MAREA_INTEGER_SIZE(1),
    .roles = MAREA_TYPE_ROLE_OWNER,
    .ownershipClass = MAREA_TYPE_OWNERSHIP_CLASS_MOVE,
    .hasIndexProtocol = MAREA_BOOLEAN_FALSE,
};

static const MAREA_TypeDescriptor MAREA_G_SLICE = {
    .namespaceText = "marea.memory",
    .namespaceLength = MAREA_INTEGER_SIZE(12),
    .name = "Slice",
    .nameLength = MAREA_INTEGER_SIZE(5),
    .typeParameterCount = MAREA_INTEGER_SIZE(1),
    .roles = MAREA_TYPE_ROLE_VIEW,
    .ownershipClass = MAREA_TYPE_OWNERSHIP_CLASS_COPY,
    .hasIndexProtocol = MAREA_BOOLEAN_TRUE,
};

static const MAREA_TypeDescriptor MAREA_G_ARENA = {
    .namespaceText = "marea.memory",
    .namespaceLength = MAREA_INTEGER_SIZE(12),
    .name = "Arena",
    .nameLength = MAREA_INTEGER_SIZE(5),
    .typeParameterCount = MAREA_INTEGER_SIZE(0),
    .roles = MAREA_TYPE_ROLE_NONE,
    .ownershipClass = MAREA_TYPE_OWNERSHIP_CLASS_MOVE,
    .hasIndexProtocol = MAREA_BOOLEAN_FALSE,
};

#define MAREA_G_REGISTER(registry, name, function) MAREA_REGISTER_FUNCTION((registry), MAREA_LIBRARY_FLAG_MAREA_MEMORY, "marea.memory", MAREA_INTEGER_SIZE(12), (name), strlen(name), function)

MAREA_TypeRegistryExcept MAREA_CALL MAREA_MemoryLibraryRegister(MAREA_TypeRegistry *registry)
{
    // INFO: the owner and the arena take their blocks from c.memory, so this library needs that one on.
    MAREA_TypeRegistryExcept except = MAREA_TypeRegistryDeclareDependency(registry, MAREA_LIBRARY_FLAG_MAREA_MEMORY, MAREA_LIBRARY_FLAG_C_MEMORY);

    if (except == MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        except = MAREA_TypeRegistryRegisterInternal(registry, MAREA_LIBRARY_FLAG_MAREA_MEMORY, &MAREA_G_OWNED);
    }

    if (except == MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        except = MAREA_TypeRegistryRegisterInternal(registry, MAREA_LIBRARY_FLAG_MAREA_MEMORY, &MAREA_G_SLICE);
    }

    if (except == MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        except = MAREA_TypeRegistryRegisterInternal(registry, MAREA_LIBRARY_FLAG_MAREA_MEMORY, &MAREA_G_ARENA);
    }

    if (except == MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        except = MAREA_G_REGISTER(registry, "runtime_owned_create", MAREA_MemoryOwnedCreate);
    }

    if (except == MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        except = MAREA_G_REGISTER(registry, "runtime_owned_destroy", MAREA_MemoryOwnedDestroy);
    }

    if (except == MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        except = MAREA_G_REGISTER(registry, "runtime_slice_element", MAREA_MemorySliceElement);
    }

    if (except == MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        except = MAREA_G_REGISTER(registry, "runtime_arena_create", MAREA_MemoryArenaCreate);
    }

    if (except == MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        except = MAREA_G_REGISTER(registry, "runtime_arena_allocate", MAREA_MemoryArenaAllocate);
    }

    if (except == MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        except = MAREA_G_REGISTER(registry, "runtime_arena_destroy", MAREA_MemoryArenaDestroy);
    }

    return except;
}
