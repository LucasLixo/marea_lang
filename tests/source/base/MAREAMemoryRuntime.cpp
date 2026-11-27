// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAMemoryHelpers.hpp"

#include "marea/base/MAREAMemoryRuntime.h"

#include <array>
#include <cstdint>
#include <cstring>

TEST(MAREAMemoryRuntime, HeadersHaveTheirFixedSizes)
{
    GTEST_COUT << "Validating the words of an owner, a view and an arena." << std::endl;

    EXPECT_EQ(sizeof(MAREA_MemoryOwnedHeader), 8U) << "An owner is one address.";

    EXPECT_EQ(sizeof(MAREA_MemorySliceHeader), 16U) << "A view is an address and a size.";

    EXPECT_EQ(sizeof(MAREA_MemoryArenaHeader), 24U) << "An arena is an address, a capacity and the bytes in use.";
}

TEST(MAREAMemoryRuntime, OwnedCreateTakesAZeroedBlockAndDestroyGivesItBack)
{
    GTEST_COUT << "Validating construct allocates one block of zeros and destroy frees it once." << std::endl;

    MAREA_CountedRun run;

    MAREA_MemoryOwnedHeader owned{};

    MAREA_MemoryOwnedCreate(&run.context, &owned, 32U, 16U);

    ASSERT_NE(owned.data, 0U) << "The owner holds a block.";

    EXPECT_EQ(owned.data % 16U, 0U) << "The block has the asked alignment.";

    const std::array<std::uint8_t, 32> ZEROS{};

    EXPECT_EQ(std::memcmp(MAREA_bytesAt(owned.data), ZEROS.data(), ZEROS.size()), 0) << "The block holds only zeros.";

    EXPECT_EQ(MAREA_countedAllocations, 1) << "One block.";

    MAREA_MemoryOwnedDestroy(&run.context, &owned);

    EXPECT_EQ(owned.data, 0U) << "A destroyed owner holds nothing.";

    EXPECT_EQ(MAREA_countedReleases, 1) << "The block went back.";

    MAREA_MemoryOwnedDestroy(&run.context, &owned);

    EXPECT_EQ(MAREA_countedReleases, 1) << "A second destroy does nothing.";
}

TEST(MAREAMemoryRuntime, AMovedOwnerFreesNothing)
{
    GTEST_COUT << "Validating the block goes with a move, so only the new owner frees it." << std::endl;

    MAREA_CountedRun run;

    MAREA_MemoryOwnedHeader first{};

    MAREA_MemoryOwnedCreate(&run.context, &first, 8U, 8U);

    ASSERT_NE(first.data, 0U) << "The first owner holds a block.";

    MAREA_MemoryOwnedHeader second{ first };

    first = MAREA_MemoryOwnedHeader{};

    MAREA_MemoryOwnedDestroy(&run.context, &first);

    EXPECT_EQ(MAREA_countedReleases, 0) << "The moved name holds nothing.";

    MAREA_MemoryOwnedDestroy(&run.context, &second);

    EXPECT_EQ(MAREA_countedReleases, 1) << "The new owner frees the block.";
}

TEST(MAREAMemoryRuntime, OwnedCreateOfNoByteStillOwnsABlock)
{
    GTEST_COUT << "Validating a value of no size has a block of its own." << std::endl;

    MAREA_CountedRun run;

    MAREA_MemoryOwnedHeader owned{};

    MAREA_MemoryOwnedCreate(&run.context, &owned, 0U, 1U);

    EXPECT_NE(owned.data, 0U) << "The owner of a value of no size is not null.";

    MAREA_MemoryOwnedDestroy(&run.context, &owned);

    EXPECT_EQ(MAREA_countedReleases, MAREA_countedAllocations) << "Every block went back.";
}

TEST(MAREAMemoryRuntime, AFailedOwnedCreateIsOutOfMemory)
{
    GTEST_COUT << "Validating a construct with no block raises OUT_OF_MEMORY and leaves the owner empty." << std::endl;

    MAREA_RefusingRun run;

    MAREA_MemoryOwnedHeader owned{};

    MAREA_MemoryOwnedCreate(&run.context, &owned, 8U, 8U);

    EXPECT_EQ(MAREA_HostContextGetTrap(&run.context), MAREA_HOST_TRAP_OUT_OF_MEMORY) << "No block is out of memory.";

    EXPECT_EQ(owned.data, 0U) << "The header is unchanged.";
}

