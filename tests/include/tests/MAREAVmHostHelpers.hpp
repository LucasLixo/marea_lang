// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_TESTS_MAREAVMHOSTHELPERS_HPP)
    #define MAREA_TESTS_MAREAVMHOSTHELPERS_HPP

    #include "marea/vm/MAREAVm.h"

    #include "tests/MAREAHostSignatureHelpers.hpp"

    #include <algorithm>
    #include <cstdint>
    #include <cstring>
    #include <string>
    #include <vector>

struct MAREA_HostPair
{
    std::int32_t x;
    std::int32_t y;
};

struct MAREA_HostWidePair
{
    double first;
    double second;
};

struct MAREA_HostFive
{
    std::int64_t values[5];
};

inline std::vector<std::int64_t> &MAREA_hostLog()
{
    static std::vector<std::int64_t> log;

    return log;
}

extern "C" {
    inline MAREA_Sinteger32 MAREA_testHostAdd(MAREA_Sinteger32 left, MAREA_Sinteger32 right)
    {
        return left + right;
    }

    inline MAREA_Float64 MAREA_testHostWide(MAREA_Uinteger64 first, MAREA_Sinteger8 second, MAREA_Float32 third)
    {
        return static_cast<MAREA_Float64>(first) + static_cast<MAREA_Float64>(second) + static_cast<MAREA_Float64>(third);
    }

    inline MAREA_Sinteger32 MAREA_testHostIdentity(MAREA_Sinteger32 value)
    {
        return value;
    }

    inline MAREA_Float64 MAREA_testHostThird()
    {
        return 1.0 / 3.0;
    }

    inline MAREA_Boolean MAREA_testHostNegate(MAREA_Boolean value)
    {
        return !value;
    }

    inline void MAREA_testHostSum(const MAREA_HostPair *left, const MAREA_HostPair *right, MAREA_HostPair *outSum)
    {
        *outSum = MAREA_HostPair{ left->x + right->x, left->y + right->y };
    }

    inline MAREA_Sinteger32 MAREA_testHostSquare(const MAREA_HostPair *value)
    {
        return (value->x * value->x) + (value->y * value->y);
    }

    inline void MAREA_testHostSwap(const MAREA_HostWidePair *value, MAREA_HostWidePair *outSwapped)
    {
        *outSwapped = MAREA_HostWidePair{ value->second, value->first };
    }

    inline void MAREA_testHostReverse(const MAREA_HostFive *value, MAREA_HostFive *outReversed)
    {
        std::reverse_copy(value->values, value->values + 5, outReversed->values);
    }

    inline void MAREA_testHostBump(MAREA_HostPair *value)
    {
        value->x += 1;

        MAREA_hostLog().push_back(0);
    }

    inline void MAREA_testHostShift(MAREA_HostPair *value, MAREA_Sinteger32 by)
    {
        value->x += by;
        value->y -= by;
    }

    inline MAREA_Uinteger64 MAREA_testHostAddress(const MAREA_HostPair *value)
    {
        return static_cast<MAREA_Uinteger64>(reinterpret_cast<std::uintptr_t>(value));
    }

    inline void MAREA_testHostRecord(MAREA_Sinteger32 value)
    {
        MAREA_hostLog().push_back(value);
    }

    inline MAREA_Sinteger32 MAREA_testHostTwice(MAREA_Sinteger32 value)
    {
        return value * 2;
    }

    inline MAREA_Boolean MAREA_testHostBlock(MAREA_HostContext *context, MAREA_Uinteger64 size)
    {
        MAREA_Null block{ nullptr };

        if (MAREA_HostContextAllocate(context, static_cast<MAREA_IntegerSize>(size), 16, &block) != MAREA_HOST_CONTEXT_EXCEPT_SUCCESS) {
            MAREA_HostContextRaiseTrap(context, MAREA_HOST_TRAP_OUT_OF_MEMORY);

            return MAREA_BOOLEAN_FALSE;
        }

        auto *bytes{ static_cast<MAREA_Byte *>(block) };

        const bool ZEROED{ std::all_of(bytes, bytes + size, [](MAREA_Byte byte) { return byte == 0; }) };

        std::fill(bytes, bytes + size, static_cast<MAREA_Byte>(0xFF));

        MAREA_HostContextFree(context, block);

        return ZEROED;
    }

    #define MAREA_TEST_PARAMETER(index) MAREA_Sinteger32 p##index

    inline MAREA_Sinteger64 MAREA_testHostThirtyTwo(MAREA_TEST_PARAMETER(1), MAREA_TEST_PARAMETER(2), MAREA_TEST_PARAMETER(3), MAREA_TEST_PARAMETER(4), MAREA_TEST_PARAMETER(5), MAREA_TEST_PARAMETER(6), MAREA_TEST_PARAMETER(7), MAREA_TEST_PARAMETER(8), MAREA_TEST_PARAMETER(9), MAREA_TEST_PARAMETER(10), MAREA_TEST_PARAMETER(11), MAREA_TEST_PARAMETER(12), MAREA_TEST_PARAMETER(13), MAREA_TEST_PARAMETER(14), MAREA_TEST_PARAMETER(15), MAREA_TEST_PARAMETER(16), MAREA_TEST_PARAMETER(17), MAREA_TEST_PARAMETER(18), MAREA_TEST_PARAMETER(19), MAREA_TEST_PARAMETER(20), MAREA_TEST_PARAMETER(21), MAREA_TEST_PARAMETER(22), MAREA_TEST_PARAMETER(23), MAREA_TEST_PARAMETER(24), MAREA_TEST_PARAMETER(25), MAREA_TEST_PARAMETER(26), MAREA_TEST_PARAMETER(27), MAREA_TEST_PARAMETER(28), MAREA_TEST_PARAMETER(29), MAREA_TEST_PARAMETER(30), MAREA_TEST_PARAMETER(31), MAREA_TEST_PARAMETER(32))
    {
        const MAREA_Sinteger32 VALUES[32]{ p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32 };

        MAREA_Sinteger64 weighted{ 0 };

        for (int index{ 0 }; index < 32; ++index) {
            weighted += static_cast<MAREA_Sinteger64>(index + 1) * VALUES[index];
        }

        return weighted;
    }

    #undef MAREA_TEST_PARAMETER

    inline void MAREA_CALL MAREA_testHostRaw(MAREA_HostContext *context, const MAREA_Null *arguments, MAREA_Null result)
    {
        std::int64_t value{ 0 };

        std::memcpy(&value, arguments[0], sizeof(value));

        MAREA_hostLog().push_back((context != nullptr) ? 1 : 0);
        MAREA_hostLog().push_back(value);
        MAREA_hostLog().push_back((result == nullptr) ? 0 : -1);
    }
}

