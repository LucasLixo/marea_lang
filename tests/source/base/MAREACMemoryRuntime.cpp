// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAMemoryHelpers.hpp"

#include "marea/base/MAREACMemoryRuntime.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

TEST(MAREACMemoryRuntime, AllocateGivesAnAlignedBlockOfZeros)
{
    GTEST_COUT << "Validating a new block has the asked alignment and holds only zeros." << std::endl;

    MAREA_MemoryRun run;

    for (const std::uint64_t alignment : { 1U, 2U, 8U, 64U }) {
        const std::uint64_t ADDRESS{ MAREA_CMemoryAllocate(&run.context, 48U, alignment) };

        ASSERT_NE(ADDRESS, 0U) << "A block must be given for the alignment " << alignment;

        EXPECT_EQ(ADDRESS % alignment, 0U) << "The address is a multiple of the alignment " << alignment;

        const std::array<std::uint8_t, 48> ZEROS{};

        EXPECT_EQ(std::memcmp(MAREA_bytesAt(ADDRESS), ZEROS.data(), ZEROS.size()), 0) << "A new block is filled with zeros.";

        MAREA_CMemoryFree(&run.context, ADDRESS);
    }

    EXPECT_EQ(MAREA_HostContextGetTrap(&run.context), MAREA_HOST_TRAP_NONE) << "No allocation failed.";
}

TEST(MAREACMemoryRuntime, AllocateOfNoByteStillGivesABlock)
{
    GTEST_COUT << "Validating a request of 0 bytes gives an address that free takes." << std::endl;

    MAREA_MemoryRun run;

    const std::uint64_t ADDRESS{ MAREA_CMemoryAllocate(&run.context, 0U, 8U) };

    EXPECT_NE(ADDRESS, 0U) << "A block of no byte is not the null address.";

    EXPECT_EQ(MAREA_HostContextGetTrap(&run.context), MAREA_HOST_TRAP_NONE) << "A request of 0 bytes is no fault.";

    MAREA_CMemoryFree(&run.context, ADDRESS);
}

TEST(MAREACMemoryRuntime, AllocateAndFreeUseTheHooksOfTheContext)
{
    GTEST_COUT << "Validating alloc and free go through the replaced runtime hooks." << std::endl;

    MAREA_CountedRun run;

    const std::uint64_t ADDRESS{ MAREA_CMemoryAllocate(&run.context, 24U, 8U) };

    ASSERT_NE(ADDRESS, 0U) << "The hook gives the block.";

    EXPECT_EQ(MAREA_countedAllocations, 1) << "One block came from the hook alloc.";

    MAREA_CMemoryFree(&run.context, ADDRESS);

    EXPECT_EQ(MAREA_countedReleases, 1) << "The block went back to the hook free.";
}

TEST(MAREACMemoryRuntime, FreeOfTheNullAddressDoesNothing)
{
    GTEST_COUT << "Validating free of the null address reaches no hook." << std::endl;

    MAREA_CountedRun run;

    MAREA_CMemoryFree(&run.context, 0U);

    EXPECT_EQ(MAREA_countedReleases, 0) << "Nothing was freed.";

    EXPECT_EQ(MAREA_HostContextGetTrap(&run.context), MAREA_HOST_TRAP_NONE) << "The null address is no fault.";
}

