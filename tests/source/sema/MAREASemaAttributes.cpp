// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

    Codes MAREA_codesOf(const std::string &body)
    {
        MAREA_SemaFixture fixture;

        if (fixture.MAREA_sema("namespace main; " + body) != MAREA_SEMA_EXCEPT_SUCCESS) {
            return { 0 };
        }

        return fixture.MAREA_semaCodes();
    }
} // namespace

TEST(MAREASemaAttributes, VisibilityAloneIsValid)
{
    GTEST_COUT << "Validating public and private each alone raise nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("#[public] function a() end").empty()) << "A public attribute alone must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("#[private] function a() end").empty()) << "A private attribute alone must raise nothing.";
}

TEST(MAREASemaAttributes, PublicAndPrivateTogetherConflict)
{
    GTEST_COUT << "Validating public and private on one symbol conflict." << std::endl;

    EXPECT_EQ(MAREA_codesOf("#[public, private] function a() end"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_CONFLICTING_ATTRIBUTES }) << "One list with both must raise CONFLICTING_ATTRIBUTES.";

    EXPECT_EQ(MAREA_codesOf("#[public] #[private] function a() end"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_CONFLICTING_ATTRIBUTES }) << "Two lists with both must raise CONFLICTING_ATTRIBUTES.";
}

TEST(MAREASemaAttributes, InlineTakesAnEnumMember)
{
    GTEST_COUT << "Validating inline accepts force, never and auto only." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("#[inline = force] function a() end").empty()) << "inline = force must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("#[inline = never] function a() end").empty()) << "inline = never must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("#[inline = auto] function a() end").empty()) << "inline = auto must raise nothing.";

    EXPECT_EQ(MAREA_codesOf("#[inline = maybe] function a() end"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_ATTRIBUTE_VALUE }) << "An unknown member must raise INVALID_ATTRIBUTE_VALUE.";

    EXPECT_EQ(MAREA_codesOf("#[inline] function a() end"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_ATTRIBUTE_VALUE }) << "An enum key without a member must raise INVALID_ATTRIBUTE_VALUE.";
}

TEST(MAREASemaAttributes, BuildTakesAnEnumMember)
{
    GTEST_COUT << "Validating build accepts debug and release only." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("#[build = debug] function a() end").empty()) << "build = debug must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("#[build = release] function a() end").empty()) << "build = release must raise nothing.";

    EXPECT_EQ(MAREA_codesOf("#[build = 1] function a() end"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_ATTRIBUTE_VALUE }) << "An integer must raise INVALID_ATTRIBUTE_VALUE.";
}

TEST(MAREASemaAttributes, SimdTakesABoolean)
{
    GTEST_COUT << "Validating simd accepts a boolean only." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("#[simd = true] function a() end").empty()) << "simd = true must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("#[simd = false] function a() end").empty()) << "simd = false must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("#[simd] function a() end").empty()) << "A bare simd means true and must raise nothing.";

    EXPECT_EQ(MAREA_codesOf("#[simd = force] function a() end"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_ATTRIBUTE_VALUE }) << "A member name must raise INVALID_ATTRIBUTE_VALUE.";
}

TEST(MAREASemaAttributes, LayoutTakesAnEnumMember)
{
    GTEST_COUT << "Validating layout accepts c and auto only." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("#[layout = c] structure S end").empty()) << "layout = c must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("#[layout = auto] structure S end").empty()) << "layout = auto must raise nothing.";

    EXPECT_EQ(MAREA_codesOf("#[layout = packed] structure S end"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_ATTRIBUTE_VALUE }) << "An unknown member must raise INVALID_ATTRIBUTE_VALUE.";
}

TEST(MAREASemaAttributes, AlignMustBeAPowerOfTwo)
{
    GTEST_COUT << "Validating align accepts a power of two only." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("#[align = 16] structure S end").empty()) << "align = 16 must raise nothing.";

    EXPECT_EQ(MAREA_codesOf("#[align = 15] structure S end"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_ALIGNMENT }) << "align = 15 must raise INVALID_ALIGNMENT.";

    EXPECT_EQ(MAREA_codesOf("#[align = 0] structure S end"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_ALIGNMENT }) << "align = 0 must raise INVALID_ALIGNMENT.";

    EXPECT_EQ(MAREA_codesOf("#[align = true] structure S end"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_ATTRIBUTE_VALUE }) << "A boolean must raise INVALID_ATTRIBUTE_VALUE.";
}

