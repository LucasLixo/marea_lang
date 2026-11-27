// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "marea/base/MAREAHostContext.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <vector>

namespace
{
    std::vector<MAREA_IntegerSize> marea_g_sizes;

    int marea_g_releases{ 0 };

    MAREA_Null MAREA_CALL MAREA_dirtyBlock(MAREA_IntegerSize size, MAREA_IntegerSize)
    {
        marea_g_sizes.push_back(size);

        auto *block{ static_cast<MAREA_Byte *>(::operator new(size, std::align_val_t{ 64 })) };

        std::memset(block, 0xAB, size);

        return block;
    }

    MAREA_Null MAREA_CALL MAREA_noBlock(MAREA_IntegerSize size, MAREA_IntegerSize)
    {
        marea_g_sizes.push_back(size);

        return nullptr;
    }

    void MAREA_CALL MAREA_releaseBlock(MAREA_Null address)
    {
        ++marea_g_releases;

        ::operator delete(address, std::align_val_t{ 64 });
    }

    MAREA_HostContext MAREA_contextOf(MAREA_RuntimeAllocateFunction allocate, MAREA_RuntimeFreeFunction release)
    {
        marea_g_sizes.clear();

        marea_g_releases = 0;

        MAREA_HostContext context{};

        context.allocate = allocate;

        context.release = release;

        return context;
    }
} // namespace

TEST(MAREAHostContext, HostContextAllocateGivesAZeroedBlockOfTheRuntimeAllocator)
{
    GTEST_COUT << "Validating a block comes from the allocator of the runtime and is filled with zeros, and 0 bytes still give a block." << std::endl;

    MAREA_HostContext context{ MAREA_contextOf(&MAREA_dirtyBlock, &MAREA_releaseBlock) };

    MAREA_Null block{ nullptr };

    ASSERT_EQ(MAREA_HostContextAllocate(&context, 32, 8, &block), MAREA_HOST_CONTEXT_EXCEPT_SUCCESS) << "32 bytes must be given.";

    ASSERT_NE(block, nullptr) << "A block must be given.";

    EXPECT_EQ(std::vector<MAREA_Byte>(static_cast<MAREA_Byte *>(block), static_cast<MAREA_Byte *>(block) + 32), std::vector<MAREA_Byte>(32, 0)) << "Every byte is zero.";

    MAREA_HostContextFree(&context, block);

    EXPECT_EQ(marea_g_releases, 1) << "The block must be freed.";

    ASSERT_EQ(MAREA_HostContextAllocate(&context, 0, 1, &block), MAREA_HOST_CONTEXT_EXCEPT_SUCCESS) << "0 bytes still give a block.";

    ASSERT_NE(block, nullptr) << "0 bytes still give an address.";

    EXPECT_EQ(marea_g_sizes, (std::vector<MAREA_IntegerSize>{ 32, 1 })) << "0 bytes ask for 1 byte.";

    MAREA_HostContextFree(&context, block);

    MAREA_HostContextFree(&context, nullptr);

    EXPECT_EQ(marea_g_releases, 2) << "The null address is not given to the runtime.";

    EXPECT_FALSE(context.unpaired) << "The two hooks are paired.";
}

TEST(MAREAHostContext, HostContextAllocateReportsAFailedAndAWrongRequest)
{
    GTEST_COUT << "Validating a null block is out of memory and an alignment that is not a power of two is refused with no call." << std::endl;

    MAREA_HostContext context{ MAREA_contextOf(&MAREA_noBlock, &MAREA_releaseBlock) };

    MAREA_Null block{ &context };

    EXPECT_EQ(MAREA_HostContextAllocate(&context, 16, 16, &block), MAREA_HOST_CONTEXT_EXCEPT_ERROR_OUT_OF_MEMORY) << "A null block is out of memory.";

    EXPECT_EQ(block, nullptr) << "No block is given.";

    EXPECT_EQ(MAREA_HostContextGetTrap(&context), MAREA_HOST_TRAP_NONE) << "The function decides whether a failure is a trap.";

    for (const MAREA_IntegerSize ALIGNMENT : { 0, 3, 24 }) {
        EXPECT_EQ(MAREA_HostContextAllocate(&context, 16, ALIGNMENT, &block), MAREA_HOST_CONTEXT_EXCEPT_ERROR_OUT_OF_RANGE) << "Alignment " << ALIGNMENT;
    }

    EXPECT_EQ(marea_g_sizes.size(), 1U) << "A wrong alignment reaches no allocator.";

    EXPECT_EQ(MAREA_HostContextAllocate(nullptr, 16, 16, &block), MAREA_HOST_CONTEXT_EXCEPT_ERROR_NULL_POINTER) << "A null context must be refused.";

    EXPECT_EQ(MAREA_HostContextAllocate(&context, 16, 16, nullptr), MAREA_HOST_CONTEXT_EXCEPT_ERROR_NULL_POINTER) << "A null out pointer must be refused.";

    MAREA_HostContextFree(nullptr, &context);

    EXPECT_EQ(marea_g_releases, 0) << "A null context frees nothing.";
}

