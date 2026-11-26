// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAParityHelpers.hpp"

#include <string>
#include <vector>

namespace
{
    constexpr int MAREA_TRAP_OUT_OF_MEMORY{ 9 };
    constexpr int MAREA_TRAP_HOST_FUNCTION_MISSING{ 10 };

    const std::string MAREA_WIDE{ "structure Wide first: float64; second: float64; end structure Block a: sinteger64; b: sinteger64; c: sinteger64; d: sinteger64; e: sinteger64; end " };

    MAREA_ParityResult MAREA_run(const std::string &body, const MAREA_ParityHost &host, const std::vector<MAREA_ParityCall> &calls = {})
    {
        return MAREA_parityCheck(MAREA_PARITY_HOST_PRELUDE + body, calls, &host);
    }

    MAREA_ParityResult MAREA_run(const std::string &body, const std::vector<MAREA_ParityCall> &calls = {})
    {
        return MAREA_run(body, MAREA_parityHost(), calls);
    }

    std::vector<std::int64_t> MAREA_log(const std::vector<std::int64_t> &entries)
    {
        return entries;
    }
} // namespace

TEST(MAREACgenHostCalls, ScalarsGoInAndComeBackNormalized)
{
    GTEST_COUT << "Validating scalar arguments and results of a host call, normalized to the signature of sema as in the vm." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_run("function f(a: sinteger32): sinteger32 return host.add(a, 22) + host.add(1, -1); end function g(): float64 return host.wide(5000000000, -1, 2.5); end function h(): sinteger8 return host.narrow(200); end function i(): boolean return host.flag(5); end function j(): float32 return host.third(); end function k(a: boolean): boolean return host.negate(a); end function l(): sinteger64 return host.thirtytwo(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32); end") };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("h()")), -56) << "200 read as sinteger8 is -56.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("i()")), 1) << "A boolean result is 0 or 1.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("l()")), 11440) << "The weighted sum of 32 values.";
}

TEST(MAREACgenHostCalls, StructuresAreBorrowedAndAStructureResultIsRead)
{
    GTEST_COUT << "Validating borrows of structures, a write borrow, a structure result and the address of a borrow." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_run("function f(): sinteger32 local a: Pair = Pair{ x = 1, y = 2 }; local b: Pair = Pair{ x = 30, y = 40 }; local r: Pair = host.sum(a, b); host.bump(a); host.bump(a); host.shift(a, 5); return (r.x * 100000) + (r.y * 1000) + (a.x * 10) + a.y + host.square(a); end function g(): boolean local a: Pair = Pair{ x = 1, y = 2 }; return host.address(a) == host.address(a); end") };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("g()")), 1) << "The same local has one address.";

    EXPECT_EQ(RESULT.MAREA_outcome("f()").log, MAREA_log({ 0, 0 })) << "The two bumps ran.";
}

TEST(MAREACgenHostCalls, NestedAndLiteralStructureArgumentsAndWideStructures)
{
    GTEST_COUT << "Validating a literal and a nested call as structure arguments, a result over an argument and structures of 16 and 40 bytes." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_run(MAREA_WIDE + "function f(): sinteger32 local a: Pair = Pair{ x = 1, y = 2 }; local r: Pair = host.sum(Pair{ x = 10, y = 20 }, host.sum(a, a)); for i in 1 .. 3 do a = host.sum(a, a); end host.sum(a, a); host.add(1, 2); return (r.x * 100000) + (r.y * 1000) + (a.x * 100) + a.y; end function g(): float64 local w: Wide = Wide{ first = 1.5, second = 2.5 }; local s: Wide = host.swap(w); return (s.first * 10.0) + s.second; end function h(): sinteger64 local block: Block = Block{ a = 1, b = 2, c = 3, d = 4, e = 5 }; local r: Block = host.reverse(block); return (r.a * 10000) + (r.b * 1000) + (r.c * 100) + (r.d * 10) + r.e; end") };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("h()")), 54321) << "The block is reversed.";
}

