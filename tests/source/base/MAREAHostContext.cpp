// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.
// The internal header is used because only a runtime creates a host context, so no public function makes one.

#include "tests/dependencies.hpp"

#include "marea/base/MAREAHostContext.h"
#include "marea/internal/base/MAREAHostContextInternal.h"

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

    EXPECT_EQ(MAREA_HostContextFree(&context, block), MAREA_HOST_CONTEXT_EXCEPT_SUCCESS) << "The block must be freed.";

    ASSERT_EQ(MAREA_HostContextAllocate(&context, 0, 1, &block), MAREA_HOST_CONTEXT_EXCEPT_SUCCESS) << "0 bytes still give a block.";

    ASSERT_NE(block, nullptr) << "0 bytes still give an address.";

    EXPECT_EQ(marea_g_sizes, (std::vector<MAREA_IntegerSize>{ 32, 1 })) << "0 bytes ask for 1 byte.";

    EXPECT_EQ(MAREA_HostContextFree(&context, block), MAREA_HOST_CONTEXT_EXCEPT_SUCCESS) << "The block must be freed.";

    EXPECT_EQ(MAREA_HostContextFree(&context, nullptr), MAREA_HOST_CONTEXT_EXCEPT_SUCCESS) << "The null address does nothing.";

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

    EXPECT_EQ(MAREA_HostContextFree(nullptr, nullptr), MAREA_HOST_CONTEXT_EXCEPT_ERROR_NULL_POINTER) << "A null context must be refused.";
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

    EXPECT_EQ(MAREA_HostContextFree(&releaseOnly, &releaseOnly), MAREA_HOST_CONTEXT_EXCEPT_ERROR_UNPAIRED_ALLOCATOR) << "A block must not go to one hook only.";

    EXPECT_TRUE(releaseOnly.unpaired) << "The runtime must learn the fault.";

    EXPECT_EQ(marea_g_releases, 0) << "No hook runs.";
}

TEST(MAREAHostContext, HostContextRaiseTrapKeepsTheFirstTrap)
{
    GTEST_COUT << "Validating a raised trap is read back, the first one stays and a value that is no trap is refused." << std::endl;

    MAREA_HostContext context{ MAREA_contextOf(&MAREA_dirtyBlock, &MAREA_releaseBlock) };

    EXPECT_EQ(MAREA_HostContextGetTrap(&context), MAREA_HOST_TRAP_NONE) << "A new context has no trap.";

    EXPECT_EQ(MAREA_HostContextRaiseTrap(&context, MAREA_HOST_TRAP_NONE), MAREA_HOST_CONTEXT_EXCEPT_ERROR_OUT_OF_RANGE) << "NONE is not a trap to raise.";

    EXPECT_EQ(MAREA_HostContextRaiseTrap(&context, static_cast<MAREA_HostTrap>(1)), MAREA_HOST_CONTEXT_EXCEPT_ERROR_OUT_OF_RANGE) << "A host raises only the traps of the enum.";

    EXPECT_EQ(MAREA_HostContextGetTrap(&context), MAREA_HOST_TRAP_NONE) << "A refused value leaves no trap.";

    EXPECT_EQ(MAREA_HostContextRaiseTrap(&context, MAREA_HOST_TRAP_OUT_OF_MEMORY), MAREA_HOST_CONTEXT_EXCEPT_SUCCESS) << "OUT_OF_MEMORY must be raised.";

    EXPECT_EQ(MAREA_HostContextRaiseTrap(&context, MAREA_HOST_TRAP_OUT_OF_MEMORY), MAREA_HOST_CONTEXT_EXCEPT_SUCCESS) << "A second raise keeps the first trap.";

    EXPECT_EQ(MAREA_HostContextGetTrap(&context), MAREA_HOST_TRAP_OUT_OF_MEMORY) << "The trap must be read back.";

    EXPECT_EQ(MAREA_HostContextRaiseTrap(nullptr, MAREA_HOST_TRAP_OUT_OF_MEMORY), MAREA_HOST_CONTEXT_EXCEPT_ERROR_NULL_POINTER) << "A null context must be refused.";

    EXPECT_EQ(MAREA_HostContextGetTrap(nullptr), MAREA_HOST_TRAP_NONE) << "A null context has no trap.";
}