TEST(MAREAHostContext, HostContextAllocateAndFreeReportOneReplacedHook)
{
    GTEST_COUT << "Validating a runtime with no pair of hooks gives no block, frees nothing and records the fault for the runtime." << std::endl;

    MAREA_HostContext allocateOnly{ MAREA_contextOf(&MAREA_dirtyBlock, nullptr) };

    MAREA_Null block{ nullptr };

    EXPECT_EQ(MAREA_HostContextAllocate(&allocateOnly, 8, 8, &block), MAREA_HOST_CONTEXT_EXCEPT_ERROR_UNPAIRED_ALLOCATOR) << "A block must not come from one hook only.";

    EXPECT_TRUE(allocateOnly.unpaired) << "The runtime must learn the fault.";

    EXPECT_TRUE(marea_g_sizes.empty()) << "No hook runs.";

    MAREA_HostContext releaseOnly{ MAREA_contextOf(nullptr, &MAREA_releaseBlock) };

    MAREA_HostContextFree(&releaseOnly, &releaseOnly);

    EXPECT_TRUE(releaseOnly.unpaired) << "The runtime must learn the fault.";

    EXPECT_EQ(marea_g_releases, 0) << "No hook runs.";
}

TEST(MAREAHostContext, HostContextRaiseTrapKeepsTheFirstTrap)
{
    GTEST_COUT << "Validating a raised trap is read back, the first one stays and a value that is no trap does nothing." << std::endl;

    MAREA_HostContext context{ MAREA_contextOf(&MAREA_dirtyBlock, &MAREA_releaseBlock) };

    EXPECT_EQ(MAREA_HostContextGetTrap(&context), MAREA_HOST_TRAP_NONE) << "A new context has no trap.";

    MAREA_HostContextRaiseTrap(&context, MAREA_HOST_TRAP_NONE);

    MAREA_HostContextRaiseTrap(&context, static_cast<MAREA_HostTrap>(2));

    EXPECT_EQ(MAREA_HostContextGetTrap(&context), MAREA_HOST_TRAP_NONE) << "NONE and a value outside the enum leave no trap.";

    MAREA_HostContextRaiseTrap(&context, MAREA_HOST_TRAP_OUT_OF_MEMORY);

    MAREA_HostContextRaiseTrap(&context, MAREA_HOST_TRAP_OUT_OF_MEMORY);

    EXPECT_EQ(MAREA_HostContextGetTrap(&context), MAREA_HOST_TRAP_OUT_OF_MEMORY) << "The first trap stays and is read back.";

    MAREA_HostContextRaiseTrap(nullptr, MAREA_HOST_TRAP_OUT_OF_MEMORY);

    EXPECT_EQ(MAREA_HostContextGetTrap(nullptr), MAREA_HOST_TRAP_NONE) << "A null context has no trap.";
}

TEST(MAREAHostContext, HostContextRaisesTheDivisionByZeroTrap)
{
    GTEST_COUT << "Validating a C function can raise the trap of a division by zero." << std::endl;

    MAREA_HostContext context{ MAREA_contextOf(&MAREA_dirtyBlock, &MAREA_releaseBlock) };

    MAREA_HostContextRaiseTrap(&context, MAREA_HOST_TRAP_DIVISION_BY_ZERO);

    MAREA_HostContextRaiseTrap(&context, MAREA_HOST_TRAP_OUT_OF_MEMORY);

    EXPECT_EQ(MAREA_HostContextGetTrap(&context), MAREA_HOST_TRAP_DIVISION_BY_ZERO) << "The first trap stays.";
}

TEST(MAREAHostContext, HostContextInitializeGivesTheContextOfOneCall)
{
    GTEST_COUT << "Validating a new context has the two hooks it is given, no trap and no fault." << std::endl;

    marea_g_sizes.clear();

    marea_g_releases = 0;

    MAREA_HostContext context{};

    std::memset(&context, 0xCD, sizeof(context));

    ASSERT_EQ(MAREA_HostContextInitialize(&context, &MAREA_dirtyBlock, &MAREA_releaseBlock), MAREA_HOST_CONTEXT_EXCEPT_SUCCESS) << "Two hooks must be accepted.";

    EXPECT_EQ(MAREA_HostContextGetTrap(&context), MAREA_HOST_TRAP_NONE) << "A new context has no trap.";

    EXPECT_FALSE(context.unpaired) << "A new context has no fault.";

    MAREA_Null block{ nullptr };

    ASSERT_EQ(MAREA_HostContextAllocate(&context, 32, 8, &block), MAREA_HOST_CONTEXT_EXCEPT_SUCCESS) << "The given hook must allocate.";

    MAREA_HostContextFree(&context, block);

    EXPECT_EQ(marea_g_sizes, (std::vector<MAREA_IntegerSize>{ 32 })) << "The given allocate hook ran.";

    EXPECT_EQ(marea_g_releases, 1) << "The given release hook ran.";
}

