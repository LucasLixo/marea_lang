// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAParityHelpers.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace
{
    constexpr int MAREA_TRAP_OUT_OF_MEMORY{ 9 };

    const std::string MAREA_PRELUDE{ "namespace main; require c.memory as memory; " };

    MAREA_ParityHost MAREA_cMemoryHost()
    {
        MAREA_ParityHost host;

        host.cFunctions["c.memory.alloc"] = { "MAREA_CMemoryAllocate", 0 };
        host.cFunctions["c.memory.free"] = { "MAREA_CMemoryFree", 0 };
        host.cFunctions["c.memory.copy_bytes"] = { "MAREA_CMemoryCopyBytes", MAREA_CGEN_HOST_NO_CONTEXT };
        host.cFunctions["c.memory.fill_bytes"] = { "MAREA_CMemoryFillBytes", MAREA_CGEN_HOST_NO_CONTEXT };
        host.cFunctions["c.memory.offset"] = { "MAREA_CMemoryOffset", MAREA_CGEN_HOST_NO_CONTEXT };
        host.cFunctions["c.memory.is_null"] = { "MAREA_CMemoryIsNull", MAREA_CGEN_HOST_NO_CONTEXT };

        return host;
    }
} // namespace

TEST(MAREACgenCMemoryLibrary, AProgramAllocatesAndFreesThroughTheRuntimeHooks)
{
    GTEST_COUT << "Validating a program calls alloc, the raw functions and free in both backends, through the replaced hooks." << std::endl;

    MAREA_ParityHost host{ MAREA_cMemoryHost() };

    host.vmAllocate = &marea_parity_allocate;
    host.vmRelease = &marea_parity_release;
    host.cAllocate = "marea_parity_allocate";
    host.cRelease = "marea_parity_release";

    const MAREA_ParityResult RESULT{ MAREA_parityCheck(MAREA_PRELUDE + "function f(): boolean local first: uinteger64 = memory.alloc(24, 8); local second: uinteger64 = memory.alloc(24, 8); memory.fill_bytes(first, 7, 24); memory.copy_bytes(second, first, 24); local given: boolean = not memory.is_null(first) and not memory.is_null(second); memory.free(second); memory.free(first); memory.free(0); return given; end", { MAREA_parityCall("f", "boolean") }, &host) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("f()")), 1) << "Both blocks were given.";

    EXPECT_EQ(RESULT.MAREA_outcome("f()").log, (std::vector<std::int64_t>{ 1024, 1024, 2000, 2000 })) << "Two blocks from the hook alloc and two back to the hook free.";
}

TEST(MAREACgenCMemoryLibrary, PointerArithmeticAndTheNullCheckAnswerAlike)
{
    GTEST_COUT << "Validating offset and is_null give the same values in both backends." << std::endl;

    const MAREA_ParityHost HOST{ MAREA_cMemoryHost() };

    const MAREA_ParityResult RESULT{ MAREA_parityCheck(MAREA_PRELUDE + "function f(): uinteger64 return memory.offset(memory.offset(1000, 5, 4), -2, 4); end function g(): boolean return memory.is_null(0) and not memory.is_null(memory.offset(0, 1, 1)); end", { MAREA_parityCall("f", "uinteger64"), MAREA_parityCall("g", "boolean") }, &HOST) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("f()")), 1012) << "Five elements forward and two back.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("g()")), 1) << "Null and not null.";
}

TEST(MAREACgenCMemoryLibrary, AFailedAllocationTrapsAlike)
{
    GTEST_COUT << "Validating an allocation with no block stops both backends with OUT_OF_MEMORY." << std::endl;

    MAREA_ParityHost host{ MAREA_cMemoryHost() };

    host.vmAllocate = &marea_parity_no_block;
    host.vmRelease = &marea_parity_release;
    host.cAllocate = "marea_parity_no_block";
    host.cRelease = "marea_parity_release";

    const MAREA_ParityResult RESULT{ MAREA_parityCheck(MAREA_PRELUDE + "function f(): uinteger64 local block: uinteger64 = memory.alloc(8, 8); memory.free(block); return 1; end", { MAREA_parityCall("f", "uinteger64") }, &host) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(RESULT.MAREA_outcome("f()").trap, MAREA_TRAP_OUT_OF_MEMORY) << "The trap of a failed allocation.";

    EXPECT_EQ(RESULT.MAREA_outcome("f()").log, (std::vector<std::int64_t>{ 3008 })) << "Nothing runs after the trap.";
}

namespace
{
    // INFO: p is a typed pointer to the block at a, of 32 bytes, inside an unsafe block; r is the result.
    std::string MAREA_typedProgram(const std::string &element, const std::string &result, const std::string &body)
    {
        const std::string POINTER{ "c.memory.Pointer<" + element + ">" };

        return "namespace main; require c.memory; function f(): " + result + " local a: uinteger64 = c.memory.alloc(32, 8); local r: " + result + " = 0; unsafe local p: " + POINTER + " = convert<" + POINTER + ">(a); " + body + " end c.memory.free(a); return r; end";
    }
} // namespace

TEST(MAREACgenCMemoryLibrary, ATypedPointerReadsWritesAndStepsByElements)
{
    GTEST_COUT << "Validating a write, a compound assignment and a read through a typed pointer, also at a negative index." << std::endl;

    const MAREA_ParityHost HOST{ MAREA_cMemoryHost() };

    const MAREA_ParityResult RESULT{ MAREA_parityCheck(MAREA_typedProgram("sinteger32", "sinteger32", "p[0] = 10; p[1] = 20; p[1] += p[0]; p[2] = p[1] * 2; local last: c.memory.Pointer<sinteger32> = convert<c.memory.Pointer<sinteger32>>(c.memory.offset(a, 2, 4)); r = last[-2] + p[1] + last[0];"), { MAREA_parityCall("f", "sinteger32") }, &HOST) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("f()")), 100) << "10, then 30, then 60.";
}

