// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/base/MAREACThreadsLibrary.h"

#include "marea/base/MAREACThreadsRuntime.h"
#include "marea/base/MAREAHostFunction.h"

#include <stdint.h>
#include <string.h>

// INFO: thread_create, thread_join, mutex_init, mutex_lock and mutex_unlock wrap the raw functions of
// MAREACThreadsRuntime.h in a thin host-facing function of their own: MAREA_HOST_FUNCTION only knows a
// scalar, a borrow and the context, never the MAREA_HostContextExcept the raw functions return (their
// signature is fixed by this library's own tests), so each wrapper turns a failure into the generic
// MAREA_HOST_TRAP_OUT_OF_MEMORY trap instead, the same fallback the rest of the language uses for a
// resource the platform could not give. atomic32_load, atomic32_store and atomic32_add bind their raw
// function directly: none of the three ever fails, so none needs this adapter.

static void MAREA_CALL MAREA_CThreadsHostThreadCreate(MAREA_HostContext *context, MAREA_Uinteger64 entryAddress, MAREA_Uinteger64 argumentAddress, MAREA_CThreadsThread *outThread)
{
    // INFO: an address the caller itself built from a real entry function, the same address-as-integer
    // representation c.memory already uses for a raw pointer.
    const MAREA_CThreadsEntryFunction ENTRY = (MAREA_CThreadsEntryFunction)(uintptr_t)entryAddress;
    MAREA_Null const ARGUMENT = (MAREA_Null)(uintptr_t)argumentAddress;

    if (MAREA_CThreadsCreate(context, ENTRY, ARGUMENT, outThread) != MAREA_HOST_CONTEXT_EXCEPT_SUCCESS) {
        MAREA_HostContextRaiseTrap(context, MAREA_HOST_TRAP_OUT_OF_MEMORY);
    }
}

MAREA_HOST_FUNCTION(MAREA_CThreadsHostThreadCreate, MAREA_HOST_RETURNS_NOTHING, MAREA_HOST_CONTEXT, MAREA_HOST_VALUE(uinteger64), MAREA_HOST_VALUE(uinteger64), MAREA_HOST_BORROW_MUTABLE(MAREA_CThreadsThread));

static MAREA_Sinteger32 MAREA_CALL MAREA_CThreadsHostThreadJoin(MAREA_HostContext *context, const MAREA_CThreadsThread *thread)
{
    MAREA_Sinteger32 result = MAREA_SINTEGER_32(0);

    if (MAREA_CThreadsJoin(context, *thread, &result) != MAREA_HOST_CONTEXT_EXCEPT_SUCCESS) {
        MAREA_HostContextRaiseTrap(context, MAREA_HOST_TRAP_OUT_OF_MEMORY);

        return MAREA_SINTEGER_32(0);
    }

    return result;
}

MAREA_HOST_FUNCTION(MAREA_CThreadsHostThreadJoin, MAREA_HOST_RETURNS(sinteger32), MAREA_HOST_CONTEXT, MAREA_HOST_BORROW(MAREA_CThreadsThread));

static void MAREA_CALL MAREA_CThreadsHostMutexInit(MAREA_HostContext *context, MAREA_CThreadsMutex *mutex)
{
    if (MAREA_CThreadsMutexInit(context, mutex) != MAREA_HOST_CONTEXT_EXCEPT_SUCCESS) {
        MAREA_HostContextRaiseTrap(context, MAREA_HOST_TRAP_OUT_OF_MEMORY);
    }
}

MAREA_HOST_FUNCTION(MAREA_CThreadsHostMutexInit, MAREA_HOST_RETURNS_NOTHING, MAREA_HOST_CONTEXT, MAREA_HOST_BORROW_MUTABLE(MAREA_CThreadsMutex));

static void MAREA_CALL MAREA_CThreadsHostMutexLock(MAREA_HostContext *context, MAREA_CThreadsMutex *mutex)
{
    if (MAREA_CThreadsMutexLock(context, mutex) != MAREA_HOST_CONTEXT_EXCEPT_SUCCESS) {
        MAREA_HostContextRaiseTrap(context, MAREA_HOST_TRAP_OUT_OF_MEMORY);
    }
}

MAREA_HOST_FUNCTION(MAREA_CThreadsHostMutexLock, MAREA_HOST_RETURNS_NOTHING, MAREA_HOST_CONTEXT, MAREA_HOST_BORROW_MUTABLE(MAREA_CThreadsMutex));

static void MAREA_CALL MAREA_CThreadsHostMutexUnlock(MAREA_HostContext *context, MAREA_CThreadsMutex *mutex)
{
    if (MAREA_CThreadsMutexUnlock(context, mutex) != MAREA_HOST_CONTEXT_EXCEPT_SUCCESS) {
        MAREA_HostContextRaiseTrap(context, MAREA_HOST_TRAP_OUT_OF_MEMORY);
    }
}

MAREA_HOST_FUNCTION(MAREA_CThreadsHostMutexUnlock, MAREA_HOST_RETURNS_NOTHING, MAREA_HOST_CONTEXT, MAREA_HOST_BORROW_MUTABLE(MAREA_CThreadsMutex));

MAREA_HOST_FUNCTION(MAREA_CThreadsAtomic32Load, MAREA_HOST_RETURNS(sinteger32), MAREA_HOST_BORROW(MAREA_CThreadsAtomic32));
MAREA_HOST_FUNCTION(MAREA_CThreadsAtomic32Store, MAREA_HOST_RETURNS_NOTHING, MAREA_HOST_BORROW_MUTABLE(MAREA_CThreadsAtomic32), MAREA_HOST_VALUE(sinteger32));
MAREA_HOST_FUNCTION(MAREA_CThreadsAtomic32Add, MAREA_HOST_RETURNS(sinteger32), MAREA_HOST_BORROW_MUTABLE(MAREA_CThreadsAtomic32), MAREA_HOST_VALUE(sinteger32));

#define MAREA_G_REGISTER(registry, name, function) MAREA_REGISTER_FUNCTION((registry), MAREA_LIBRARY_FLAG_C_THREADS, "c.threads", MAREA_INTEGER_SIZE(9), (name), strlen(name), function)

MAREA_TypeRegistryExcept MAREA_CALL MAREA_CThreadsLibraryRegister(MAREA_TypeRegistry *registry)
{
    MAREA_TypeRegistryExcept except = MAREA_G_REGISTER(registry, "thread_create", MAREA_CThreadsHostThreadCreate);

    if (except == MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        except = MAREA_G_REGISTER(registry, "thread_join", MAREA_CThreadsHostThreadJoin);
    }

    if (except == MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        except = MAREA_G_REGISTER(registry, "mutex_init", MAREA_CThreadsHostMutexInit);
    }

    if (except == MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        except = MAREA_G_REGISTER(registry, "mutex_lock", MAREA_CThreadsHostMutexLock);
    }

    if (except == MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        except = MAREA_G_REGISTER(registry, "mutex_unlock", MAREA_CThreadsHostMutexUnlock);
    }

    if (except == MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        except = MAREA_G_REGISTER(registry, "atomic32_load", MAREA_CThreadsAtomic32Load);
    }

    if (except == MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        except = MAREA_G_REGISTER(registry, "atomic32_store", MAREA_CThreadsAtomic32Store);
    }

    if (except == MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        except = MAREA_G_REGISTER(registry, "atomic32_add", MAREA_CThreadsAtomic32Add);
    }

    return except;
}
