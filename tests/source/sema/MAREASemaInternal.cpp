// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

// INFO: internal header on purpose (issue #28): a size overflow is not reachable through the public API.
#include "marea/internal/sema/MAREASemaInternal.h"

#include "tests/MAREASemaHelpers.hpp"

#include <cstring>

TEST(MAREASemaInternal, ReportNamedWritesTheQuotedName)
{
    GTEST_COUT << "Validating MAREA_SemaReportNamed builds the message prefix 'name' as before." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_NE(fixture.context, nullptr) << "The fixture must create a context.";

    const MAREA_SourcePosition position{};

    ASSERT_EQ(MAREA_SemaReportNamed(fixture.context, MAREA_SEMA_DIAGNOSTIC_CODE_UNKNOWN_TYPE, position, "unknown type:", "Foo", 3), MAREA_SEMA_EXCEPT_SUCCESS) << "A normal name must be reported.";

    const MAREA_Diagnostic *diagnostic{ nullptr };

    ASSERT_EQ(MAREA_DiagnosticSinkGetAt(fixture.parser.lexer.sink, 0, &diagnostic), MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) << "One diagnostic must be in the sink.";

    EXPECT_STREQ(MAREA_DiagnosticGetMessage(diagnostic), "unknown type: 'Foo'") << "The message must quote the name.";
}

TEST(MAREASemaInternal, ReportNamedRejectsASizeThatOverflows)
{
    GTEST_COUT << "Validating MAREA_SemaReportNamed returns out of memory when prefix + name + 4 wraps around." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_NE(fixture.context, nullptr) << "The fixture must create a context.";

    const MAREA_SourcePosition position{};

    const MAREA_Character8 name[] = "x";

    EXPECT_EQ(MAREA_SemaReportNamed(fixture.context, MAREA_SEMA_DIAGNOSTIC_CODE_UNKNOWN_TYPE, position, "unknown type:", name, (MAREA_INTEGER_SIZE_MAXIMUM - 1)), MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY) << "A wrapped size must be rejected before any copy.";

    EXPECT_EQ(MAREA_DiagnosticSinkGetCount(fixture.parser.lexer.sink), 0U) << "A rejected report must add no diagnostic.";
}

TEST(MAREASemaInternal, TableAppendGrowsAndKeepsTheEntries)
{
    GTEST_COUT << "Validating MAREA_SemaTableAppend still grows past its first capacity and keeps every entry." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_NE(fixture.context, nullptr) << "The fixture must create a context.";

    MAREA_SemaNameTable table{};

    for (MAREA_IntegerSize index = 0; index < 40; ++index) {
        const MAREA_SemaName entry{ "n", nullptr, index, MAREA_SEMA_NAME_KIND_DECLARATION };

        ASSERT_EQ(MAREA_SemaTableAppend(fixture.context, &table, entry), MAREA_SEMA_EXCEPT_SUCCESS) << "A normal append must succeed.";
    }

    ASSERT_EQ(table.count, 40U) << "Every entry must be counted.";

    EXPECT_EQ(table.entries[0].length, 0U) << "The first entry must survive the growth.";

    EXPECT_EQ(table.entries[39].length, 39U) << "The last entry must be stored.";
}

TEST(MAREASemaInternal, TableAppendRejectsACapacityThatOverflows)
{
    GTEST_COUT << "Validating MAREA_SemaTableAppend returns out of memory when the grown byte size wraps around." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_NE(fixture.context, nullptr) << "The fixture must create a context.";

    MAREA_IntegerSize doubled{ (MAREA_INTEGER_SIZE_MAXIMUM / sizeof(MAREA_SemaName)) + 2 };

    if ((doubled % 2) != 0) {
        doubled += 1;
    }

    const MAREA_IntegerSize wrappedBytes{ doubled * sizeof(MAREA_SemaName) };

    ASSERT_LT(wrappedBytes, 4096U) << "The chosen capacity must wrap the byte size to a small block.";

    MAREA_SemaNameTable table{};

    table.capacity = (doubled / 2);
    table.count = table.capacity;

    const MAREA_SemaName entry{ "n", nullptr, 1, MAREA_SEMA_NAME_KIND_DECLARATION };

    EXPECT_EQ(MAREA_SemaTableAppend(fixture.context, &table, entry), MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY) << "A wrapped byte size must be rejected before any copy.";

    EXPECT_EQ(table.count, table.capacity) << "A rejected append must leave the table unchanged.";
}

