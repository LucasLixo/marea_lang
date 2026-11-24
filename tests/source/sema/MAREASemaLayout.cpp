// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "marea/sema/MAREASemaLayout.h"

#include "tests/MAREASemaHelpers.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace
{
    using Offsets = std::vector<std::uint64_t>;

    struct MAREA_LayoutProbe
    {
        MAREA_SemaFixture fixture;
        const MAREA_SemaTypedNode *root{ nullptr };

        explicit MAREA_LayoutProbe(MAREA_ResolveCallback resolve = nullptr)
            : fixture(resolve)
        {
        }

        // Checks the source with zero errors and builds the typed tree.
        bool MAREA_build(const std::string &source)
        {
            if ((fixture.MAREA_sema(source) != MAREA_SEMA_EXCEPT_SUCCESS) || (fixture.MAREA_errorCount() != MAREA_INTEGER_SIZE(0))) {
                return false;
            }

            return MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &root) == MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
        }

        const MAREA_SemaTypedNode *MAREA_nodeOf(MAREA_AstKind kind, std::size_t nth) const
        {
            std::size_t seen{ 0 };

            return MAREA_find(root, kind, nth, seen);
        }

        const MAREA_SemaTypeLayout *MAREA_layoutOf(MAREA_AstKind kind, std::size_t nth) const
        {
            const MAREA_SemaTypedNode *node{ MAREA_nodeOf(kind, nth) };

            const MAREA_SemaTypeLayout *layout{ nullptr };

            if ((node == nullptr) || (MAREA_SemaTypedNodeGetLayout(node, &layout) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS)) {
                return nullptr;
            }

            return layout;
        }

      private:
        static const MAREA_SemaTypedNode *MAREA_find(const MAREA_SemaTypedNode *node, MAREA_AstKind kind, std::size_t nth, std::size_t &seen)
        {
            if (node == nullptr) {
                return nullptr;
            }

            if (MAREA_AstNodeGetKind(MAREA_SemaTypedNodeGetAstNode(node)) == kind) {
                if (seen == nth) {
                    return node;
                }

                seen += 1;
            }

            for (MAREA_IntegerSize index{ 0 }; index < MAREA_SemaTypedNodeGetChildCount(node); ++index) {
                const MAREA_SemaTypedNode *child{ nullptr };

                if (MAREA_SemaTypedNodeGetChildAt(node, index, &child) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
                    continue;
                }

                const MAREA_SemaTypedNode *found{ MAREA_find(child, kind, nth, seen) };

                if (found != nullptr) {
                    return found;
                }
            }

            return nullptr;
        }
    };

    std::uint64_t MAREA_sizeOf(const MAREA_SemaTypeLayout *layout)
    {
        MAREA_Uinteger64 value{ 0 };

        EXPECT_EQ(MAREA_SemaTypeLayoutGetSize(layout, &value), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS);

        return value;
    }

    std::uint64_t MAREA_alignmentOf(const MAREA_SemaTypeLayout *layout)
    {
        MAREA_Uinteger64 value{ 0 };

        EXPECT_EQ(MAREA_SemaTypeLayoutGetAlignment(layout, &value), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS);

        return value;
    }

    std::uint64_t MAREA_tagSizeOf(const MAREA_SemaTypeLayout *layout)
    {
        MAREA_Uinteger64 value{ 0 };

        EXPECT_EQ(MAREA_SemaTypeLayoutGetTagSize(layout, &value), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS);

        return value;
    }

    Offsets MAREA_membersOf(const MAREA_SemaTypeLayout *layout)
    {
        Offsets result;

        for (MAREA_IntegerSize index{ 0 }; index < MAREA_SemaTypeLayoutGetMemberCount(layout); ++index) {
            MAREA_Uinteger64 value{ 0 };

            EXPECT_EQ(MAREA_SemaTypeLayoutGetMemberOffset(layout, index, &value), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS);

            result.push_back(value);
        }

        return result;
    }

    Offsets MAREA_variantFieldsOf(const MAREA_SemaTypeLayout *layout, MAREA_IntegerSize variant)
    {
        Offsets result;

        for (MAREA_IntegerSize index{ 0 }; index < MAREA_SemaTypeLayoutGetVariantFieldCount(layout, variant); ++index) {
            MAREA_Uinteger64 value{ 0 };

            EXPECT_EQ(MAREA_SemaTypeLayoutGetVariantFieldOffset(layout, variant, index, &value), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS);

            result.push_back(value);
        }

        return result;
    }

    void MAREA_expectStructure(const std::string &source, std::size_t nth, std::uint64_t size, std::uint64_t alignment, const Offsets &offsets)
    {
        MAREA_LayoutProbe probe;

        ASSERT_TRUE(probe.MAREA_build(source)) << "The source must check with zero errors: " << source;

        const MAREA_SemaTypeLayout *layout{ probe.MAREA_layoutOf(MAREA_AST_KIND_STRUCTURE_DECLARATION, nth) };

        ASSERT_NE(layout, nullptr) << "The structure must have a layout in: " << source;

        EXPECT_EQ(MAREA_SemaTypeLayoutGetVerdict(layout), MAREA_SEMA_MEASURE_VERDICT_MEASURED) << source;

        EXPECT_EQ(MAREA_SemaTypeLayoutGetReason(layout), MAREA_SEMA_MEASURE_REASON_NONE) << source;

        EXPECT_EQ(MAREA_sizeOf(layout), size) << "The size is wrong in: " << source;

        EXPECT_EQ(MAREA_alignmentOf(layout), alignment) << "The alignment is wrong in: " << source;

        EXPECT_EQ(MAREA_membersOf(layout), offsets) << "The field offsets are wrong in: " << source;
    }

    void MAREA_expectEnum(const std::string &source, std::size_t nth, std::uint64_t size, std::uint64_t alignment, std::uint64_t tagSize, const Offsets &payloadOffsets)
    {
        MAREA_LayoutProbe probe;

        ASSERT_TRUE(probe.MAREA_build(source)) << "The source must check with zero errors: " << source;

        const MAREA_SemaTypeLayout *layout{ probe.MAREA_layoutOf(MAREA_AST_KIND_ENUM_DECLARATION, nth) };

        ASSERT_NE(layout, nullptr) << "The enum must have a layout in: " << source;

        EXPECT_EQ(MAREA_SemaTypeLayoutGetVerdict(layout), MAREA_SEMA_MEASURE_VERDICT_MEASURED) << source;

        EXPECT_EQ(MAREA_sizeOf(layout), size) << "The size is wrong in: " << source;

        EXPECT_EQ(MAREA_alignmentOf(layout), alignment) << "The alignment is wrong in: " << source;

        EXPECT_EQ(MAREA_tagSizeOf(layout), tagSize) << "The tag size is wrong in: " << source;

        EXPECT_EQ(MAREA_membersOf(layout), payloadOffsets) << "The payload offsets are wrong in: " << source;
    }

    void MAREA_expectUndecided(const std::string &source, MAREA_AstKind kind, std::size_t nth, MAREA_SemaMeasureReason reason, MAREA_IntegerSize memberCount, MAREA_ResolveCallback resolve = nullptr)
    {
        MAREA_LayoutProbe probe(resolve);

        ASSERT_TRUE(probe.MAREA_build(source)) << "The source must check with zero errors: " << source;

        const MAREA_SemaTypeLayout *layout{ probe.MAREA_layoutOf(kind, nth) };

        ASSERT_NE(layout, nullptr) << "The declaration must have a layout in: " << source;

        EXPECT_EQ(MAREA_SemaTypeLayoutGetVerdict(layout), MAREA_SEMA_MEASURE_VERDICT_UNDECIDED) << source;

        EXPECT_EQ(MAREA_SemaTypeLayoutGetReason(layout), reason) << "The reason is wrong in: " << source;

        EXPECT_EQ(MAREA_SemaTypeLayoutGetMemberCount(layout), memberCount) << "The member count is known even when undecided, in: " << source;

        MAREA_Uinteger64 value{ 0 };

        EXPECT_EQ(MAREA_SemaTypeLayoutGetSize(layout, &value), MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_UNDECIDED) << source;

        EXPECT_EQ(MAREA_SemaTypeLayoutGetAlignment(layout, &value), MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_UNDECIDED) << source;

        EXPECT_EQ(MAREA_SemaTypeLayoutGetTagSize(layout, &value), MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_UNDECIDED) << source;

        if (memberCount > 0) {
            EXPECT_EQ(MAREA_SemaTypeLayoutGetMemberOffset(layout, 0, &value), MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_UNDECIDED) << source;
        }
    }

    const MAREA_ResolveCallback MAREA_EXTERNAL_RESOLVE{ [](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; } };
} // namespace

