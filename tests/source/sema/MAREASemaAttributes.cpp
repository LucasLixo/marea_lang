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

TEST(MAREASemaAttributes, VisibilityTakesPublicOrPrivate)
{
    GTEST_COUT << "Validating visibility accepts public and private only." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("#[visibility = public] function a() end").empty()) << "visibility = public must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("#[visibility = private] function a() end").empty()) << "visibility = private must raise nothing.";

    EXPECT_EQ(MAREA_codesOf("#[visibility = hidden] function a() end"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_ATTRIBUTE_VALUE }) << "An unknown member must raise INVALID_ATTRIBUTE_VALUE.";

    EXPECT_EQ(MAREA_codesOf("#[visibility] function a() end"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_ATTRIBUTE_VALUE }) << "A bare visibility must raise INVALID_ATTRIBUTE_VALUE.";
}

TEST(MAREASemaAttributes, PublicAndPrivateAreNoLongerKeys)
{
    GTEST_COUT << "Validating the retired keys public and private raise UNKNOWN_ATTRIBUTE_KEY." << std::endl;

    EXPECT_EQ(MAREA_codesOf("#[public] function a() end"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_UNKNOWN_ATTRIBUTE_KEY }) << "public must raise UNKNOWN_ATTRIBUTE_KEY.";

    EXPECT_EQ(MAREA_codesOf("#[private] function a() end"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_UNKNOWN_ATTRIBUTE_KEY }) << "private must raise UNKNOWN_ATTRIBUTE_KEY.";
}

TEST(MAREASemaAttributes, VisibilityGivenTwiceConflicts)
{
    GTEST_COUT << "Validating visibility given twice on one symbol conflicts." << std::endl;

    EXPECT_EQ(MAREA_codesOf("#[visibility = public, visibility = private] function a() end"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_CONFLICTING_ATTRIBUTES }) << "One list with two must raise CONFLICTING_ATTRIBUTES.";

    EXPECT_EQ(MAREA_codesOf("#[visibility = public] #[visibility = private] function a() end"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_CONFLICTING_ATTRIBUTES }) << "Two lists with two must raise CONFLICTING_ATTRIBUTES.";
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

TEST(MAREASemaAttributes, SimdIsNoLongerAKey)
{
    GTEST_COUT << "Validating the retired key simd raises UNKNOWN_ATTRIBUTE_KEY." << std::endl;

    EXPECT_EQ(MAREA_codesOf("#[simd = true] function a() end"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_UNKNOWN_ATTRIBUTE_KEY }) << "simd = true must raise UNKNOWN_ATTRIBUTE_KEY.";

    EXPECT_EQ(MAREA_codesOf("#[simd] function a() end"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_UNKNOWN_ATTRIBUTE_KEY }) << "A bare simd must raise UNKNOWN_ATTRIBUTE_KEY.";
}

TEST(MAREASemaAttributes, LayoutAutoIsRejected)
{
    GTEST_COUT << "Validating the old layout member auto is rejected." << std::endl;

    EXPECT_EQ(MAREA_codesOf("#[layout = auto] structure S end"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_ATTRIBUTE_VALUE }) << "layout = auto must raise INVALID_ATTRIBUTE_VALUE.";
}

TEST(MAREASemaAttributes, LayoutTakesAnEnumMember)
{
    GTEST_COUT << "Validating layout accepts c and marea only." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("#[layout = c] structure S end").empty()) << "layout = c must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("#[layout = marea] structure S end").empty()) << "layout = marea must raise nothing.";

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

TEST(MAREASemaAttributes, MoveOnlyIsAFlag)
{
    GTEST_COUT << "Validating move_only accepts a flag and no other shape." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("#[move_only] structure S end").empty()) << "A move_only flag must raise nothing.";

    EXPECT_EQ(MAREA_codesOf("#[move_only = 3] structure S end"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_ATTRIBUTE_VALUE }) << "An integer value must raise INVALID_ATTRIBUTE_VALUE.";
}

TEST(MAREASemaAttributes, ValueIsNoLongerAKey)
{
    GTEST_COUT << "Validating the retired key value raises UNKNOWN_ATTRIBUTE_KEY." << std::endl;

    EXPECT_EQ(MAREA_codesOf("#[value] structure S end"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_UNKNOWN_ATTRIBUTE_KEY }) << "A bare value must raise UNKNOWN_ATTRIBUTE_KEY.";

    EXPECT_EQ(MAREA_codesOf("#[value = 3] structure S end"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_UNKNOWN_ATTRIBUTE_KEY }) << "value = 3 must raise UNKNOWN_ATTRIBUTE_KEY.";
}

TEST(MAREASemaAttributes, DestroyIsNoLongerAKey)
{
    GTEST_COUT << "Validating the retired key destroy raises UNKNOWN_ATTRIBUTE_KEY." << std::endl;

    EXPECT_EQ(MAREA_codesOf("#[destroy = my_destroy] structure S end"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_UNKNOWN_ATTRIBUTE_KEY }) << "destroy must raise UNKNOWN_ATTRIBUTE_KEY.";
}

