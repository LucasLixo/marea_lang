// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_TESTS_MAREAVMHELPERS_HPP)
    #define MAREA_TESTS_MAREAVMHELPERS_HPP

    #include "marea/vm/MAREAVm.h"

    #include "tests/MAREABytecodeGenerateHelpers.hpp"

    #include <cstdint>
    #include <string>
    #include <vector>

struct MAREA_VmFixture
{
    MAREA_GenerateFixture generate;
    MAREA_VmContext *context{ nullptr };

    MAREA_VmFixture()
    {
        MAREA_IGNORE(MAREA_VmContextCreate(generate.ir.sema.parser.lexer.arena, generate.ir.sema.parser.lexer.sink, &context));
    }

    MAREA_VmFixture(const MAREA_VmFixture &) = delete;
    MAREA_VmFixture &operator=(const MAREA_VmFixture &) = delete;

    bool MAREA_build(const std::string &source)
    {
        return generate.MAREA_generate("namespace main; " + source);
    }

    MAREA_VmResult MAREA_call(const std::string &name, const std::vector<std::uint64_t> &arguments = {})
    {
        MAREA_VmResult result{};

        const std::string FULL{ "main." + name };

        const MAREA_VmExcept EXCEPT{ MAREA_VmCallByName(context, generate.module, FULL.c_str(), static_cast<MAREA_IntegerSize>(FULL.size()), (arguments.empty() ? nullptr : reinterpret_cast<const MAREA_Uinteger64 *>(arguments.data())), static_cast<std::uint32_t>(arguments.size()), &result) };

        // INFO: a wrong except here (an unknown function, a bad argument count, a malformed module) would
        // otherwise look like a silent zero result to every test that only reads the returned MAREA_VmResult.
        EXPECT_EQ(EXCEPT, MAREA_VM_EXCEPT_SUCCESS) << "The call of " << name << " must not fail as an except.";

        return result;
    }
};

#endif
