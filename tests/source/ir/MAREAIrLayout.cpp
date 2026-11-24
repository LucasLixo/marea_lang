// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "marea/ir/MAREAIrLayout.h"
#include "marea/sema/MAREASemaLayout.h"

#include "tests/MAREAIrHelpers.hpp"

#include <cstdint>
#include <set>

namespace
{
    using Offsets = std::vector<std::uint64_t>;

    const MAREA_ResolveCallback RESOLVE_EXTERNAL{ [](const std::string &text, const MAREA_AstNode **) {
        const bool EXTERNAL{ (text.compare(0, 12, "collections.") == 0) || (text.compare(0, 2, "c.") == 0) };

        return EXTERNAL ? MAREA_SEMA_IMPORT_RESULT_EXTERNAL : MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
    } };

    const MAREA_IrLayout *MAREA_layoutAt(const MAREA_IrNode *module, MAREA_IntegerSize index)
    {
        const MAREA_IrLayout *layout{ nullptr };

        if (MAREA_IrModuleGetLayoutAt(module, index, &layout) != MAREA_IR_LAYOUT_EXCEPT_SUCCESS) {
            return nullptr;
        }

        return layout;
    }

    std::string MAREA_nameOf(const MAREA_IrLayout *layout)
    {
        return std::string(MAREA_IrLayoutGetName(layout), MAREA_IrLayoutGetNameLength(layout));
    }

    std::uint64_t MAREA_numberOf(MAREA_IrLayoutExcept (*getter)(const MAREA_IrLayout *, MAREA_Uinteger64 *), const MAREA_IrLayout *layout)
    {
        MAREA_Uinteger64 value{ 0 };

        EXPECT_EQ(getter(layout, &value), MAREA_IR_LAYOUT_EXCEPT_SUCCESS) << "The getter must succeed for a measured entry.";

        return value;
    }

    Offsets MAREA_membersOf(const MAREA_IrLayout *layout)
    {
        Offsets result;

        for (MAREA_IntegerSize index{ 0 }; index < MAREA_IrLayoutGetMemberCount(layout); ++index) {
            MAREA_Uinteger64 value{ 0 };

            EXPECT_EQ(MAREA_IrLayoutGetMemberOffset(layout, index, &value), MAREA_IR_LAYOUT_EXCEPT_SUCCESS) << "MAREA_IrLayoutGetMemberOffset must succeed for a measured entry.";

            result.push_back(value);
        }

        return result;
    }

    Offsets MAREA_variantFieldsOf(const MAREA_IrLayout *layout, MAREA_IntegerSize variant)
    {
        Offsets result;

        for (MAREA_IntegerSize index{ 0 }; index < MAREA_IrLayoutGetVariantFieldCount(layout, variant); ++index) {
            MAREA_Uinteger64 value{ 0 };

            EXPECT_EQ(MAREA_IrLayoutGetVariantFieldOffset(layout, variant, index, &value), MAREA_IR_LAYOUT_EXCEPT_SUCCESS) << "MAREA_IrLayoutGetVariantFieldOffset must succeed for a measured entry.";

            result.push_back(value);
        }

        return result;
    }

    void MAREA_expectStructure(const MAREA_IrNode *module, MAREA_IntegerSize index, const std::string &name, std::uint64_t size, std::uint64_t alignment, const Offsets &offsets)
    {
        const MAREA_IrLayout *layout{ MAREA_layoutAt(module, index) };

        ASSERT_NE(layout, nullptr) << name;

        EXPECT_EQ(MAREA_nameOf(layout), name) << name << " The entry must carry the name of its declaration.";

        EXPECT_EQ(MAREA_IrLayoutGetKind(layout), MAREA_IR_LAYOUT_KIND_STRUCTURE) << name;

        EXPECT_EQ(MAREA_IrLayoutGetVerdict(layout), MAREA_SEMA_MEASURE_VERDICT_MEASURED) << name;

        EXPECT_EQ(MAREA_IrLayoutGetReason(layout), MAREA_SEMA_MEASURE_REASON_NONE) << name;

        EXPECT_EQ(MAREA_numberOf(MAREA_IrLayoutGetSize, layout), size) << name;

        EXPECT_EQ(MAREA_numberOf(MAREA_IrLayoutGetAlignment, layout), alignment) << name;

        EXPECT_EQ(MAREA_numberOf(MAREA_IrLayoutGetTagSize, layout), 0U) << name << " A structure has no tag.";

        EXPECT_EQ(MAREA_membersOf(layout), offsets) << name;
    }