MAREA_HOST_FUNCTION(MAREA_testHostAdd, MAREA_HOST_RETURNS(sinteger32), MAREA_HOST_VALUE(sinteger32), MAREA_HOST_VALUE(sinteger32));
MAREA_HOST_FUNCTION(MAREA_testHostWide, MAREA_HOST_RETURNS(float64), MAREA_HOST_VALUE(uinteger64), MAREA_HOST_VALUE(sinteger8), MAREA_HOST_VALUE(float32));
MAREA_HOST_FUNCTION(MAREA_testHostIdentity, MAREA_HOST_RETURNS(sinteger32), MAREA_HOST_VALUE(sinteger32));
MAREA_HOST_FUNCTION(MAREA_testHostThird, MAREA_HOST_RETURNS(float64));
MAREA_HOST_FUNCTION(MAREA_testHostNegate, MAREA_HOST_RETURNS(boolean), MAREA_HOST_VALUE(boolean));
MAREA_HOST_FUNCTION(MAREA_testHostSum, MAREA_HOST_RETURNS_STRUCTURE(MAREA_HostPair), MAREA_HOST_BORROW(MAREA_HostPair), MAREA_HOST_BORROW(MAREA_HostPair));
MAREA_HOST_FUNCTION(MAREA_testHostSquare, MAREA_HOST_RETURNS(sinteger32), MAREA_HOST_BORROW(MAREA_HostPair));
MAREA_HOST_FUNCTION(MAREA_testHostSwap, MAREA_HOST_RETURNS_STRUCTURE(MAREA_HostWidePair), MAREA_HOST_BORROW(MAREA_HostWidePair));
MAREA_HOST_FUNCTION(MAREA_testHostReverse, MAREA_HOST_RETURNS_STRUCTURE(MAREA_HostFive), MAREA_HOST_BORROW(MAREA_HostFive));
MAREA_HOST_FUNCTION(MAREA_testHostBump, MAREA_HOST_RETURNS_NOTHING, MAREA_HOST_BORROW_MUTABLE(MAREA_HostPair));
MAREA_HOST_FUNCTION(MAREA_testHostShift, MAREA_HOST_RETURNS_NOTHING, MAREA_HOST_BORROW_MUTABLE(MAREA_HostPair), MAREA_HOST_VALUE(sinteger32));
MAREA_HOST_FUNCTION(MAREA_testHostAddress, MAREA_HOST_RETURNS(uinteger64), MAREA_HOST_BORROW(MAREA_HostPair));
MAREA_HOST_FUNCTION(MAREA_testHostRecord, MAREA_HOST_RETURNS_NOTHING, MAREA_HOST_VALUE(sinteger32));
MAREA_HOST_FUNCTION(MAREA_testHostTwice, MAREA_HOST_RETURNS(sinteger32), MAREA_HOST_VALUE(sinteger32));
MAREA_HOST_FUNCTION(MAREA_testHostBlock, MAREA_HOST_RETURNS(boolean), MAREA_HOST_CONTEXT, MAREA_HOST_VALUE(uinteger64));

    #define MAREA_TEST_VALUE MAREA_HOST_VALUE(sinteger32)