TEST(MAREASemaLayout, StructureFollowsTheDeclarationOrderWithNaturalPadding)
{
    GTEST_COUT << "Validating a structure keeps the declaration order and pads each field to its own alignment." << std::endl;

    MAREA_expectStructure("namespace main; structure P a: uinteger8; b: uinteger32; c: uinteger8; end", 0, 12, 4, { 0, 4, 8 });

    MAREA_expectStructure("namespace main; structure P a: uinteger8; b: float64; c: uinteger16; end", 0, 24, 8, { 0, 8, 16 });
}

TEST(MAREASemaLayout, BothLayoutValuesGiveTheSameResult)
{
    GTEST_COUT << "Validating the layout keys c and marea, and no key, measure the same structure the same way." << std::endl;

    const std::string BODY{ "structure P a: uinteger8; b: float64; c: uinteger16; end" };

    MAREA_expectStructure("namespace main; " + BODY, 0, 24, 8, { 0, 8, 16 });

    MAREA_expectStructure("namespace main; #[layout = c] " + BODY, 0, 24, 8, { 0, 8, 16 });

    MAREA_expectStructure("namespace main; #[layout = marea] " + BODY, 0, 24, 8, { 0, 8, 16 });
}

TEST(MAREASemaLayout, EveryFixedWidthPrimitiveHasItsOwnWidth)
{
    GTEST_COUT << "Validating the width and the alignment of each fixed-width primitive." << std::endl;

    MAREA_expectStructure("namespace main; structure P a: boolean; b: character; c: sinteger8; d: uinteger8; e: sinteger16; f: uinteger16; g: sinteger32; h: uinteger32; i: float32; j: sinteger64; k: uinteger64; l: float64; end", 0, 48, 8, { 0, 1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 40 });
}

