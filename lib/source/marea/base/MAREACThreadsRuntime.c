// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/base/MAREACThreadsRuntime.h"

#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

// INFO: <threads.h> gives thrd_t and mtx_t directly on this toolchain; a platform whose C11 threads are
// incomplete would fall back to the POSIX pthread functions behind the same five raw entry points below,
// with no change to this file's public surface.

MAREA_STATIC_ASSERT(sizeof(thrd_t) <= sizeof(MAREA_Uinteger64), "MAREA_CThreadsThread: thrd_t must fit in the 64-bit handle on this platform.");

MAREA_STATIC_ASSERT(sizeof(mtx_t) <= sizeof(((MAREA_CThreadsMutex *)0)->storage), "MAREA_CThreadsMutex: mtx_t must fit in the embedded storage on this platform.");

// INFO: the real entry a created thread's trampoline reads back, with the caller's own argument. Heap
// allocated because the calling thread can return from MAREA_CThreadsCreate before the new thread reads it;
// the trampoline frees it once it has both fields.
typedef struct MAREA_G_ThreadBridge
{
    MAREA_CThreadsEntryFunction entry;
    MAREA_Null argument;
} MAREA_G_ThreadBridge;

static int MAREA_CALL MAREA_G_ThreadTrampoline(void *argument)
{
    MAREA_G_ThreadBridge *const BRIDGE = (MAREA_G_ThreadBridge *)argument;

    const MAREA_CThreadsEntryFunction ENTRY = BRIDGE->entry;
    MAREA_Null const REAL_ARGUMENT = BRIDGE->argument;

    free(BRIDGE);

    return (int)ENTRY(REAL_ARGUMENT);
}

static mtx_t *MAREA_CALL MAREA_G_MutexNative(MAREA_CThreadsMutex *mutex)
{
    return (mtx_t *)(void *)mutex->storage;
}

MAREA_HostContextExcept MAREA_CALL MAREA_CThreadsCreate(MAREA_HostContext *context, MAREA_CThreadsEntryFunction entry, MAREA_Null argument, MAREA_CThreadsThread *outThread)
{
    if ((context == MAREA_NULLPTR) || (entry == MAREA_NULLPTR) || (outThread == MAREA_NULLPTR)) {
        return MAREA_HOST_CONTEXT_EXCEPT_ERROR_NULL_POINTER;
    }

    MAREA_G_ThreadBridge *const BRIDGE = (MAREA_G_ThreadBridge *)malloc(sizeof(MAREA_G_ThreadBridge));

    if (BRIDGE == MAREA_NULLPTR) {
        return MAREA_HOST_CONTEXT_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    BRIDGE->entry = entry;
    BRIDGE->argument = argument;

    thrd_t native;

    if (thrd_create(&native, &MAREA_G_ThreadTrampoline, BRIDGE) != thrd_success) {
        free(BRIDGE);

        return MAREA_HOST_CONTEXT_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    outThread->handle = MAREA_UINTEGER_64(0);

    memcpy(&outThread->handle, &native, sizeof(native));

    return MAREA_HOST_CONTEXT_EXCEPT_SUCCESS;
}

MAREA_HostContextExcept MAREA_CALL MAREA_CThreadsJoin(MAREA_HostContext *context, MAREA_CThreadsThread thread, MAREA_Sinteger32 *outResult)
{
    if ((context == MAREA_NULLPTR) || (outResult == MAREA_NULLPTR)) {
        return MAREA_HOST_CONTEXT_EXCEPT_ERROR_NULL_POINTER;
    }

    thrd_t native;

    memcpy(&native, &thread.handle, sizeof(native));

    int result = 0;

    if (thrd_join(native, &result) != thrd_success) {
        return MAREA_HOST_CONTEXT_EXCEPT_ERROR_OUT_OF_RANGE;
    }

    *outResult = (MAREA_Sinteger32)result;

    return MAREA_HOST_CONTEXT_EXCEPT_SUCCESS;
}

MAREA_HostContextExcept MAREA_CALL MAREA_CThreadsMutexInit(MAREA_HostContext *context, MAREA_CThreadsMutex *mutex)
{
    if ((context == MAREA_NULLPTR) || (mutex == MAREA_NULLPTR)) {
        return MAREA_HOST_CONTEXT_EXCEPT_ERROR_NULL_POINTER;
    }

    if (mtx_init(MAREA_G_MutexNative(mutex), mtx_plain) != thrd_success) {
        return MAREA_HOST_CONTEXT_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    return MAREA_HOST_CONTEXT_EXCEPT_SUCCESS;
}

MAREA_HostContextExcept MAREA_CALL MAREA_CThreadsMutexLock(MAREA_HostContext *context, MAREA_CThreadsMutex *mutex)
{
    if ((context == MAREA_NULLPTR) || (mutex == MAREA_NULLPTR)) {
        return MAREA_HOST_CONTEXT_EXCEPT_ERROR_NULL_POINTER;
    }

    if (mtx_lock(MAREA_G_MutexNative(mutex)) != thrd_success) {
        return MAREA_HOST_CONTEXT_EXCEPT_ERROR_OUT_OF_RANGE;
    }

    return MAREA_HOST_CONTEXT_EXCEPT_SUCCESS;
}

MAREA_HostContextExcept MAREA_CALL MAREA_CThreadsMutexUnlock(MAREA_HostContext *context, MAREA_CThreadsMutex *mutex)
{
    if ((context == MAREA_NULLPTR) || (mutex == MAREA_NULLPTR)) {
        return MAREA_HOST_CONTEXT_EXCEPT_ERROR_NULL_POINTER;
    }

    if (mtx_unlock(MAREA_G_MutexNative(mutex)) != thrd_success) {
        return MAREA_HOST_CONTEXT_EXCEPT_ERROR_OUT_OF_RANGE;
    }

    return MAREA_HOST_CONTEXT_EXCEPT_SUCCESS;
}

// INFO: an object that is never declared _Atomic may still be read, written and added to through the atomic
// functions of <stdatomic.h> by way of a pointer cast to its atomic-qualified type: the two share the same
// size, alignment and representation for a lock-free type such as a 32-bit integer, the usual, widely used
// way to keep the atomic keyword out of a header that C++ test files also include.

MAREA_Sinteger32 MAREA_CALL MAREA_CThreadsAtomic32Load(const MAREA_CThreadsAtomic32 *atomic)
{
    return (MAREA_Sinteger32)atomic_load((const volatile _Atomic(MAREA_Sinteger32) *)&atomic->value);
}

void MAREA_CALL MAREA_CThreadsAtomic32Store(MAREA_CThreadsAtomic32 *atomic, MAREA_Sinteger32 value)
{
    atomic_store((volatile _Atomic(MAREA_Sinteger32) *)&atomic->value, value);
}

MAREA_Sinteger32 MAREA_CALL MAREA_CThreadsAtomic32Add(MAREA_CThreadsAtomic32 *atomic, MAREA_Sinteger32 delta)
{
    return (MAREA_Sinteger32)atomic_fetch_add((volatile _Atomic(MAREA_Sinteger32) *)&atomic->value, delta);
}
