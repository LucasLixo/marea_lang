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
