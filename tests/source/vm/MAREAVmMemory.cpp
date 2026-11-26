// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREABytecodeHelpers.hpp"
#include "tests/MAREAVmHelpers.hpp"

#include <cstring>
#include <vector>

namespace
{
    int marea_g_allocations{ 0 };
    int marea_g_releases{ 0 };
    std::vector<MAREA_Byte *> marea_g_blocks;

    MAREA_Null MAREA_CALL MAREA_dirtyAllocate(MAREA_IntegerSize size, MAREA_IntegerSize)
    {
        auto *block{ new MAREA_Byte[size] };

        std::memset(block, 0xAB, size);

        ++marea_g_allocations;

        marea_g_blocks.push_back(block);

        return block;
    }

    MAREA_Null MAREA_CALL MAREA_failingAllocate(MAREA_IntegerSize, MAREA_IntegerSize)
    {
        ++marea_g_allocations;

        return nullptr;
    }

    void MAREA_CALL MAREA_countingFree(MAREA_Null address)
    {
        ++marea_g_releases;

        delete[] static_cast<MAREA_Byte *>(address);
    }

    struct MAREA_MemoryFixture : MAREA_BytecodeFixture
    {
        MAREA_VmContext *context{ nullptr };
        MAREA_TypeRegistry *registry{ nullptr };

        MAREA_MemoryFixture()
        {
            marea_g_allocations = 0;
            marea_g_releases = 0;
            marea_g_blocks.clear();

            MAREA_IGNORE(MAREA_VmContextCreate(arena, sink, &context));
            MAREA_IGNORE(MAREA_TypeRegistryCreate(arena, MAREA_LIBRARY_MASK_DEFAULT, &registry));
            MAREA_IGNORE(MAREA_VmContextSetTypeRegistry(context, registry));
        }

        void MAREA_replace(const char *name, MAREA_Null function)
        {
            EXPECT_EQ(MAREA_TypeRegistryReplaceRuntimeFunction(registry, name, std::strlen(name), function), MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) << "The host must replace " << name;
        }

        std::uint32_t MAREA_allocator(MAREA_BytecodeModule *module, std::uint16_t alignment)
        {
            const std::uint32_t FUNCTION{ MAREA_addFunction(module, MAREA_BYTECODE_NONE, MAREA_BYTECODE_NONE, 2, 0, 0) };
            const std::uint32_t INTEGER{ MAREA_addType(module, MAREA_BYTECODE_TYPE_KIND_PRIMITIVE, MAREA_addString(module, "uinteger64"), 8, 8) };

            MAREA_addParameter(module, FUNCTION, INTEGER, 0);
            MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_ALLOCATE, 1, 0, alignment);
            MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_RETURN, 1, 0, 0);

            return FUNCTION;
        }

        std::uint32_t MAREA_releaser(MAREA_BytecodeModule *module)
        {
            const std::uint32_t FUNCTION{ MAREA_addFunction(module, MAREA_BYTECODE_NONE, MAREA_BYTECODE_NONE, 1, 0, 0) };
            const std::uint32_t INTEGER{ MAREA_addType(module, MAREA_BYTECODE_TYPE_KIND_PRIMITIVE, MAREA_addString(module, "uinteger64"), 8, 8) };

            MAREA_addParameter(module, FUNCTION, INTEGER, 0);
            MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_FREE, 0, 0, 0);
            MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_RETURN_VOID, 0, 0, 0);

            return FUNCTION;
        }

        std::uint32_t MAREA_roundTrip(MAREA_BytecodeModule *module)
        {
            const std::uint32_t FUNCTION{ MAREA_addFunction(module, MAREA_BYTECODE_NONE, MAREA_BYTECODE_NONE, 4, 0, 0) };

            MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_LOAD_IMMEDIATE, 0, 16, 0);
            MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_ALLOCATE, 1, 0, 8);
            MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_LOAD, 2, 1, MAREA_BYTECODE_NUMBER_KIND_SINTEGER_64);
            MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_LOAD_IMMEDIATE, 3, 41, 0);
            MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_ADD, 2, 3, MAREA_BYTECODE_NUMBER_KIND_SINTEGER_64);
            MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_STORE, 1, 2, MAREA_BYTECODE_NUMBER_KIND_SINTEGER_64);
            MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_LOAD, 3, 1, MAREA_BYTECODE_NUMBER_KIND_SINTEGER_64);
            MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_FREE, 1, 0, 0);
            MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_RETURN, 3, 0, 0);

            return FUNCTION;
        }

        MAREA_VmExcept MAREA_run(MAREA_BytecodeModule *module, std::uint32_t function, const std::vector<std::uint64_t> &arguments, MAREA_VmResult &result)
        {
            return MAREA_VmCall(context, module, function, (arguments.empty() ? nullptr : reinterpret_cast<const MAREA_Uinteger64 *>(arguments.data())), static_cast<std::uint32_t>(arguments.size()), &result);
        }
    };

    MAREA_Byte *MAREA_asAddress(std::uint64_t bits)
    {
        MAREA_Byte *address{ nullptr };

        std::memcpy(&address, &bits, sizeof(address));

        return address;
    }
} // namespace