MAREA_HOST_FUNCTION(MAREA_testHostThirtyTwo, MAREA_HOST_RETURNS(sinteger64), MAREA_TEST_VALUE, MAREA_TEST_VALUE, MAREA_TEST_VALUE, MAREA_TEST_VALUE, MAREA_TEST_VALUE, MAREA_TEST_VALUE, MAREA_TEST_VALUE, MAREA_TEST_VALUE, MAREA_TEST_VALUE, MAREA_TEST_VALUE, MAREA_TEST_VALUE, MAREA_TEST_VALUE, MAREA_TEST_VALUE, MAREA_TEST_VALUE, MAREA_TEST_VALUE, MAREA_TEST_VALUE, MAREA_TEST_VALUE, MAREA_TEST_VALUE, MAREA_TEST_VALUE, MAREA_TEST_VALUE, MAREA_TEST_VALUE, MAREA_TEST_VALUE, MAREA_TEST_VALUE, MAREA_TEST_VALUE, MAREA_TEST_VALUE, MAREA_TEST_VALUE, MAREA_TEST_VALUE, MAREA_TEST_VALUE, MAREA_TEST_VALUE, MAREA_TEST_VALUE, MAREA_TEST_VALUE, MAREA_TEST_VALUE);

    #undef MAREA_TEST_VALUE

    #define MAREA_TEST_REGISTER(registry, name, function) EXPECT_EQ(MAREA_REGISTER_FUNCTION(registry, MAREA_LIBRARY_FLAG_NONE, "demo.host", 9, name, std::strlen(name), function), MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) << "The host must register " << name

struct MAREA_VmHostFixture
{
    MAREA_HostGenerateFixture generate;
    MAREA_TypeRegistry *registry{ nullptr };
    MAREA_VmContext *context{ nullptr };

    MAREA_VmHostFixture()
    {
        MAREA_Arena *arena{ generate.ir.sema.parser.lexer.arena };

        MAREA_hostLog().clear();

        MAREA_IGNORE(MAREA_TypeRegistryCreate(arena, MAREA_LIBRARY_MASK_DEFAULT, &registry));
        MAREA_IGNORE(MAREA_VmContextCreate(arena, generate.ir.sema.parser.lexer.sink, &context));
        MAREA_IGNORE(MAREA_VmContextSetTypeRegistry(context, registry));

        MAREA_TEST_REGISTER(registry, "add", MAREA_testHostAdd);
        MAREA_TEST_REGISTER(registry, "wide", MAREA_testHostWide);
        MAREA_TEST_REGISTER(registry, "narrow", MAREA_testHostIdentity);
        MAREA_TEST_REGISTER(registry, "flag", MAREA_testHostIdentity);
        MAREA_TEST_REGISTER(registry, "third", MAREA_testHostThird);
        MAREA_TEST_REGISTER(registry, "negate", MAREA_testHostNegate);
        MAREA_TEST_REGISTER(registry, "sum", MAREA_testHostSum);
        MAREA_TEST_REGISTER(registry, "square", MAREA_testHostSquare);
        MAREA_TEST_REGISTER(registry, "bump", MAREA_testHostBump);
        MAREA_TEST_REGISTER(registry, "shift", MAREA_testHostShift);
        MAREA_TEST_REGISTER(registry, "address", MAREA_testHostAddress);
        MAREA_TEST_REGISTER(registry, "record", MAREA_testHostRecord);
        MAREA_TEST_REGISTER(registry, "swap", MAREA_testHostSwap);
        MAREA_TEST_REGISTER(registry, "reverse", MAREA_testHostReverse);
        MAREA_TEST_REGISTER(registry, "block", MAREA_testHostBlock);
        MAREA_TEST_REGISTER(registry, "thirtytwo", MAREA_testHostThirtyTwo);
    }

    MAREA_VmHostFixture(const MAREA_VmHostFixture &) = delete;
    MAREA_VmHostFixture &operator=(const MAREA_VmHostFixture &) = delete;

    bool MAREA_build(const std::string &body)
    {
        return generate.MAREA_build(body);
    }

    MAREA_VmResult MAREA_call(const std::string &name, const std::vector<std::uint64_t> &arguments = {})
    {
        MAREA_VmResult result{};

        const std::string FULL{ "main." + name };

        const MAREA_VmExcept EXCEPT{ MAREA_VmCallByName(context, generate.module, FULL.c_str(), FULL.size(), (arguments.empty() ? nullptr : reinterpret_cast<const MAREA_Uinteger64 *>(arguments.data())), static_cast<std::uint32_t>(arguments.size()), &result) };

        EXPECT_EQ(EXCEPT, MAREA_VM_EXCEPT_SUCCESS) << "The call of " << name << " must not fail as an except.";

        return result;
    }
};

#endif
