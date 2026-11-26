// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAVmHostHelpers.hpp"

#include <cstring>

TEST(MAREAVmHostCalls, ContextKeepsTheRegistry)
{
    GTEST_COUT << "Validating the context keeps the registry it is given and rejects a null context." << std::endl;

    MAREA_VmHostFixture fixture;

    EXPECT_EQ(MAREA_VmContextGetTypeRegistry(fixture.context), fixture.registry) << "The context must keep the registry.";

    EXPECT_EQ(MAREA_VmContextSetTypeRegistry(fixture.context, nullptr), MAREA_VM_EXCEPT_SUCCESS) << "A null registry clears it.";

    EXPECT_EQ(MAREA_VmContextGetTypeRegistry(fixture.context), nullptr) << "The context must have none now.";

    EXPECT_EQ(MAREA_VmContextSetTypeRegistry(nullptr, fixture.registry), MAREA_VM_EXCEPT_ERROR_NULL_POINTER) << "A null context must be rejected.";

    EXPECT_EQ(MAREA_VmContextGetTypeRegistry(nullptr), nullptr) << "A null context has no registry.";
}

TEST(MAREAVmHostCalls, CallsARegisteredFunctionWithScalars)
{
    GTEST_COUT << "Validating bytecode calls a registered C function with scalars and reads its result." << std::endl;

    MAREA_VmHostFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function f(a: sinteger32): sinteger32 return host.add(a, 22) + host.add(1, -1); end")) << "The unit must generate.";

    const MAREA_VmResult RESULT{ fixture.MAREA_call("f", { 20 }) };

    EXPECT_FALSE(RESULT.trapped) << "No trap.";

    EXPECT_EQ(static_cast<std::int32_t>(RESULT.value), 42) << "20 + 22 + 0 is 42.";
}

TEST(MAREAVmHostCalls, PassesALiteralInTheWholeRegister)
{
    GTEST_COUT << "Validating a literal argument wider than 32 bits and a negative literal reach the function unchanged." << std::endl;

    MAREA_VmHostFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function f(): float64 return host.wide(5000000000, -1, 2.5); end")) << "The unit must generate.";

    const MAREA_VmResult RESULT{ fixture.MAREA_call("f") };

    double value{ 0.0 };

    std::memcpy(&value, &RESULT.value, sizeof(value));

    EXPECT_DOUBLE_EQ(value, 5000000001.5) << "5000000000 - 1 + 2.5.";
}

TEST(MAREAVmHostCalls, NormalizesTheScalarResult)
{
    GTEST_COUT << "Validating the vm normalizes a scalar result to the declared result type of the signature." << std::endl;

    MAREA_VmHostFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function f(): sinteger8 return host.narrow(200); end function g(): boolean return host.flag(5); end function h(): float32 return host.third(); end function i(a: boolean): boolean return host.negate(a); end")) << "The unit must generate.";

    EXPECT_EQ(static_cast<std::int64_t>(fixture.MAREA_call("f").value), -56) << "200 read as sinteger8 is -56, sign extended.";

    EXPECT_EQ(fixture.MAREA_call("g").value, 1U) << "A boolean result is 0 or 1.";

    const MAREA_VmResult THIRD{ fixture.MAREA_call("h") };

    double value{ 0.0 };

    std::memcpy(&value, &THIRD.value, sizeof(value));

    EXPECT_EQ(value, static_cast<double>(static_cast<float>(1.0 / 3.0))) << "A float32 result is rounded to float32.";

    EXPECT_EQ(fixture.MAREA_call("i", { 0 }).value, 1U) << "A boolean argument reaches the function.";
}

TEST(MAREAVmHostCalls, PassesStructuresAsBorrowsAndReadsAStructureResult)
{
    GTEST_COUT << "Validating two borrowed structures go in and a structure comes out." << std::endl;

    MAREA_VmHostFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function f(): sinteger32 local a: Pair = Pair{ x = 1, y = 2 }; local b: Pair = Pair{ x = 30, y = 40 }; local r: Pair = host.sum(a, b); return (r.x * 100) + r.y; end")) << "The unit must generate.";

    const MAREA_VmResult RESULT{ fixture.MAREA_call("f") };

    EXPECT_FALSE(RESULT.trapped) << "No trap.";

    EXPECT_EQ(static_cast<std::int32_t>(RESULT.value), 3142) << "The sum is (31, 42).";
}

