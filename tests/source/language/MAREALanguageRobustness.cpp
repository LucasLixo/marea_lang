// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <thread>

namespace
{
    // The whole sweep must stay inside this budget. A watchdog aborts the run when a case hangs.
    constexpr std::chrono::seconds WATCHDOG_LIMIT{ 60 };

    struct MAREA_Example
    {
        std::string path;
        std::string text;
    };

    struct MAREA_TokenSpan
    {
        std::size_t offset;
        std::size_t length;
    };

    // Names the case that is running, so a hang or a failure points at its input.
    class MAREA_Watchdog
    {
      public:
        MAREA_Watchdog()
            : worker([this] { MAREA_watch(); })
        {
        }

        ~MAREA_Watchdog()
        {
            {
                std::lock_guard<std::mutex> guard(lock);
                finished = true;
            }

            wake.notify_all();
            worker.join();
        }

        MAREA_Watchdog(const MAREA_Watchdog &) = delete;
        MAREA_Watchdog &operator=(const MAREA_Watchdog &) = delete;

        void MAREA_setCase(const std::string &name)
        {
            std::lock_guard<std::mutex> guard(lock);
            current = name;
        }

      private:
        void MAREA_watch()
        {
            std::unique_lock<std::mutex> guard(lock);

            if (!wake.wait_for(guard, WATCHDOG_LIMIT, [this] { return finished; })) {
                std::cerr << "[ HANG     ] the sweep ran past its limit in case: " << current << std::endl;
                std::abort();
            }
        }

        std::mutex lock;
        std::condition_variable wake;
        std::string current;
        bool finished{ false };
        std::thread worker;
    };

    std::string MAREA_readFile(const std::filesystem::path &path)
    {
        std::ifstream stream(path, std::ios::binary);

        std::ostringstream buffer;

        buffer << stream.rdbuf();

        return buffer.str();
    }

    std::vector<MAREA_Example> MAREA_collectExamples()
    {
        std::vector<MAREA_Example> result;

        const std::filesystem::path ROOT{ MAREA_TESTS_EXAMPLES_DIRECTORY };

        for (const std::filesystem::directory_entry &entry : std::filesystem::recursive_directory_iterator(ROOT)) {
            if (entry.is_regular_file() && (entry.path().extension() == ".marea")) {
                result.push_back({ std::filesystem::relative(entry.path(), ROOT).generic_string(), MAREA_readFile(entry.path()) });
            }
        }

        std::sort(result.begin(), result.end(), [](const MAREA_Example &left, const MAREA_Example &right) { return left.path < right.path; });

        return result;
    }

    // A diagnostic line is valid when it is 1-based and no later than the last line of the text.
    bool MAREA_lineInsideFile(MAREA_Uinteger32 line, const std::string &text)
    {
        const std::size_t LINE_COUNT{ static_cast<std::size_t>(std::count(text.begin(), text.end(), '\n')) + 1 };

        return (line >= 1) && (line <= LINE_COUNT);
    }

    // Runs lex, parse and sema on one text and returns "" or the reason it broke the contract.
    std::string MAREA_checkCase(const std::string &text, const std::string &path)
    {
        MAREA_SemaFixture fixture;

        MAREA_IGNORE(fixture.MAREA_sema(text, path));

        const MAREA_IntegerSize COUNT{ MAREA_DiagnosticSinkGetCount(fixture.parser.lexer.sink) };

        for (MAREA_IntegerSize index{ 0 }; index < COUNT; ++index) {
            const MAREA_Diagnostic *diagnostic{ nullptr };

            MAREA_IGNORE(MAREA_DiagnosticSinkGetAt(fixture.parser.lexer.sink, index, &diagnostic));

            const MAREA_SourcePosition POSITION{ MAREA_DiagnosticGetPosition(diagnostic) };

            if (!MAREA_lineInsideFile(POSITION.line, text)) {
                return "diagnostic " + std::to_string(MAREA_DiagnosticGetCode(diagnostic)) + " has line " + std::to_string(POSITION.line) + " outside the file";
            }
        }

        return "";
    }