TEST(MAREACgenHostCalls, AnOrderOfCallsAndAFaultStopTheLaterCalls)
{
    GTEST_COUT << "Validating host calls run in the order of the source and no call runs after a fault." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_run("function note(value: sinteger32): sinteger32 host.record(value); return value; end function f(a: sinteger32): sinteger32 host.record(1); local r: sinteger32 = host.add(note(2), note(3)) + (note(4) * 1); host.record(10 / a); host.record(5); return r; end function g(a: sinteger32): sinteger32 if (10 / a) == 0 then host.record(6); else host.record(7); end return 0; end", { MAREA_parityCall("f", "sinteger32", { MAREA_parityInt(5) }), MAREA_parityCall("f", "sinteger32", { MAREA_parityInt(0) }), MAREA_parityCall("g", "sinteger32", { MAREA_parityInt(0) }) }) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(RESULT.generated[0].log, MAREA_log({ 1, 2, 3, 4, 2, 5 })) << "Left to right.";

    EXPECT_EQ(RESULT.generated[1].log, MAREA_log({ 1, 2, 3, 4 })) << "Nothing runs after the fault.";

    EXPECT_TRUE(RESULT.generated[2].log.empty()) << "A fault in a test runs no branch.";
}

TEST(MAREACgenHostCalls, AContextFunctionAllocatesThroughTheRuntimeAllocator)
{
    GTEST_COUT << "Validating a function that asks for the context gets a working one, with the allocator of the runtime." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_run("function f(): boolean return host.block(64); end") };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("f()")), 1) << "The block is zeroed.";
}

TEST(MAREACgenHostCalls, TheHostReplacesTheAllocatorHooks)
{
    GTEST_COUT << "Validating a host that replaces alloc and free sees its own functions in both backends." << std::endl;

    MAREA_ParityHost host{ MAREA_parityHost() };

    host.vmAllocate = &marea_parity_allocate;
    host.vmRelease = &marea_parity_release;
    host.cAllocate = "marea_parity_allocate";
    host.cRelease = "marea_parity_release";

    const MAREA_ParityResult RESULT{ MAREA_run("function f(): boolean return host.block(64); end", host) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(RESULT.MAREA_outcome("f()").log, MAREA_log({ 1064, 2000 })) << "The block came from the host and went back to it.";
}

TEST(MAREACgenHostCalls, AFailedAllocationIsTheOutOfMemoryTrap)
{
    GTEST_COUT << "Validating a function that raises out of memory stops the run, and nothing after the call runs." << std::endl;

    MAREA_ParityHost host{ MAREA_parityHost() };

    host.vmAllocate = &marea_parity_no_block;
    host.vmRelease = &marea_parity_release;
    host.cAllocate = "marea_parity_no_block";
    host.cRelease = "marea_parity_release";

    const MAREA_ParityResult RESULT{ MAREA_run("function f(): boolean host.record(1); local ok: boolean = host.block(8); host.record(2); return ok; end function g(): boolean return f(); end", host) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(RESULT.MAREA_outcome("g()").trap, MAREA_TRAP_OUT_OF_MEMORY) << "The trap goes up to the caller.";

    EXPECT_EQ(RESULT.MAREA_outcome("g()").log, MAREA_log({ 1, 3008 })) << "Nothing after the call ran.";
}

TEST(MAREACgenHostCalls, TheHostChoosesTheCFunctionBehindAName)
{
    GTEST_COUT << "Validating a host that binds a name to another C function changes both backends in the same way." << std::endl;

    MAREA_ParityHost host{ MAREA_parityHost() };

    host.MAREA_bind("add", &MAREA_HOST_WRAPPER(marea_parity_subtract), "marea_parity_subtract");

    const MAREA_ParityResult RESULT{ MAREA_run("function f(): sinteger32 return host.add(10, 3); end", host) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("f()")), 7) << "The name is the subtraction now.";
}

TEST(MAREACgenHostCalls, ANameThatNothingBindsTrapsInBothBackends)
{
    GTEST_COUT << "Validating a registered name of sema that the host binds nowhere is the missing function trap." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_run("function f(): sinteger32 host.record(1); local v: sinteger32 = host.missing(); host.record(2); return v; end") };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(RESULT.MAREA_outcome("f()").trap, MAREA_TRAP_HOST_FUNCTION_MISSING) << "The call traps.";

    EXPECT_EQ(RESULT.MAREA_outcome("f()").log, MAREA_log({ 1 })) << "Nothing after the call runs.";
}

TEST(MAREACgenHostCalls, CallsThroughAFullPathAndThroughAnImportedName)
{
    GTEST_COUT << "Validating a full path and a member alias name the same C function." << std::endl;

    const MAREA_ParityHost HOST{ MAREA_parityHost() };

    const MAREA_ParityResult RESULT{ MAREA_parityCheck("namespace main; require demo.host; require demo.host.add as plus; function f(): sinteger32 return demo.host.add(1, 2) + plus(3, 4); end", {}, &HOST) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("f()")), 10) << "3 + 7.";
}

