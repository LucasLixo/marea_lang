// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAContainerHelpers.hpp"

#include <string>
#include <vector>

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

    const std::string MAREA_PRELUDE{ "namespace main; require marea.tuple; require marea.array; structure Point x: sinteger32; y: sinteger32; end " };

    MAREA_HolderLayout MAREA_layoutOf(const std::string &type)
    {
        return MAREA_holderLayout(MAREA_PRELUDE + "structure Holder tag: uinteger8; value: " + type + "; end");
    }
} // namespace

TEST(MAREASemaTupleLibrary, TupleLibraryRequireIsUnresolvedWhenTheLibraryIsOff)
{
    GTEST_COUT << "Validating a require of marea.tuple resolves from the registry only when the tuple library is on." << std::endl;

    const MAREA_ShippedRegistry ON;
    const MAREA_ShippedRegistry OFF{ static_cast<MAREA_LibraryMask>(MAREA_LIBRARY_MASK_DEFAULT & ~static_cast<MAREA_LibraryMask>(MAREA_LIBRARY_FLAG_MAREA_TUPLE)) };

    ASSERT_TRUE(MAREA_registerTupleLibrary(ON.registry) && MAREA_registerTupleLibrary(OFF.registry)) << "marea/base/MAREATupleLibrary.h must declare the registration of the tuple library.";

    EXPECT_EQ(MAREA_requireCodes("marea.tuple", "Tuple", ON.registry), Codes{}) << "The library is on.";

    EXPECT_EQ(MAREA_requireCodes("marea.tuple", "Tuple", OFF.registry), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_UNRESOLVED_REQUIRE }) << "The library is off.";
}

TEST(MAREASemaTupleLibrary, TupleLayoutFollowsTheRuleOfAStructure)
{
    GTEST_COUT << "Validating each element sits at the next multiple of its alignment and the size is rounded up." << std::endl;

    const MAREA_HolderLayout PAIR{ MAREA_layoutOf("marea.tuple.Tuple<uinteger8, uinteger32>") };

    ASSERT_TRUE(PAIR.measured) << "A tuple of primitives is measured.";

    EXPECT_EQ(PAIR.size, 12U) << "A tuple of 8 bytes aligned to 4, after the tag.";

    EXPECT_EQ(PAIR.offsets, (std::vector<std::uint64_t>{ 0, 4 })) << "The alignment of the widest element.";

    const MAREA_HolderLayout THREE{ MAREA_layoutOf("marea.tuple.Tuple<uinteger8, uinteger16, uinteger64>") };

    ASSERT_TRUE(THREE.measured) << "A tuple of three primitives is measured.";

    EXPECT_EQ(THREE.size, 24U) << "A tuple of 16 bytes aligned to 8, after the tag.";

    const MAREA_HolderLayout MIXED{ MAREA_layoutOf("marea.tuple.Tuple<Point, marea.array.Array<uinteger8, 3>>") };

    ASSERT_TRUE(MIXED.measured) << "A tuple of a structure and an array is measured.";

    EXPECT_EQ(MIXED.size, 16U) << "8 bytes, 3 bytes, one byte of tail padding, after the tag.";
}

TEST(MAREASemaTupleLibrary, TupleLayoutOfAnUndecidedElementIsUndecided)
{
    GTEST_COUT << "Validating a tuple that holds a type with no measured layout has the reason of that element." << std::endl;

    const MAREA_HolderLayout LAYOUT{ MAREA_layoutOf("marea.tuple.Tuple<sinteger32, string>") };

    ASSERT_TRUE(LAYOUT.found) << "The source must be accepted.";

    EXPECT_FALSE(LAYOUT.measured) << "A string has no measured layout.";

    EXPECT_EQ(LAYOUT.reason, MAREA_SEMA_MEASURE_REASON_LIBRARY_TYPE) << "The reason of the first element that is undecided.";

    EXPECT_TRUE(MAREA_layoutOf("marea.tuple.Tuple<sinteger32, boolean>").measured) << "The same shape with measured elements is measured.";
}
