// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAContainerHelpers.hpp"

#include <string>
#include <vector>

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;
} // namespace

TEST(MAREASemaVectorLibrary, VectorLibraryRequireIsUnresolvedWhenTheLibraryIsOff)
{
    GTEST_COUT << "Validating a require of marea.vector resolves from the registry only when the vector library is on." << std::endl;

    const MAREA_ShippedRegistry ON;
    const MAREA_ShippedRegistry OFF{ static_cast<MAREA_LibraryMask>(MAREA_LIBRARY_MASK_DEFAULT & ~static_cast<MAREA_LibraryMask>(MAREA_LIBRARY_FLAG_MAREA_VECTOR)) };

    ASSERT_TRUE(MAREA_registerVectorLibrary(ON.registry) && MAREA_registerVectorLibrary(OFF.registry)) << "marea/base/MAREAVectorLibrary.h must declare the registration of the vector library.";

    EXPECT_EQ(MAREA_requireCodes("marea.vector", "Vector", ON.registry), Codes{}) << "The library is on.";

    EXPECT_EQ(MAREA_requireCodes("marea.vector", "Vector", OFF.registry), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_UNRESOLVED_REQUIRE }) << "The library is off.";
}

TEST(MAREASemaVectorLibrary, VectorLayoutIsTheHeaderOfThreeWords)
{
    GTEST_COUT << "Validating a vector is its address, size and capacity: 24 bytes aligned to 8, for any element." << std::endl;

    for (const std::string &element : { std::string("float32"), std::string("uinteger8"), std::string("marea.vector.Vector<float32>") }) {
        const MAREA_HolderLayout LAYOUT{ MAREA_holderLayout("namespace main; require marea.vector; structure Holder tag: uinteger8; value: marea.vector.Vector<" + element + ">; end") };

        ASSERT_TRUE(LAYOUT.measured) << "The header of a vector is measured: " << element;

        EXPECT_EQ(LAYOUT.size, 32U) << "The tag, its padding and 24 bytes: " << element;

        EXPECT_EQ(LAYOUT.offsets, (std::vector<std::uint64_t>{ 0, 8 })) << "The header is aligned to 8: " << element;
    }
}