    std::vector<MAREA_TokenSpan> MAREA_tokenSpans(const std::string &text)
    {
        MAREA_LexFixture lexer;

        std::vector<MAREA_TokenSpan> result;

        if (lexer.MAREA_lex(text) != MAREA_LEX_EXCEPT_SUCCESS) {
            return result;
        }

        const MAREA_IntegerSize COUNT{ MAREA_TokenListGetCount(lexer.list) };

        for (MAREA_IntegerSize index{ 0 }; index < COUNT; ++index) {
            const MAREA_Token *token{ nullptr };

            MAREA_IGNORE(MAREA_TokenListGetAt(lexer.list, index, &token));

            const MAREA_SourcePosition POSITION{ MAREA_TokenGetPosition(token) };

            if (MAREA_TokenGetLength(token) > 0) {
                result.push_back({ POSITION.offset, static_cast<std::size_t>(MAREA_TokenGetLength(token)) });
            }
        }

        return result;
    }

    // Fixed-seed generator: the sweep gives the same inputs on every run.
    class MAREA_Random
    {
      public:
        explicit MAREA_Random(std::uint32_t seed)
            : state(seed)
        {
        }

        std::uint32_t MAREA_next()
        {
            state ^= state << 13;
            state ^= state >> 17;
            state ^= state << 5;

            return state;
        }

      private:
        std::uint32_t state;
    };

    std::string MAREA_repeat(const std::string &unit, std::size_t count)
    {
        std::string result;

        result.reserve(unit.size() * count);

        for (std::size_t index{ 0 }; index < count; ++index) {
            result += unit;
        }

        return result;
    }

    // The depth that every shape must survive. A deeper depth that breaks a stage is a recorded defect.
    constexpr std::size_t DEPTH{ 1000 };

    std::string MAREA_shape(const std::string &shape, std::size_t depth)
    {
        if (shape == "parentheses") {
            return "namespace main;\nconstant x: sinteger32 = " + MAREA_repeat("(", depth) + "1" + MAREA_repeat(")", depth) + ";\n";
        }

        if (shape == "unary") {
            return "namespace main;\nconstant x: sinteger32 = " + MAREA_repeat("-", depth) + "1;\n";
        }

        if (shape == "binary") {
            return "namespace main;\nconstant x: sinteger32 = 1" + MAREA_repeat(" + 1", depth) + ";\n";
        }

        if (shape == "blocks") {
            return "namespace main;\nfunction f()\n" + MAREA_repeat("do\n", depth) + MAREA_repeat("end\n", depth) + "end\n";
        }

        if (shape == "if") {
            return "namespace main;\nfunction f()\n" + MAREA_repeat("if true then\n", depth) + MAREA_repeat("end\n", depth) + "end\n";
        }

        if (shape == "generic-arguments") {
            return "namespace main;\nfunction f(values: " + MAREA_repeat("c.array<", depth) + "sinteger32" + MAREA_repeat(">", depth) + ")\nend\n";
        }

        if (shape == "unclosed-parentheses") {
            return "namespace main;\nconstant x: sinteger32 = " + MAREA_repeat("(", depth);
        }

        return "namespace main;\nfunction f(values: " + MAREA_repeat("c.array<", depth);
    }

    const std::vector<std::string> SHAPES{ "parentheses", "unary", "binary", "blocks", "if", "generic-arguments", "unclosed-parentheses", "unclosed-generic-arguments" };

    // Collects the failure of each case, so one run lists every defect.
    class MAREA_Sweep
    {
      public:
        explicit MAREA_Sweep(MAREA_Watchdog &dog)
            : watchdog(dog)
        {
        }

        void MAREA_run(const std::string &name, const std::string &text, const std::string &path = "main.marea")
        {
            watchdog.MAREA_setCase(name);

            const std::string REASON{ MAREA_checkCase(text, path) };

            ++cases;

            if (!REASON.empty()) {
                failures.push_back(name + ": " + REASON);
            }
        }

        std::size_t cases{ 0 };
        std::vector<std::string> failures;

      private:
        MAREA_Watchdog &watchdog;
    };

    void MAREA_expectClean(const MAREA_Sweep &sweep)
    {
        for (const std::string &failure : sweep.failures) {
            ADD_FAILURE() << failure;
        }
    }

    void MAREA_runShape(const std::string &shape, std::size_t depth)
    {
        MAREA_Watchdog watchdog;

        MAREA_Sweep sweep(watchdog);

        sweep.MAREA_run(shape + "-" + std::to_string(depth), MAREA_shape(shape, depth));

        MAREA_expectClean(sweep);
    }
} // namespace