TEST(MAREACMemoryRuntime, AFailedAllocationIsOutOfMemory)
{
    GTEST_COUT << "Validating a hook with no block, a huge size and a bad alignment raise OUT_OF_MEMORY and give 0." << std::endl;

    MAREA_RefusingRun refusing;

    EXPECT_EQ(MAREA_CMemoryAllocate(&refusing.context, 8U, 8U), 0U) << "No block, no address.";

    EXPECT_EQ(MAREA_HostContextGetTrap(&refusing.context), MAREA_HOST_TRAP_OUT_OF_MEMORY) << "A hook with no block is out of memory.";

    MAREA_MemoryRun huge;

    EXPECT_EQ(MAREA_CMemoryAllocate(&huge.context, UINT64_MAX, 8U), 0U) << "No machine holds this size.";

    EXPECT_EQ(MAREA_HostContextGetTrap(&huge.context), MAREA_HOST_TRAP_OUT_OF_MEMORY) << "A huge size is out of memory.";

    MAREA_MemoryRun crooked;

    EXPECT_EQ(MAREA_CMemoryAllocate(&crooked.context, 8U, 3U), 0U) << "3 is no power of two.";

    EXPECT_EQ(MAREA_HostContextGetTrap(&crooked.context), MAREA_HOST_TRAP_OUT_OF_MEMORY) << "A bad alignment gives no block.";
}

TEST(MAREACMemoryRuntime, IsNullIsTrueForTheAddressZeroOnly)
{
    GTEST_COUT << "Validating the null check, also one element past the null address." << std::endl;

    EXPECT_TRUE(MAREA_CMemoryIsNull(0U)) << "The address 0 is null.";

    EXPECT_FALSE(MAREA_CMemoryIsNull(MAREA_CMemoryOffset(0U, 1, 8U))) << "One element past null is not null.";

    EXPECT_FALSE(MAREA_CMemoryIsNull(UINT64_MAX)) << "The last address is not null.";
}

TEST(MAREACMemoryRuntime, OffsetStepsByElementsAndWraps)
{
    GTEST_COUT << "Validating pointer arithmetic in both directions, with a stride, and past the ends." << std::endl;

    EXPECT_EQ(MAREA_CMemoryOffset(1000U, 3, 4U), 1012U) << "Three elements of 4 bytes forward.";

    EXPECT_EQ(MAREA_CMemoryOffset(1012U, -3, 4U), 1000U) << "The same three elements back.";

    EXPECT_EQ(MAREA_CMemoryOffset(1000U, 3, 0U), 1000U) << "An element of no size does not move.";

    EXPECT_EQ(MAREA_CMemoryOffset(0U, -1, 1U), UINT64_MAX) << "Below 0 wraps.";

    EXPECT_EQ(MAREA_CMemoryOffset(UINT64_MAX, 1, 1U), 0U) << "Past the last address wraps.";

    EXPECT_EQ(MAREA_CMemoryOffset(8U, INT64_MIN, 2U), 8U) << "A product past 64 bits wraps.";
}

TEST(MAREACMemoryRuntime, AWalkForwardAndBackReachesEveryElement)
{
    GTEST_COUT << "Validating a walk by one element reads and writes every element, forward and then back." << std::endl;

    std::array<std::int32_t, 16> values{};

    values.fill(5);

    const std::uint64_t BEGIN{ MAREA_addressOf(values.data()) };

    const std::uint64_t
END
{ MAREA_CMemoryOffset(BEGIN, 16, sizeof(std::int32_t)) };

std::uint64_t forward{ BEGIN };

for (int index{ 0 }; index < 16; ++index) {
    std::int32_t value{ 0 };

    MAREA_CMemoryCopyBytes(MAREA_addressOf(&value), forward, sizeof(value));

    ASSERT_EQ(value, 5) << "The walk reads the element " << index;

    value += 5;

    MAREA_CMemoryCopyBytes(forward, MAREA_addressOf(&value), sizeof(value));

    forward = MAREA_CMemoryOffset(forward, 1, sizeof(std::int32_t));
}

EXPECT_EQ(forward, END) << "Sixteen steps of one element reach the end.";

std::uint64_t cursor{ END };

int walked{ 0 };

while ((cursor != BEGIN) && (walked < 16)) {
    cursor = MAREA_CMemoryOffset(cursor, -1, sizeof(std::int32_t));

    ++walked;
}

EXPECT_EQ(walked, 16) << "The walk back ends at the first element.";

std::array<std::int32_t, 16> expected{};

expected.fill(10);

EXPECT_EQ(values, expected) << "Every element was written once.";
}

