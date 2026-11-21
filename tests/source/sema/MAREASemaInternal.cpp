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
