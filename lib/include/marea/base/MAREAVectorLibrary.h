// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BASE_MAREAVECTORLIBRARY_H)
    #define MAREA_INCLUDE_BASE_MAREAVECTORLIBRARY_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREATypeRegistry.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Registers the library marea.vector: the type marea.vector.Vector<Element> and the runtime functions
     * of MAREAVectorRuntime.h under the namespace marea.vector (runtime_reserve, runtime_grow, runtime_open,
     * runtime_close, runtime_shrink and runtime_release), which the vm calls for a vector. The type is a growable
     * run of elements with one owner. The compiler implements its members and its loops, because a generic type
     * cannot be a C export.
     * @param registry Registry to change. Must not be null.
     * @return MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS, also when the vector library is off in the mask of the
     * registry, which then holds neither the type nor the functions; otherwise the result of the registry
     * operation that failed.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_TypeRegistryExcept MAREA_CALL MAREA_VectorLibraryRegister(MAREA_TypeRegistry * registry);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_BASE_MAREAVECTORLIBRARY_H */