TEST(MAREALanguageRobustness, TheLineCheckRejectsALineOutsideTheFile)
{
    GTEST_COUT << "Validating the line check can fail: line 0 and a line past the end are both invalid." << std::endl;

    EXPECT_FALSE(MAREA_lineInsideFile(0, "a\nb\n"));
    EXPECT_FALSE(MAREA_lineInsideFile(4, "a\nb\n"));
    EXPECT_TRUE(MAREA_lineInsideFile(1, "a\nb\n"));
    EXPECT_TRUE(MAREA_lineInsideFile(3, "a\nb\n"));
    EXPECT_TRUE(MAREA_lineInsideFile(1, ""));
}

TEST(MAREALanguageRobustness, TheSweepFindsTheExamplesAndEveryExampleHasTokens)
{
    GTEST_COUT << "Validating the sweep reads all 23 examples and each one has tokens to cut." << std::endl;

    const std::vector<MAREA_Example> EXAMPLES{ MAREA_collectExamples() };

    ASSERT_EQ(EXAMPLES.size(), 23U);

    for (const MAREA_Example &example : EXAMPLES) {
        EXPECT_FALSE(MAREA_tokenSpans(example.text).empty()) << example.path;
    }
}

TEST(MAREALanguageRobustness, DegenerateFilesNeverBreakTheStages)
{
    GTEST_COUT << "Validating an empty file, only comments, a NUL byte and a very long line each return with valid lines." << std::endl;

    MAREA_Watchdog watchdog;

    MAREA_Sweep sweep(watchdog);

    sweep.MAREA_run("empty", "");
    sweep.MAREA_run("only-newlines", "\n\n\n");
    sweep.MAREA_run("only-comments", "-- one\n-- two\n-- three");
    sweep.MAREA_run("single-nul", std::string(1, '\0'));
    sweep.MAREA_run("nul-inside-source", std::string("namespace main;\nfunction f()\n") + std::string(1, '\0') + "\nend\n");
    sweep.MAREA_run("nul-inside-comment", std::string("namespace main;\n-- a") + std::string(1, '\0') + "b\n");
    sweep.MAREA_run("long-line-identifier", "namespace main;\nconstant " + MAREA_repeat("a", 1000000) + ": sinteger32 = 1;\n");
    sweep.MAREA_run("long-line-comment", "namespace main;\n-- " + MAREA_repeat("x", 1000000));
    sweep.MAREA_run("unterminated-string", "namespace main;\nconstant x: string = \"abc");
    sweep.MAREA_run("only-high-bytes", std::string("\xFF\xFE\xC3\x28\x80\x80", 6));

    MAREA_expectClean(sweep);
}

TEST(MAREALanguageRobustness, DeepNestingNeverBreaksTheStages)
{
    GTEST_COUT << "Validating nested parentheses, operators, blocks and generic arguments, closed and unclosed, return at depth 100 and 1000." << std::endl;

    MAREA_Watchdog watchdog;

    MAREA_Sweep sweep(watchdog);

    for (const std::string &shape : SHAPES) {
        for (const std::size_t depth : { std::size_t{ 100 }, DEPTH }) {
            sweep.MAREA_run(shape + "-" + std::to_string(depth), MAREA_shape(shape, depth));
        }
    }

    MAREA_expectClean(sweep);
}

TEST(MAREALanguageRobustness, TenThousandParenthesesAndUnaryOperatorsReturn)
{
    GTEST_COUT << "Validating depth 10000 returns for parentheses, unary operators and unclosed parentheses." << std::endl;

    for (const char *shape : { "parentheses", "unary", "unclosed-parentheses" }) {
        MAREA_runShape(shape, 10000);
    }
}

// INFO: each test below breaks a stage today, so it is disabled; its issue is named on the line above it.

// BUG: issue #192
TEST(MAREALanguageRobustness, DISABLED_KnownDefectBinaryChainOf100000OperatorsOverflowsTheStack)
{
    GTEST_COUT << "Defect: a chain of 100000 binary operators crashes sema with a stack overflow." << std::endl;

    MAREA_runShape("binary", 100000);
}

// BUG: issue #193
TEST(MAREALanguageRobustness, DISABLED_KnownDefectGenericArgumentsNested10000DoNotReturnInTime)
{
    GTEST_COUT << "Defect: 10000 nested generic arguments run past 60 seconds." << std::endl;

    MAREA_runShape("generic-arguments", 10000);
}

// BUG: issue #193
TEST(MAREALanguageRobustness, DISABLED_KnownDefectUnclosedGenericArguments10000AreSlow)
{
    GTEST_COUT << "Defect: 10000 unclosed generic arguments take seconds." << std::endl;

    MAREA_runShape("unclosed-generic-arguments", 10000);
}