    void MAREA_expectEnum(const MAREA_IrNode *module, MAREA_IntegerSize index, const std::string &name, std::uint64_t size, std::uint64_t alignment, std::uint64_t tagSize, const Offsets &payloads)
    {
        const MAREA_IrLayout *layout{ MAREA_layoutAt(module, index) };

        ASSERT_NE(layout, nullptr) << name;

        EXPECT_EQ(MAREA_nameOf(layout), name) << name << " The entry must carry the name of its declaration.";

        EXPECT_EQ(MAREA_IrLayoutGetKind(layout), MAREA_IR_LAYOUT_KIND_ENUM) << name;

        EXPECT_EQ(MAREA_IrLayoutGetVerdict(layout), MAREA_SEMA_MEASURE_VERDICT_MEASURED) << name;

        EXPECT_EQ(MAREA_numberOf(MAREA_IrLayoutGetSize, layout), size) << name;

        EXPECT_EQ(MAREA_numberOf(MAREA_IrLayoutGetAlignment, layout), alignment) << name;

        EXPECT_EQ(MAREA_numberOf(MAREA_IrLayoutGetTagSize, layout), tagSize) << name;

        EXPECT_EQ(MAREA_membersOf(layout), payloads) << name;
    }

    void MAREA_expectUndecided(const MAREA_IrNode *module, MAREA_IntegerSize index, const std::string &name, MAREA_SemaMeasureReason reason, MAREA_IntegerSize members)
    {
        const MAREA_IrLayout *layout{ MAREA_layoutAt(module, index) };

        ASSERT_NE(layout, nullptr) << name;

        EXPECT_EQ(MAREA_nameOf(layout), name) << name << " The entry must carry the name of its declaration.";

        EXPECT_EQ(MAREA_IrLayoutGetVerdict(layout), MAREA_SEMA_MEASURE_VERDICT_UNDECIDED) << name;

        EXPECT_EQ(MAREA_IrLayoutGetReason(layout), reason) << name;

        EXPECT_EQ(MAREA_IrLayoutGetMemberCount(layout), members) << name << " The member count is known when the layout is undecided.";

        MAREA_Uinteger64 value{ 77 };

        EXPECT_EQ(MAREA_IrLayoutGetSize(layout, &value), MAREA_IR_LAYOUT_EXCEPT_ERROR_UNDECIDED) << name;

        EXPECT_EQ(MAREA_IrLayoutGetAlignment(layout, &value), MAREA_IR_LAYOUT_EXCEPT_ERROR_UNDECIDED) << name;

        EXPECT_EQ(MAREA_IrLayoutGetTagSize(layout, &value), MAREA_IR_LAYOUT_EXCEPT_ERROR_UNDECIDED) << name;

        EXPECT_EQ(MAREA_IrLayoutGetMemberOffset(layout, 0, &value), MAREA_IR_LAYOUT_EXCEPT_ERROR_UNDECIDED) << name;

        EXPECT_EQ(MAREA_IrLayoutGetVariantFieldOffset(layout, 0, 0, &value), MAREA_IR_LAYOUT_EXCEPT_ERROR_UNDECIDED) << name;

        EXPECT_EQ(value, 77U) << name << " A refused call must not write the output.";
    }

    std::string MAREA_dumpOf(MAREA_IrFixture &fixture)
    {
        const MAREA_Character8 *text{ nullptr };

        MAREA_IntegerSize length{ 0 };

        if (MAREA_IrModuleDump(fixture.sema.parser.lexer.arena, fixture.module, &text, &length) != MAREA_IR_EXCEPT_SUCCESS) {
            return "<dump failed>";
        }

        return std::string(text, length);
    }

    std::string MAREA_sectionOf(MAREA_IrFixture &fixture)
    {
        const std::string DUMP{ MAREA_dumpOf(fixture) };

        const std::size_t START{ DUMP.find("LAYOUT_TABLE\n") };

        return (START == std::string::npos) ? "" : DUMP.substr(START);
    }
} // namespace

TEST(MAREAIrLayout, MeasuredStructureHasSizeAlignmentAndFieldOffsets)
{
    GTEST_COUT << "Validating a structure entry holds the verdict, the size, the alignment and the offset of each field." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; structure P a: uinteger8; b: uinteger32; c: uinteger8; end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    ASSERT_EQ(MAREA_IrModuleGetLayoutCount(fixture.module), 1U) << "One structure must give one entry.";

    MAREA_expectStructure(fixture.module, 0, "P", 12, 4, { 0, 4, 8 });
}

TEST(MAREAIrLayout, NestedStructureHasAnEntryEach)
{
    GTEST_COUT << "Validating a nested structure gives two entries in the declaration order, the outer one measured with the inner one." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; structure Outer x: uinteger8; i: Inner; y: uinteger64; end structure Inner a: uinteger8; b: uinteger16; end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    ASSERT_EQ(MAREA_IrModuleGetLayoutCount(fixture.module), 2U) << "The unit must give the expected number of entries.";

    MAREA_expectStructure(fixture.module, 0, "Outer", 16, 8, { 0, 2, 8 });

    MAREA_expectStructure(fixture.module, 1, "Inner", 4, 2, { 0, 2 });
}