TEST(MAREASemaLayout, AliasOfAPrimitiveIsThePrimitive)
{
    GTEST_COUT << "Validating a type alias of a primitive has the layout of the primitive." << std::endl;

    MAREA_expectStructure("namespace main; type Byte = uinteger8; structure P a: Byte; b: uinteger16; end", 0, 4, 2, { 0, 2 });
}

TEST(MAREASemaLayout, PackAndAlignChangeTheLayout)
{
    GTEST_COUT << "Validating pack caps the alignment and align raises it, on a structure and on a field." << std::endl;

    MAREA_expectStructure("namespace main; #[pack = 1] structure P a: uinteger8; b: uinteger32; end", 0, 5, 1, { 0, 1 });

    MAREA_expectStructure("namespace main; #[align = 16] structure P a: uinteger8; end", 0, 16, 16, { 0 });

    MAREA_expectStructure("namespace main; structure P a: uinteger8; #[align = 8] b: uinteger8; end", 0, 16, 8, { 0, 8 });

    MAREA_expectStructure("namespace main; #![pack = 1] structure P a: uinteger8; b: uinteger32; end", 0, 5, 1, { 0, 1 });
}

TEST(MAREASemaLayout, StructureHeldByValueIsMeasuredInPlace)
{
    GTEST_COUT << "Validating a nested structure, declared before or after its holder, uses its size and alignment." << std::endl;

    MAREA_expectStructure("namespace main; structure Outer x: uinteger8; i: Inner; y: uinteger64; end structure Inner a: uinteger8; b: uinteger16; end", 0, 16, 8, { 0, 2, 8 });

    MAREA_expectStructure("namespace main; structure Outer x: uinteger8; i: Inner; y: uinteger64; end structure Inner a: uinteger8; b: uinteger16; end", 1, 4, 2, { 0, 2 });

    MAREA_expectStructure("namespace main; #[layout = c] structure Outer i: Inner; end structure Inner a: uinteger8; end", 0, 1, 1, { 0 });
}

TEST(MAREASemaLayout, EmptyStructureHasNoSize)
{
    GTEST_COUT << "Validating a structure with no field has size 0, alignment 1 and no member." << std::endl;

    MAREA_expectStructure("namespace main; structure E end", 0, 0, 1, {});
}