TEST(MAREASemaAttributes, PackAndAlignTogetherConflict)
{
    GTEST_COUT << "Validating pack alone is valid and pack with align conflicts." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("#[pack = 1] structure S end").empty()) << "pack alone must raise nothing.";

    EXPECT_EQ(MAREA_codesOf("#[pack = 1, align = 16] structure S end"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_CONFLICTING_ATTRIBUTES }) << "pack with align must raise CONFLICTING_ATTRIBUTES.";
}

TEST(MAREASemaAttributes, ValueAndMoveOnlyAreFlags)
{
    GTEST_COUT << "Validating value and move_only accept a flag and no other shape." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("#[value] structure S end").empty()) << "A value flag must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("#[move_only] structure S end").empty()) << "A move_only flag must raise nothing.";

    EXPECT_EQ(MAREA_codesOf("#[value = 3] structure S end"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_ATTRIBUTE_VALUE }) << "An integer value must raise INVALID_ATTRIBUTE_VALUE.";
}

TEST(MAREASemaAttributes, DestroyOperatorAndExportAreShapeChecked)
{
    GTEST_COUT << "Validating destroy, operator and export are checked for shape only." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("#[destroy = my_destroy] structure S end").empty()) << "destroy with a name must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("#[operator = \"add\"] function a() end").empty()) << "operator with a string must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("#[operator = add] function a() end").empty()) << "operator with a name must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("#[export] function a() end").empty()) << "A bare export must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("#[export = c] function a() end").empty()) << "export with a name must raise nothing.";

    EXPECT_EQ(MAREA_codesOf("#[destroy = 5] structure S end"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_ATTRIBUTE_VALUE }) << "destroy with an integer must raise INVALID_ATTRIBUTE_VALUE.";

    EXPECT_EQ(MAREA_codesOf("#[destroy] structure S end"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_ATTRIBUTE_VALUE }) << "destroy without a name must raise INVALID_ATTRIBUTE_VALUE.";
}

TEST(MAREASemaAttributes, UnknownKeyIsRejected)
{
    GTEST_COUT << "Validating an unknown attribute key is rejected." << std::endl;

    EXPECT_EQ(MAREA_codesOf("#[frobnicate = true] function a() end"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_UNKNOWN_ATTRIBUTE_KEY }) << "An unknown key must raise UNKNOWN_ATTRIBUTE_KEY.";
}

TEST(MAREASemaAttributes, DuplicateKeyConflicts)
{
    GTEST_COUT << "Validating a key given twice on one symbol conflicts." << std::endl;

    EXPECT_EQ(MAREA_codesOf("#[inline = force, inline = never] function a() end"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_CONFLICTING_ATTRIBUTES }) << "A repeated key must raise CONFLICTING_ATTRIBUTES.";
}

TEST(MAREASemaAttributes, FileAttributesAreValidatedToo)
{
    GTEST_COUT << "Validating file attributes follow the same key table." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("#![build = debug] function a() end").empty()) << "A valid file attribute must raise nothing.";

    EXPECT_EQ(MAREA_codesOf("#![build = fast] function a() end"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_ATTRIBUTE_VALUE }) << "An invalid file attribute must raise INVALID_ATTRIBUTE_VALUE.";

    EXPECT_EQ(MAREA_codesOf("#![nonsense] function a() end"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_UNKNOWN_ATTRIBUTE_KEY }) << "An unknown file key must raise UNKNOWN_ATTRIBUTE_KEY.";
}

TEST(MAREASemaAttributes, SymbolAttributeOverridesFileAttribute)
{
    GTEST_COUT << "Validating a symbol attribute that disagrees with the file attribute is not a conflict." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("#![build = debug] #[build = release] function a() end").empty()) << "The symbol attribute wins and nothing is raised.";

    EXPECT_TRUE(MAREA_codesOf("#![public] #[private] function a() end").empty()) << "A private symbol in a public file is not a conflict.";
}

TEST(MAREASemaAttributes, ErrorNodeInPlaceOfAttributeIsSkipped)
{
    GTEST_COUT << "Validating a broken attribute raises no sema diagnostic and does not crash." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("#[ structure S end").empty()) << "A broken attribute must raise no sema diagnostic.";

    EXPECT_TRUE(MAREA_codesOf("#[public, ] function a() end").empty()) << "A broken item must raise no sema diagnostic.";
}