TEST(MAREAHostContext, HostContextInitializeUsesItsOwnAllocatorWithNoHook)
{
    GTEST_COUT << "Validating a context with no hook gives zeroed, aligned blocks of its own allocator and takes them back." << std::endl;

    MAREA_HostContext context{};

    ASSERT_EQ(MAREA_HostContextInitialize(&context, nullptr, nullptr), MAREA_HOST_CONTEXT_EXCEPT_SUCCESS) << "No hook must give the allocator of the runtime.";

    for (const MAREA_IntegerSize ALIGNMENT : { 1, 8, 16, 64, 4096 }) {
        MAREA_Null block{ nullptr };

        ASSERT_EQ(MAREA_HostContextAllocate(&context, 100, ALIGNMENT, &block), MAREA_HOST_CONTEXT_EXCEPT_SUCCESS) << "Alignment " << ALIGNMENT;

        ASSERT_NE(block, nullptr) << "A block must be given.";

        EXPECT_EQ(reinterpret_cast<std::uintptr_t>(block) % ALIGNMENT, 0U) << "The block must have the alignment " << ALIGNMENT;

        auto *bytes{ static_cast<MAREA_Byte *>(block) };

        EXPECT_EQ(std::vector<MAREA_Byte>(bytes, bytes + 100), std::vector<MAREA_Byte>(100, 0)) << "Every byte is zero.";

        std::memset(block, 0xFF, 100);

        MAREA_HostContextFree(&context, block);
    }

    MAREA_Null block{ nullptr };

    ASSERT_EQ(MAREA_HostContextAllocate(&context, 0, 8, &block), MAREA_HOST_CONTEXT_EXCEPT_SUCCESS) << "0 bytes still give a block.";

    EXPECT_NE(block, nullptr) << "0 bytes still give an address.";

    MAREA_HostContextFree(&context, block);

    MAREA_HostContextFree(&context, nullptr);

    EXPECT_EQ(MAREA_HostContextAllocate(&context, SIZE_MAX - 4, 8, &block), MAREA_HOST_CONTEXT_EXCEPT_ERROR_OUT_OF_MEMORY) << "A size that the machine cannot hold is out of memory.";

    EXPECT_FALSE(context.unpaired) << "The allocator of the runtime is paired.";
}

TEST(MAREAHostContext, HostContextInitializeRefusesOneHookAndANullContext)
{
    GTEST_COUT << "Validating a context takes two hooks or none, and that a null context is refused." << std::endl;

    MAREA_HostContext context{};

    EXPECT_EQ(MAREA_HostContextInitialize(nullptr, nullptr, nullptr), MAREA_HOST_CONTEXT_EXCEPT_ERROR_NULL_POINTER) << "A null context must be refused.";

    EXPECT_EQ(MAREA_HostContextInitialize(&context, &MAREA_dirtyBlock, nullptr), MAREA_HOST_CONTEXT_EXCEPT_ERROR_UNPAIRED_ALLOCATOR) << "An allocate hook alone must be refused.";

    EXPECT_EQ(MAREA_HostContextInitialize(&context, nullptr, &MAREA_releaseBlock), MAREA_HOST_CONTEXT_EXCEPT_ERROR_UNPAIRED_ALLOCATOR) << "A release hook alone must be refused.";
}

TEST(MAREAHostContext, HostContextLayoutIsExplicit)
{
    GTEST_COUT << "Validating the layout of the open context, because generated C makes one on its stack." << std::endl;

    EXPECT_EQ(offsetof(MAREA_HostContext, allocate), 0U) << "The allocate hook is first.";

    EXPECT_EQ(offsetof(MAREA_HostContext, release), sizeof(void *)) << "The release hook follows.";

    EXPECT_EQ(offsetof(MAREA_HostContext, trap), 2 * sizeof(void *)) << "The trap follows the hooks.";

    EXPECT_EQ(offsetof(MAREA_HostContext, unpaired), (2 * sizeof(void *)) + 4) << "The fault flag follows the trap.";

    EXPECT_EQ(offsetof(MAREA_HostContext, padding), (2 * sizeof(void *)) + 5) << "The padding is the tail.";

    EXPECT_EQ(sizeof(MAREA_HostContext), (2 * sizeof(void *)) + 8) << "The size has no implicit padding.";

    EXPECT_EQ(alignof(MAREA_HostContext), 8U) << "The alignment is explicit.";
}