TEST(MAREASemaLayout, EnumWithoutDataIsItsTag)
{
    GTEST_COUT << "Validating an enum with no data is its tag, sinteger32 by default and the written tag type else." << std::endl;

    MAREA_expectEnum("namespace main; enum Color Red; Green; Blue; end", 0, 4, 4, 4, { 4, 4, 4 });

    MAREA_expectEnum("namespace main; enum Small : uinteger8 A; B; end", 0, 1, 1, 1, { 1, 1 });

    MAREA_expectEnum("namespace main; enum Mid : sinteger16 A; end", 0, 2, 2, 2, { 2 });

    MAREA_expectEnum("namespace main; enum Wide : uinteger64 A; end", 0, 8, 8, 8, { 8 });

    MAREA_LayoutProbe probe;

    ASSERT_TRUE(probe.MAREA_build("namespace main; enum Color Red; Green; end"));

    const MAREA_SemaTypeLayout *layout{ probe.MAREA_layoutOf(MAREA_AST_KIND_ENUM_DECLARATION, 0) };

    ASSERT_NE(layout, nullptr);

    EXPECT_EQ(MAREA_SemaTypeLayoutGetVariantFieldCount(layout, 0), MAREA_INTEGER_SIZE(0)) << "A variant with no data has no field.";
}

TEST(MAREASemaLayout, EnumWithDataIsTheTagThenAUnion)
{
    GTEST_COUT << "Validating an enum with data is the tag, then the largest variant, with the largest alignment." << std::endl;

    const std::string SHAPE{ "namespace main; enum Shape : uinteger8 Circle(radius: float32); Rect(w: float64, h: float64); Empty; end" };

    MAREA_expectEnum(SHAPE, 0, 24, 8, 1, { 8, 8, 8 });

    MAREA_LayoutProbe probe;

    ASSERT_TRUE(probe.MAREA_build(SHAPE));

    const MAREA_SemaTypeLayout *layout{ probe.MAREA_layoutOf(MAREA_AST_KIND_ENUM_DECLARATION, 0) };

    ASSERT_NE(layout, nullptr);

    EXPECT_EQ(MAREA_variantFieldsOf(layout, 0), Offsets({ 8 })) << "The field of the first variant sits at the payload.";

    EXPECT_EQ(MAREA_variantFieldsOf(layout, 1), Offsets({ 8, 16 })) << "The fields of a variant follow the rules of a structure.";

    EXPECT_TRUE(MAREA_variantFieldsOf(layout, 2).empty()) << "A variant with no data has no field.";

    MAREA_expectEnum("namespace main; enum Opt : uinteger32 None; Some(v: uinteger8); end", 0, 8, 4, 4, { 4, 4 });

    MAREA_expectEnum("namespace main; enum E : uinteger8 A(v: uinteger8); end", 0, 2, 1, 1, { 1 });

    MAREA_LayoutProbe mixed;

    ASSERT_TRUE(mixed.MAREA_build("namespace main; enum E : uinteger8 Mixed(a: uinteger8, b: uinteger32); end"));

    const MAREA_SemaTypeLayout *mixedLayout{ mixed.MAREA_layoutOf(MAREA_AST_KIND_ENUM_DECLARATION, 0) };

    ASSERT_NE(mixedLayout, nullptr);

    EXPECT_EQ(MAREA_sizeOf(mixedLayout), 12U) << "The payload starts at 4, the variant is 8 bytes.";

    EXPECT_EQ(MAREA_alignmentOf(mixedLayout), 4U);

    EXPECT_EQ(MAREA_variantFieldsOf(mixedLayout, 0), Offsets({ 4, 8 }));
}

TEST(MAREASemaLayout, EnumTakesPackAndAlign)
{
    GTEST_COUT << "Validating pack and align change the layout of an enum as they do for a structure." << std::endl;

    MAREA_expectEnum("namespace main; #[align = 16] enum E : uinteger8 A; end", 0, 16, 16, 1, { 1 });

    MAREA_expectEnum("namespace main; #[pack = 1] enum E : uinteger32 A(v: uinteger8); end", 0, 5, 1, 4, { 4 });
}