TEST(MAREACgenHostCalls, AnUntypedHostCallUsedAsAValueIsReported)
{
    GTEST_COUT << "Validating a host call that sema could not type is a diagnostic when its value is used, as in the vm." << std::endl;

    const MAREA_ParityHost HOST{ MAREA_parityHost() };

    EXPECT_EQ(MAREA_parityRejected("namespace main; require demo.other as other; function f(): sinteger32 return other.value(); end", &HOST).count(MAREA_CGEN_DIAGNOSTIC_CODE_UNTYPED_OPERAND), 1U) << "The call has no type.";
}

TEST(MAREACgenHostCalls, TheContextSettersRejectWhatTheyCannotUse)
{
    GTEST_COUT << "Validating the host settings of the context refuse null arguments and one allocator hook alone." << std::endl;

    MAREA_IrFixture fixture;

    MAREA_Arena *arena{ fixture.sema.parser.lexer.arena };

    MAREA_CgenContext *context{ nullptr };

    ASSERT_EQ(MAREA_CgenContextCreate(arena, fixture.sema.parser.lexer.sink, &context), MAREA_CGEN_EXCEPT_SUCCESS) << "The context must be created.";

    EXPECT_EQ(MAREA_CgenContextSetHostResolver(nullptr, &MAREA_parityResolveHost, nullptr), MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER) << "A null context must be refused.";

    EXPECT_EQ(MAREA_CgenContextSetHostResolver(context, nullptr, nullptr), MAREA_CGEN_EXCEPT_SUCCESS) << "A null resolver clears it.";

    EXPECT_EQ(MAREA_CgenContextAddInclude(nullptr, "a.h", 3), MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER) << "A null context must be refused.";

    EXPECT_EQ(MAREA_CgenContextAddInclude(context, nullptr, 3), MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER) << "A null name must be refused.";

    EXPECT_EQ(MAREA_CgenContextSetAllocator(nullptr, "a", 1, "r", 1), MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER) << "A null context must be refused.";

    EXPECT_EQ(MAREA_CgenContextSetAllocator(context, "a", 1, nullptr, 0), MAREA_CGEN_EXCEPT_ERROR_UNPAIRED_ALLOCATOR) << "One hook alone must be refused.";

    EXPECT_EQ(MAREA_CgenContextSetAllocator(context, nullptr, 0, "r", 1), MAREA_CGEN_EXCEPT_ERROR_UNPAIRED_ALLOCATOR) << "One hook alone must be refused.";

    EXPECT_EQ(MAREA_CgenContextSetAllocator(context, nullptr, 0, nullptr, 0), MAREA_CGEN_EXCEPT_SUCCESS) << "No hook is the allocator of the runtime.";
}

TEST(MAREACgenHostCalls, TheGeneratedTextIncludesTheHeadersOfTheContext)
{
    GTEST_COUT << "Validating the headers of the context are written before any code, so the C functions are declared." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; function f(): sinteger32 return 1; end"), MAREA_IR_EXCEPT_SUCCESS) << "The module must lower.";

    MAREA_CgenContext *context{ nullptr };

    ASSERT_EQ(MAREA_CgenContextCreate(fixture.sema.parser.lexer.arena, fixture.sema.parser.lexer.sink, &context), MAREA_CGEN_EXCEPT_SUCCESS) << "The context must be created.";

    ASSERT_EQ(MAREA_CgenContextAddInclude(context, "demo/host.h", 11), MAREA_CGEN_EXCEPT_SUCCESS) << "The first header.";

    ASSERT_EQ(MAREA_CgenContextAddInclude(context, "other.h", 7), MAREA_CGEN_EXCEPT_SUCCESS) << "The second header.";

    const MAREA_Character8 *text{ nullptr };

    MAREA_IntegerSize length{ 0 };

    ASSERT_EQ(MAREA_CgenGenerateModule(context, fixture.module, &text, &length), MAREA_CGEN_EXCEPT_SUCCESS) << "The module must generate.";

    const std::string TEXT(text, length);

    const std::size_t FIRST{ TEXT.find("#include \"demo/host.h\"\n") };

    const std::size_t SECOND{ TEXT.find("#include \"other.h\"\n") };

    ASSERT_NE(FIRST, std::string::npos) << "The first header must be written.";

    ASSERT_NE(SECOND, std::string::npos) << "The second header must be written.";

    EXPECT_LT(FIRST, SECOND) << "The headers keep their order.";

    EXPECT_LT(SECOND, TEXT.find("f_f(")) << "The headers come before the code.";
}