TEST(MAREAMemoryRuntime, SliceElementIsCheckedAgainstTheSize)
{
    GTEST_COUT << "Validating the index protocol: an address inside the view and 0 outside of it." << std::endl;

    const std::array<std::uint32_t, 5> VALUES{ 1, 2, 3, 4, 5 };

    const MAREA_MemorySliceHeader WHOLE{ MAREA_addressOf(VALUES.data()), VALUES.size() };

    for (std::uint64_t index{ 0 }; index < VALUES.size(); ++index) {
        EXPECT_EQ(MAREA_MemorySliceElement(&WHOLE, index, 4U), MAREA_addressOf(&VALUES[index])) << "The element " << index;
    }

    const MAREA_MemorySliceHeader PREFIX{ MAREA_addressOf(VALUES.data()), 2U };

    EXPECT_EQ(MAREA_MemorySliceElement(&PREFIX, 1U, 4U), MAREA_addressOf(&VALUES[1])) << "The last element of the prefix.";

    EXPECT_EQ(MAREA_MemorySliceElement(&PREFIX, 2U, 4U), 0U) << "The size of the view is the limit, not the buffer.";

    EXPECT_EQ(MAREA_MemorySliceElement(&WHOLE, UINT64_MAX, 4U), 0U) << "A huge index does not wrap into the buffer.";

    const MAREA_MemorySliceHeader EMPTY{};

    EXPECT_EQ(MAREA_MemorySliceElement(&EMPTY, 0U, 4U), 0U) << "A view of zeros is empty.";
}

TEST(MAREAMemoryRuntime, SliceTrustsTheAddressAndTheSizeThatItHolds)
{
    GTEST_COUT << "Validating a view computes from its two words alone, whatever the address is." << std::endl;

    const MAREA_MemorySliceHeader FOREIGN{ 123U, 456U };

    EXPECT_EQ(MAREA_MemorySliceElement(&FOREIGN, 455U, 1U), 578U) << "No memory is read.";

    EXPECT_EQ(MAREA_MemorySliceElement(&FOREIGN, 456U, 1U), 0U) << "One past the size.";
}

TEST(MAREAMemoryRuntime, ArenaAllocationsFollowEachOtherInOneBlock)
{
    GTEST_COUT << "Validating a bump allocator takes one block and gives aligned zeroed parts of it that do not overlap." << std::endl;

    MAREA_CountedRun run;

    MAREA_MemoryArenaHeader arena{};

    MAREA_MemoryArenaCreate(&run.context, &arena, 256U);

    ASSERT_NE(arena.data, 0U) << "The arena holds its block.";

    EXPECT_EQ(arena.capacity, 256U) << "The asked capacity.";

    EXPECT_EQ(arena.used, 0U) << "Nothing is taken yet.";

    const std::uint64_t FIRST{ MAREA_MemoryArenaAllocate(&run.context, &arena, 3U, 1U) };

    const std::uint64_t SECOND{ MAREA_MemoryArenaAllocate(&run.context, &arena, 16U, 64U) };

    const std::uint64_t THIRD{ MAREA_MemoryArenaAllocate(&run.context, &arena, 8U, 8U) };

    ASSERT_TRUE((FIRST != 0U) && (SECOND != 0U) && (THIRD != 0U)) << "All three fit.";

    EXPECT_EQ(FIRST, arena.data) << "The first part starts the block.";

    EXPECT_EQ(SECOND % 64U, 0U) << "An alignment larger than the one of the block.";

    EXPECT_EQ(THIRD % 8U, 0U) << "The third part is aligned.";

    EXPECT_GE(SECOND, FIRST + 3U) << "No overlap with the first part.";

    EXPECT_GE(THIRD, SECOND + 16U) << "No overlap with the second part.";

    EXPECT_EQ(arena.used, (THIRD + 8U) - arena.data) << "The bytes in use end with the last part.";

    const std::array<std::uint8_t, 16> ZEROS{};

    EXPECT_EQ(std::memcmp(MAREA_bytesAt(SECOND), ZEROS.data(), ZEROS.size()), 0) << "A part holds only zeros.";

    EXPECT_EQ(MAREA_countedAllocations, 1) << "One block for all the parts.";

    MAREA_MemoryArenaDestroy(&run.context, &arena);

    EXPECT_EQ(MAREA_countedReleases, 1) << "Destroy frees the one block at once.";

    EXPECT_TRUE((arena.data == 0U) && (arena.capacity == 0U) && (arena.used == 0U)) << "A destroyed arena holds nothing.";

    MAREA_MemoryArenaDestroy(&run.context, &arena);

    EXPECT_EQ(MAREA_countedReleases, 1) << "A second destroy does nothing.";
}