TEST(MAREASemaLayout, StructureHoldsAnEnumByValue)
{
    GTEST_COUT << "Validating an enum held by value in a structure is measured." << std::endl;

    MAREA_expectStructure("namespace main; enum Kind : uinteger8 A; B; end structure S a: uinteger8; k: Kind; b: uinteger32; end", 0, 8, 4, { 0, 1, 4 });

    MAREA_expectStructure("namespace main; enum E : uinteger8 X(v: float64); end structure S a: uinteger8; e: E; end", 0, 24, 8, { 0, 8 });
}

TEST(MAREASemaLayout, LibraryTypeIsUndecided)
{
    GTEST_COUT << "Validating a structure or an enum that holds a library type is undecided, with the library reason." << std::endl;

    const std::vector<std::string> LIBRARY_TYPES{ "string", "array<sinteger32, 4>", "vector<sinteger32>", "pointer<sinteger32>", "slice<sinteger32>", "task<sinteger32>", "owned<sinteger32>" };

    for (const std::string &type : LIBRARY_TYPES) {
        MAREA_expectUndecided("namespace main; structure S a: uinteger8; b: " + type + "; end", MAREA_AST_KIND_STRUCTURE_DECLARATION, 0, MAREA_SEMA_MEASURE_REASON_LIBRARY_TYPE, 2);

        MAREA_expectUndecided("namespace main; enum E A; B(v: " + type + "); end", MAREA_AST_KIND_ENUM_DECLARATION, 0, MAREA_SEMA_MEASURE_REASON_LIBRARY_TYPE, 2);
    }

    MAREA_expectUndecided("namespace main; import c.stdio; structure S a: uinteger8; f: stdio.File; end", MAREA_AST_KIND_STRUCTURE_DECLARATION, 0, MAREA_SEMA_MEASURE_REASON_LIBRARY_TYPE, 2, MAREA_EXTERNAL_RESOLVE);
}

TEST(MAREASemaLayout, HoldingAnUndecidedTypeIsUndecided)
{
    GTEST_COUT << "Validating an undecided type makes every structure and enum that holds it undecided, with its reason." << std::endl;

    const std::string INNER{ "structure Inner s: string; end " };

    MAREA_expectUndecided("namespace main; " + INNER + "structure Outer a: uinteger8; i: Inner; end", MAREA_AST_KIND_STRUCTURE_DECLARATION, 1, MAREA_SEMA_MEASURE_REASON_LIBRARY_TYPE, 2);

    MAREA_expectUndecided("namespace main; " + INNER + "enum E A(i: Inner); end", MAREA_AST_KIND_ENUM_DECLARATION, 0, MAREA_SEMA_MEASURE_REASON_LIBRARY_TYPE, 1);

    MAREA_expectUndecided("namespace main; enum E A(s: string); end structure S k: E; end", MAREA_AST_KIND_STRUCTURE_DECLARATION, 0, MAREA_SEMA_MEASURE_REASON_LIBRARY_TYPE, 1);
}

TEST(MAREASemaLayout, GenericStructureIsUndecided)
{
    GTEST_COUT << "Validating a generic structure is undecided with the generic reason." << std::endl;

    MAREA_expectUndecided("namespace main; structure Box<T> v: T; end", MAREA_AST_KIND_STRUCTURE_DECLARATION, 0, MAREA_SEMA_MEASURE_REASON_GENERIC, 1);
}

TEST(MAREASemaLayout, FunctionTypeFieldIsUndecided)
{
    GTEST_COUT << "Validating a field of a function type is undecided with its own reason." << std::endl;

    MAREA_expectUndecided("namespace main; structure S op: function(x: sinteger32): sinteger32; end", MAREA_AST_KIND_STRUCTURE_DECLARATION, 0, MAREA_SEMA_MEASURE_REASON_FUNCTION_TYPE, 1);
}

TEST(MAREASemaLayout, AlignThatSemaCannotComputeIsUndecided)
{
    GTEST_COUT << "Validating an align above the limit sema computes with is undecided, not an error." << std::endl;

    MAREA_expectUndecided("namespace main; #[align = 8589934592] structure S a: uinteger8; end", MAREA_AST_KIND_STRUCTURE_DECLARATION, 0, MAREA_SEMA_MEASURE_REASON_INVALID_ATTRIBUTE, 1);
}