TEST(MAREAVmMemory, AllocatesZeroedMemory)
{
    GTEST_COUT << "Validating a new block of 1024 bytes holds only zeros and can be written and freed." << std::endl;

    MAREA_MemoryFixture fixture;
    MAREA_BytecodeModule *module{ fixture.MAREA_newModule() };
    const std::uint32_t ALLOCATE{ fixture.MAREA_allocator(module, 1) };
    const std::uint32_t RELEASE{ fixture.MAREA_releaser(module) };

    MAREA_VmResult result{};

    ASSERT_EQ(fixture.MAREA_run(module, ALLOCATE, { 1024 }, result), MAREA_VM_EXCEPT_SUCCESS) << "The run must succeed.";

    ASSERT_FALSE(result.trapped) << "1024 bytes are there.";

    MAREA_Byte *block{ MAREA_asAddress(result.value) };

    ASSERT_NE(block, nullptr) << "The address must not be null.";

    EXPECT_EQ(std::vector<MAREA_Byte>(block, block + 1024), std::vector<MAREA_Byte>(1024, 0)) << "Every byte is zero.";

    std::memset(block, 0xFF, 1024);

    EXPECT_EQ(fixture.MAREA_run(module, RELEASE, { result.value }, result), MAREA_VM_EXCEPT_SUCCESS) << "The block must be freed.";
}

TEST(MAREAVmMemory, AlignsEveryRequest)
{
    GTEST_COUT << "Validating every address is a multiple of the requested alignment, also for a size below the alignment." << std::endl;

    for (const std::uint16_t ALIGNMENT : { 4, 8, 16, 32, 256 }) {
        MAREA_MemoryFixture fixture;
        MAREA_BytecodeModule *module{ fixture.MAREA_newModule() };
        const std::uint32_t ALLOCATE{ fixture.MAREA_allocator(module, ALIGNMENT) };
        const std::uint32_t RELEASE{ fixture.MAREA_releaser(module) };

        for (const std::uint64_t SIZE : { static_cast<std::uint64_t>(ALIGNMENT / 2), static_cast<std::uint64_t>(ALIGNMENT - 1) }) {
            std::vector<std::uint64_t> blocks;

            for (int iteration{ 0 }; iteration < 128; ++iteration) {
                MAREA_VmResult result{};

                ASSERT_EQ(fixture.MAREA_run(module, ALLOCATE, { SIZE }, result), MAREA_VM_EXCEPT_SUCCESS) << "The run must succeed.";

                ASSERT_NE(result.value, 0U) << "The address must not be null.";

                EXPECT_EQ(result.value % ALIGNMENT, 0U) << "Alignment " << ALIGNMENT << " size " << SIZE;

                blocks.push_back(result.value);
            }

            for (const std::uint64_t BLOCK : blocks) {
                MAREA_VmResult result{};

                EXPECT_EQ(fixture.MAREA_run(module, RELEASE, { BLOCK }, result), MAREA_VM_EXCEPT_SUCCESS) << "Every block must be freed.";
            }
        }
    }
}