// BUG: issue #194
TEST(MAREALanguageRobustness, DISABLED_KnownDefectNestedIfs10000AreSlow)
{
    GTEST_COUT << "Defect: 10000 nested ifs take about 39 seconds in the debug build." << std::endl;

    MAREA_runShape("if", 10000);
}

// BUG: issue #194
TEST(MAREALanguageRobustness, DISABLED_KnownDefectNestedBlocks10000AreSlow)
{
    GTEST_COUT << "Defect: 10000 nested blocks take about 12 seconds in the debug build." << std::endl;

    MAREA_runShape("blocks", 10000);
}

TEST(MAREALanguageRobustness, TruncatingAnExampleAtEveryTokenBoundaryNeverBreaksTheStages)
{
    GTEST_COUT << "Validating each example cut at every token boundary returns with valid lines." << std::endl;

    MAREA_Watchdog watchdog;

    MAREA_Sweep sweep(watchdog);

    for (const MAREA_Example &example : MAREA_collectExamples()) {
        const std::vector<MAREA_TokenSpan> SPANS{ MAREA_tokenSpans(example.text) };

        for (std::size_t index{ 0 }; index < SPANS.size(); ++index) {
            sweep.MAREA_run("truncate " + example.path + " at token " + std::to_string(index), example.text.substr(0, SPANS[index].offset), example.path);
        }
    }

    GTEST_COUT << "Truncation cases run: " << sweep.cases << std::endl;

    MAREA_expectClean(sweep);
}

TEST(MAREALanguageRobustness, MutatingOneTokenOfAnExampleNeverBreaksTheStages)
{
    GTEST_COUT << "Validating each example with one token deleted, duplicated or swapped returns with valid lines." << std::endl;

    MAREA_Watchdog watchdog;

    MAREA_Sweep sweep(watchdog);

    for (const MAREA_Example &example : MAREA_collectExamples()) {
        const std::vector<MAREA_TokenSpan> SPANS{ MAREA_tokenSpans(example.text) };

        for (std::size_t index{ 0 }; index < SPANS.size(); ++index) {
            const MAREA_TokenSpan &span{ SPANS[index] };

            const std::string TAG{ example.path + " at token " + std::to_string(index) };

            sweep.MAREA_run("delete " + TAG, example.text.substr(0, span.offset) + example.text.substr(span.offset + span.length), example.path);

            sweep.MAREA_run("duplicate " + TAG, example.text.substr(0, span.offset + span.length) + example.text.substr(span.offset, span.length) + example.text.substr(span.offset + span.length), example.path);

            if ((index + 1) < SPANS.size()) {
                const MAREA_TokenSpan &next{ SPANS[index + 1] };

                std::string swapped{ example.text };

                swapped.replace(span.offset, span.length, example.text.substr(next.offset, next.length));
                swapped.replace(span.offset + next.length + (next.offset - (span.offset + span.length)), next.length, example.text.substr(span.offset, span.length));

                sweep.MAREA_run("swap " + TAG, swapped, example.path);
            }
        }
    }

    GTEST_COUT << "Mutation cases run: " << sweep.cases << std::endl;

    MAREA_expectClean(sweep);
}

TEST(MAREALanguageRobustness, FlippingBytesOfAnExampleWithAFixedSeedNeverBreaksTheStages)
{
    GTEST_COUT << "Validating each example with random bytes replaced, from a fixed seed, returns with valid lines." << std::endl;

    constexpr std::size_t FLIPS_PER_EXAMPLE{ 300 };

    MAREA_Watchdog watchdog;

    MAREA_Sweep sweep(watchdog);

    MAREA_Random random(0x4D415245U);

    for (const MAREA_Example &example : MAREA_collectExamples()) {
        for (std::size_t flip{ 0 }; flip < FLIPS_PER_EXAMPLE; ++flip) {
            std::string text{ example.text };

            const std::size_t FLIP_COUNT{ 1 + (random.MAREA_next() % 4U) };

            for (std::size_t count{ 0 }; count < FLIP_COUNT; ++count) {
                text[random.MAREA_next() % text.size()] = static_cast<char>(random.MAREA_next() & 0xFFU);
            }

            sweep.MAREA_run("flip " + example.path + " number " + std::to_string(flip), text, example.path);
        }
    }

    GTEST_COUT << "Byte flip cases run: " << sweep.cases << std::endl;

    MAREA_expectClean(sweep);
}