TEST(MAREASemaLayout, ChainOfMoreThanEightLevelsIsUndecided)
{
    GTEST_COUT << "Validating a chain of structures deeper than 8 levels is undecided, and one of 8 levels is measured." << std::endl;

    std::string source{ "namespace main; structure S0 a: uinteger8; end" };

    for (int level{ 1 }; level <= 9; ++level) {
        source += " structure S" + std::to_string(level) + " a: S" + std::to_string(level - 1) + "; end";
    }

    MAREA_LayoutProbe probe;

    ASSERT_TRUE(probe.MAREA_build(source));

    const MAREA_SemaTypeLayout *eight{ probe.MAREA_layoutOf(MAREA_AST_KIND_STRUCTURE_DECLARATION, 8) };

    ASSERT_NE(eight, nullptr);

    EXPECT_EQ(MAREA_SemaTypeLayoutGetVerdict(eight), MAREA_SEMA_MEASURE_VERDICT_MEASURED);

    EXPECT_EQ(MAREA_sizeOf(eight), 1U);

    const MAREA_SemaTypeLayout *nine{ probe.MAREA_layoutOf(MAREA_AST_KIND_STRUCTURE_DECLARATION, 9) };

    ASSERT_NE(nine, nullptr);

    EXPECT_EQ(MAREA_SemaTypeLayoutGetVerdict(nine), MAREA_SEMA_MEASURE_VERDICT_UNDECIDED);

    EXPECT_EQ(MAREA_SemaTypeLayoutGetReason(nine), MAREA_SEMA_MEASURE_REASON_TOO_DEEP);
}

TEST(MAREASemaLayout, NodeThatIsNoStructureOrEnumHasNoLayout)
{
    GTEST_COUT << "Validating MAREA_SemaTypedNodeGetLayout on the error paths." << std::endl;

    MAREA_LayoutProbe probe;

    ASSERT_TRUE(probe.MAREA_build("namespace main; opaque structure Handle; function f() end structure S a: uinteger8; end"));

    const MAREA_SemaTypeLayout *layout{ nullptr };

    EXPECT_EQ(MAREA_SemaTypedNodeGetLayout(probe.MAREA_nodeOf(MAREA_AST_KIND_FUNCTION_DECLARATION, 0), &layout), MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NO_LAYOUT);

    EXPECT_EQ(MAREA_SemaTypedNodeGetLayout(probe.MAREA_nodeOf(MAREA_AST_KIND_OPAQUE_DECLARATION, 0), &layout), MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NO_LAYOUT) << "An opaque structure has no known layout.";

    EXPECT_EQ(MAREA_SemaTypedNodeGetLayout(nullptr, &layout), MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER);

    EXPECT_EQ(MAREA_SemaTypedNodeGetLayout(probe.MAREA_nodeOf(MAREA_AST_KIND_STRUCTURE_DECLARATION, 0), nullptr), MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER);
}

