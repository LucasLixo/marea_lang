// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_TESTS_MAREABYTECODEGENERATEHELPERS_HPP)
    #define MAREA_TESTS_MAREABYTECODEGENERATEHELPERS_HPP

    #include "marea/bytecode/MAREABytecodeGenerate.h"

    #include "tests/MAREAIrHelpers.hpp"

    #include <cstdint>
    #include <string>
    #include <utility>
    #include <vector>

struct MAREA_GenerateFixture
{
    MAREA_IrFixture ir;
    MAREA_BytecodeContext *context{ nullptr };
    MAREA_BytecodeModule *module{ nullptr };
    MAREA_BytecodeExcept except{ MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER };

    explicit MAREA_GenerateFixture(MAREA_ResolveCallback resolve = nullptr)
        : ir(std::move(resolve))
    {
        MAREA_IGNORE(MAREA_BytecodeContextCreate(ir.sema.parser.lexer.arena, ir.sema.parser.lexer.sink, &context));
    }

    MAREA_GenerateFixture(const MAREA_GenerateFixture &) = delete;
    MAREA_GenerateFixture &operator=(const MAREA_GenerateFixture &) = delete;

    bool MAREA_generate(const std::string &source, const std::string &filePath = "main.marea")
    {
        if (ir.MAREA_lower(source, filePath) != MAREA_IR_EXCEPT_SUCCESS) {
            return false;
        }

        except = MAREA_BytecodeGenerate(context, ir.module, &module);

        return ((except == MAREA_BYTECODE_EXCEPT_SUCCESS) && (module != nullptr));
    }

    std::vector<MAREA_Uinteger32> MAREA_codes() const
    {
        std::vector<MAREA_Uinteger32> result;

        for (MAREA_Uinteger32 code : ir.sema.parser.MAREA_codes()) {
            if ((code >= 5000) && (code < 6000)) {
                result.push_back(code);
            }
        }

        return result;
    }

    std::string MAREA_functionName(std::uint32_t index) const
    {
        MAREA_BytecodeFunctionInfo info{};

        if (MAREA_BytecodeModuleGetFunction(module, index, &info) != MAREA_BYTECODE_EXCEPT_SUCCESS) {
            return "<no function>";
        }

        const MAREA_Character8 *text{ nullptr };

        MAREA_IntegerSize length{ 0 };

        if (MAREA_BytecodeModuleGetString(module, info.name, &text, &length) != MAREA_BYTECODE_EXCEPT_SUCCESS) {
            return "<no name>";
        }

        return std::string(text, length);
    }

    std::int64_t MAREA_findFunction(const std::string &name) const
    {
        for (std::uint32_t index{ 0 }; index < MAREA_BytecodeModuleGetFunctionCount(module); ++index) {
            if (MAREA_functionName(index) == name) {
                return index;
            }
        }

        return -1;
    }

    std::vector<std::string> MAREA_instructions(std::uint32_t functionIndex) const
    {
        std::vector<std::string> result;

        MAREA_BytecodeFunctionInfo info{};

        if (MAREA_BytecodeModuleGetFunction(module, functionIndex, &info) != MAREA_BYTECODE_EXCEPT_SUCCESS) {
            return result;
        }

        for (std::uint32_t index{ 0 }; index < info.instructionCount; ++index) {
            MAREA_BytecodeInstruction instruction{};

            MAREA_IGNORE(MAREA_BytecodeModuleGetInstruction(module, functionIndex, index, &instruction));

            result.push_back(std::string(MAREA_BytecodeOpcodeGetName(instruction.opcode)) + " " + std::to_string(instruction.a) + " " + std::to_string(instruction.b) + " " + std::to_string(instruction.c));
        }

        return result;
    }

    std::uint32_t MAREA_registerCount(std::uint32_t functionIndex) const
    {
        MAREA_BytecodeFunctionInfo info{};

        MAREA_IGNORE(MAREA_BytecodeModuleGetFunction(module, functionIndex, &info));

        return info.registerCount;
    }

    std::vector<std::uint8_t> MAREA_data(std::uint32_t index) const
    {
        const MAREA_Byte *bytes{ nullptr };

        MAREA_IntegerSize size{ 0 };

        if (MAREA_BytecodeModuleGetData(module, index, &bytes, &size) != MAREA_BYTECODE_EXCEPT_SUCCESS) {
            return {};
        }

        return std::vector<std::uint8_t>(bytes, (bytes + size));
    }
};

#endif
