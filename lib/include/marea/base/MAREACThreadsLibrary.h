// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BASE_MAREACTHREADSLIBRARY_H)
    #define MAREA_INCLUDE_BASE_MAREACTHREADSLIBRARY_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREATypeRegistry.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Registers the library c.threads: the functions thread_create, thread_join, mutex_init,
     * mutex_lock, mutex_unlock, atomic32_load, atomic32_store and atomic32_add under the namespace c.threads,
     * real bindings over the platform's threads.h and stdatomic.h (the runtime of MAREACThreadsRuntime.h). The
     * library registers no type of its own: a caller reaches a thread, a mutex or an atomic through these
     * functions and the structs MAREACThreadsRuntime.h declares.
     * @param registry Registry to change. Must not be null.
     * @return MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS, also when the library is off in the mask of the registry,
     * which then holds none of its functions; otherwise the result of the registry operation that failed.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_TypeRegistryExcept MAREA_CALL MAREA_CThreadsLibraryRegister(MAREA_TypeRegistry * registry);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_BASE_MAREACTHREADSLIBRARY_H */
