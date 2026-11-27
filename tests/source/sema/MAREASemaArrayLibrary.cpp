// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAContainerHelpers.hpp"
#include "tests/MAREASemaCallerHelpers.hpp"

#include <string>
#include <vector>

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

    const std::string MAREA_PRELUDE{ "namespace main; require marea.array; structure Point x: sinteger32; y: sinteger32; end " };

    MAREA_HolderLayout MAREA_layoutOf(const std::string &type)
    {
        return MAREA_holderLayout(MAREA_PRELUDE + "structure Holder tag: uinteger8; value: " + type + "; end");
    }
} // namespace

TEST(MAREASemaArrayLibrary, ArrayLibraryRequireIsUnresolvedWhenTheLibraryIsOff)
{
    GTEST_COUT << "Validating a require of marea.array resolves from the registry only when the array library is on." << std::endl;

    const MAREA_ShippedRegistry ON;
    const MAREA_ShippedRegistry OFF{ static_cast<MAREA_LibraryMask>(MAREA_LIBRARY_MASK_DEFAULT & ~static_cast<MAREA_LibraryMask>(MAREA_LIBRARY_FLAG_MAREA_ARRAY)) };

    ASSERT_TRUE(MAREA_registerArrayLibrary(ON.registry) && MAREA_registerArrayLibrary(OFF.registry)) << "marea/base/MAREAArrayLibrary.h must declare the registration of the array library.";

    EXPECT_EQ(MAREA_requireCodes("marea.array", "Array", ON.registry), Codes{}) << "The library is on.";

    EXPECT_EQ(MAREA_requireCodes("marea.array", "Array", OFF.registry), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_UNRESOLVED_REQUIRE }) << "The library is off.";
}

TEST(MAREASemaArrayLibrary, ArrayLayoutIsTheCountTimesTheElement)
{
    GTEST_COUT << "Validating an array of structures, of arrays and of bytes is measured with no padding between elements." << std::endl;

    const MAREA_HolderLayout POINTS{ MAREA_layoutOf("marea.array.Array<Point, 3>") };

    ASSERT_TRUE(POINTS.measured) << "An array of a measured structure is measured.";

    EXPECT_EQ(POINTS.size, 28U) << "The tag, three bytes of padding and 3 times 8 bytes.";

    EXPECT_EQ(POINTS.offsets, (std::vector<std::uint64_t>{ 0, 4 })) << "The array has the alignment of the element.";

    const MAREA_HolderLayout NESTED{ MAREA_layoutOf("marea.array.Array<marea.array.Array<float32, 4>, 2>") };

    ASSERT_TRUE(NESTED.measured) << "An array of arrays is measured.";

    EXPECT_EQ(NESTED.size, 36U) << "2 times 16 bytes after the tag.";

    const MAREA_HolderLayout BYTES{ MAREA_layoutOf("marea.array.Array<uinteger8, 12>") };

    ASSERT_TRUE(BYTES.measured) << "An array of bytes is measured.";

    EXPECT_EQ(BYTES.size, 13U) << "12 bytes after the tag.";

    EXPECT_EQ(BYTES.alignment, 1U) << "The alignment of a byte.";
}

TEST(MAREASemaArrayLibrary, ArrayLayoutOfACountOfZeroHasNoSize)
{
    GTEST_COUT << "Validating a count of zero takes no byte and keeps the alignment of the element." << std::endl;

    const MAREA_HolderLayout EMPTY{ MAREA_layoutOf("marea.array.Array<float32, 0>") };

    ASSERT_TRUE(EMPTY.measured) << "A count of zero is measured.";

    EXPECT_EQ(EMPTY.size, 4U) << "Only the tag and its padding.";

    EXPECT_EQ(EMPTY.offsets, (std::vector<std::uint64_t>{ 0, 4 })) << "The alignment of the element holds.";
}

TEST(MAREASemaArrayLibrary, IncludeBytesGivesAnArrayFromTheShippedMacro)
{
    GTEST_COUT << "Validating the macro that marea.array registers gives an array of the bytes of the file." << std::endl;

    MAREA_CallerRun run("namespace main;\nrequire marea.array as array;\nfunction check()\n    local data: marea.array.Array<uinteger8, 12> = array.include_bytes!(\"logo.bin\");\nend\n", [] {
        MAREA_CallerInputs inputs;

        inputs.hasIncludes = true;
        inputs.files = { { "logo.bin", 12 } };

        return inputs;
    }());

    const MAREA_ShippedRegistry SHIPPED;

    ASSERT_TRUE(MAREA_registerArrayLibrary(SHIPPED.registry)) << "marea/base/MAREAArrayLibrary.h must declare the registration of the array library.";

    EXPECT_EQ(run.MAREA_codes(), Codes{}) << "The call is accepted.";
}

TEST(MAREASemaArrayLibrary, IncludeBytesOfAPathOutsideTheRootIsRejected)
{
    GTEST_COUT << "Validating an absolute path is an invalid macro argument, even when the include resolver knows the file." << std::endl;

    MAREA_CallerInputs inputs;

    inputs.hasIncludes = true;
    inputs.files = { { "/etc/passwd", 12 } };

    EXPECT_EQ(MAREA_codesWith("namespace main;\nrequire marea.array;\nfunction check()\n    marea.array.include_bytes!(\"/etc/passwd\");\nend\n", inputs), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_MACRO_ARGUMENT }) << "The build cannot read an arbitrary file.";
}