TEST(MAREASemaInternal, TableAppendRejectsADoublingThatOverflows)
{
    GTEST_COUT << "Validating MAREA_SemaTableAppend returns out of memory when doubling the capacity wraps around." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_NE(fixture.context, nullptr) << "The fixture must create a context.";

    MAREA_SemaNameTable table{};

    table.capacity = ((MAREA_INTEGER_SIZE_MAXIMUM / 2) + 2);
    table.count = table.capacity;

    const MAREA_SemaName entry{ "n", nullptr, 1, MAREA_SEMA_NAME_KIND_DECLARATION };

    EXPECT_EQ(MAREA_SemaTableAppend(fixture.context, &table, entry), MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY) << "A wrapped capacity must be rejected.";
}

TEST(MAREASemaInternal, CheckJumpRejectsANullArgument)
{
    GTEST_COUT << "Validating the jump check rejects a null context and a null node." << std::endl;

    EXPECT_EQ(MAREA_SemaCheckJump(nullptr, nullptr), MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER) << "Two null arguments must be rejected.";

    MAREA_SemaFixture fixture;

    EXPECT_EQ(MAREA_SemaCheckJump(fixture.context, nullptr), MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER) << "A null node must be rejected.";
}

TEST(MAREASemaInternal, ExpressionChecksRejectANullArgument)
{
    GTEST_COUT << "Validating the expression checks and the binder declaration reject a null context and a null node." << std::endl;

    EXPECT_EQ(MAREA_SemaCheckExpressions(nullptr, nullptr), MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER) << "Two null arguments must be rejected by the expression check.";

    EXPECT_EQ(MAREA_SemaDeclareBinders(nullptr, nullptr), MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER) << "Two null arguments must be rejected by the binder declaration.";

    EXPECT_EQ(MAREA_SemaCheckExpressionFit(nullptr, nullptr, nullptr), MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER) << "A null context must be rejected by the fit check.";

    MAREA_SemaFixture fixture;

    EXPECT_EQ(MAREA_SemaCheckExpressions(fixture.context, nullptr), MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER) << "A null node must be rejected by the expression check.";

    EXPECT_EQ(MAREA_SemaDeclareBinders(fixture.context, nullptr), MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER) << "A null node must be rejected by the binder declaration.";

    EXPECT_EQ(MAREA_SemaCheckExpressionFit(fixture.context, nullptr, nullptr), MAREA_SEMA_EXCEPT_SUCCESS) << "A null type and a null expression must be accepted.";
}

namespace
{
    // INFO: the nth structure declaration of the unit, in the order written.
    const MAREA_AstNode *MAREA_structureAt(const MAREA_AstNode *root, MAREA_IntegerSize position)
    {
        MAREA_IntegerSize seen{ 0 };

        for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(root); ++index) {
            const MAREA_AstNode *child{ nullptr };

            if ((MAREA_AstNodeGetChildAt(root, index, &child) == MAREA_AST_EXCEPT_SUCCESS) && (MAREA_AstNodeGetKind(child) == MAREA_AST_KIND_STRUCTURE_DECLARATION)) {
                if (seen == position) {
                    return child;
                }

                seen += 1;
            }
        }

        return nullptr;
    }

    MAREA_SemaLayoutVerdict MAREA_verdictOf(const std::string &source, MAREA_IntegerSize from, MAREA_IntegerSize to)
    {
        MAREA_SemaFixture fixture;

        EXPECT_EQ(fixture.MAREA_sema("namespace main; " + source), MAREA_SEMA_EXCEPT_SUCCESS) << source;

        const MAREA_AstNode *fromNode{ MAREA_structureAt(fixture.parser.root, from) };

        const MAREA_AstNode *toNode{ MAREA_structureAt(fixture.parser.root, to) };

        EXPECT_NE(fromNode, nullptr) << source;

        EXPECT_NE(toNode, nullptr) << source;

        return MAREA_SemaCompareStructureLayout(fixture.context, fromNode, toNode);
    }
} // namespace