TEST(MAREAIrLayout, EnumWithoutDataIsItsTag)
{
    GTEST_COUT << "Validating an enum with no data has its tag as the whole value and no variant field." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; enum Color Red; Green; Blue; end enum Small : uinteger8 A; B; end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    ASSERT_EQ(MAREA_IrModuleGetLayoutCount(fixture.module), 2U) << "The unit must give the expected number of entries.";

    MAREA_expectEnum(fixture.module, 0, "Color", 4, 4, 4, { 4, 4, 4 });

    MAREA_expectEnum(fixture.module, 1, "Small", 1, 1, 1, { 1, 1 });

    const MAREA_IrLayout *color{ MAREA_layoutAt(fixture.module, 0) };

    EXPECT_EQ(MAREA_IrLayoutGetVariantFieldCount(color, 0), 0U) << "A variant with no data has no field.";
}

TEST(MAREAIrLayout, EnumWithDataHasTheVariantFieldOffsets)
{
    GTEST_COUT << "Validating an enum with data holds the tag size, the payload offset and each variant field offset." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; enum Shape : uinteger8 Circle(radius: float32); Rect(w: float64, h: float64); Empty; end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    ASSERT_EQ(MAREA_IrModuleGetLayoutCount(fixture.module), 1U) << "The unit must give the expected number of entries.";

    MAREA_expectEnum(fixture.module, 0, "Shape", 24, 8, 1, { 8, 8, 8 });

    const MAREA_IrLayout *layout{ MAREA_layoutAt(fixture.module, 0) };

    EXPECT_EQ(MAREA_variantFieldsOf(layout, 0), Offsets({ 8 })) << "The variant must hold the offsets of its fields.";

    EXPECT_EQ(MAREA_variantFieldsOf(layout, 1), Offsets({ 8, 16 })) << "The variant must hold the offsets of its fields.";

    EXPECT_TRUE(MAREA_variantFieldsOf(layout, 2).empty()) << "A variant with no data must have no field offset.";
}

TEST(MAREAIrLayout, CLayoutAndDefaultStructureGiveTheSameNumbers)
{
    GTEST_COUT << "Validating a structure with layout = c and a default structure get an entry each, with the numbers sema measured." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; #[layout = c] structure Packed a: uinteger8; b: float64; c: uinteger16; end structure Plain a: uinteger8; b: float64; c: uinteger16; end #[layout = c, pack = 1] structure Header magic: uinteger32; version: uinteger8; end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    ASSERT_EQ(MAREA_IrModuleGetLayoutCount(fixture.module), 3U) << "The unit must give the expected number of entries.";

    MAREA_expectStructure(fixture.module, 0, "Packed", 24, 8, { 0, 8, 16 });

    MAREA_expectStructure(fixture.module, 1, "Plain", 24, 8, { 0, 8, 16 });

    MAREA_expectStructure(fixture.module, 2, "Header", 5, 1, { 0, 4 });
}

