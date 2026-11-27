// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BASE_MAREAMATHLIBRARY_H)
    #define MAREA_INCLUDE_BASE_MAREAMATHLIBRARY_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREAMathVector2.h"
    #include "marea/base/MAREAMathVector3.h"
    #include "marea/base/MAREAMathVector4.h"
    #include "marea/base/MAREATypeRegistry.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Registers the C functions of the library marea.math, owner MAREA_LIBRARY_FLAG_MAREA_MATH. The types
     * of marea.math are Marea structures of its source file, so no type descriptor is registered. A registry whose
     * mask has the library off stores nothing.
     * @param registry Registry to change. Must not be null.
     * @return MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS, ERROR_NULL_POINTER when registry is null, or the error of the
     * registry.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_TypeRegistryExcept MAREA_CALL MAREA_MathLibraryRegister(MAREA_TypeRegistry * registry);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_BASE_MAREAMATHLIBRARY_H */