TEST(MAREASemaAttributes, OperatorTakesTheFixedVocabulary)
{
    GTEST_COUT << "Validating operator accepts every spelling of its fixed vocabulary." << std::endl;

    const char *const SPELLINGS[] = {
        "+",
        "-",
        "*",
        "/",
        "%",
        "**",
        "+%",
        "-%",
        "*%",
        "+|",
        "-|",
        "*|",
        "~",
        "&",
        "|",
        "^",
        "<<",
        ">>",
        "==",
        "!=",
        "<",
        "<=",
        ">",
        ">=",
        "=",
        "+=",
        "-=",
        "*=",
        "/=",
        "%=",
        "**=",
        "<<=",
        ">>=",
        "&=",
        "|=",
        "^=",
        "[]",
        "()",
        "convert",
        "construct",
        "destroy",
    };

    for (const char *spelling : SPELLINGS) {
        // INFO: construct and destroy are valid only inside an implementation of a structure or an enum.
        const bool LIFECYCLE{ (std::string(spelling) == "construct") || (std::string(spelling) == "destroy") };

        const std::string SOURCE{ LIFECYCLE ? ("structure P x: sinteger32; end implementation P #[operator = \"" + std::string(spelling) + "\"] function a() end end") : ("#[operator = \"" + std::string(spelling) + "\"] function a() end") };

        // INFO: the vocabulary check is the point here. A free function is no place for operator by the placement matrix, so the spellings written on one raise MISPLACED_ATTRIBUTE and nothing about their value.
        const Codes EXPECTED{ LIFECYCLE ? Codes{} : Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_MISPLACED_ATTRIBUTE } };

        EXPECT_EQ(MAREA_codesOf(SOURCE), EXPECTED) << "operator \"" << spelling << "\" must be a value of the vocabulary.";
    }
}

TEST(MAREASemaAttributes, OperatorRejectsAnythingElse)
{
    GTEST_COUT << "Validating operator rejects a string outside the vocabulary, a bare name and a flag." << std::endl;

    const Codes INVALID{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_ATTRIBUTE_VALUE };

    EXPECT_EQ(MAREA_codesOf("#[operator = \"add\"] function a() end"), INVALID) << "A named operator string must raise INVALID_ATTRIBUTE_VALUE.";

    EXPECT_EQ(MAREA_codesOf("#[operator = add] function a() end"), INVALID) << "A bare name must raise INVALID_ATTRIBUTE_VALUE.";

    EXPECT_EQ(MAREA_codesOf("#[operator = \"\"] function a() end"), INVALID) << "An empty string must raise INVALID_ATTRIBUTE_VALUE.";

    EXPECT_EQ(MAREA_codesOf("#[operator = \"+++\"] function a() end"), INVALID) << "An unknown token string must raise INVALID_ATTRIBUTE_VALUE.";

    EXPECT_EQ(MAREA_codesOf("#[operator = \"Convert\"] function a() end"), INVALID) << "The vocabulary is case sensitive.";

    EXPECT_EQ(MAREA_codesOf("#[operator] function a() end"), INVALID) << "A bare operator must raise INVALID_ATTRIBUTE_VALUE.";

    EXPECT_EQ(MAREA_codesOf("#[operator = true] function a() end"), INVALID) << "A boolean must raise INVALID_ATTRIBUTE_VALUE.";
}

TEST(MAREASemaAttributes, ExportIsShapeChecked)
{
    GTEST_COUT << "Validating export is checked for shape only." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("#[export = \"a_label\"] function a() end").empty()) << "export with a string must raise nothing.";

    EXPECT_EQ(MAREA_codesOf("#[export] function a() end"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_ATTRIBUTE_VALUE }) << "A bare export must raise INVALID_ATTRIBUTE_VALUE.";

    EXPECT_EQ(MAREA_codesOf("#[export = c] function a() end"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_ATTRIBUTE_VALUE }) << "export with a name must raise INVALID_ATTRIBUTE_VALUE.";

    EXPECT_EQ(MAREA_codesOf("#[export = true] function a() end"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_ATTRIBUTE_VALUE }) << "export with a boolean must raise INVALID_ATTRIBUTE_VALUE.";
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

    EXPECT_TRUE(MAREA_codesOf("#![visibility = public] #[visibility = private] function a() end").empty()) << "A private symbol in a public file is not a conflict.";
}

TEST(MAREASemaAttributes, ErrorNodeInPlaceOfAttributeIsSkipped)
{
    GTEST_COUT << "Validating a broken attribute raises no sema diagnostic and does not crash." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("#[ structure S end").empty()) << "A broken attribute must raise no sema diagnostic.";

    EXPECT_TRUE(MAREA_codesOf("#[visibility = public, ] function a() end").empty()) << "A broken item must raise no sema diagnostic.";
}