TEST(MAREAIrLayout, StructureHoldingALibraryTypeIsUndecided)
{
    GTEST_COUT << "Validating a structure that holds a library type is undecided with its reason and no number." << std::endl;

    MAREA_IrFixture fixture(RESOLVE_EXTERNAL);

    ASSERT_EQ(fixture.MAREA_lower("namespace main; import collections.array; structure S a: uinteger8; b: array<sinteger32, 4>; end enum E A; B(v: array<sinteger32, 4>); end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    ASSERT_EQ(MAREA_IrModuleGetLayoutCount(fixture.module), 2U) << "The unit must give the expected number of entries.";

    MAREA_expectUndecided(fixture.module, 0, "S", MAREA_SEMA_MEASURE_REASON_LIBRARY_TYPE, 2);

    MAREA_expectUndecided(fixture.module, 1, "E", MAREA_SEMA_MEASURE_REASON_LIBRARY_TYPE, 2);

    EXPECT_EQ(MAREA_IrLayoutGetKind(MAREA_layoutAt(fixture.module, 1)), MAREA_IR_LAYOUT_KIND_ENUM) << "An undecided entry keeps its kind.";
}

TEST(MAREAIrLayout, OpaqueStructureIsUndecidedWithTheOpaqueReason)
{
    GTEST_COUT << "Validating an opaque structure has an entry that is undecided with the opaque reason and no member." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; opaque structure Texture;"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    ASSERT_EQ(MAREA_IrModuleGetLayoutCount(fixture.module), 1U) << "The unit must give the expected number of entries.";

    MAREA_expectUndecided(fixture.module, 0, "Texture", MAREA_SEMA_MEASURE_REASON_OPAQUE, 0);

    EXPECT_EQ(MAREA_IrLayoutGetKind(MAREA_layoutAt(fixture.module, 0)), MAREA_IR_LAYOUT_KIND_STRUCTURE) << "The entry must describe a structure.";
}

TEST(MAREAIrLayout, FunctionTypeFieldKeepsItsOwnReason)
{
    GTEST_COUT << "Validating the reason of sema is kept as it is, here the function type reason." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; structure S op: function(x: sinteger32): sinteger32; end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    MAREA_expectUndecided(fixture.module, 0, "S", MAREA_SEMA_MEASURE_REASON_FUNCTION_TYPE, 1);
}

TEST(MAREAIrLayout, EntriesAreTheValuesOfSemaAsTheyAre)
{
    GTEST_COUT << "Validating each entry equals what sema recorded for its declaration, so the ir recomputes nothing." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; enum Kind : uinteger8 A; B(v: float64); end structure S a: uinteger8; k: Kind; b: uinteger32; end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    ASSERT_EQ(MAREA_IrModuleGetLayoutCount(fixture.module), 2U) << "The unit must give the expected number of entries.";

    MAREA_IntegerSize entry{ 0 };

    for (MAREA_IntegerSize index{ 0 }; index < MAREA_SemaTypedNodeGetChildCount(fixture.typedRoot); ++index) {
        const MAREA_SemaTypedNode *item{ nullptr };

        MAREA_IGNORE(MAREA_SemaTypedNodeGetChildAt(fixture.typedRoot, index, &item));

        const MAREA_SemaTypeLayout *source{ nullptr };

        if (MAREA_SemaTypedNodeGetLayout(item, &source) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
            continue;
        }

        const MAREA_IrLayout *layout{ MAREA_layoutAt(fixture.module, entry) };

        ++entry;

        ASSERT_NE(layout, nullptr) << "Each declaration must have an entry.";

        MAREA_Uinteger64 expected{ 0 };

        ASSERT_EQ(MAREA_SemaTypeLayoutGetSize(source, &expected), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "Sema must answer for a measured layout.";

        EXPECT_EQ(MAREA_numberOf(MAREA_IrLayoutGetSize, layout), expected) << "The entry must hold the number that sema recorded.";

        ASSERT_EQ(MAREA_SemaTypeLayoutGetAlignment(source, &expected), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "Sema must answer for a measured layout.";

        EXPECT_EQ(MAREA_numberOf(MAREA_IrLayoutGetAlignment, layout), expected) << "The entry must hold the number that sema recorded.";

        ASSERT_EQ(MAREA_SemaTypeLayoutGetTagSize(source, &expected), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "Sema must answer for a measured layout.";

        EXPECT_EQ(MAREA_numberOf(MAREA_IrLayoutGetTagSize, layout), expected) << "The entry must hold the number that sema recorded.";

        ASSERT_EQ(MAREA_IrLayoutGetMemberCount(layout), MAREA_SemaTypeLayoutGetMemberCount(source)) << "The entry must hold the member count of sema.";

        for (MAREA_IntegerSize member{ 0 }; member < MAREA_SemaTypeLayoutGetMemberCount(source); ++member) {
            ASSERT_EQ(MAREA_SemaTypeLayoutGetMemberOffset(source, member, &expected), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "Sema must answer for a measured layout.";

            MAREA_Uinteger64 actual{ 0 };

            ASSERT_EQ(MAREA_IrLayoutGetMemberOffset(layout, member, &actual), MAREA_IR_LAYOUT_EXCEPT_SUCCESS) << "MAREA_IrLayoutGetMemberOffset must succeed for a measured entry.";

            EXPECT_EQ(actual, expected) << "The offset must be the offset that sema recorded.";

            ASSERT_EQ(MAREA_IrLayoutGetVariantFieldCount(layout, member), MAREA_SemaTypeLayoutGetVariantFieldCount(source, member)) << "The entry must hold the variant field count of sema.";

            for (MAREA_IntegerSize field{ 0 }; field < MAREA_SemaTypeLayoutGetVariantFieldCount(source, member); ++field) {
                ASSERT_EQ(MAREA_SemaTypeLayoutGetVariantFieldOffset(source, member, field, &expected), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "Sema must answer for a measured layout.";

                ASSERT_EQ(MAREA_IrLayoutGetVariantFieldOffset(layout, member, field, &actual), MAREA_IR_LAYOUT_EXCEPT_SUCCESS) << "MAREA_IrLayoutGetVariantFieldOffset must succeed for a measured entry.";

                EXPECT_EQ(actual, expected) << "The offset must be the offset that sema recorded.";
            }
        }
    }

    EXPECT_EQ(entry, 2U) << "Every structure and enum of the unit must have been compared.";
}

TEST(MAREAIrLayout, OnlyStructuresAndEnumsHaveEntries)
{
    GTEST_COUT << "Validating a module with no structure and no enum has an empty table, and a function or a constant adds none." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; constant A: sinteger32 = 1; function f() end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    EXPECT_EQ(MAREA_IrModuleGetLayoutCount(fixture.module), 0U) << "The unit must give the expected number of entries.";

    EXPECT_EQ(MAREA_layoutAt(fixture.module, 0), nullptr) << "An empty table has no entry at 0.";
}

TEST(MAREAIrLayout, DroppedDeclarationHasNoEntry)
{
    GTEST_COUT << "Validating an enum that lowering drops whole has no entry, and the declarations around it keep theirs." << std::endl;

    MAREA_IrFixture fixture(RESOLVE_EXTERNAL);

    ASSERT_EQ(fixture.MAREA_lower("namespace main; structure Before a: uinteger8; end enum Gone A(v: uinteger64 = size_of<function(value: sinteger32): sinteger32>()); end structure After b: uinteger16; end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    ASSERT_EQ(fixture.MAREA_irCodes().size(), 1U) << "The gap inside the enum must be reported once.";

    ASSERT_EQ(MAREA_IrModuleGetLayoutCount(fixture.module), 2U) << "The dropped enum must not leave an entry.";

    MAREA_expectStructure(fixture.module, 0, "Before", 1, 1, { 0 });

    MAREA_expectStructure(fixture.module, 1, "After", 2, 2, { 0 });
}

TEST(MAREAIrLayout, GenericStructureHasNoEntry)
{
    GTEST_COUT << "Validating a generic structure that the ir leaves out has no entry." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; structure Box<T> v: T; end structure Plain a: uinteger8; end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    ASSERT_EQ(MAREA_IrModuleGetLayoutCount(fixture.module), 1U) << "The unit must give the expected number of entries.";

    MAREA_expectStructure(fixture.module, 0, "Plain", 1, 1, { 0 });
}

TEST(MAREAIrLayout, FindLayoutReadsAnEntryByName)
{
    GTEST_COUT << "Validating an entry is found by its name, and a missing name is refused." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; structure A a: uinteger8; end enum B X; end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    const MAREA_IrLayout *found{ nullptr };

    ASSERT_EQ(MAREA_IrModuleFindLayout(fixture.module, "B", 1, &found), MAREA_IR_LAYOUT_EXCEPT_SUCCESS) << "MAREA_IrModuleFindLayout must succeed for a measured entry.";

    EXPECT_EQ(found, MAREA_layoutAt(fixture.module, 1)) << "The found entry must be the entry at the same index.";

    EXPECT_EQ(MAREA_IrModuleFindLayout(fixture.module, "C", 1, &found), MAREA_IR_LAYOUT_EXCEPT_ERROR_NOT_FOUND) << "An unknown name must be refused.";

    EXPECT_EQ(MAREA_IrModuleFindLayout(fixture.module, "AB", 2, &found), MAREA_IR_LAYOUT_EXCEPT_ERROR_NOT_FOUND) << "A name of another length must not match.";

    EXPECT_EQ(MAREA_IrModuleFindLayout(fixture.module, "A", 1, &found), MAREA_IR_LAYOUT_EXCEPT_SUCCESS) << "MAREA_IrModuleFindLayout must succeed for a measured entry.";

    EXPECT_EQ(found, MAREA_layoutAt(fixture.module, 0)) << "The found entry must be the entry at the same index.";
}

TEST(MAREAIrLayout, AccessorsRefuseWhatTheyCannotAnswer)
{
    GTEST_COUT << "Validating the null, range and kind errors of the module and layout accessors." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; structure A a: uinteger8; end enum B X(v: uinteger8); end function f() end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    const MAREA_IrLayout *layout{ nullptr };

    MAREA_Uinteger64 value{ 0 };

    EXPECT_EQ(MAREA_IrModuleGetLayoutCount(nullptr), 0U) << "The unit must give the expected number of entries.";

    EXPECT_EQ(MAREA_IrModuleGetLayoutCount(MAREA_irChild(fixture.module, 0)), 0U) << "A node that is not the module has no table.";

    EXPECT_EQ(MAREA_IrModuleGetLayoutAt(nullptr, 0, &layout), MAREA_IR_LAYOUT_EXCEPT_ERROR_NULL_POINTER) << "MAREA_IrModuleGetLayoutAt must refuse a null pointer.";

    EXPECT_EQ(MAREA_IrModuleGetLayoutAt(fixture.module, 0, nullptr), MAREA_IR_LAYOUT_EXCEPT_ERROR_NULL_POINTER) << "MAREA_IrModuleGetLayoutAt must refuse a null pointer.";

    EXPECT_EQ(MAREA_IrModuleGetLayoutAt(fixture.module, 2, &layout), MAREA_IR_LAYOUT_EXCEPT_ERROR_OUT_OF_RANGE) << "MAREA_IrModuleGetLayoutAt must refuse an index out of range.";

    EXPECT_EQ(MAREA_IrModuleGetLayoutAt(MAREA_irChild(fixture.module, 0), 0, &layout), MAREA_IR_LAYOUT_EXCEPT_ERROR_OUT_OF_RANGE) << "A node that is not the module has an empty table.";

    EXPECT_EQ(MAREA_IrModuleFindLayout(nullptr, "A", 1, &layout), MAREA_IR_LAYOUT_EXCEPT_ERROR_NULL_POINTER) << "MAREA_IrModuleFindLayout must refuse a null pointer.";

    EXPECT_EQ(MAREA_IrModuleFindLayout(fixture.module, nullptr, 1, &layout), MAREA_IR_LAYOUT_EXCEPT_ERROR_NULL_POINTER) << "MAREA_IrModuleFindLayout must refuse a null pointer.";

    EXPECT_EQ(MAREA_IrModuleFindLayout(fixture.module, "A", 1, nullptr), MAREA_IR_LAYOUT_EXCEPT_ERROR_NULL_POINTER) << "MAREA_IrModuleFindLayout must refuse a null pointer.";

    EXPECT_EQ(MAREA_IrLayoutGetKind(nullptr), MAREA_IR_LAYOUT_KIND_COUNT) << "MAREA_IrLayoutGetKind must give its neutral value for a null entry.";

    EXPECT_EQ(MAREA_IrLayoutGetName(nullptr), nullptr) << "MAREA_IrLayoutGetName must give its neutral value for a null entry.";

    EXPECT_EQ(MAREA_IrLayoutGetNameLength(nullptr), 0U) << "MAREA_IrLayoutGetNameLength must give its neutral value for a null entry.";

    EXPECT_EQ(MAREA_IrLayoutGetVerdict(nullptr), MAREA_SEMA_MEASURE_VERDICT_UNDECIDED) << "MAREA_IrLayoutGetVerdict must give its neutral value for a null entry.";

    EXPECT_EQ(MAREA_IrLayoutGetReason(nullptr), MAREA_SEMA_MEASURE_REASON_UNRESOLVED) << "MAREA_IrLayoutGetReason must give its neutral value for a null entry.";

    EXPECT_EQ(MAREA_IrLayoutGetMemberCount(nullptr), 0U) << "MAREA_IrLayoutGetMemberCount must give its neutral value for a null entry.";

    EXPECT_EQ(MAREA_IrLayoutGetVariantFieldCount(nullptr, 0), 0U) << "MAREA_IrLayoutGetVariantFieldCount must give its neutral value for a null entry.";

    EXPECT_EQ(MAREA_IrLayoutGetSize(nullptr, &value), MAREA_IR_LAYOUT_EXCEPT_ERROR_NULL_POINTER) << "MAREA_IrLayoutGetSize must refuse a null pointer.";

    EXPECT_EQ(MAREA_IrLayoutGetAlignment(nullptr, &value), MAREA_IR_LAYOUT_EXCEPT_ERROR_NULL_POINTER) << "MAREA_IrLayoutGetAlignment must refuse a null pointer.";

    EXPECT_EQ(MAREA_IrLayoutGetTagSize(nullptr, &value), MAREA_IR_LAYOUT_EXCEPT_ERROR_NULL_POINTER) << "MAREA_IrLayoutGetTagSize must refuse a null pointer.";

    EXPECT_EQ(MAREA_IrLayoutGetMemberOffset(nullptr, 0, &value), MAREA_IR_LAYOUT_EXCEPT_ERROR_NULL_POINTER) << "MAREA_IrLayoutGetMemberOffset must refuse a null pointer.";

    EXPECT_EQ(MAREA_IrLayoutGetVariantFieldOffset(nullptr, 0, 0, &value), MAREA_IR_LAYOUT_EXCEPT_ERROR_NULL_POINTER) << "MAREA_IrLayoutGetVariantFieldOffset must refuse a null pointer.";

    const MAREA_IrLayout *structure{ MAREA_layoutAt(fixture.module, 0) };

    const MAREA_IrLayout *enumeration{ MAREA_layoutAt(fixture.module, 1) };

    ASSERT_NE(structure, nullptr) << "The entry must exist.";

    ASSERT_NE(enumeration, nullptr) << "The entry must exist.";

    EXPECT_EQ(MAREA_IrLayoutGetSize(structure, nullptr), MAREA_IR_LAYOUT_EXCEPT_ERROR_NULL_POINTER) << "MAREA_IrLayoutGetSize must refuse a null pointer.";

    EXPECT_EQ(MAREA_IrLayoutGetMemberOffset(structure, 0, nullptr), MAREA_IR_LAYOUT_EXCEPT_ERROR_NULL_POINTER) << "MAREA_IrLayoutGetMemberOffset must refuse a null pointer.";

    EXPECT_EQ(MAREA_IrLayoutGetVariantFieldOffset(enumeration, 0, 0, nullptr), MAREA_IR_LAYOUT_EXCEPT_ERROR_NULL_POINTER) << "MAREA_IrLayoutGetVariantFieldOffset must refuse a null pointer.";

    EXPECT_EQ(MAREA_IrLayoutGetMemberOffset(structure, 1, &value), MAREA_IR_LAYOUT_EXCEPT_ERROR_OUT_OF_RANGE) << "A structure with one field has no member 1.";

    EXPECT_EQ(MAREA_IrLayoutGetVariantFieldCount(structure, 0), 0U) << "A structure has no variant.";

    EXPECT_EQ(MAREA_IrLayoutGetVariantFieldCount(enumeration, 1), 0U) << "A variant index out of range has no field.";

    EXPECT_EQ(MAREA_IrLayoutGetVariantFieldOffset(enumeration, 1, 0, &value), MAREA_IR_LAYOUT_EXCEPT_ERROR_OUT_OF_RANGE) << "MAREA_IrLayoutGetVariantFieldOffset must refuse an index out of range.";

    EXPECT_EQ(MAREA_IrLayoutGetVariantFieldOffset(enumeration, 0, 1, &value), MAREA_IR_LAYOUT_EXCEPT_ERROR_OUT_OF_RANGE) << "MAREA_IrLayoutGetVariantFieldOffset must refuse an index out of range.";

    EXPECT_EQ(MAREA_IrLayoutGetVariantFieldOffset(structure, 0, 0, &value), MAREA_IR_LAYOUT_EXCEPT_ERROR_OUT_OF_RANGE) << "A structure has no variant field.";
}

TEST(MAREAIrLayout, DumpWritesTheTableAfterTheTree)
{
    GTEST_COUT << "Validating the dump ends with a LAYOUT_TABLE section, in the declaration order, one line per entry." << std::endl;

    MAREA_IrFixture fixture(RESOLVE_EXTERNAL);

    ASSERT_EQ(fixture.MAREA_lower("namespace main; import collections.array; structure P a: uinteger8; b: uinteger32; end enum Shape : uinteger8 Circle(radius: float32); Rect(w: float64, h: float64); Empty; end structure S v: array<sinteger32, 4>; end opaque structure Texture; enum Color Red; end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    EXPECT_EQ(MAREA_sectionOf(fixture), MAREA_lines({
                                            "LAYOUT_TABLE",
                                            "  STRUCTURE \"P\" verdict=measured size=8 alignment=4 members=2 offsets=0,4",
                                            "  ENUM \"Shape\" verdict=measured size=24 alignment=8 tag=1 members=3",
                                            "    VARIANT 0 payload=8 fields=8",
                                            "    VARIANT 1 payload=8 fields=8,16",
                                            "    VARIANT 2 payload=8 fields=",
                                            "  STRUCTURE \"S\" verdict=undecided reason=library_type members=1",
                                            "  STRUCTURE \"Texture\" verdict=undecided reason=opaque members=0",
                                            "  ENUM \"Color\" verdict=measured size=4 alignment=4 tag=4 members=1",
                                            "    VARIANT 0 payload=4 fields=",
                                        }))
        << "The section must follow the tree, one line per entry and one per variant.";

    const std::string DUMP{ MAREA_dumpOf(fixture) };

    EXPECT_EQ(DUMP.find("LAYOUT_TABLE"), DUMP.rfind("LAYOUT_TABLE")) << "The section must appear once.";

    EXPECT_EQ(DUMP, MAREA_dumpOf(fixture)) << "The dump must be deterministic.";

    for (char byte : DUMP) {
        EXPECT_LT(static_cast<unsigned char>(byte), 0x80U) << "The dump must be ASCII.";
    }
}

TEST(MAREAIrLayout, DumpNamesEveryReason)
{
    GTEST_COUT << "Validating the dump gives each undecided reason its own lower-case name." << std::endl;

    MAREA_IrFixture function(RESOLVE_EXTERNAL);

    ASSERT_EQ(function.MAREA_lower("namespace main; structure S op: function(x: sinteger32): sinteger32; end #[align = 8589934592] structure T a: uinteger8; end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    EXPECT_EQ(MAREA_sectionOf(function), MAREA_lines({
                                             "LAYOUT_TABLE",
                                             "  STRUCTURE \"S\" verdict=undecided reason=function_type members=1",
                                             "  STRUCTURE \"T\" verdict=undecided reason=invalid_attribute members=1",
                                         }))
        << "The dump must name the reason of each undecided entry.";
}

TEST(MAREAIrLayout, DumpWritesNumbersBeyond32Bits)
{
    GTEST_COUT << "Validating the dump writes a size and an alignment above 32 bits in full." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; #[align = 4294967296] structure Big a: uinteger8; end #[align = 2147483648] structure Half a: uinteger8; end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    EXPECT_EQ(MAREA_sectionOf(fixture), MAREA_lines({
                                            "LAYOUT_TABLE",
                                            "  STRUCTURE \"Big\" verdict=measured size=4294967296 alignment=4294967296 members=1 offsets=0",
                                            "  STRUCTURE \"Half\" verdict=measured size=2147483648 alignment=2147483648 members=1 offsets=0",
                                        }))
        << "A number above 32 bits must not be cut.";
}

TEST(MAREAIrLayout, DumpHasNoSectionWithoutAStructureOrAnEnum)
{
    GTEST_COUT << "Validating a module with no structure and no enum dumps with no LAYOUT_TABLE line." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; function f() end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    EXPECT_EQ(MAREA_dumpOf(fixture).find("LAYOUT_TABLE"), std::string::npos) << "A module with no structure and no enum must have no section.";
}

TEST(MAREAIrLayout, EveryStructureAndEnumOfTheExamplesHasAnEntry)
{
    GTEST_COUT << "Validating that for each of the 23 examples every structure and enum of the lowered module has an entry, in order." << std::endl;

    const MAREA_IrExampleSet EXAMPLES;

    ASSERT_EQ(EXAMPLES.files.size(), 23U) << "The example set must hold 23 files.";

    std::size_t declarations{ 0 };

    std::size_t measured{ 0 };

    std::size_t undecided{ 0 };

    for (const MAREA_IrExample &file : EXAMPLES.files) {
        MAREA_IrFixture fixture(EXAMPLES.MAREA_resolver());

        ASSERT_EQ(fixture.MAREA_lower(file.source, file.relativePath), MAREA_IR_EXCEPT_SUCCESS) << file.relativePath << " must lower.";

        MAREA_IntegerSize entry{ 0 };

        for (MAREA_IntegerSize index{ 0 }; index < MAREA_IrNodeGetChildCount(fixture.module); ++index) {
            const MAREA_IrNode *child{ MAREA_irChild(fixture.module, index) };

            const MAREA_IrKind KIND{ MAREA_IrNodeGetKind(child) };

            if ((KIND != MAREA_IR_KIND_STRUCTURE_DECLARATION) && (KIND != MAREA_IR_KIND_ENUM_DECLARATION)) {
                continue;
            }

            const MAREA_IrLayout *layout{ MAREA_layoutAt(fixture.module, entry) };

            ASSERT_NE(layout, nullptr) << file.relativePath << ": " << MAREA_irText(child) << " has no entry.";

            EXPECT_EQ(MAREA_nameOf(layout), MAREA_irText(child)) << file.relativePath << ": the entries must follow the declaration order.";

            EXPECT_EQ(MAREA_IrLayoutGetKind(layout), (KIND == MAREA_IR_KIND_ENUM_DECLARATION) ? MAREA_IR_LAYOUT_KIND_ENUM : MAREA_IR_LAYOUT_KIND_STRUCTURE) << file.relativePath << ": " << MAREA_irText(child);

            if (MAREA_IrLayoutGetVerdict(layout) == MAREA_SEMA_MEASURE_VERDICT_MEASURED) {
                ++measured;

                const std::uint64_t ALIGNMENT{ MAREA_numberOf(MAREA_IrLayoutGetAlignment, layout) };

                EXPECT_GT(ALIGNMENT, 0U) << file.relativePath << ": " << MAREA_irText(child);

                EXPECT_EQ(MAREA_numberOf(MAREA_IrLayoutGetSize, layout) % ALIGNMENT, 0U) << file.relativePath << ": the size must be a multiple of the alignment.";
            } else {
                ++undecided;

                EXPECT_NE(MAREA_IrLayoutGetReason(layout), MAREA_SEMA_MEASURE_REASON_NONE) << file.relativePath << ": an undecided entry must say why.";
            }

            ++entry;

            ++declarations;
        }

        EXPECT_EQ(MAREA_IrModuleGetLayoutCount(fixture.module), entry) << file.relativePath << ": the table must hold no entry more than the module declares.";
    }

    EXPECT_GT(declarations, 0U) << "The examples must hold structures or enums.";

    EXPECT_GT(measured, 0U) << "At least one example declaration must be measured.";

    EXPECT_GT(undecided, 0U) << "At least one example declaration must be undecided.";
}
