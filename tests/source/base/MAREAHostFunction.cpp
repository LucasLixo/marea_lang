// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.
// The internal header is used because only a runtime creates a host context, so no public function makes one.

#include "tests/dependencies.hpp"

#include "marea/base/MAREAHostFunction.h"
#include "marea/internal/base/MAREAHostContextInternal.h"

#include "tests/MAREAScratchDirectory.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#include <sys/wait.h>

namespace
{
    struct MAREA_HostPoint
    {
        std::int32_t x;
        std::int32_t y;
    };

    int marea_g_idleRuns{ 0 };

    MAREA_HostContext *marea_g_context{ nullptr };

    void MAREA_idle()
    {
        ++marea_g_idleRuns;
    }

    MAREA_HOST_FUNCTION(MAREA_idle, MAREA_HOST_RETURNS_NOTHING);

    MAREA_Float64 MAREA_half()
    {
        return 0.5;
    }

    MAREA_HOST_FUNCTION(MAREA_half, MAREA_HOST_RETURNS(float64));

    void MAREA_origin(MAREA_HostPoint *outPoint)
    {
        outPoint->x += 7;
        outPoint->y += 9;
    }

    MAREA_HOST_FUNCTION(MAREA_origin, MAREA_HOST_RETURNS_STRUCTURE(MAREA_HostPoint));

#define MAREA_TEST_IDENTITY(tag, type)    \
    type MAREA_identity_##tag(type value) \
    {                                     \
        return value;                     \
    }                                     \
    MAREA_HOST_FUNCTION(MAREA_identity_##tag, MAREA_HOST_RETURNS(tag), MAREA_HOST_VALUE(tag))

    MAREA_TEST_IDENTITY(boolean, MAREA_Boolean);
    MAREA_TEST_IDENTITY(sinteger8, MAREA_Sinteger8);
    MAREA_TEST_IDENTITY(sinteger16, MAREA_Sinteger16);
    MAREA_TEST_IDENTITY(sinteger32, MAREA_Sinteger32);
    MAREA_TEST_IDENTITY(sinteger64, MAREA_Sinteger64);
    MAREA_TEST_IDENTITY(uinteger8, MAREA_Uinteger8);
    MAREA_TEST_IDENTITY(uinteger16, MAREA_Uinteger16);
    MAREA_TEST_IDENTITY(uinteger32, MAREA_Uinteger32);
    MAREA_TEST_IDENTITY(uinteger64, MAREA_Uinteger64);
    MAREA_TEST_IDENTITY(float32, MAREA_Float32);
    MAREA_TEST_IDENTITY(float64, MAREA_Float64);

#undef MAREA_TEST_IDENTITY

    void MAREA_sum(const MAREA_HostPoint *left, const MAREA_HostPoint *right, MAREA_HostPoint *outSum)
    {
        outSum->x = left->x + right->x;
        outSum->y = left->y + right->y;
    }

    MAREA_HOST_FUNCTION(MAREA_sum, MAREA_HOST_RETURNS_STRUCTURE(MAREA_HostPoint), MAREA_HOST_BORROW(MAREA_HostPoint), MAREA_HOST_BORROW(MAREA_HostPoint));

    void MAREA_shift(MAREA_HostPoint *point, MAREA_Sinteger32 by)
    {
        point->x += by;
    }

    MAREA_HOST_FUNCTION(MAREA_shift, MAREA_HOST_RETURNS_NOTHING, MAREA_HOST_BORROW_MUTABLE(MAREA_HostPoint), MAREA_HOST_VALUE(sinteger32));

    MAREA_Sinteger64 MAREA_norm(const MAREA_HostPoint *point)
    {
        return (static_cast<MAREA_Sinteger64>(point->x) * point->x) + (static_cast<MAREA_Sinteger64>(point->y) * point->y);
    }

    MAREA_HOST_FUNCTION(MAREA_norm, MAREA_HOST_RETURNS(sinteger64), MAREA_HOST_BORROW(MAREA_HostPoint));

    MAREA_Uinteger64 MAREA_next(MAREA_HostContext *context, MAREA_Uinteger64 value)
    {
        marea_g_context = context;

        return value + 1;
    }

    MAREA_HOST_FUNCTION(MAREA_next, MAREA_HOST_RETURNS(uinteger64), MAREA_HOST_CONTEXT, MAREA_HOST_VALUE(uinteger64));

    void MAREA_remember(MAREA_HostContext *context)
    {
        marea_g_context = context;
    }

    MAREA_HOST_FUNCTION(MAREA_remember, MAREA_HOST_RETURNS_NOTHING, MAREA_HOST_CONTEXT);

    MAREA_Sinteger32 MAREA_middle(MAREA_Sinteger32 left, MAREA_HostContext *context, MAREA_Sinteger32 right)
    {
        marea_g_context = context;

        return left - right;
    }

    MAREA_HOST_FUNCTION(MAREA_middle, MAREA_HOST_RETURNS(sinteger32), MAREA_HOST_VALUE(sinteger32), MAREA_HOST_CONTEXT, MAREA_HOST_VALUE(sinteger32));

#define MAREA_P(index) MAREA_Sinteger32 p##index
#define MAREA_V        MAREA_HOST_VALUE(sinteger32)

    MAREA_Sinteger64 MAREA_thirtyTwo(MAREA_HostContext *context, MAREA_P(1), MAREA_P(2), MAREA_P(3), MAREA_P(4), MAREA_P(5), MAREA_P(6), MAREA_P(7), MAREA_P(8), MAREA_P(9), MAREA_P(10), MAREA_P(11), MAREA_P(12), MAREA_P(13), MAREA_P(14), MAREA_P(15), MAREA_P(16), MAREA_P(17), MAREA_P(18), MAREA_P(19), MAREA_P(20), MAREA_P(21), MAREA_P(22), MAREA_P(23), MAREA_P(24), MAREA_P(25), MAREA_P(26), MAREA_P(27), MAREA_P(28), MAREA_P(29), MAREA_P(30), MAREA_P(31), MAREA_P(32))
    {
        marea_g_context = context;

        const std::array<MAREA_Sinteger32, 32> VALUES{ p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32 };

        MAREA_Sinteger64 weighted{ 0 };

        for (std::size_t index{ 0 }; index < VALUES.size(); ++index) {
            weighted += static_cast<MAREA_Sinteger64>(index + 1) * VALUES[index];
        }

        return weighted;
    }

    MAREA_HOST_FUNCTION(MAREA_thirtyTwo, MAREA_HOST_RETURNS(sinteger64), MAREA_HOST_CONTEXT, MAREA_V, MAREA_V, MAREA_V, MAREA_V, MAREA_V, MAREA_V, MAREA_V, MAREA_V, MAREA_V, MAREA_V, MAREA_V, MAREA_V, MAREA_V, MAREA_V, MAREA_V, MAREA_V, MAREA_V, MAREA_V, MAREA_V, MAREA_V, MAREA_V, MAREA_V, MAREA_V, MAREA_V, MAREA_V, MAREA_V, MAREA_V, MAREA_V, MAREA_V, MAREA_V, MAREA_V, MAREA_V);

#undef MAREA_P
#undef MAREA_V

    std::uint64_t MAREA_bitsOf(double value)
    {
        std::uint64_t bits{ 0 };

        std::memcpy(&bits, &value, sizeof(bits));

        return bits;
    }

    struct MAREA_HostCallFixture
    {
        std::array<std::uint64_t, 33> slots{};
        std::array<MAREA_Null, 33> arguments{};
        MAREA_HostContext context{};
        std::uint64_t result{ 0 };

        MAREA_HostCallFixture()
        {
            for (std::size_t index{ 0 }; index < slots.size(); ++index) {
                arguments[index] = &slots[index];
            }

            marea_g_context = nullptr;
        }

        void MAREA_call(MAREA_HostFunction function, MAREA_Null target)
        {
            result = 0;

            function(&context, arguments.data(), target);
        }

        std::uint64_t MAREA_scalar(MAREA_HostFunction function, std::uint64_t bits)
        {
            slots[0] = bits;

            MAREA_call(function, &result);

            return result;
        }
    };

    std::vector<std::string> MAREA_cCompilers()
    {
        std::vector<std::string> compilers;

#if defined(MAREA_TESTS_CGEN_CLANG)
        compilers.push_back(MAREA_TESTS_CGEN_CLANG);
#endif
#if defined(MAREA_TESTS_CGEN_GCC)
        compilers.push_back(MAREA_TESTS_CGEN_GCC);
#endif

        if (compilers.empty()) {
            compilers.push_back(MAREA_TESTS_CGEN_C_COMPILER);
        }

        return compilers;
    }

    bool MAREA_compilesAsC(const std::string &compiler, const std::string &flags, const std::string &source, std::string &outOutput)
    {
        const MAREA_ScratchDirectory SCRATCH;

        const std::string SOURCE_PATH{ SCRATCH.MAREA_file("binding.c") };

        {
            std::ofstream stream(SOURCE_PATH, std::ios::binary | std::ios::trunc);

            stream << source;
        }

        const std::string COMMAND{ "\"" + compiler + "\" -std=c11 " + flags + " -c -I \"" + MAREA_TESTS_LIB_INCLUDE_DIRECTORY + "\" -o \"" + SCRATCH.MAREA_file("binding.o") + "\" \"" + SOURCE_PATH + "\" 2>&1" };

        FILE *pipe{ popen(COMMAND.c_str(), "r") };

        if (pipe == nullptr) {
            return false;
        }

        outOutput.clear();

        char chunk[256];

        while (fgets(chunk, sizeof(chunk), pipe) != nullptr) {
            outOutput += chunk;
        }

        const int STATUS{ pclose(pipe) };

        return WIFEXITED(STATUS) && (WEXITSTATUS(STATUS) == 0);
    }

    const std::string MAREA_PRELUDE{ "#include \"marea/base/MAREAHostFunction.h\"\n#include \"marea/base/MAREATypeRegistry.h\"\n"
                                     "typedef struct Pair { MAREA_Sinteger32 x; MAREA_Sinteger32 y; } Pair;\n" };

    const std::string MAREA_BINDINGS{ R"(
static MAREA_Sinteger32 add(MAREA_Sinteger32 a, MAREA_Sinteger32 b) { return a + b; }
MAREA_HOST_FUNCTION(add, MAREA_HOST_RETURNS(sinteger32), MAREA_HOST_VALUE(sinteger32), MAREA_HOST_VALUE(sinteger32));
static void sum(const Pair *left, const Pair *right, Pair *outSum) { outSum->x = left->x + right->x; outSum->y = left->y + right->y; }
MAREA_HOST_FUNCTION(sum, MAREA_HOST_RETURNS_STRUCTURE(Pair), MAREA_HOST_BORROW(Pair), MAREA_HOST_BORROW(Pair));
static void grow(MAREA_HostContext *context, Pair *value)
{
    MAREA_Null block = MAREA_NULLPTR;
    if (MAREA_HostContextAllocate(context, 8, 8, &block) != MAREA_HOST_CONTEXT_EXCEPT_SUCCESS) {
        MAREA_HostContextRaiseTrap(context, MAREA_HOST_TRAP_OUT_OF_MEMORY);
        return;
    }
    MAREA_HostContextFree(context, block);
    value->x += (MAREA_Sinteger32)MAREA_HostContextGetTrap(context);
}
MAREA_HOST_FUNCTION(grow, MAREA_HOST_RETURNS_NOTHING, MAREA_HOST_CONTEXT, MAREA_HOST_BORROW_MUTABLE(Pair));
static void idle(void) { }
MAREA_HOST_FUNCTION(idle, MAREA_HOST_RETURNS_NOTHING);
static MAREA_Float64 half(void) { return 0.5; }
MAREA_HOST_FUNCTION(half, MAREA_HOST_RETURNS(float64));
static void origin(Pair *outPoint) { outPoint->x = 0; outPoint->y = 0; }
MAREA_HOST_FUNCTION(origin, MAREA_HOST_RETURNS_STRUCTURE(Pair));
static MAREA_Boolean every(MAREA_Boolean a, MAREA_Sinteger8 b, MAREA_Sinteger16 c, MAREA_Sinteger64 d, MAREA_Uinteger8 e, MAREA_Uinteger16 f, MAREA_Uinteger32 g, MAREA_Uinteger64 h, MAREA_Float32 i)
{
    return a && (b == 0) && (c == 0) && (d == 0) && (e == 0) && (f == 0) && (g == 0) && (h == 0) && (i == 0);
}
MAREA_HOST_FUNCTION(every, MAREA_HOST_RETURNS(boolean), MAREA_HOST_VALUE(boolean), MAREA_HOST_VALUE(sinteger8), MAREA_HOST_VALUE(sinteger16), MAREA_HOST_VALUE(sinteger64), MAREA_HOST_VALUE(uinteger8), MAREA_HOST_VALUE(uinteger16), MAREA_HOST_VALUE(uinteger32), MAREA_HOST_VALUE(uinteger64), MAREA_HOST_VALUE(float32));
#define P(n) MAREA_Sinteger32 p##n
#define V MAREA_HOST_VALUE(sinteger32)
static void many(MAREA_HostContext *c, P(1), P(2), P(3), P(4), P(5), P(6), P(7), P(8), P(9), P(10), P(11), P(12), P(13), P(14), P(15), P(16), P(17), P(18), P(19), P(20), P(21), P(22), P(23), P(24), P(25), P(26), P(27), P(28), P(29), P(30), P(31), P(32))
{
    MAREA_IGNORE(c);
    MAREA_IGNORE(p1 + p2 + p3 + p4 + p5 + p6 + p7 + p8 + p9 + p10 + p11 + p12 + p13 + p14 + p15 + p16);
    MAREA_IGNORE(p17 + p18 + p19 + p20 + p21 + p22 + p23 + p24 + p25 + p26 + p27 + p28 + p29 + p30 + p31 + p32);
}
MAREA_HOST_FUNCTION(many, MAREA_HOST_RETURNS_NOTHING, MAREA_HOST_CONTEXT, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V);
MAREA_TypeRegistryExcept bind(MAREA_TypeRegistry *registry)
{
    return MAREA_REGISTER_FUNCTION(registry, MAREA_LIBRARY_FLAG_NONE, "demo.host", 9, "add", 3, add);
}
)" };

    struct MAREA_WrongBinding
    {
        const char *source;
        const char *message;
    };

    const std::array<MAREA_WrongBinding, 9> MAREA_WRONG_BINDINGS{ {
        { "static MAREA_Sinteger32 f(MAREA_Sinteger64 a) { return (MAREA_Sinteger32)a; } MAREA_HOST_FUNCTION(f, MAREA_HOST_RETURNS(sinteger32), MAREA_HOST_VALUE(sinteger32));", "does not match its description" },
        { "static void f(Pair *a) { a->x = 0; } MAREA_HOST_FUNCTION(f, MAREA_HOST_RETURNS_NOTHING, MAREA_HOST_BORROW(Pair));", "does not match its description" },
        { "static void f(const Pair *a) { MAREA_IGNORE(a); } MAREA_HOST_FUNCTION(f, MAREA_HOST_RETURNS_NOTHING, MAREA_HOST_BORROW_MUTABLE(Pair));", "does not match its description" },
        { "static Pair f(void) { Pair p = { 0, 0 }; return p; } MAREA_HOST_FUNCTION(f, MAREA_HOST_RETURNS_STRUCTURE(Pair));", "does not match its description" },
        { "static void f(MAREA_Sinteger32 a, MAREA_Sinteger32 b) { MAREA_IGNORE(a + b); } MAREA_HOST_FUNCTION(f, MAREA_HOST_RETURNS_NOTHING, MAREA_HOST_VALUE(sinteger32));", "does not match its description" },
        { "static void f(MAREA_HostContext *c) { MAREA_IGNORE(c); } MAREA_HOST_FUNCTION(f, MAREA_HOST_RETURNS_NOTHING);", "does not match its description" },
        { "static void f(void) { } MAREA_HOST_FUNCTION(f, MAREA_HOST_RETURNS(sinteger32));", "does not match its description" },
        { "#define P(n) MAREA_Sinteger32 p##n\n#define V MAREA_HOST_VALUE(sinteger32)\n"
          "static void f(P(1), P(2), P(3), P(4), P(5), P(6), P(7), P(8), P(9), P(10), P(11), P(12), P(13), P(14), P(15), P(16), P(17), P(18), P(19), P(20), P(21), P(22), P(23), P(24), P(25), P(26), P(27), P(28), P(29), P(30), P(31), P(32), P(33)) { }\n"
          "MAREA_HOST_FUNCTION(f, MAREA_HOST_RETURNS_NOTHING, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V);",
          "at most 32 parameters" },
        { "static void f(void) { } MAREA_TypeRegistryExcept bind(MAREA_TypeRegistry *registry) { f(); return MAREA_REGISTER_FUNCTION(registry, MAREA_LIBRARY_FLAG_NONE, \"demo.host\", 9, \"f\", 1, f); }", "MAREA_HostFunction_f" },
    } };
} // namespace

TEST(MAREAHostFunction, HostFunctionWrapperReadsEveryScalarAndWritesItsRegisterForm)
{
    GTEST_COUT << "Validating each scalar kind is read from the 8 bytes of the register form and written back in the same form." << std::endl;

    MAREA_HostCallFixture call;

    EXPECT_EQ(call.MAREA_scalar(MAREA_HOST_WRAPPER(MAREA_identity_boolean), 1), 1U) << "boolean";
    EXPECT_EQ(call.MAREA_scalar(MAREA_HOST_WRAPPER(MAREA_identity_boolean), 0), 0U) << "boolean";
    EXPECT_EQ(call.MAREA_scalar(MAREA_HOST_WRAPPER(MAREA_identity_sinteger8), static_cast<std::uint64_t>(-56)), static_cast<std::uint64_t>(-56)) << "sinteger8 is sign extended.";
    EXPECT_EQ(call.MAREA_scalar(MAREA_HOST_WRAPPER(MAREA_identity_sinteger16), static_cast<std::uint64_t>(-300)), static_cast<std::uint64_t>(-300)) << "sinteger16";
    EXPECT_EQ(call.MAREA_scalar(MAREA_HOST_WRAPPER(MAREA_identity_sinteger32), static_cast<std::uint64_t>(-70000)), static_cast<std::uint64_t>(-70000)) << "sinteger32";
    EXPECT_EQ(call.MAREA_scalar(MAREA_HOST_WRAPPER(MAREA_identity_sinteger64), static_cast<std::uint64_t>(INT64_MIN)), static_cast<std::uint64_t>(INT64_MIN)) << "sinteger64";
    EXPECT_EQ(call.MAREA_scalar(MAREA_HOST_WRAPPER(MAREA_identity_uinteger8), 200), 200U) << "uinteger8 is zero extended.";
    EXPECT_EQ(call.MAREA_scalar(MAREA_HOST_WRAPPER(MAREA_identity_uinteger16), 60000), 60000U) << "uinteger16";
    EXPECT_EQ(call.MAREA_scalar(MAREA_HOST_WRAPPER(MAREA_identity_uinteger32), 4000000000U), 4000000000U) << "uinteger32";
    EXPECT_EQ(call.MAREA_scalar(MAREA_HOST_WRAPPER(MAREA_identity_uinteger64), UINT64_MAX), UINT64_MAX) << "uinteger64";
    EXPECT_EQ(call.MAREA_scalar(MAREA_HOST_WRAPPER(MAREA_identity_float32), MAREA_bitsOf(1.0 / 3.0)), MAREA_bitsOf(static_cast<double>(static_cast<float>(1.0 / 3.0)))) << "float32 travels as a float64.";
    EXPECT_EQ(call.MAREA_scalar(MAREA_HOST_WRAPPER(MAREA_identity_float64), MAREA_bitsOf(1.0 / 3.0)), MAREA_bitsOf(1.0 / 3.0)) << "float64";
}

TEST(MAREAHostFunction, HostFunctionWrapperPassesBorrowsAndAStructureResult)
{
    GTEST_COUT << "Validating a read borrow, a write borrow and a trailing structure result." << std::endl;

    MAREA_HostCallFixture call;

    MAREA_HostPoint left{ 1, 2 };

    MAREA_HostPoint right{ 30, 40 };

    MAREA_HostPoint sum{ 0, 0 };

    call.arguments[0] = &left;

    call.arguments[1] = &right;

    call.MAREA_call(MAREA_HOST_WRAPPER(MAREA_sum), &sum);

    EXPECT_EQ(sum.x, 31) << "x is added.";

    EXPECT_EQ(sum.y, 42) << "y is added.";

    call.arguments[1] = &call.slots[1];

    call.slots[1] = 5;

    call.MAREA_call(MAREA_HOST_WRAPPER(MAREA_shift), nullptr);

    EXPECT_EQ(left.x, 6) << "The write borrow is the value itself.";

    call.MAREA_call(MAREA_HOST_WRAPPER(MAREA_norm), &call.result);

    EXPECT_EQ(call.result, 40U) << "6 * 6 + 2 * 2.";
}

TEST(MAREAHostFunction, HostFunctionWrapperTakesNoParameterAndGivesEveryKindOfResult)
{
    GTEST_COUT << "Validating a function with no parameter and no result, a scalar result or a structure result." << std::endl;

    MAREA_HostCallFixture call;

    marea_g_idleRuns = 0;

    call.MAREA_call(MAREA_HOST_WRAPPER(MAREA_idle), nullptr);

    EXPECT_EQ(marea_g_idleRuns, 1) << "The function runs.";

    call.MAREA_call(MAREA_HOST_WRAPPER(MAREA_half), &call.result);

    EXPECT_EQ(call.result, MAREA_bitsOf(0.5)) << "The scalar result is written.";

    MAREA_HostPoint origin{ 0, 0 };

    call.MAREA_call(MAREA_HOST_WRAPPER(MAREA_origin), &origin);

    EXPECT_EQ(origin.x, 7) << "The result bytes are the out pointer.";

    EXPECT_EQ(origin.y, 9) << "The result bytes are the out pointer.";
}

TEST(MAREAHostFunction, HostFunctionWrapperDropsAResultThatNobodyWants)
{
    GTEST_COUT << "Validating a call with a null result still runs the function and drops what it gives." << std::endl;

    MAREA_HostCallFixture call;

    call.MAREA_call(MAREA_HOST_WRAPPER(MAREA_half), nullptr);

    MAREA_HostPoint left{ 1, 2 };

    call.arguments[0] = &left;

    call.arguments[1] = &left;

    call.MAREA_call(MAREA_HOST_WRAPPER(MAREA_sum), nullptr);

    call.MAREA_call(MAREA_HOST_WRAPPER(MAREA_origin), nullptr);

    EXPECT_EQ(left.x, 1) << "Nothing else is written.";
}

TEST(MAREAHostFunction, HostFunctionWrapperGivesTheContextOnlyToAFunctionThatAsksForIt)
{
    GTEST_COUT << "Validating the context reaches the parameter that asks for it, at any place, and takes no argument slot." << std::endl;

    MAREA_HostCallFixture call;

    EXPECT_EQ(call.MAREA_scalar(MAREA_HOST_WRAPPER(MAREA_next), 41), 42U) << "The first argument is the first value.";

    EXPECT_EQ(marea_g_context, &call.context) << "The context of the call is given.";

    marea_g_context = nullptr;

    call.MAREA_call(MAREA_HOST_WRAPPER(MAREA_remember), nullptr);

    EXPECT_EQ(marea_g_context, &call.context) << "A function with only the context gets it.";

    marea_g_context = nullptr;

    call.slots[0] = 50;

    call.slots[1] = 8;

    call.MAREA_call(MAREA_HOST_WRAPPER(MAREA_middle), &call.result);

    EXPECT_EQ(call.result, 42U) << "The two values around the context are the two arguments.";

    EXPECT_EQ(marea_g_context, &call.context) << "A context in the middle gets the same context.";
}

TEST(MAREAHostFunction, HostFunctionWrapperPassesThirtyTwoParametersAndTheContext)
{
    GTEST_COUT << "Validating the 32 parameters of the vm limit arrive in order, beside the context." << std::endl;

    MAREA_HostCallFixture call;

    std::int64_t expected{ 0 };

    for (std::size_t index{ 0 }; index < 32; ++index) {
        call.slots[index] = 100 + index;

        expected += static_cast<std::int64_t>(index + 1) * static_cast<std::int64_t>(100 + index);
    }

    call.MAREA_call(MAREA_HOST_WRAPPER(MAREA_thirtyTwo), &call.result);

    EXPECT_EQ(static_cast<std::int64_t>(call.result), expected) << "Every parameter has its own argument, in order.";

    EXPECT_EQ(marea_g_context, &call.context) << "The context is not one of the 32.";
}

TEST(MAREAHostFunction, HostFunctionMacroCompilesAsC11UnderEveryCompiler)
{
    GTEST_COUT << "Validating the binding macros and the registration compile as C11 with every warning an error, under clang and gcc." << std::endl;

    for (const std::string &compiler : MAREA_cCompilers()) {
        std::string output;

        EXPECT_TRUE(MAREA_compilesAsC(compiler, "-Wall -Wextra -Werror -pedantic", MAREA_PRELUDE + MAREA_BINDINGS, output)) << compiler << ": " << output;
    }
}

TEST(MAREAHostFunction, HostFunctionMacroRefusesASignatureThatDoesNotMatch)
{
    GTEST_COUT << "Validating a wrong binding is a hard error with warnings off, so it fails any build." << std::endl;

    for (const std::string &compiler : MAREA_cCompilers()) {
        for (const MAREA_WrongBinding &wrong : MAREA_WRONG_BINDINGS) {
            std::string output;

            EXPECT_FALSE(MAREA_compilesAsC(compiler, "-w", MAREA_PRELUDE + wrong.source + "\n", output)) << compiler << " accepts " << wrong.source;

            EXPECT_NE(output.find(wrong.message), std::string::npos) << compiler << ": " << output;
        }
    }
}