TEST(MAREAMemoryRuntime, ArenaGivesItsLastByteAndNoMore)
{
    GTEST_COUT << "Validating a full arena raises OUT_OF_MEMORY, keeps its state and does not grow." << std::endl;

    MAREA_CountedRun run;

    MAREA_MemoryArenaHeader arena{};

    MAREA_MemoryArenaCreate(&run.context, &arena, 16U);

    ASSERT_NE(MAREA_MemoryArenaAllocate(&run.context, &arena, 16U, 1U), 0U) << "The whole block fits.";

    ASSERT_EQ(MAREA_HostContextGetTrap(&run.context), MAREA_HOST_TRAP_NONE) << "No fault yet.";

    EXPECT_EQ(MAREA_MemoryArenaAllocate(&run.context, &arena, 1U, 1U), 0U) << "No byte is left.";

    EXPECT_EQ(MAREA_HostContextGetTrap(&run.context), MAREA_HOST_TRAP_OUT_OF_MEMORY) << "A full arena is out of memory.";

    EXPECT_EQ(arena.used, 16U) << "The header is unchanged.";

    EXPECT_EQ(MAREA_countedAllocations, 1) << "The arena does not grow.";

    MAREA_MemoryArenaDestroy(&run.context, &arena);
}

TEST(MAREAMemoryRuntime, ArenaRefusesASizeThatWrapsAndABadAlignment)
{
    GTEST_COUT << "Validating a size near the largest integer, a bad alignment and an arena with no block give no part." << std::endl;

    MAREA_MemoryRun huge;

    MAREA_MemoryArenaHeader arena{};

    MAREA_MemoryArenaCreate(&huge.context, &arena, 64U);

    ASSERT_NE(MAREA_MemoryArenaAllocate(&huge.context, &arena, 8U, 8U), 0U) << "The first part fits.";

    EXPECT_EQ(MAREA_MemoryArenaAllocate(&huge.context, &arena, UINT64_MAX - 4U, 8U), 0U) << "The sum would wrap.";

    EXPECT_EQ(MAREA_HostContextGetTrap(&huge.context), MAREA_HOST_TRAP_OUT_OF_MEMORY) << "The size does not fit.";

    EXPECT_EQ(arena.used, 8U) << "The header is unchanged.";

    MAREA_MemoryRun crooked;

    EXPECT_EQ(MAREA_MemoryArenaAllocate(&crooked.context, &arena, 8U, 6U), 0U) << "6 is no power of two.";

    EXPECT_EQ(MAREA_HostContextGetTrap(&crooked.context), MAREA_HOST_TRAP_OUT_OF_MEMORY) << "A bad alignment gives no part.";

    MAREA_MemoryArenaDestroy(&crooked.context, &arena);

    MAREA_MemoryRun destroyed;

    EXPECT_EQ(MAREA_MemoryArenaAllocate(&destroyed.context, &arena, 1U, 1U), 0U) << "A destroyed arena gives nothing.";

    EXPECT_EQ(MAREA_HostContextGetTrap(&destroyed.context), MAREA_HOST_TRAP_OUT_OF_MEMORY) << "It has no block.";
}

TEST(MAREAMemoryRuntime, AFailedArenaCreateIsOutOfMemory)
{
    GTEST_COUT << "Validating a construct with no block raises OUT_OF_MEMORY and leaves the arena empty." << std::endl;

    MAREA_RefusingRun run;

    MAREA_MemoryArenaHeader arena{};

    MAREA_MemoryArenaCreate(&run.context, &arena, 64U);

    EXPECT_EQ(MAREA_HostContextGetTrap(&run.context), MAREA_HOST_TRAP_OUT_OF_MEMORY) << "No block is out of memory.";

    EXPECT_TRUE((arena.data == 0U) && (arena.capacity == 0U)) << "The header is unchanged.";
}