TEST(MAREACgenCMemoryLibrary, TheUncheckedConversionReadsTheSameBytesAsAnotherType)
{
    GTEST_COUT << "Validating a conversion to a pointer of another type reads the same bytes, also at an address with no alignment, and converts back to the address." << std::endl;

    const MAREA_ParityHost HOST{ MAREA_cMemoryHost() };

    const MAREA_ParityResult RESULT{ MAREA_parityCheck(MAREA_typedProgram("uinteger8", "uinteger32", "p[0] = 1; p[1] = 2; p[2] = 3; p[3] = 4; p[4] = 5; local odd: c.memory.Pointer<uinteger32> = convert<c.memory.Pointer<uinteger32>>(c.memory.offset(convert<uinteger64>(p), 1, 1)); r = odd[0];"), { MAREA_parityCall("f", "uinteger32") }, &HOST) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("f()")), 0x05040302) << "The bytes 2 to 5, the low byte first.";
}

TEST(MAREACgenCMemoryLibrary, ATypedPointerHoldsFloatsAndBooleans)
{
    GTEST_COUT << "Validating a pointer to a float and to a boolean." << std::endl;

    const MAREA_ParityHost HOST{ MAREA_cMemoryHost() };

    const MAREA_ParityResult RESULT{ MAREA_parityCheck(MAREA_typedProgram("float64", "sinteger32", "p[1] = 2.5; p[1] *= 4.0; local flags: c.memory.Pointer<boolean> = convert<c.memory.Pointer<boolean>>(a); flags[0] = true; if flags[0] then r = convert<sinteger32>(p[1]); end"), { MAREA_parityCall("f", "sinteger32") }, &HOST) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("f()")), 10) << "2.5 times 4.";
}

TEST(MAREACgenCMemoryLibrary, APointerIsCheckedForNullAndComparedAlike)
{
    GTEST_COUT << "Validating the null check of a pointer through its address, a copy of a pointer and a comparison." << std::endl;

    const MAREA_ParityHost HOST{ MAREA_cMemoryHost() };

    const MAREA_ParityResult RESULT{ MAREA_parityCheck(MAREA_typedProgram("sinteger32", "sinteger32", "local none: c.memory.Pointer<sinteger32> = convert<c.memory.Pointer<sinteger32>>(0); local same: c.memory.Pointer<sinteger32> = none; if c.memory.is_null(convert<uinteger64>(same)) and not c.memory.is_null(convert<uinteger64>(p)) then r = 1; end if same == none and same != p then r += 2; end same = p; if same == p then r += 4; end"), { MAREA_parityCall("f", "sinteger32") }, &HOST) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("f()")), 7) << "The three checks hold.";
}

TEST(MAREACgenCMemoryLibrary, WhatAPointerDoesNotDoIsRefusedByBothBackends)
{
    GTEST_COUT << "Validating arithmetic with an operator on a pointer and a pointer to a structure have no form in a backend." << std::endl;

    const MAREA_ParityHost HOST{ MAREA_cMemoryHost() };

    for (const std::string &program : {
             MAREA_typedProgram("sinteger32", "sinteger32", "local q: c.memory.Pointer<sinteger32> = p + p;"),
             MAREA_typedProgram("sinteger32", "sinteger32", "p += p;"),
             std::string("namespace main; require c.memory; structure S x: sinteger32; end function f(): sinteger32 unsafe local p: c.memory.Pointer<S> = convert<c.memory.Pointer<S>>(0); return p[0].x; end return 0; end"),
         }) {
        const MAREA_ParityResult RESULT{ MAREA_parityCheck(program, { MAREA_parityCall("f", "sinteger32") }, &HOST) };

        EXPECT_EQ(RESULT.failures.size(), 2U) << "Both backends refuse: " << program << " " << RESULT.MAREA_failures();
    }
}
