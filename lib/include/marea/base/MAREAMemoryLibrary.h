// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BASE_MAREAMEMORYLIBRARY_H)
    #define MAREA_INCLUDE_BASE_MAREAMEMORYLIBRARY_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREATypeRegistry.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Registers the library marea.memory, which depends on c.memory: the types marea.memory.Owned<Element>
     * (the owner role, moved), marea.memory.Slice<Element> (the view role, held by copy, indexed) and
     * marea.memory.Arena (moved), and the functions runtime_owned_create, runtime_owned_destroy,
     * runtime_slice_element, runtime_arena_create, runtime_arena_allocate and runtime_arena_destroy under the
     * namespace marea.memory, which are the runtime of MAREAMemoryRuntime.h.
     * @param registry Registry to change. Must not be null.
     * @return MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS, also when the library is off in the mask of the registry,
     * which then holds neither the types nor the functions; MAREA_TYPE_REGISTRY_EXCEPT_ERROR_MISSING_DEPENDENCY
     * when the library is on and c.memory is off; otherwise the result of the registry operation that failed.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_TypeRegistryExcept MAREA_CALL MAREA_MemoryLibraryRegister(MAREA_TypeRegistry * registry);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_BASE_MAREAMEMORYLIBRARY_H */