TEST(MAREACMemoryRuntime, CopyBytesReadsAndWritesThroughAnAddress)
{
    GTEST_COUT << "Validating a write through the address of a member changes the structure, and a copy of one element between buffers." << std::endl;

    struct Pair
    {
        std::int64_t first;
        std::int64_t second;
    } pair{ 10, 20 };

    const std::int64_t WRITTEN{ 30 };

    MAREA_CMemoryCopyBytes(MAREA_addressOf(&pair), MAREA_addressOf(&WRITTEN), sizeof(WRITTEN));

    EXPECT_EQ(pair.first, 30) << "The first member lies at the address of the structure.";

    EXPECT_EQ(pair.second, 20) << "The write is not wider than asked.";

    const std::array<std::uint16_t, 3> SOURCE{ 32000, 32001, 32002 };

    std::array<std::uint16_t, 3> destination{};

    const auto ELEMENT{ [](const std::uint16_t *base, std::int64_t index) { return MAREA_CMemoryOffset(MAREA_addressOf(base), index, sizeof(std::uint16_t)); } };

    MAREA_CMemoryCopyBytes(ELEMENT(destination.data(), 1), ELEMENT(SOURCE.data(), 1), sizeof(std::uint16_t));

    EXPECT_EQ(destination, (std::array<std::uint16_t, 3>{ 0, 32001, 0 })) << "Only the middle element.";

    MAREA_CMemoryCopyBytes(ELEMENT(destination.data(), 0), ELEMENT(SOURCE.data(), 2), sizeof(std::uint16_t));
    MAREA_CMemoryCopyBytes(ELEMENT(destination.data(), 2), ELEMENT(SOURCE.data(), 0), sizeof(std::uint16_t));

    EXPECT_EQ(destination, (std::array<std::uint16_t, 3>{ 32002, 32001, 32000 })) << "The other two elements.";
}

TEST(MAREACMemoryRuntime, CopyBytesKeepsEveryByteAndAllowsAnOverlap)
{
    GTEST_COUT << "Validating a copy reads no type, handles an overlap in both directions and ignores an empty range." << std::endl;

    std::uint8_t source{ 5 };

    std::uint8_t destination{ 6 };

    MAREA_CMemoryCopyBytes(MAREA_addressOf(&destination), MAREA_addressOf(&source), 1U);

    EXPECT_EQ(destination, 5) << "The byte keeps its value.";

    std::vector<std::uint8_t> bytes{ 1, 2, 3, 4, 5 };

    MAREA_CMemoryCopyBytes(MAREA_addressOf(bytes.data() + 1), MAREA_addressOf(bytes.data()), 4U);

    EXPECT_EQ(bytes, (std::vector<std::uint8_t>{ 1, 1, 2, 3, 4 })) << "A copy upward over itself.";

    MAREA_CMemoryCopyBytes(MAREA_addressOf(bytes.data()), MAREA_addressOf(bytes.data() + 1), 4U);

    EXPECT_EQ(bytes, (std::vector<std::uint8_t>{ 1, 2, 3, 4, 4 })) << "A copy downward over itself.";

    MAREA_CMemoryCopyBytes(0U, 0U, 0U);

    EXPECT_EQ(bytes.size(), 5U) << "An empty copy reads no address.";
}

TEST(MAREACMemoryRuntime, FillBytesSetsEachByteOfTheRange)
{
    GTEST_COUT << "Validating a fill writes the value to each byte of the range and to no other." << std::endl;

    std::array<std::uint8_t, 20> bytes{};

    MAREA_CMemoryFillBytes(MAREA_addressOf(bytes.data()), 5U, 19U);

    std::array<std::uint8_t, 20> expected{};

    expected.fill(5);

    expected[19] = 0;

    EXPECT_EQ(bytes, expected) << "19 bytes hold 5 and the last one is untouched.";

    MAREA_CMemoryFillBytes(0U, 5U, 0U);

    EXPECT_EQ(bytes[19], 0) << "An empty fill reads no address.";
}
