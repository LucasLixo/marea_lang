// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/base/MAREAMathLibrary.h"

#include "marea/base/MAREACTypes.h"
#include "marea/base/MAREAMacros.h"
#include "marea/base/MAREATypeRegistry.h"

MAREA_TypeRegistryExcept MAREA_CALL MAREA_MathLibraryRegister(MAREA_TypeRegistry *registry)
{
    if (registry == MAREA_NULLPTR) {
        return MAREA_TYPE_REGISTRY_EXCEPT_ERROR_NULL_POINTER;
    }

    // INFO: no operation of marea.math exists yet, so there is no function to register.
    return MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS;
}
