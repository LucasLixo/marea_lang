// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BASE_MAREACTHREADSRUNTIME_H)
    #define MAREA_INCLUDE_BASE_MAREACTHREADSRUNTIME_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREAHostContext.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief The runtime of c.threads: a real thread, a real mutex and a real 32-bit atomic, bound directly to
     * the platform's own C11 threads (<threads.h>) and atomics (<stdatomic.h>). None of this is called by
     * fork_join, spawn or partition, which stay sequential as the vm's own lowering keeps them; a caller that
     * wants a real thread, a real lock or a real atomic calls one of these functions itself.
     */

    /**
     * @brief The entry of a real thread: the C function a created thread runs, with the one argument given to
     * MAREA_CThreadsCreate. Its result is what MAREA_CThreadsJoin gives back once the thread ends.
     */
    typedef MAREA_Sinteger32(MAREA_CALL * MAREA_CThreadsEntryFunction)(MAREA_Null argument);

    /**
     * @brief The value of c.threads.Thread: the handle of a real thread, native to the platform. A header of
     * zeros names no thread.
     */
    typedef struct MAREA_ALIGNED(8) MAREA_CThreadsThread
    {
        MAREA_Uinteger64 handle;
    } MAREA_CThreadsThread;

    /**
     * @brief The value of c.threads.Mutex: the bytes of one real platform mutex, embedded so the caller owns
     * its storage (on the stack or in an arena) with no extra allocation. MAREA_CThreadsMutexInit gives the
     * storage the real mutex of the platform; a header of zeros holds no mutex yet.
     */
    typedef struct MAREA_ALIGNED(16) MAREA_CThreadsMutex
    {
        MAREA_Uinteger8 storage[64];
    } MAREA_CThreadsMutex;

    /**
     * @brief The value of c.threads.Atomic32: one 32-bit integer, read, written and added to with the atomic
     * operations of <stdatomic.h>, so many threads can touch it with no lost update and no mutex.
     */
    typedef struct MAREA_ALIGNED(4) MAREA_CThreadsAtomic32
    {
        MAREA_Sinteger32 value;
    } MAREA_CThreadsAtomic32;

    /**
     * @brief Creates and starts a real thread that runs entry with argument.
     * @param context Context of the call. Must not be null.
     * @param entry Function the new thread runs. Must not be null.
     * @param argument Argument given to entry, read by the new thread only; it must stay valid until entry
     * reads it.
     * @param outThread Receives the handle of the new, running thread. Must not be null.
     * @return MAREA_HOST_CONTEXT_EXCEPT_SUCCESS, ERROR_NULL_POINTER when context, entry or outThread is null,
     * or ERROR_OUT_OF_MEMORY when the platform cannot start the thread.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_HostContextExcept MAREA_CALL MAREA_CThreadsCreate(MAREA_HostContext * context, MAREA_CThreadsEntryFunction entry, MAREA_Null argument, MAREA_CThreadsThread * outThread);

    /**
     * @brief Waits for a real thread to end and gives back the result its entry returned.
     * @param context Context of the call. Must not be null.
     * @param thread Thread to wait for, as MAREA_CThreadsCreate gave it.
     * @param outResult Receives the result of the entry. Must not be null.
     * @return MAREA_HOST_CONTEXT_EXCEPT_SUCCESS, ERROR_NULL_POINTER when context or outResult is null, or
     * ERROR_OUT_OF_RANGE when the platform cannot join the thread.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_HostContextExcept MAREA_CALL MAREA_CThreadsJoin(MAREA_HostContext * context, MAREA_CThreadsThread thread, MAREA_Sinteger32 * outResult);

    /**
     * @brief Initializes a real mutex in place, in mutex's own storage.
     * @param context Context of the call. Must not be null.
     * @param mutex Storage to initialize. Must not be null.
     * @return MAREA_HOST_CONTEXT_EXCEPT_SUCCESS, ERROR_NULL_POINTER when context or mutex is null, or
     * ERROR_OUT_OF_MEMORY when the platform cannot initialize the mutex.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_HostContextExcept MAREA_CALL MAREA_CThreadsMutexInit(MAREA_HostContext * context, MAREA_CThreadsMutex * mutex);

    /**
     * @brief Locks a real, already initialized mutex, waiting for it when another thread holds it.
     * @param context Context of the call. Must not be null.
     * @param mutex Mutex to lock. Must not be null.
     * @return MAREA_HOST_CONTEXT_EXCEPT_SUCCESS, ERROR_NULL_POINTER when context or mutex is null, or
     * ERROR_OUT_OF_RANGE when the platform reports a fault locking it.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_HostContextExcept MAREA_CALL MAREA_CThreadsMutexLock(MAREA_HostContext * context, MAREA_CThreadsMutex * mutex);

    /**
     * @brief Unlocks a real mutex this caller holds.
     * @param context Context of the call. Must not be null.
     * @param mutex Mutex to unlock, already locked by this caller. Must not be null.
     * @return MAREA_HOST_CONTEXT_EXCEPT_SUCCESS, ERROR_NULL_POINTER when context or mutex is null, or
     * ERROR_OUT_OF_RANGE when the platform reports a fault unlocking it.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_HostContextExcept MAREA_CALL MAREA_CThreadsMutexUnlock(MAREA_HostContext * context, MAREA_CThreadsMutex * mutex);

    /**
     * @brief Reads a 32-bit atomic, ordered against every other atomic operation on it from any thread.
     * @param atomic Atomic to read. Must not be null.
     * @return The value.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Sinteger32 MAREA_CALL MAREA_CThreadsAtomic32Load(const MAREA_CThreadsAtomic32 *atomic);

    /**
     * @brief Writes a 32-bit atomic, ordered against every other atomic operation on it from any thread.
     * @param atomic Atomic to write. Must not be null.
     * @param value Value to store.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_CThreadsAtomic32Store(MAREA_CThreadsAtomic32 * atomic, MAREA_Sinteger32 value);

    /**
     * @brief Adds to a 32-bit atomic as one read-modify-write, with no mutex and no lost update across threads.
     * @param atomic Atomic to change. Must not be null.
     * @param delta Value to add.
     * @return The value the atomic held right before the add.
     */
    MAREA_EXTERN_DECL MAREA_Sinteger32 MAREA_CALL MAREA_CThreadsAtomic32Add(MAREA_CThreadsAtomic32 * atomic, MAREA_Sinteger32 delta);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_BASE_MAREACTHREADSRUNTIME_H */
