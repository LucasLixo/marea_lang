// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <string>
#include <vector>

#if defined(__GLIBC__) && defined(__linux__)
    #define MAREA_TESTS_CAN_FAIL_MALLOC 1
#else
    #define MAREA_TESTS_CAN_FAIL_MALLOC 0
#endif

#if MAREA_TESTS_CAN_FAIL_MALLOC

namespace
{

    std::atomic<bool> MAREA_G_FAIL_MALLOC{ false };

} // namespace

extern "C" void *__libc_malloc(std::size_t size);

extern "C" void *malloc(std::size_t size) noexcept
{
    if (MAREA_G_FAIL_MALLOC.load()) {
        return nullptr;
    }

    return __libc_malloc(size);
}

namespace
{

    struct MAREA_OutOfMemoryRow
    {
        const char *name;
        std::string source;
    };

    const std::vector<MAREA_OutOfMemoryRow> MAREA_G_ROWS{
        { "unknown byte", "@" },
        { "unterminated string", "\"abc" },
        { "invalid string escape", "\"\\q\"" },
        { "invalid string hex escape", "\"\\xZ\"" },
        { "bad byte in string", "\"\x80\"" },
        { "malformed hexadecimal number", "0x" },
        { "empty character", "''" },
        { "too many characters", "'ab'" },
        { "unterminated character", "'a" },
        { "invalid character escape", "'\\q'" },
        { "invalid character hex escape", "'\\xZ'" },
        { "bad byte in character", "'\x80'" },
    };

    struct MAREA_OutOfMemoryFixture
    {
        MAREA_Arena *lexerArena{ nullptr };
        MAREA_Arena *sinkArena{ nullptr };
        MAREA_DiagnosticSink *sink{ nullptr };
        MAREA_LexContext *context{ nullptr };

        MAREA_OutOfMemoryFixture()
        {
            MAREA_IGNORE(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(0), &lexerArena));
            MAREA_IGNORE(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(1), &sinkArena));
            MAREA_IGNORE(MAREA_DiagnosticSinkCreate(sinkArena, &sink));
            MAREA_IGNORE(MAREA_LexContextCreate(lexerArena, sink, &context));
        }

        ~MAREA_OutOfMemoryFixture()
        {
            MAREA_ArenaDestroy(sinkArena);
            MAREA_ArenaDestroy(lexerArena);
        }

        MAREA_OutOfMemoryFixture(const MAREA_OutOfMemoryFixture &) = delete;
        MAREA_OutOfMemoryFixture &operator=(const MAREA_OutOfMemoryFixture &) = delete;

        MAREA_LexExcept MAREA_tokenize(const std::string &source, bool failMalloc)
        {
            MAREA_TokenList *list{ nullptr };

            MAREA_G_FAIL_MALLOC.store(failMalloc);

            const MAREA_LexExcept RESULT{ MAREA_LexTokenize(context, source.c_str(), static_cast<MAREA_IntegerSize>(source.size()), &list) };

            MAREA_G_FAIL_MALLOC.store(false);

            return RESULT;
        }
    };

} // namespace

TEST(MAREALexerOutOfMemory, EveryDiagnosticSourceLexesWhenMemoryIsAvailable)
{
    GTEST_COUT << "Validating the sources of the out of memory test lex with a diagnostic when malloc works." << std::endl;

    for (const MAREA_OutOfMemoryRow &row : MAREA_G_ROWS) {
        MAREA_OutOfMemoryFixture fixture;

        EXPECT_EQ(fixture.MAREA_tokenize(row.source, false), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing '" << row.name << "' must succeed with memory.";

        EXPECT_GE(MAREA_DiagnosticSinkGetCount(fixture.sink), 1U) << "Lexing '" << row.name << "' must report a diagnostic.";
    }
}

TEST(MAREALexerOutOfMemory, SinkOutOfMemoryIsReported)
{
    GTEST_COUT << "Validating a diagnostic that the sink cannot store ends the lexer with the out of memory except." << std::endl;

    for (const MAREA_OutOfMemoryRow &row : MAREA_G_ROWS) {
        MAREA_OutOfMemoryFixture fixture;

        EXPECT_EQ(fixture.MAREA_tokenize(row.source, true), MAREA_LEX_EXCEPT_ERROR_OUT_OF_MEMORY) << "Lexing '" << row.name << "' must report out of memory when the sink cannot store the diagnostic.";

        EXPECT_EQ(MAREA_DiagnosticSinkGetCount(fixture.sink), 0U) << "No diagnostic can be stored when '" << row.name << "' fails.";
    }
}

TEST(MAREALexerOutOfMemory, SourceWithoutDiagnosticNeedsNoSinkMemory)
{
    GTEST_COUT << "Validating a source with no diagnostic lexes even when the sink cannot allocate." << std::endl;

    MAREA_OutOfMemoryFixture fixture;

    EXPECT_EQ(fixture.MAREA_tokenize("namespace main;", true), MAREA_LEX_EXCEPT_SUCCESS) << "Without a diagnostic the sink is never touched.";
}

#else

TEST(MAREALexerOutOfMemory, NeedsAFailingMalloc)
{
    GTEST_SKIP() << "A failing malloc is only available on glibc.";
}

#endif
