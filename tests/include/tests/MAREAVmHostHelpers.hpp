// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_TESTS_MAREAVMHOSTHELPERS_HPP)
    #define MAREA_TESTS_MAREAVMHOSTHELPERS_HPP

    #include "marea/vm/MAREAVm.h"

    #include "tests/MAREAHostSignatureHelpers.hpp"

    #include <cstdint>
    #include <cstring>
    #include <string>
    #include <vector>

struct MAREA_HostPair
{
    std::int32_t x;
    std::int32_t y;
};

inline std::vector<std::int64_t> &MAREA_hostLog()
{
    static std::vector<std::int64_t> log;

    return log;
}

template <typename T>
inline T MAREA_hostRead(const MAREA_Null *arguments, int index)
{
    T value{};

    std::memcpy(&value, arguments[index], sizeof(value));

    return value;
}

template <typename T>
inline void MAREA_hostWrite(MAREA_Null result, const T &value)
{
    std::memcpy(result, &value, sizeof(value));
}

extern "C"
{
    inline void MAREA_CALL MAREA_testHostAdd(const MAREA_Null *arguments, MAREA_Null result)
    {
        MAREA_hostWrite(result, MAREA_hostRead<std::int64_t>(arguments, 0) + MAREA_hostRead<std::int64_t>(arguments, 1));
    }

    inline void MAREA_CALL MAREA_testHostWide(const MAREA_Null *arguments, MAREA_Null result)
    {
        MAREA_hostWrite(result, static_cast<double>(MAREA_hostRead<std::uint64_t>(arguments, 0)) + static_cast<double>(MAREA_hostRead<std::int64_t>(arguments, 1)) + MAREA_hostRead<double>(arguments, 2));
    }

    inline void MAREA_CALL MAREA_testHostIdentity(const MAREA_Null *arguments, MAREA_Null result)
    {
        MAREA_hostWrite(result, MAREA_hostRead<std::int64_t>(arguments, 0));
    }

    inline void MAREA_CALL MAREA_testHostThird(const MAREA_Null *, MAREA_Null result)
    {
        MAREA_hostWrite(result, 1.0 / 3.0);
    }

    inline void MAREA_CALL MAREA_testHostNegate(const MAREA_Null *arguments, MAREA_Null result)
    {
        MAREA_hostWrite(result, static_cast<std::uint64_t>((MAREA_hostRead<std::uint64_t>(arguments, 0) == 0) ? 1 : 0));
    }

    inline void MAREA_CALL MAREA_testHostSum(const MAREA_Null *arguments, MAREA_Null result)
    {
        const MAREA_HostPair LEFT{ MAREA_hostRead<MAREA_HostPair>(arguments, 0) };

        const MAREA_HostPair RIGHT{ MAREA_hostRead<MAREA_HostPair>(arguments, 1) };

        MAREA_hostWrite(result, MAREA_HostPair{ LEFT.x + RIGHT.x, LEFT.y + RIGHT.y });
    }

    inline void MAREA_CALL MAREA_testHostSquare(const MAREA_Null *arguments, MAREA_Null result)
    {
        const MAREA_HostPair VALUE{ MAREA_hostRead<MAREA_HostPair>(arguments, 0) };

        MAREA_hostWrite(result, static_cast<std::int64_t>((VALUE.x * VALUE.x) + (VALUE.y * VALUE.y)));
    }

    inline void MAREA_CALL MAREA_testHostBump(const MAREA_Null *arguments, MAREA_Null result)
    {
        static_cast<MAREA_HostPair *>(arguments[0])->x += 1;

        MAREA_hostLog().push_back((result == nullptr) ? 0 : -1);
    }

    inline void MAREA_CALL MAREA_testHostAddress(const MAREA_Null *arguments, MAREA_Null result)
    {
        MAREA_hostWrite(result, static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(arguments[0])));
    }

    inline void MAREA_CALL MAREA_testHostRecord(const MAREA_Null *arguments, MAREA_Null)
    {
        MAREA_hostLog().push_back(MAREA_hostRead<std::int64_t>(arguments, 0));
    }

    inline void MAREA_CALL MAREA_testHostTwice(const MAREA_Null *arguments, MAREA_Null result)
    {
        MAREA_hostWrite(result, MAREA_hostRead<std::int64_t>(arguments, 0) * 2);
    }
}

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

        MAREA_register("add", &MAREA_testHostAdd);
        MAREA_register("wide", &MAREA_testHostWide);
        MAREA_register("narrow", &MAREA_testHostIdentity);
        MAREA_register("flag", &MAREA_testHostIdentity);
        MAREA_register("third", &MAREA_testHostThird);
        MAREA_register("negate", &MAREA_testHostNegate);
        MAREA_register("sum", &MAREA_testHostSum);
        MAREA_register("square", &MAREA_testHostSquare);
        MAREA_register("bump", &MAREA_testHostBump);
        MAREA_register("address", &MAREA_testHostAddress);
        MAREA_register("record", &MAREA_testHostRecord);
    }

    MAREA_VmHostFixture(const MAREA_VmHostFixture &) = delete;
    MAREA_VmHostFixture &operator=(const MAREA_VmHostFixture &) = delete;

    void MAREA_register(const char *name, MAREA_HostFunction function)
    {
        EXPECT_EQ(MAREA_REGISTER_FUNCTION(registry, MAREA_LIBRARY_FLAG_NONE, "demo.host", 9, name, std::strlen(name), function), MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) << "The host must register " << name;
    }

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
