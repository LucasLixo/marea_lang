// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/base/MAREATupleLibrary.h"

static const MAREA_TypeDescriptor MAREA_G_TUPLE = {
    .namespaceText = "marea.tuple",
    .namespaceLength = MAREA_INTEGER_SIZE(11),
    .name = "Tuple",
    .nameLength = MAREA_INTEGER_SIZE(5),
    .typeParameterCount = MAREA_TYPE_PARAMETER_COUNT_VARIADIC,
    .roles = MAREA_TYPE_ROLE_NONE,
    .ownershipClass = MAREA_TYPE_OWNERSHIP_CLASS_MOVE,
    .hasIndexProtocol = MAREA_BOOLEAN_FALSE,
};

MAREA_TypeRegistryExcept MAREA_CALL MAREA_TupleLibraryRegister(MAREA_TypeRegistry *registry)
{
    return MAREA_TypeRegistryRegisterInternal(registry, MAREA_LIBRARY_FLAG_MAREA_TUPLE, &MAREA_G_TUPLE);
}