TEST(MAREASemaLayout, AccessorsRefuseWhatTheyCannotAnswer)
{
    GTEST_COUT << "Validating the null and out of range paths of the layout accessors." << std::endl;

    MAREA_LayoutProbe probe;

    ASSERT_TRUE(probe.MAREA_build("namespace main; structure S a: uinteger8; b: uinteger8; end enum E A(v: uinteger8); end"));

    const MAREA_SemaTypeLayout *structure{ probe.MAREA_layoutOf(MAREA_AST_KIND_STRUCTURE_DECLARATION, 0) };

    const MAREA_SemaTypeLayout *enumeration{ probe.MAREA_layoutOf(MAREA_AST_KIND_ENUM_DECLARATION, 0) };

    ASSERT_NE(structure, nullptr);

    ASSERT_NE(enumeration, nullptr);

    MAREA_Uinteger64 value{ 99 };

    EXPECT_EQ(MAREA_SemaTypeLayoutGetVerdict(nullptr), MAREA_SEMA_MEASURE_VERDICT_UNDECIDED);

    EXPECT_EQ(MAREA_SemaTypeLayoutGetReason(nullptr), MAREA_SEMA_MEASURE_REASON_UNRESOLVED);

    EXPECT_EQ(MAREA_SemaTypeLayoutGetMemberCount(nullptr), MAREA_INTEGER_SIZE(0));

    EXPECT_EQ(MAREA_SemaTypeLayoutGetVariantFieldCount(nullptr, 0), MAREA_INTEGER_SIZE(0));

    EXPECT_EQ(MAREA_SemaTypeLayoutGetSize(nullptr, &value), MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER);

    EXPECT_EQ(MAREA_SemaTypeLayoutGetSize(structure, nullptr), MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER);

    EXPECT_EQ(MAREA_SemaTypeLayoutGetAlignment(nullptr, &value), MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER);

    EXPECT_EQ(MAREA_SemaTypeLayoutGetAlignment(structure, nullptr), MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER);

    EXPECT_EQ(MAREA_SemaTypeLayoutGetTagSize(nullptr, &value), MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER);

    EXPECT_EQ(MAREA_SemaTypeLayoutGetTagSize(structure, nullptr), MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER);

    EXPECT_EQ(MAREA_SemaTypeLayoutGetTagSize(structure, &value), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS);

    EXPECT_EQ(value, 0U) << "A structure has no tag.";

    EXPECT_EQ(MAREA_SemaTypeLayoutGetMemberOffset(nullptr, 0, &value), MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER);

    EXPECT_EQ(MAREA_SemaTypeLayoutGetMemberOffset(structure, 0, nullptr), MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER);

    EXPECT_EQ(MAREA_SemaTypeLayoutGetMemberOffset(structure, 2, &value), MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER) << "An index past the last field is refused.";

    EXPECT_EQ(MAREA_SemaTypeLayoutGetVariantFieldCount(structure, 0), MAREA_INTEGER_SIZE(0)) << "A structure has no variant.";

    EXPECT_EQ(MAREA_SemaTypeLayoutGetVariantFieldCount(enumeration, 5), MAREA_INTEGER_SIZE(0));

    EXPECT_EQ(MAREA_SemaTypeLayoutGetVariantFieldOffset(nullptr, 0, 0, &value), MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER);

    EXPECT_EQ(MAREA_SemaTypeLayoutGetVariantFieldOffset(enumeration, 0, 0, nullptr), MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER);

    EXPECT_EQ(MAREA_SemaTypeLayoutGetVariantFieldOffset(enumeration, 0, 1, &value), MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER) << "A field index past the last field of the variant is refused.";

    EXPECT_EQ(MAREA_SemaTypeLayoutGetVariantFieldOffset(enumeration, 1, 0, &value), MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER) << "A variant index past the last variant is refused.";

    EXPECT_EQ(MAREA_SemaTypeLayoutGetVariantFieldOffset(structure, 0, 0, &value), MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER) << "A structure has no variant field.";
}

TEST(MAREASemaLayout, ConvertDiagnosticsStayTheSame)
{
    GTEST_COUT << "Validating convert between structures still raises the same code now that enums and marea layouts are measured." << std::endl;

    auto codesOf = [](const std::string &source) {
        MAREA_SemaFixture fixture;

        MAREA_IGNORE(fixture.MAREA_sema(source));

        return fixture.MAREA_semaCodes();
    };

    const std::string HEAD{ "namespace main; enum Kind : uinteger8 A; B; end " };

    EXPECT_EQ(codesOf(HEAD + "structure P k: Kind; end structure Q k: Kind; end function f(p: P) local q: Q = convert<Q>(p); end"), std::vector<MAREA_Uinteger32>{ 3011 }) << "A structure with an enum field is still not compared, so convert is still refused.";

    EXPECT_EQ(codesOf("namespace main; structure Inner a: uinteger8; end #[layout = c] structure P i: Inner; end #[layout = c] structure Q i: Inner; end function f(p: P) local q: Q = convert<Q>(p); end"), std::vector<MAREA_Uinteger32>{ 3011 }) << "A marea structure inside a c structure is still not compared.";

    EXPECT_TRUE(codesOf("namespace main; #[layout = c] structure P a: uinteger8; b: uinteger32; end #[layout = c] structure Q c: uinteger8; d: uinteger32; end function f(p: P) local q: Q = convert<Q>(p); end").empty()) << "Two c structures with the same shape are still compatible.";
}
