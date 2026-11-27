// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include <cstdio>

#include "MareaDiagnosticPrinter.h"

namespace
{
    class MareaDiagnosticPrinterTest : public ::testing::Test
    {
      protected:
        void SetUp() override
        {
            ASSERT_EQ(MAREA_ArenaCreate(0, &arena), MAREA_ARENA_EXCEPT_SUCCESS);
            ASSERT_EQ(MAREA_DiagnosticSinkCreate(arena, &sink), MAREA_DIAGNOSTIC_EXCEPT_SUCCESS);
        }

        void TearDown() override { MAREA_ArenaDestroy(arena); }

        MAREA_Arena *arena{ nullptr };
        MAREA_DiagnosticSink *sink{ nullptr };
    };
} // namespace

TEST_F(MareaDiagnosticPrinterTest, NullSinkIsSuccess)
{
    ASSERT_TRUE(MareaDiagnosticPrinterPrint(nullptr, stderr));
}

TEST_F(MareaDiagnosticPrinterTest, EmptySinkIsSuccess)
{
    ASSERT_TRUE(MareaDiagnosticPrinterPrint(sink, stderr));
}

TEST_F(MareaDiagnosticPrinterTest, WarningOnlySinkIsStillSuccess)
{
    MAREA_SourcePosition position{};
    position.file = "test.marea";
    position.line = 1;
    position.column = 1;

    ASSERT_EQ(
        MAREA_DiagnosticSinkAdd(sink, 1000, MAREA_DIAGNOSTIC_SEVERITY_WARNING, position, "a warning"),
        MAREA_DIAGNOSTIC_EXCEPT_SUCCESS);

    ASSERT_TRUE(MareaDiagnosticPrinterPrint(sink, stderr));
}

TEST_F(MareaDiagnosticPrinterTest, ErrorSinkIsFailure)
{
    MAREA_SourcePosition position{};
    position.file = "test.marea";
    position.line = 2;
    position.column = 3;

    ASSERT_EQ(
        MAREA_DiagnosticSinkAdd(sink, 2000, MAREA_DIAGNOSTIC_SEVERITY_ERROR, position, "an error"),
        MAREA_DIAGNOSTIC_EXCEPT_SUCCESS);

    ASSERT_FALSE(MareaDiagnosticPrinterPrint(sink, stderr));
}