TEST(MAREAVmHostCalls, PassesTheValueItselfNotACopyOfIt)
{
    GTEST_COUT << "Validating a borrow is the address of the local, so a write of the C function is seen, and a call with no result gets a null result." << std::endl;

    MAREA_VmHostFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function f(): sinteger32 local a: Pair = Pair{ x = 1, y = 2 }; host.bump(a); host.bump(a); return a.x; end function g(): boolean local a: Pair = Pair{ x = 1, y = 2 }; return host.address(a) == host.address(a); end")) << "The unit must generate.";

    EXPECT_EQ(static_cast<std::int32_t>(fixture.MAREA_call("f").value), 3) << "Two bumps change the local itself.";

    EXPECT_EQ(MAREA_hostLog(), (std::vector<std::int64_t>{ 0, 0 })) << "A function with no result gets a null result pointer.";

    EXPECT_EQ(fixture.MAREA_call("g").value, 1U) << "The same local has the same address on both calls.";
}

TEST(MAREAVmHostCalls, GivesAScalarForABorrow)
{
    GTEST_COUT << "Validating a borrowed structure with a scalar result." << std::endl;

    MAREA_VmHostFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function f(): sinteger32 local a: Pair = Pair{ x = 3, y = 4 }; return host.square(a); end")) << "The unit must generate.";

    EXPECT_EQ(static_cast<std::int32_t>(fixture.MAREA_call("f").value), 25) << "9 + 16.";
}

TEST(MAREAVmHostCalls, BuildsAStructureArgumentFromALiteralAndFromAnotherResult)
{
    GTEST_COUT << "Validating a literal and a nested host call can be structure arguments." << std::endl;

    MAREA_VmHostFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function f(): sinteger32 local a: Pair = Pair{ x = 1, y = 2 }; local r: Pair = host.sum(Pair{ x = 10, y = 20 }, host.sum(a, a)); return (r.x * 100) + r.y; end")) << "The unit must generate.";

    EXPECT_EQ(static_cast<std::int32_t>(fixture.MAREA_call("f").value), 1224) << "(10 + 2, 20 + 4).";
}

TEST(MAREAVmHostCalls, AssignsAStructureResultOverAnArgumentOfTheSameCall)
{
    GTEST_COUT << "Validating a = host.sum(a, a) reads the old a, in a loop that reuses the same frame storage." << std::endl;

    MAREA_VmHostFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function f(): sinteger32 local a: Pair = Pair{ x = 1, y = 2 }; for i in 1 .. 3 do a = host.sum(a, a); end return (a.x * 100) + a.y; end")) << "The unit must generate.";

    EXPECT_EQ(static_cast<std::int32_t>(fixture.MAREA_call("f").value), 816) << "Three doublings give (8, 16).";
}

TEST(MAREAVmHostCalls, DiscardsAResultThatNoOneReads)
{
    GTEST_COUT << "Validating a call statement with a structure or scalar result runs and throws the result away." << std::endl;

    MAREA_VmHostFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function f(): sinteger32 local a: Pair = Pair{ x = 1, y = 2 }; host.sum(a, a); host.add(1, 2); return a.x + 6; end")) << "The unit must generate.";

    EXPECT_EQ(static_cast<std::int32_t>(fixture.MAREA_call("f").value), 7) << "The discarded results change nothing.";
}

TEST(MAREAVmHostCalls, CallsThroughAFullPathAndThroughAnImportedName)
{
    GTEST_COUT << "Validating a full path and a member alias name the same registration." << std::endl;

    MAREA_VmHostFixture fixture;

    ASSERT_TRUE(fixture.generate.MAREA_generate("namespace main; require demo.host; require demo.host.add as plus; function f(): sinteger32 return demo.host.add(1, 2) + plus(3, 4); end")) << "The unit must generate.";

    EXPECT_EQ(static_cast<std::int32_t>(fixture.MAREA_call("f").value), 10) << "3 + 7.";
}