TEST(MAREASemaInternal, CompareStructureLayoutGivesAVerdict)
{
    GTEST_COUT << "Validating MAREA_SemaCompareStructureLayout separates compatible, incompatible and undecided pairs." << std::endl;

    const std::string C{ "#[layout = c] " };

    EXPECT_EQ(MAREA_verdictOf(C + "structure A x: sinteger32; end " + C + "structure B y: sinteger32; end", 0, 1), MAREA_SEMA_LAYOUT_VERDICT_COMPATIBLE) << "The same fields in the C layout, under other names, are compatible.";

    EXPECT_EQ(MAREA_verdictOf(C + "structure A x: sinteger32; end " + C + "structure B y: uinteger32; end", 0, 1), MAREA_SEMA_LAYOUT_VERDICT_INCOMPATIBLE) << "A field of another primitive name is incompatible.";

    EXPECT_EQ(MAREA_verdictOf(C + "structure A x: sinteger32; end " + C + "structure B y: sinteger32; z: sinteger8; end", 0, 1), MAREA_SEMA_LAYOUT_VERDICT_INCOMPATIBLE) << "Another field count is incompatible.";

    EXPECT_EQ(MAREA_verdictOf(C + "structure A x: uinteger8; y: uinteger32; end #[layout = c, pack = 1] structure B x: uinteger8; y: uinteger32; end", 0, 1), MAREA_SEMA_LAYOUT_VERDICT_INCOMPATIBLE) << "Pack 1 changes the alignment and the size.";

    EXPECT_EQ(MAREA_verdictOf(C + "structure A x: uinteger8; end #[layout = c, align = 8] structure B x: uinteger8; end", 0, 1), MAREA_SEMA_LAYOUT_VERDICT_INCOMPATIBLE) << "Align 8 changes the alignment and the size.";

    EXPECT_EQ(MAREA_verdictOf(C + "structure A x: uinteger8; y: uinteger16; z: uinteger8; end " + C + "structure B x: uinteger8; y: uinteger16; z: uinteger8; end", 0, 1), MAREA_SEMA_LAYOUT_VERDICT_COMPATIBLE) << "Padding is the same for the same fields.";

    EXPECT_EQ(MAREA_verdictOf(C + "structure A x: uinteger8; end structure B x: uinteger8; end", 0, 1), MAREA_SEMA_LAYOUT_VERDICT_UNDECIDED) << "A c layout against an auto layout has no common size.";

    EXPECT_EQ(MAREA_verdictOf("#![layout = c] structure A x: sinteger32; end structure B y: sinteger32; end", 0, 1), MAREA_SEMA_LAYOUT_VERDICT_COMPATIBLE) << "A file attribute gives the layout when the structure has none.";

    EXPECT_EQ(MAREA_verdictOf(C + "structure A x: string; end " + C + "structure B x: sinteger32; end", 0, 1), MAREA_SEMA_LAYOUT_VERDICT_UNDECIDED) << "A string field has no width in sema.";

    EXPECT_EQ(MAREA_verdictOf(C + "structure A x: sinteger32; end #[layout = c, align = 4294967297] structure B x: sinteger32; end", 0, 1), MAREA_SEMA_LAYOUT_VERDICT_UNDECIDED) << "An alignment that is too large to compute with is left to the backend.";

    EXPECT_EQ(MAREA_verdictOf(C + "structure A x: sinteger32; end", 0, 0), MAREA_SEMA_LAYOUT_VERDICT_COMPATIBLE) << "The same declaration is compatible with itself.";
}

