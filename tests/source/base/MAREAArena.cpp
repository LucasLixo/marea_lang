#include "tests/dependencies.hpp"

#include <cstdint>
#include <cstring>

TEST(MAREAArena, CreateAndDestroy)
{
    GTEST_COUT << "Validating MAREA_ArenaCreate and MAREA_ArenaDestroy succeed on a fresh arena." << std::endl;

    MAREA_Arena *arena{ nullptr };

    ASSERT_EQ(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(0), &arena), MAREA_ARENA_EXCEPT_SUCCESS) << "MAREA_ArenaCreate must succeed with the default chunk size.";

    ASSERT_NE(arena, nullptr) << "MAREA_ArenaCreate must hand back a non-null arena on success.";

    MAREA_ArenaDestroy(arena);
}

TEST(MAREAArena, CreateRejectsNullOutput)
{
    GTEST_COUT << "Validating MAREA_ArenaCreate rejects a null output pointer." << std::endl;

    EXPECT_EQ(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(128), nullptr), MAREA_ARENA_EXCEPT_ERROR_NULL_POINTER) << "MAREA_ArenaCreate must reject a null outArena.";
}

TEST(MAREAArena, DestroyAcceptsNull)
{
    GTEST_COUT << "Validating MAREA_ArenaDestroy accepts a null arena as a no-op." << std::endl;

    MAREA_ArenaDestroy(nullptr);
}

TEST(MAREAArena, AllocateReturnsAlignedMemory)
{
    GTEST_COUT << "Validating MAREA_ArenaAllocate returns memory aligned to every requested power of two." << std::endl;

    MAREA_Arena *arena{ nullptr };

    ASSERT_EQ(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(0), &arena), MAREA_ARENA_EXCEPT_SUCCESS) << "MAREA_ArenaCreate must succeed.";

    const MAREA_IntegerSize ALIGNMENTS[]{ MAREA_INTEGER_SIZE(1), MAREA_INTEGER_SIZE(2), MAREA_INTEGER_SIZE(4), MAREA_INTEGER_SIZE(8), MAREA_INTEGER_SIZE(16), MAREA_INTEGER_SIZE(64) };

    for (const MAREA_IntegerSize ALIGNMENT : ALIGNMENTS) {
        MAREA_Null memory{ nullptr };

        ASSERT_EQ(MAREA_ArenaAllocate(arena, MAREA_INTEGER_SIZE(8), ALIGNMENT, &memory), MAREA_ARENA_EXCEPT_SUCCESS) << "MAREA_ArenaAllocate must succeed for alignment " << ALIGNMENT << ".";

        ASSERT_NE(memory, nullptr) << "MAREA_ArenaAllocate must hand back a non-null pointer for alignment " << ALIGNMENT << ".";

        EXPECT_EQ((reinterpret_cast<std::uintptr_t>(memory) % ALIGNMENT), MAREA_INTEGER_SIZE(0)) << "The returned pointer must be aligned to " << ALIGNMENT << " bytes.";
    }

    MAREA_ArenaDestroy(arena);
}

TEST(MAREAArena, AllocateKeepsValuesAcrossChunks)
{
    GTEST_COUT << "Validating MAREA_ArenaAllocate keeps every value intact across many chunk rollovers." << std::endl;

    MAREA_Arena *arena{ nullptr };

    ASSERT_EQ(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(256), &arena), MAREA_ARENA_EXCEPT_SUCCESS) << "MAREA_ArenaCreate must succeed with a small chunk size.";

    constexpr int ALLOCATION_COUNT{ 1000 };
    constexpr MAREA_IntegerSize ALLOCATION_SIZE{ MAREA_INTEGER_SIZE(24) };

    MAREA_Uinteger8 *pointers[ALLOCATION_COUNT]{};

    for (int index{ 0 }; index < ALLOCATION_COUNT; ++index) {
        MAREA_Null memory{ nullptr };

        ASSERT_EQ(MAREA_ArenaAllocate(arena, ALLOCATION_SIZE, MAREA_INTEGER_SIZE(8), &memory), MAREA_ARENA_EXCEPT_SUCCESS) << "MAREA_ArenaAllocate must succeed for allocation " << index << ".";

        pointers[index] = static_cast<MAREA_Uinteger8 *>(memory);

        std::memset(pointers[index], static_cast<int>(index % 256), ALLOCATION_SIZE);
    }

    for (int index{ 0 }; index < ALLOCATION_COUNT; ++index) {
        for (MAREA_IntegerSize byte{ 0 }; byte < ALLOCATION_SIZE; ++byte) {
            ASSERT_EQ(pointers[index][byte], static_cast<MAREA_Uinteger8>(index % 256)) << "Allocation " << index << " must keep its own value intact.";
        }
    }

    MAREA_ArenaDestroy(arena);
}