TEST(MAREAVmHostCalls, ANameThatNothingRegisteredTraps)
{
    GTEST_COUT << "Validating a call of a name the registry does not have is the trap HOST_FUNCTION_MISSING." << std::endl;

    MAREA_VmHostFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function f(): sinteger32 host.record(1); local v: sinteger32 = host.missing(); host.record(2); return v; end function g(): sinteger32 return f(); end")) << "The unit must generate.";

    const MAREA_VmResult RESULT{ fixture.MAREA_call("g") };

    ASSERT_TRUE(RESULT.trapped) << "A missing name must trap through every caller.";

    EXPECT_EQ(RESULT.trap, MAREA_BYTECODE_TRAP_CODE_HOST_FUNCTION_MISSING) << "The trap code must say a host function is missing.";

    EXPECT_EQ(MAREA_hostLog(), (std::vector<std::int64_t>{ 1 })) << "Nothing runs after the trap.";
}

TEST(MAREAVmHostCalls, AContextWithNoRegistryTraps)
{
    GTEST_COUT << "Validating a context with no registry traps on a host call." << std::endl;

    MAREA_VmHostFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function f(): sinteger32 return host.add(1, 2); end")) << "The unit must generate.";

    ASSERT_EQ(MAREA_VmContextSetTypeRegistry(fixture.context, nullptr), MAREA_VM_EXCEPT_SUCCESS) << "The registry must be cleared.";

    EXPECT_EQ(fixture.MAREA_call("f").trap, MAREA_BYTECODE_TRAP_CODE_HOST_FUNCTION_MISSING) << "No registry has no function.";
}

TEST(MAREAVmHostCalls, AHostCannotRegisterUnderAReservedRoot)
{
    GTEST_COUT << "Validating a host registration under c or marea is rejected, so a call of that name finds nothing." << std::endl;

    MAREA_VmHostFixture fixture;

    EXPECT_EQ(MAREA_REGISTER_FUNCTION(fixture.registry, MAREA_LIBRARY_FLAG_NONE, "c.test", 6, "twice", 5, &MAREA_testHostTwice), MAREA_TYPE_REGISTRY_EXCEPT_ERROR_RESERVED_NAMESPACE_ROOT) << "The root c is reserved.";

    EXPECT_EQ(MAREA_REGISTER_FUNCTION(fixture.registry, MAREA_LIBRARY_FLAG_NONE, "marea.test", 10, "twice", 5, &MAREA_testHostTwice), MAREA_TYPE_REGISTRY_EXCEPT_ERROR_RESERVED_NAMESPACE_ROOT) << "The root marea is reserved.";

    ASSERT_TRUE(fixture.generate.MAREA_generate("namespace main; require c.test; require marea.test; function f(): sinteger32 return c.test.twice(1); end function g(): sinteger32 return marea.test.twice(1); end")) << "The unit must generate.";

    EXPECT_EQ(fixture.MAREA_call("f").trap, MAREA_BYTECODE_TRAP_CODE_HOST_FUNCTION_MISSING) << "The rejected name under c is not there.";

    EXPECT_EQ(fixture.MAREA_call("g").trap, MAREA_BYTECODE_TRAP_CODE_HOST_FUNCTION_MISSING) << "The rejected name under marea is not there.";
}

TEST(MAREAVmHostCalls, AShippedLibraryRegistersUnderAReservedRoot)
{
    GTEST_COUT << "Validating a function of a shipped library, registered through the internal entry, is called under c." << std::endl;

    MAREA_VmHostFixture fixture;

    ASSERT_EQ(MAREA_REGISTER_FUNCTION(fixture.registry, MAREA_LIBRARY_FLAG_C_MEMORY, "c.test", 6, "twice", 5, &MAREA_testHostTwice), MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) << "The library must register.";

    ASSERT_TRUE(fixture.generate.MAREA_generate("namespace main; require c.test; function f(): sinteger32 return c.test.twice(21); end")) << "The unit must generate.";

    EXPECT_EQ(static_cast<std::int32_t>(fixture.MAREA_call("f").value), 42) << "21 twice.";
}