TEST(MAREASemaInternal, CompareStructureLayoutMeasuresTheShapesItCan)
{
    GTEST_COUT << "Validating MAREA_SemaCompareStructureLayout for the auto layout, a field align, a nested structure and a generic structure." << std::endl;

    const std::string C{ "#[layout = c] " };

    EXPECT_EQ(MAREA_verdictOf("structure A x: sinteger32; y: uinteger8; end structure B p: sinteger32; q: uinteger8; end", 0, 1), MAREA_SEMA_LAYOUT_VERDICT_COMPATIBLE) << "Two auto layouts with the same fields are the same.";

    EXPECT_EQ(MAREA_verdictOf("structure A x: sinteger32; end structure B x: float32; end", 0, 1), MAREA_SEMA_LAYOUT_VERDICT_INCOMPATIBLE) << "Two auto layouts with other fields differ.";

    EXPECT_EQ(MAREA_verdictOf("structure A x: string; end structure B x: string; end", 0, 1), MAREA_SEMA_LAYOUT_VERDICT_UNDECIDED) << "An auto layout with a string field cannot be measured.";

    EXPECT_EQ(MAREA_verdictOf(C + "structure A #[align = 16] x: sinteger32; end " + C + "structure B #[align = 16] y: sinteger32; end", 0, 1), MAREA_SEMA_LAYOUT_VERDICT_COMPATIBLE) << "The same field align is compatible.";

    EXPECT_EQ(MAREA_verdictOf(C + "structure A #[align = 16] x: sinteger32; end " + C + "structure B x: sinteger32; end", 0, 1), MAREA_SEMA_LAYOUT_VERDICT_INCOMPATIBLE) << "A field align against none differs.";

    EXPECT_EQ(MAREA_verdictOf(C + "structure A #[align = 16] x: sinteger32; end " + C + "structure B #[align = 8] x: sinteger32; end", 0, 1), MAREA_SEMA_LAYOUT_VERDICT_INCOMPATIBLE) << "Two field aligns differ.";

    EXPECT_EQ(MAREA_verdictOf(C + "structure A #[align = 0] x: sinteger32; end " + C + "structure B x: sinteger32; end", 0, 1), MAREA_SEMA_LAYOUT_VERDICT_UNDECIDED) << "A field align that sema cannot read is not decided.";

    const std::string NESTED{ C + "structure N x: sinteger32; end " + C + "structure M y: sinteger32; end " + C + "structure W z: sinteger64; end " };

    EXPECT_EQ(MAREA_verdictOf(NESTED + C + "structure A i: N; end " + C + "structure B j: M; end", 3, 4), MAREA_SEMA_LAYOUT_VERDICT_COMPATIBLE) << "Two nested structures of the same layout are compatible.";

    EXPECT_EQ(MAREA_verdictOf(NESTED + C + "structure A i: N; end " + C + "structure B j: W; end", 3, 4), MAREA_SEMA_LAYOUT_VERDICT_INCOMPATIBLE) << "Two nested structures of another layout differ.";

    EXPECT_EQ(MAREA_verdictOf(NESTED + C + "structure A i: N; end " + C + "structure B j: sinteger32; end", 3, 4), MAREA_SEMA_LAYOUT_VERDICT_INCOMPATIBLE) << "A nested structure is not a flat field.";

    EXPECT_EQ(MAREA_verdictOf("structure N x: sinteger32; end " + C + "structure A i: N; end " + C + "structure B i: N; end", 1, 2), MAREA_SEMA_LAYOUT_VERDICT_UNDECIDED) << "An auto structure inside a c structure has no size.";

    EXPECT_EQ(MAREA_verdictOf(C + "structure A<T> v: T; end " + C + "structure B<T> v: T; end", 0, 1), MAREA_SEMA_LAYOUT_VERDICT_UNDECIDED) << "A generic structure cannot be measured.";

    EXPECT_EQ(MAREA_verdictOf(C + "structure A x: A; end " + C + "structure B x: A; end", 0, 1), MAREA_SEMA_LAYOUT_VERDICT_UNDECIDED) << "A structure that holds itself is cut off by the depth limit.";
}

TEST(MAREASemaInternal, CompareStructureLayoutRejectsANullArgument)
{
    GTEST_COUT << "Validating MAREA_SemaCompareStructureLayout gives UNDECIDED for a null context or a null structure." << std::endl;

    EXPECT_EQ(MAREA_SemaCompareStructureLayout(nullptr, nullptr, nullptr), MAREA_SEMA_LAYOUT_VERDICT_UNDECIDED) << "Three null arguments must give UNDECIDED.";

    MAREA_SemaFixture fixture;

    EXPECT_EQ(MAREA_SemaCompareStructureLayout(fixture.context, nullptr, nullptr), MAREA_SEMA_LAYOUT_VERDICT_UNDECIDED) << "Two null structures must give UNDECIDED.";
}

TEST(MAREASemaInternal, AttributeFindAndReadInteger)
{
    GTEST_COUT << "Validating MAREA_SemaAttributeFind lets the symbol win over the file and MAREA_SemaAttributeReadInteger reads decimal and hexadecimal." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_EQ(fixture.MAREA_sema("namespace main; #![pack = 2] #[pack = 0x10] structure A x: sinteger32; end structure B y: sinteger32; end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The program must pass sema.";

    const MAREA_AstNode *symbol{ MAREA_structureAt(fixture.parser.root, 0) };

    const MAREA_AstNode *other{ MAREA_structureAt(fixture.parser.root, 1) };

    MAREA_Uinteger64 value{ 0 };

    ASSERT_TRUE(MAREA_SemaAttributeReadInteger(MAREA_SemaAttributeFind(fixture.parser.root, symbol, "pack"), &value)) << "The pack of the symbol must be read.";

    EXPECT_EQ(value, 16U) << "The hexadecimal value of the symbol must win over the file attribute.";

    ASSERT_TRUE(MAREA_SemaAttributeReadInteger(MAREA_SemaAttributeFind(fixture.parser.root, other, "pack"), &value)) << "The file attribute must be read for a structure with no pack.";

    EXPECT_EQ(value, 2U) << "The decimal value of the file attribute must be read.";

    EXPECT_EQ(MAREA_SemaAttributeFind(fixture.parser.root, other, "align"), nullptr) << "A missing key must give null.";

    EXPECT_FALSE(MAREA_SemaAttributeReadInteger(nullptr, &value)) << "A null item must not be read.";

    EXPECT_FALSE(MAREA_SemaAttributeReadInteger(MAREA_SemaAttributeFind(fixture.parser.root, symbol, "pack"), nullptr)) << "A null output must be refused.";
}
