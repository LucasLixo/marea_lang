// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BASE_MAREATASKSLIBRARY_H)
    #define MAREA_INCLUDE_BASE_MAREATASKSLIBRARY_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREATypeRegistry.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Registers the library marea.tasks, which depends on c.threads: the types marea.tasks.Task<T>
     * (the spawn handle role, moved), marea.tasks.TaskState (copied) and marea.tasks.Pool (moved, a
     * sequential stand-in that schedules nothing in parallel, matching fork_join's own sequential lowering).
     * sema resolves a spawn call's type through Task<T>'s role; the state machine itself is the runtime of
     * MAREATasksRuntime.h.
     * @param registry Registry to change. Must not be null.
     * @return MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS, also when the library is off in the mask of the registry,
     * which then holds none of its types; MAREA_TYPE_REGISTRY_EXCEPT_ERROR_MISSING_DEPENDENCY when the
     * library is on and c.threads is off; otherwise the result of the registry operation that failed.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_TypeRegistryExcept MAREA_CALL MAREA_TasksLibraryRegister(MAREA_TypeRegistry * registry);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_BASE_MAREATASKSLIBRARY_H */
