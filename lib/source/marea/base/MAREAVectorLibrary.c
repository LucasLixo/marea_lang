// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/base/MAREAVectorLibrary.h"

#include "marea/base/MAREAHostFunction.h"
#include "marea/base/MAREAVectorRuntime.h"

#include <string.h>

MAREA_HOST_FUNCTION(MAREA_VectorReserve, MAREA_HOST_RETURNS_NOTHING, MAREA_HOST_CONTEXT, MAREA_HOST_BORROW_MUTABLE(MAREA_VectorHeader), MAREA_HOST_VALUE(uinteger64), MAREA_HOST_VALUE(uinteger64), MAREA_HOST_VALUE(uinteger64));
MAREA_HOST_FUNCTION(MAREA_VectorGrow, MAREA_HOST_RETURNS_NOTHING, MAREA_HOST_CONTEXT, MAREA_HOST_BORROW_MUTABLE(MAREA_VectorHeader), MAREA_HOST_VALUE(uinteger64), MAREA_HOST_VALUE(uinteger64), MAREA_HOST_VALUE(uinteger64));
MAREA_HOST_FUNCTION(MAREA_VectorOpen, MAREA_HOST_RETURNS_NOTHING, MAREA_HOST_CONTEXT, MAREA_HOST_BORROW_MUTABLE(MAREA_VectorHeader), MAREA_HOST_VALUE(uinteger64), MAREA_HOST_VALUE(uinteger64), MAREA_HOST_VALUE(uinteger64));
MAREA_HOST_FUNCTION(MAREA_VectorClose, MAREA_HOST_RETURNS_NOTHING, MAREA_HOST_BORROW_MUTABLE(MAREA_VectorHeader), MAREA_HOST_VALUE(uinteger64), MAREA_HOST_VALUE(uinteger64));
MAREA_HOST_FUNCTION(MAREA_VectorShrink, MAREA_HOST_RETURNS_NOTHING, MAREA_HOST_CONTEXT, MAREA_HOST_BORROW_MUTABLE(MAREA_VectorHeader), MAREA_HOST_VALUE(uinteger64), MAREA_HOST_VALUE(uinteger64));
MAREA_HOST_FUNCTION(MAREA_VectorRelease, MAREA_HOST_RETURNS_NOTHING, MAREA_HOST_CONTEXT, MAREA_HOST_BORROW_MUTABLE(MAREA_VectorHeader));

static const MAREA_TypeDescriptor MAREA_G_VECTOR = {
    .namespaceText = "marea.vector",
    .namespaceLength = MAREA_INTEGER_SIZE(12),
    .name = "Vector",
    .nameLength = MAREA_INTEGER_SIZE(6),
    .typeParameterCount = MAREA_INTEGER_SIZE(1),
    .roles = MAREA_TYPE_ROLE_NONE,
    .ownershipClass = MAREA_TYPE_OWNERSHIP_CLASS_MOVE,
    .hasIndexProtocol = MAREA_BOOLEAN_TRUE,
};

#define MAREA_G_REGISTER(registry, name, function) MAREA_REGISTER_FUNCTION((registry), MAREA_LIBRARY_FLAG_MAREA_VECTOR, "marea.vector", MAREA_INTEGER_SIZE(12), (name), strlen(name), function)

MAREA_TypeRegistryExcept MAREA_CALL MAREA_VectorLibraryRegister(MAREA_TypeRegistry *registry)
{
    MAREA_TypeRegistryExcept except = MAREA_TypeRegistryRegisterInternal(registry, MAREA_LIBRARY_FLAG_MAREA_VECTOR, &MAREA_G_VECTOR);

    if (except == MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        except = MAREA_G_REGISTER(registry, "runtime_reserve", MAREA_VectorReserve);
    }

    if (except == MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        except = MAREA_G_REGISTER(registry, "runtime_grow", MAREA_VectorGrow);
    }

    if (except == MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        except = MAREA_G_REGISTER(registry, "runtime_open", MAREA_VectorOpen);
    }

    if (except == MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        except = MAREA_G_REGISTER(registry, "runtime_close", MAREA_VectorClose);
    }

    if (except == MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        except = MAREA_G_REGISTER(registry, "runtime_shrink", MAREA_VectorShrink);
    }

    if (except == MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        except = MAREA_G_REGISTER(registry, "runtime_release", MAREA_VectorRelease);
    }

    return except;
}