TEST(MAREAVmMemory, GivesABlockForZeroBytesAndIgnoresANullFree)
{
    GTEST_COUT << "Validating a request of zero bytes gives a block and a free of the null address does nothing." << std::endl;

    MAREA_MemoryFixture fixture;
    MAREA_BytecodeModule *module{ fixture.MAREA_newModule() };
    const std::uint32_t ALLOCATE{ fixture.MAREA_allocator(module, 8) };
    const std::uint32_t RELEASE{ fixture.MAREA_releaser(module) };

    MAREA_VmResult result{};

    ASSERT_EQ(fixture.MAREA_run(module, ALLOCATE, { 0 }, result), MAREA_VM_EXCEPT_SUCCESS) << "The run must succeed.";

    EXPECT_FALSE(result.trapped) << "Zero bytes are not out of memory.";

    ASSERT_NE(result.value, 0U) << "Zero bytes still give an address.";

    EXPECT_EQ(fixture.MAREA_run(module, RELEASE, { result.value }, result), MAREA_VM_EXCEPT_SUCCESS) << "The block must be freed.";

    EXPECT_EQ(fixture.MAREA_run(module, RELEASE, { 0 }, result), MAREA_VM_EXCEPT_SUCCESS) << "A null free does nothing.";

    EXPECT_FALSE(result.trapped) << "A null free does not trap.";
}

TEST(MAREAVmMemory, HoldsAValueBetweenAStoreAndALoad)
{
    GTEST_COUT << "Validating a program allocates, reads zero, stores, loads and frees in one function." << std::endl;

    MAREA_MemoryFixture fixture;
    MAREA_BytecodeModule *module{ fixture.MAREA_newModule() };
    const std::uint32_t FUNCTION{ fixture.MAREA_roundTrip(module) };

    MAREA_VmResult result{};

    ASSERT_EQ(fixture.MAREA_run(module, FUNCTION, {}, result), MAREA_VM_EXCEPT_SUCCESS) << "The run must succeed.";

    EXPECT_EQ(result.value, 41U) << "Zero plus 41 goes through the block.";
}

TEST(MAREAVmMemory, UsesTheAllocatorTheHostGivesAndFillsItsBlockWithZeros)
{
    GTEST_COUT << "Validating replaced alloc and free hooks are the ones called, and the vm zero-fills what they give." << std::endl;

    MAREA_MemoryFixture fixture;

    fixture.MAREA_replace("alloc", reinterpret_cast<MAREA_Null>(&MAREA_dirtyAllocate));

    fixture.MAREA_replace("free", reinterpret_cast<MAREA_Null>(&MAREA_countingFree));

    MAREA_BytecodeModule *module{ fixture.MAREA_newModule() };
    const std::uint32_t FUNCTION{ fixture.MAREA_roundTrip(module) };

    MAREA_VmResult result{};

    ASSERT_EQ(fixture.MAREA_run(module, FUNCTION, {}, result), MAREA_VM_EXCEPT_SUCCESS) << "The run must succeed.";

    EXPECT_EQ(result.value, 41U) << "The dirty block of the host reads as zero.";

    EXPECT_EQ(marea_g_allocations, 1) << "The host alloc ran once.";

    EXPECT_EQ(marea_g_releases, 1) << "The host free ran once.";
}

