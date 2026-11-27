// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BASE_MAREACMEMORYLIBRARY_H)
    #define MAREA_INCLUDE_BASE_MAREACMEMORYLIBRARY_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREATypeRegistry.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Registers the library c.memory: the type c.memory.Pointer<Element> (the raw pointer role, held by
     * copy) and the functions alloc, free, copy_bytes, fill_bytes, offset and is_null under the namespace
     * c.memory, which are the runtime of MAREACMemoryRuntime.h.
     * @param registry Registry to change. Must not be null.
     * @return MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS, also when the library is off in the mask of the registry,
     * which then holds neither the type nor the functions; otherwise the result of the registry operation that
     * failed.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_TypeRegistryExcept MAREA_CALL MAREA_CMemoryLibraryRegister(MAREA_TypeRegistry * registry);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_BASE_MAREACMEMORYLIBRARY_H */
