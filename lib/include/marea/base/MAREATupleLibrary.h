// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BASE_MAREATUPLELIBRARY_H)
    #define MAREA_INCLUDE_BASE_MAREATUPLELIBRARY_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREATypeRegistry.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Registers the library marea.tuple: the type marea.tuple.Tuple<T0, ..., Tn>, a fixed list of 2 to 16
     * values, each with its own type, laid out as the fields of a structure. The type takes a variadic list of types
     * (MAREA_TYPE_PARAMETER_COUNT_VARIADIC). The compiler implements the literal, the constant index, == and !=, and
     * swap, because a generic type cannot be a C export. The library has no runtime function.
     * @param registry Registry to change. Must not be null.
     * @return MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS, also when the tuple library is off in the mask of the registry,
     * which then holds no tuple type; otherwise the result of the registry operation that failed.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_TypeRegistryExcept MAREA_CALL MAREA_TupleLibraryRegister(MAREA_TypeRegistry * registry);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_BASE_MAREATUPLELIBRARY_H */
