// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "marea/base/MAREAVectorRuntime.h"

#include <cstdint>
#include <cstdlib>
#include <vector>

namespace
{
    MAREA_Null MAREA_CALL MAREA_noBlock(MAREA_IntegerSize size, MAREA_IntegerSize alignment)
    {
        MAREA_IGNORE(size);
        MAREA_IGNORE(alignment);

        return nullptr;
    }

    void MAREA_CALL MAREA_noRelease(MAREA_Null address)
    {
        MAREA_IGNORE(address);
    }

    std::uint32_t *MAREA_elements(const MAREA_VectorHeader &header)
    {
        return reinterpret_cast<std::uint32_t *>(static_cast<std::uintptr_t>(header.data));
    }

    struct MAREA_Run
    {
        MAREA_HostContext context{};
        MAREA_VectorHeader header{};

        MAREA_Run()
        {
            MAREA_IGNORE(MAREA_HostContextInitialize(&context, nullptr, nullptr));
        }

        ~MAREA_Run()
        {
            MAREA_VectorRelease(&context, &header);
        }

        void MAREA_insert(std::uint64_t index, std::uint32_t value)
        {
            MAREA_VectorOpen(&context, &header, index, 4U, 4U);

            MAREA_elements(header)[index] = value;
        }

        std::vector<std::uint32_t> MAREA_values() const
        {
            return std::vector<std::uint32_t>(MAREA_elements(header), MAREA_elements(header) + header.size);
        }
    };
} // namespace

TEST(MAREAVectorRuntime, OpenGrowsToTheLargerOfFourAndTwiceTheCapacity)
{
    GTEST_COUT << "Validating a full vector grows by the one growth rule." << std::endl;

    MAREA_Run run;

    std::vector<std::uint64_t> capacities;

    for (std::uint32_t index{ 0 }; index < 9U; ++index) {
        run.MAREA_insert(run.header.size, index);
        capacities.push_back(run.header.capacity);
    }

    EXPECT_EQ(capacities, (std::vector<std::uint64_t>{ 4, 4, 4, 4, 8, 8, 8, 8, 16 }));
}

TEST(MAREAVectorRuntime, OpenAndCloseShiftTheTail)
{
    GTEST_COUT << "Validating open makes a gap at an index and close removes one." << std::endl;

    MAREA_Run run;

    run.MAREA_insert(0, 1);
    run.MAREA_insert(1, 3);
    run.MAREA_insert(1, 2);
    run.MAREA_insert(0, 0);

    EXPECT_EQ(run.MAREA_values(), (std::vector<std::uint32_t>{ 0, 1, 2, 3 }));

    MAREA_VectorClose(&run.header, 1, 4U);

    EXPECT_EQ(run.MAREA_values(), (std::vector<std::uint32_t>{ 0, 2, 3 }));
}

TEST(MAREAVectorRuntime, ReserveShrinkAndGrowSetTheCapacity)
{
    GTEST_COUT << "Validating reserve asks for the request, shrink for the size and grow for the rule or the need." << std::endl;

    MAREA_Run run;

    MAREA_VectorReserve(&run.context, &run.header, 10, 4U, 4U);
    EXPECT_EQ(run.header.capacity, 10U);

    MAREA_VectorReserve(&run.context, &run.header, 2, 4U, 4U);
    EXPECT_EQ(run.header.capacity, 10U) << "A smaller request does nothing.";

    MAREA_VectorGrow(&run.context, &run.header, 50, 4U, 4U);
    EXPECT_EQ(run.header.capacity, 50U) << "The need is larger than twice the capacity.";

    MAREA_VectorGrow(&run.context, &run.header, 51, 4U, 4U);
    EXPECT_EQ(run.header.capacity, 100U);

    MAREA_VectorShrink(&run.context, &run.header, 4U, 4U);
    EXPECT_EQ(run.header.capacity, 0U) << "An empty vector holds no storage.";
    EXPECT_EQ(run.header.data, 0U);
}

TEST(MAREAVectorRuntime, ARequestPastTheLargestSizeOrAFailedAllocationIsOutOfMemory)
{
    GTEST_COUT << "Validating the runtime raises OUT_OF_MEMORY and leaves the header unchanged." << std::endl;

    MAREA_Run large;

    EXPECT_EQ(MAREA_VectorMaxSize(4U), UINT64_C(0x1FFFFFFFFFFFFFFF));
    EXPECT_EQ(MAREA_VectorMaxSize(0U), UINT64_C(0x7FFFFFFFFFFFFFFF));

    MAREA_VectorReserve(&large.context, &large.header, MAREA_VectorMaxSize(4U) + 1U, 4U, 4U);

    EXPECT_EQ(MAREA_HostContextGetTrap(&large.context), MAREA_HOST_TRAP_OUT_OF_MEMORY);
    EXPECT_EQ(large.header.capacity, 0U);

    MAREA_Run failing;

    ASSERT_EQ(MAREA_HostContextInitialize(&failing.context, &MAREA_noBlock, &MAREA_noRelease), MAREA_HOST_CONTEXT_EXCEPT_SUCCESS);

    MAREA_VectorOpen(&failing.context, &failing.header, 0, 4U, 4U);

    EXPECT_EQ(MAREA_HostContextGetTrap(&failing.context), MAREA_HOST_TRAP_OUT_OF_MEMORY);
    EXPECT_EQ(failing.header.size, 0U);
}