TEST(MAREAVmMemory, AFailingAllocatorTrapsOutOfMemoryThroughEveryCaller)
{
    GTEST_COUT << "Validating a null block from the host allocator is the trap OUT_OF_MEMORY and nothing runs after it." << std::endl;

    MAREA_MemoryFixture fixture;

    fixture.MAREA_replace("alloc", reinterpret_cast<MAREA_Null>(&MAREA_failingAllocate));

    fixture.MAREA_replace("free", reinterpret_cast<MAREA_Null>(&MAREA_countingFree));

    MAREA_BytecodeModule *module{ fixture.MAREA_newModule() };
    const std::uint32_t INNER{ fixture.MAREA_roundTrip(module) };
    const std::uint32_t OUTER{ fixture.MAREA_addFunction(module, MAREA_BYTECODE_NONE, MAREA_BYTECODE_NONE, 1, 0, 0) };

    fixture.MAREA_addInstruction(module, OUTER, MAREA_BYTECODE_OPCODE_CALL, 0, static_cast<std::uint16_t>(INNER), 0);
    fixture.MAREA_addInstruction(module, OUTER, MAREA_BYTECODE_OPCODE_RETURN, 0, 0, 0);

    MAREA_VmResult result{};

    ASSERT_EQ(fixture.MAREA_run(module, OUTER, {}, result), MAREA_VM_EXCEPT_SUCCESS) << "An allocation failure is a trap, not an except.";

    ASSERT_TRUE(result.trapped) << "The run must trap.";

    EXPECT_EQ(result.trap, MAREA_BYTECODE_TRAP_CODE_OUT_OF_MEMORY) << "The trap code must say out of memory.";

    EXPECT_EQ(marea_g_releases, 0) << "Nothing runs after the trap.";
}

TEST(MAREAVmMemory, AnImpossibleSizeTrapsOutOfMemory)
{
    GTEST_COUT << "Validating a size that no machine has is the trap OUT_OF_MEMORY with the allocator of the vm." << std::endl;

    MAREA_MemoryFixture fixture;
    MAREA_BytecodeModule *module{ fixture.MAREA_newModule() };
    const std::uint32_t ALLOCATE{ fixture.MAREA_allocator(module, 16) };

    for (const std::uint64_t SIZE : { static_cast<std::uint64_t>(MAREA_UINTEGER_64_MAXIMUM), static_cast<std::uint64_t>(MAREA_UINTEGER_64_MAXIMUM - 16U), static_cast<std::uint64_t>(MAREA_SINTEGER_64_MAXIMUM) }) {
        MAREA_VmResult result{};

        ASSERT_EQ(fixture.MAREA_run(module, ALLOCATE, { SIZE }, result), MAREA_VM_EXCEPT_SUCCESS) << "The run must succeed.";

        ASSERT_TRUE(result.trapped) << "The size " << SIZE << " must trap.";

        EXPECT_EQ(result.trap, MAREA_BYTECODE_TRAP_CODE_OUT_OF_MEMORY) << "The trap code must say out of memory.";
    }
}

TEST(MAREAVmMemory, AnAlignmentThatIsNotAPowerOfTwoIsAMalformedModule)
{
    GTEST_COUT << "Validating an alignment of 0 or 3 is rejected as a malformed module." << std::endl;

    for (const std::uint16_t ALIGNMENT : { 0, 3, 24 }) {
        MAREA_MemoryFixture fixture;
        MAREA_BytecodeModule *module{ fixture.MAREA_newModule() };
        const std::uint32_t ALLOCATE{ fixture.MAREA_allocator(module, ALIGNMENT) };

        MAREA_VmResult result{};

        EXPECT_EQ(fixture.MAREA_run(module, ALLOCATE, { 8 }, result), MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE) << "Alignment " << ALIGNMENT;
    }
}

TEST(MAREAVmMemory, AReplacementOfOneHookOnlyIsReported)
{
    GTEST_COUT << "Validating a host that replaces only alloc or only free gets an except, not a block freed by the wrong allocator." << std::endl;

    for (const char *name : { "alloc", "free" }) {
        MAREA_MemoryFixture fixture;

        fixture.MAREA_replace(name, reinterpret_cast<MAREA_Null>(&MAREA_countingFree));

        MAREA_BytecodeModule *module{ fixture.MAREA_newModule() };
        const std::uint32_t FUNCTION{ fixture.MAREA_roundTrip(module) };

        MAREA_VmResult result{};

        EXPECT_EQ(fixture.MAREA_run(module, FUNCTION, {}, result), MAREA_VM_EXCEPT_ERROR_UNPAIRED_ALLOCATOR) << "Only " << name << " is replaced.";

        EXPECT_EQ(marea_g_releases, 0) << "No hook runs.";
    }
}