TEST(MAREAArena, AllocateLargerThanChunk)
{
    GTEST_COUT << "Validating MAREA_ArenaAllocate gives a request larger than the chunk size its own chunk." << std::endl;

    MAREA_Arena *arena{ nullptr };

    ASSERT_EQ(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(64), &arena), MAREA_ARENA_EXCEPT_SUCCESS) << "MAREA_ArenaCreate must succeed with a small chunk size.";

    MAREA_Null memory{ nullptr };

    ASSERT_EQ(MAREA_ArenaAllocate(arena, MAREA_INTEGER_SIZE(4096), MAREA_INTEGER_SIZE(8), &memory), MAREA_ARENA_EXCEPT_SUCCESS) << "MAREA_ArenaAllocate must succeed for a request larger than the chunk size.";

    ASSERT_NE(memory, nullptr) << "MAREA_ArenaAllocate must hand back a non-null pointer.";

    std::memset(memory, 0xAB, MAREA_INTEGER_SIZE(4096));

    MAREA_ArenaDestroy(arena);
}

TEST(MAREAArena, AllocateRejectsNullArena)
{
    GTEST_COUT << "Validating MAREA_ArenaAllocate rejects a null arena." << std::endl;

    MAREA_Null memory{ nullptr };

    EXPECT_EQ(MAREA_ArenaAllocate(nullptr, MAREA_INTEGER_SIZE(8), MAREA_INTEGER_SIZE(8), &memory), MAREA_ARENA_EXCEPT_ERROR_NULL_POINTER) << "MAREA_ArenaAllocate must reject a null arena.";
}

TEST(MAREAArena, AllocateRejectsNullOutput)
{
    GTEST_COUT << "Validating MAREA_ArenaAllocate rejects a null output pointer." << std::endl;

    MAREA_Arena *arena{ nullptr };

    ASSERT_EQ(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(0), &arena), MAREA_ARENA_EXCEPT_SUCCESS) << "MAREA_ArenaCreate must succeed.";

    EXPECT_EQ(MAREA_ArenaAllocate(arena, MAREA_INTEGER_SIZE(8), MAREA_INTEGER_SIZE(8), nullptr), MAREA_ARENA_EXCEPT_ERROR_NULL_POINTER) << "MAREA_ArenaAllocate must reject a null outMemory.";

    MAREA_ArenaDestroy(arena);
}

TEST(MAREAArena, AllocateRejectsZeroSize)
{
    GTEST_COUT << "Validating MAREA_ArenaAllocate rejects a zero size." << std::endl;

    MAREA_Arena *arena{ nullptr };

    ASSERT_EQ(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(0), &arena), MAREA_ARENA_EXCEPT_SUCCESS) << "MAREA_ArenaCreate must succeed.";

    MAREA_Null memory{ nullptr };

    EXPECT_EQ(MAREA_ArenaAllocate(arena, MAREA_INTEGER_SIZE(0), MAREA_INTEGER_SIZE(8), &memory), MAREA_ARENA_EXCEPT_ERROR_INVALID_ARGUMENT) << "MAREA_ArenaAllocate must reject a zero size.";

    MAREA_ArenaDestroy(arena);
}

TEST(MAREAArena, AllocateRejectsBadAlignment)
{
    GTEST_COUT << "Validating MAREA_ArenaAllocate rejects an alignment that is not a power of two." << std::endl;

    MAREA_Arena *arena{ nullptr };

    ASSERT_EQ(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(0), &arena), MAREA_ARENA_EXCEPT_SUCCESS) << "MAREA_ArenaCreate must succeed.";

    const MAREA_IntegerSize BAD_ALIGNMENTS[]{ MAREA_INTEGER_SIZE(3), MAREA_INTEGER_SIZE(0), MAREA_INTEGER_SIZE(24) };

    for (const MAREA_IntegerSize ALIGNMENT : BAD_ALIGNMENTS) {
        MAREA_Null memory{ nullptr };

        EXPECT_EQ(MAREA_ArenaAllocate(arena, MAREA_INTEGER_SIZE(8), ALIGNMENT, &memory), MAREA_ARENA_EXCEPT_ERROR_INVALID_ARGUMENT) << "MAREA_ArenaAllocate must reject alignment " << ALIGNMENT << ".";
    }

    MAREA_ArenaDestroy(arena);
}

TEST(MAREAArena, AllocateHugeSizeFails)
{
    GTEST_COUT << "Validating MAREA_ArenaAllocate reports out of memory for a size that cannot be satisfied." << std::endl;

    MAREA_Arena *arena{ nullptr };

    ASSERT_EQ(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(0), &arena), MAREA_ARENA_EXCEPT_SUCCESS) << "MAREA_ArenaCreate must succeed.";

    MAREA_Null memory{ nullptr };

    EXPECT_EQ(MAREA_ArenaAllocate(arena, SIZE_MAX, MAREA_INTEGER_SIZE(8), &memory), MAREA_ARENA_EXCEPT_ERROR_OUT_OF_MEMORY) << "MAREA_ArenaAllocate must report out of memory for SIZE_MAX.";

    MAREA_ArenaDestroy(arena);
}