TEST(MAREAVmHostCalls, AnImportIsNotAFunctionOfTheModule)
{
    GTEST_COUT << "Validating a caller of the vm cannot run an import by name or by index." << std::endl;

    MAREA_VmHostFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function f(): sinteger32 return host.add(1, 2); end")) << "The unit must generate.";

    std::uint32_t index{ 0 };

    EXPECT_EQ(MAREA_VmFindFunction(fixture.generate.module, "demo.host.add", 13, &index), MAREA_VM_EXCEPT_ERROR_UNKNOWN_FUNCTION) << "An import is never found.";

    ASSERT_EQ(MAREA_BytecodeModuleGetFunctionCount(fixture.generate.module), 2U) << "The module has f and one import.";

    const MAREA_Uinteger64 ARGUMENTS[2]{ 1, 2 };

    MAREA_VmResult result{};

    EXPECT_EQ(MAREA_VmCall(fixture.context, fixture.generate.module, 1, ARGUMENTS, 2, &result), MAREA_VM_EXCEPT_ERROR_UNKNOWN_FUNCTION) << "An import is not run from outside.";
}

TEST(MAREAVmHostCalls, PassesAndReturnsAStructureOfSixteenBytes)
{
    GTEST_COUT << "Validating a structure of 16 bytes goes in as a borrow and comes out through the result pointer." << std::endl;

    MAREA_VmHostFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("structure Wide first: float64; second: float64; end function f(): boolean local a: Wide = Wide{ first = 1.5, second = 2.5 }; local r: Wide = host.swap(a); return r.first == 2.5 and r.second == 1.5; end")) << "The unit must generate.";

    const MAREA_VmResult RESULT{ fixture.MAREA_call("f") };

    EXPECT_FALSE(RESULT.trapped) << "No trap.";

    EXPECT_EQ(RESULT.value, 1U) << "Both halves are swapped.";
}

TEST(MAREAVmHostCalls, PassesAndReturnsAStructureLargerThanSixteenBytes)
{
    GTEST_COUT << "Validating a structure of 40 bytes goes in and comes out the same way." << std::endl;

    MAREA_VmHostFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("structure Block a: sinteger64; b: sinteger64; c: sinteger64; d: sinteger64; e: sinteger64; end function f(): sinteger64 local v: Block = Block{ a = 1, b = 2, c = 3, d = 4, e = 5 }; local r: Block = host.reverse(v); return (r.a * 10000) + (r.b * 1000) + (r.c * 100) + (r.d * 10) + r.e; end")) << "The unit must generate.";

    EXPECT_EQ(static_cast<std::int64_t>(fixture.MAREA_call("f").value), 54321) << "The five fields are reversed.";
}

TEST(MAREAVmHostCalls, PassesANestedFieldAndACopyAsStructureArguments)
{
    GTEST_COUT << "Validating a nested structure field is borrowed in place and a copy is built apart from its source." << std::endl;

    MAREA_VmHostFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("structure Box count: sinteger32; inner: Pair; end function f(): sinteger32 local box: Box = Box{ count = 1, inner = Pair{ x = 3, y = 4 } }; host.bump(box.inner); return host.square(box.inner); end function g(): sinteger32 local a: Pair = Pair{ x = 3, y = 4 }; host.bump(copy a); return host.square(a); end")) << "The unit must generate.";

    EXPECT_EQ(static_cast<std::int32_t>(fixture.MAREA_call("f").value), 32) << "The field itself was bumped to (4, 4).";

    EXPECT_EQ(static_cast<std::int32_t>(fixture.MAREA_call("g").value), 25) << "The bump of the copy does not reach a.";
}

TEST(MAREAVmHostCalls, RunsAHostCallFromAModuleReadBackFromItsBinaryForm)
{
    GTEST_COUT << "Validating the imports survive the binary form and still run." << std::endl;

    MAREA_VmHostFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function f(): sinteger32 local a: Pair = Pair{ x = 1, y = 2 }; local r: Pair = host.sum(a, a); return r.y; end")) << "The unit must generate.";

    const MAREA_Byte *bytes{ nullptr };

    MAREA_IntegerSize size{ 0 };

    ASSERT_EQ(MAREA_BytecodeSerialize(fixture.generate.module, &bytes, &size), MAREA_BYTECODE_EXCEPT_SUCCESS) << "The module must serialize.";

    MAREA_BytecodeModule *back{ nullptr };

    ASSERT_EQ(MAREA_BytecodeDeserialize(fixture.generate.context, bytes, size, &back), MAREA_BYTECODE_EXCEPT_SUCCESS) << "The bytes must be read.";

    ASSERT_NE(back, nullptr) << "The module must be valid.";

    fixture.generate.module = back;

    EXPECT_EQ(static_cast<std::int32_t>(fixture.MAREA_call("f").value), 4) << "2 + 2.";
}
