// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/base/MAREACMemoryLibrary.h"

#include "marea/base/MAREACMemoryRuntime.h"
#include "marea/base/MAREAHostFunction.h"

#include <string.h>

MAREA_HOST_FUNCTION(MAREA_CMemoryAllocate, MAREA_HOST_RETURNS(uinteger64), MAREA_HOST_CONTEXT, MAREA_HOST_VALUE(uinteger64), MAREA_HOST_VALUE(uinteger64));
MAREA_HOST_FUNCTION(MAREA_CMemoryFree, MAREA_HOST_RETURNS_NOTHING, MAREA_HOST_CONTEXT, MAREA_HOST_VALUE(uinteger64));
MAREA_HOST_FUNCTION(MAREA_CMemoryCopyBytes, MAREA_HOST_RETURNS_NOTHING, MAREA_HOST_VALUE(uinteger64), MAREA_HOST_VALUE(uinteger64), MAREA_HOST_VALUE(uinteger64));
MAREA_HOST_FUNCTION(MAREA_CMemoryFillBytes, MAREA_HOST_RETURNS_NOTHING, MAREA_HOST_VALUE(uinteger64), MAREA_HOST_VALUE(uinteger8), MAREA_HOST_VALUE(uinteger64));
MAREA_HOST_FUNCTION(MAREA_CMemoryOffset, MAREA_HOST_RETURNS(uinteger64), MAREA_HOST_VALUE(uinteger64), MAREA_HOST_VALUE(sinteger64), MAREA_HOST_VALUE(uinteger64));
MAREA_HOST_FUNCTION(MAREA_CMemoryIsNull, MAREA_HOST_RETURNS(boolean), MAREA_HOST_VALUE(uinteger64));

static const MAREA_TypeDescriptor MAREA_G_POINTER = {
    .namespaceText = "c.memory",
    .namespaceLength = MAREA_INTEGER_SIZE(8),
    .name = "Pointer",
    .nameLength = MAREA_INTEGER_SIZE(7),
    .typeParameterCount = MAREA_INTEGER_SIZE(1),
    .roles = MAREA_TYPE_ROLE_RAW_POINTER,
    .ownershipClass = MAREA_TYPE_OWNERSHIP_CLASS_COPY,
    .hasIndexProtocol = MAREA_BOOLEAN_FALSE,
};

#define MAREA_G_REGISTER(registry, name, function) MAREA_REGISTER_FUNCTION((registry), MAREA_LIBRARY_FLAG_C_MEMORY, "c.memory", MAREA_INTEGER_SIZE(8), (name), strlen(name), function)

MAREA_TypeRegistryExcept MAREA_CALL MAREA_CMemoryLibraryRegister(MAREA_TypeRegistry *registry)
{
    MAREA_TypeRegistryExcept except = MAREA_TypeRegistryRegisterInternal(registry, MAREA_LIBRARY_FLAG_C_MEMORY, &MAREA_G_POINTER);

    if (except == MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        except = MAREA_G_REGISTER(registry, "alloc", MAREA_CMemoryAllocate);
    }

    if (except == MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        except = MAREA_G_REGISTER(registry, "free", MAREA_CMemoryFree);
    }

    if (except == MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        except = MAREA_G_REGISTER(registry, "copy_bytes", MAREA_CMemoryCopyBytes);
    }

    if (except == MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        except = MAREA_G_REGISTER(registry, "fill_bytes", MAREA_CMemoryFillBytes);
    }

    if (except == MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        except = MAREA_G_REGISTER(registry, "offset", MAREA_CMemoryOffset);
    }

    if (except == MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        except = MAREA_G_REGISTER(registry, "is_null", MAREA_CMemoryIsNull);
    }

    return except;
}
