// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

// Adapted from this project's own reference material on real thread, mutex and atomic behavior
// (create and join a real thread, lock and unlock a real mutex, order a real atomic): the raw
// thread lifecycle (create, run, join returns the entry result), the ordering a mutex gives a
// value several threads touch, and the read-modify-write an atomic add gives without a mutex.

#include "tests/dependencies.hpp"

#include "marea/base/MAREACThreadsRuntime.h"
#include "marea/base/MAREAHostContext.h"

namespace
{
    MAREA_Sinteger32 MAREA_threadEntryReturnsFortyTwo(void *)
    {
        return 42;
    }

    struct MAREA_ThreadCounterArgument
    {
        MAREA_CThreadsMutex *mutex;
        MAREA_Sinteger32 *counter;
    };

    MAREA_Sinteger32 MAREA_threadEntryIncrementsUnderMutex(void *argument)
    {
        MAREA_ThreadCounterArgument *typed{ static_cast<MAREA_ThreadCounterArgument *>(argument) };

        MAREA_HostContext context{};

        for (int i{ 0 }; i < 1000; ++i) {
            MAREA_IGNORE(MAREA_CThreadsMutexLock(&context, typed->mutex));
            *typed->counter += 1;
            MAREA_IGNORE(MAREA_CThreadsMutexUnlock(&context, typed->mutex));
        }

        return 0;
    }

    MAREA_Sinteger32 MAREA_threadEntryAddsToAtomic(void *argument)
    {
        MAREA_CThreadsAtomic32 *atomic{ static_cast<MAREA_CThreadsAtomic32 *>(argument) };

        for (int i{ 0 }; i < 1000; ++i) {
            MAREA_IGNORE(MAREA_CThreadsAtomic32Add(atomic, 1));
        }

        return 0;
    }
} // namespace

TEST(MAREACThreadsRuntime, CreateAndJoinGivesTheEntryResult)
{
    GTEST_COUT << "Validating a created thread actually runs its entry, and join gives back its result." << std::endl;

    MAREA_HostContext context{};

    MAREA_CThreadsThread thread{};

    ASSERT_EQ(MAREA_CThreadsCreate(&context, &MAREA_threadEntryReturnsFortyTwo, nullptr, &thread), MAREA_HOST_CONTEXT_EXCEPT_SUCCESS) << "A thread with a valid entry must be created.";

    MAREA_Sinteger32 result{ 0 };

    ASSERT_EQ(MAREA_CThreadsJoin(&context, thread, &result), MAREA_HOST_CONTEXT_EXCEPT_SUCCESS) << "Join must wait for the real thread to finish.";

    EXPECT_EQ(result, 42) << "The result is what the entry actually returned, from the other thread.";

    EXPECT_EQ(MAREA_HostContextGetTrap(&context), MAREA_HOST_TRAP_NONE) << "A clean create and join raises no trap.";
}

TEST(MAREACThreadsRuntime, CreateWithANullEntryIsRejected)
{
    GTEST_COUT << "Validating a null entry function is rejected, never silently started." << std::endl;

    MAREA_HostContext context{};

    MAREA_CThreadsThread thread{};

    EXPECT_EQ(MAREA_CThreadsCreate(&context, nullptr, nullptr, &thread), MAREA_HOST_CONTEXT_EXCEPT_ERROR_NULL_POINTER) << "A null entry must be rejected.";
}

TEST(MAREACThreadsRuntime, MutexInitLockUnlockOrdersASharedCounterAcrossThreads)
{
    GTEST_COUT << "Validating a mutex actually serializes two real threads incrementing one shared counter." << std::endl;

    MAREA_HostContext context{};

    MAREA_CThreadsMutex mutex{};

    ASSERT_EQ(MAREA_CThreadsMutexInit(&context, &mutex), MAREA_HOST_CONTEXT_EXCEPT_SUCCESS) << "The mutex must initialize.";

    MAREA_Sinteger32 counter{ 0 };

    MAREA_ThreadCounterArgument argument{ &mutex, &counter };

    MAREA_CThreadsThread first{};
    MAREA_CThreadsThread second{};

    ASSERT_EQ(MAREA_CThreadsCreate(&context, &MAREA_threadEntryIncrementsUnderMutex, &argument, &first), MAREA_HOST_CONTEXT_EXCEPT_SUCCESS);
    ASSERT_EQ(MAREA_CThreadsCreate(&context, &MAREA_threadEntryIncrementsUnderMutex, &argument, &second), MAREA_HOST_CONTEXT_EXCEPT_SUCCESS);

    MAREA_Sinteger32 unused{ 0 };

    ASSERT_EQ(MAREA_CThreadsJoin(&context, first, &unused), MAREA_HOST_CONTEXT_EXCEPT_SUCCESS);
    ASSERT_EQ(MAREA_CThreadsJoin(&context, second, &unused), MAREA_HOST_CONTEXT_EXCEPT_SUCCESS);

    EXPECT_EQ(counter, 2000) << "Without a lost update, the two threads together give exactly 2000.";
}

TEST(MAREACThreadsRuntime, Atomic32LoadStoreRoundTrip)
{
    GTEST_COUT << "Validating a store is visible to a load of the same atomic." << std::endl;

    MAREA_CThreadsAtomic32 atomic{};

    MAREA_CThreadsAtomic32Store(&atomic, 7);

    EXPECT_EQ(MAREA_CThreadsAtomic32Load(&atomic), 7) << "The stored value must be read back.";
}

TEST(MAREACThreadsRuntime, Atomic32AddGivesTheValueBeforeTheAdd)
{
    GTEST_COUT << "Validating fetch-add returns the value the atomic held right before adding." << std::endl;

    MAREA_CThreadsAtomic32 atomic{};

    MAREA_CThreadsAtomic32Store(&atomic, 10);

    EXPECT_EQ(MAREA_CThreadsAtomic32Add(&atomic, 5), 10) << "Fetch-add gives the previous value.";

    EXPECT_EQ(MAREA_CThreadsAtomic32Load(&atomic), 15) << "The add is applied.";
}

TEST(MAREACThreadsRuntime, Atomic32AddFromManyRealThreadsLosesNoUpdate)
{
    GTEST_COUT << "Validating an atomic add needs no mutex: many real threads adding to it lose no update." << std::endl;

    MAREA_CThreadsAtomic32 atomic{};

    MAREA_CThreadsAtomic32Store(&atomic, 0);

    MAREA_HostContext context{};

    MAREA_CThreadsThread first{};
    MAREA_CThreadsThread second{};

    ASSERT_EQ(MAREA_CThreadsCreate(&context, &MAREA_threadEntryAddsToAtomic, &atomic, &first), MAREA_HOST_CONTEXT_EXCEPT_SUCCESS);
    ASSERT_EQ(MAREA_CThreadsCreate(&context, &MAREA_threadEntryAddsToAtomic, &atomic, &second), MAREA_HOST_CONTEXT_EXCEPT_SUCCESS);

    MAREA_Sinteger32 unused{ 0 };

    ASSERT_EQ(MAREA_CThreadsJoin(&context, first, &unused), MAREA_HOST_CONTEXT_EXCEPT_SUCCESS);
    ASSERT_EQ(MAREA_CThreadsJoin(&context, second, &unused), MAREA_HOST_CONTEXT_EXCEPT_SUCCESS);

    EXPECT_EQ(MAREA_CThreadsAtomic32Load(&atomic), 2000) << "No update is lost between the two real threads.";
}
