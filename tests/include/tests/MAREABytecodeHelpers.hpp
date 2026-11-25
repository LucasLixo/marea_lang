// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_TESTS_MAREABYTECODEHELPERS_HPP)
    #define MAREA_TESTS_MAREABYTECODEHELPERS_HPP

    #include <cstdint>
    #include <string>
    #include <vector>

struct MAREA_BytecodeFixture
{
    MAREA_Arena *arena{ nullptr };
    MAREA_DiagnosticSink *sink{ nullptr };
    MAREA_BytecodeContext *context{ nullptr };

    MAREA_BytecodeFixture()
    {
        MAREA_IGNORE(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(0), &arena));
        MAREA_IGNORE(MAREA_DiagnosticSinkCreate(arena, &sink));
        MAREA_IGNORE(MAREA_BytecodeContextCreate(arena, sink, &context));
    }

    ~MAREA_BytecodeFixture()
    {
        MAREA_ArenaDestroy(arena);
    }

    MAREA_BytecodeFixture(const MAREA_BytecodeFixture &) = delete;
    MAREA_BytecodeFixture &operator=(const MAREA_BytecodeFixture &) = delete;

    MAREA_BytecodeModule *MAREA_newModule()
    {
        MAREA_BytecodeModule *module{ nullptr };

        MAREA_IGNORE(MAREA_BytecodeModuleCreate(context, &module));

        return module;
    }

    std::uint32_t MAREA_addString(MAREA_BytecodeModule *module, const std::string &text)
    {
        std::uint32_t index{ 0 };

        MAREA_IGNORE(MAREA_BytecodeModuleAddString(module, text.data(), text.size(), &index));

        return index;
    }

    std::uint32_t MAREA_addType(MAREA_BytecodeModule *module, MAREA_BytecodeTypeKind kind, std::uint32_t name, std::uint64_t size, std::uint32_t alignment, std::uint32_t element = MAREA_BYTECODE_NONE, std::uint64_t count = 0)
    {
        MAREA_BytecodeTypeInfo info{};

        info.kind = kind;
        info.name = name;
        info.size = size;
        info.alignment = alignment;
        info.element = element;
        info.count = count;

        std::uint32_t index{ 0 };

        MAREA_IGNORE(MAREA_BytecodeModuleAddType(module, &info, &index));

        return index;
    }

    void MAREA_addMember(MAREA_BytecodeModule *module, std::uint32_t typeIndex, std::uint32_t name, std::uint32_t type, std::uint64_t value)
    {
        MAREA_BytecodeMemberInfo member{};

        member.name = name;
        member.type = type;
        member.value = value;

        MAREA_IGNORE(MAREA_BytecodeModuleAddMember(module, typeIndex, &member));
    }

    std::uint32_t MAREA_addFunction(MAREA_BytecodeModule *module, std::uint32_t name, std::uint32_t result, std::uint32_t registerCount, std::uint64_t frameArea, std::uint32_t flags)
    {
        MAREA_BytecodeFunctionInfo info{};

        info.name = name;
        info.result = result;
        info.registerCount = registerCount;
        info.frameArea = frameArea;
        info.flags = flags;

        std::uint32_t index{ 0 };

        MAREA_IGNORE(MAREA_BytecodeModuleAddFunction(module, &info, &index));

        return index;
    }

    void MAREA_addParameter(MAREA_BytecodeModule *module, std::uint32_t functionIndex, std::uint32_t type, std::uint32_t flags)
    {
        MAREA_BytecodeParameterInfo parameter{};

        parameter.type = type;
        parameter.flags = flags;

        MAREA_IGNORE(MAREA_BytecodeModuleAddParameter(module, functionIndex, &parameter));
    }

    void MAREA_addInstruction(MAREA_BytecodeModule *module, std::uint32_t functionIndex, std::uint16_t opcode, std::uint16_t a, std::uint16_t b, std::uint16_t c)
    {
        MAREA_BytecodeInstruction instruction{};

        instruction.opcode = opcode;
        instruction.a = a;
        instruction.b = b;
        instruction.c = c;

        MAREA_IGNORE(MAREA_BytecodeModuleAddInstruction(module, functionIndex, &instruction));
    }

    std::uint32_t MAREA_addData(MAREA_BytecodeModule *module, const std::vector<std::uint8_t> &bytes)
    {
        std::uint32_t index{ 0 };

        MAREA_IGNORE(MAREA_BytecodeModuleAddData(module, bytes.data(), bytes.size(), &index));

        return index;
    }

    MAREA_BytecodeModule *MAREA_richModule()
    {
        MAREA_BytecodeModule *module{ MAREA_newModule() };

        const std::uint32_t POINT{ MAREA_addString(module, "Point") };
        const std::uint32_t X{ MAREA_addString(module, "x") };
        const std::uint32_t Y{ MAREA_addString(module, "y") };
        const std::uint32_t QUOTED{ MAREA_addString(module, std::string("a\"b\\c\n\0d\xff", 9)) };
        const std::uint32_t INTEGER{ MAREA_addType(module, MAREA_BYTECODE_TYPE_KIND_PRIMITIVE, MAREA_addString(module, "sinteger32"), 4, 4) };
        const std::uint32_t STRUCTURE{ MAREA_addType(module, MAREA_BYTECODE_TYPE_KIND_STRUCTURE, POINT, 8, 4) };

        MAREA_addMember(module, STRUCTURE, X, INTEGER, 0);

        MAREA_addMember(module, STRUCTURE, Y, INTEGER, 4);

        MAREA_IGNORE(MAREA_addType(module, MAREA_BYTECODE_TYPE_KIND_ARRAY, MAREA_BYTECODE_NONE, 32, 4, INTEGER, 8));

        const std::uint32_t FUNCTION{ MAREA_addFunction(module, QUOTED, INTEGER, 3, 16, 1) };

        MAREA_addParameter(module, FUNCTION, INTEGER, 0);

        MAREA_addParameter(module, FUNCTION, STRUCTURE, 1);

        MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_NOP, 1, 2, 65535);

        MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_RETURN_VOID, 0, 0, 0);

        MAREA_IGNORE(MAREA_addFunction(module, MAREA_BYTECODE_NONE, MAREA_BYTECODE_NONE, 0, 0, 0));

        MAREA_IGNORE(MAREA_addData(module, { 0x00, 0x7f, 0x80, 0xff }));

        MAREA_IGNORE(MAREA_addData(module, {}));

        MAREA_IGNORE(MAREA_addData(module, { 0x0a }));

        return module;
    }

    MAREA_BytecodeModule *MAREA_randomModule(std::uint64_t seed)
    {
        std::uint64_t state{ (seed * 0x9E3779B97F4A7C15ULL) | 1ULL };

        auto next = [&state](std::uint64_t bound) {
            state ^= (state << 13);
            state ^= (state >> 7);
            state ^= (state << 17);

            return (state % bound);
        };

        MAREA_BytecodeModule *module{ MAREA_newModule() };

        const std::uint32_t STRING_COUNT{ static_cast<std::uint32_t>(next(6)) };

        for (std::uint32_t index{ 0 }; index < STRING_COUNT; ++index) {
            std::string text;

            const std::uint64_t LENGTH{ next(12) };

            for (std::uint64_t position{ 0 }; position < LENGTH; ++position) {
                text.push_back(static_cast<char>(next(256)));
            }

            MAREA_IGNORE(MAREA_addString(module, text));
        }

        const std::uint32_t STRINGS{ static_cast<std::uint32_t>(MAREA_BytecodeModuleGetStringCount(module)) };

        const std::uint32_t TYPE_COUNT{ static_cast<std::uint32_t>(next(5)) };

        auto optionalString = [&]() {
            return ((STRINGS == 0) || (next(3) == 0)) ? static_cast<std::uint32_t>(MAREA_BYTECODE_NONE) : static_cast<std::uint32_t>(next(STRINGS));
        };

        auto optionalType = [&]() {
            return ((TYPE_COUNT == 0) || (next(3) == 0)) ? static_cast<std::uint32_t>(MAREA_BYTECODE_NONE) : static_cast<std::uint32_t>(next(TYPE_COUNT));
        };

        for (std::uint32_t index{ 0 }; index < TYPE_COUNT; ++index) {
            const std::uint32_t TYPE{ MAREA_addType(module, static_cast<MAREA_BytecodeTypeKind>(next(10)), optionalString(), next(1U << 20) * 0x100000001ULL, static_cast<std::uint32_t>(next(17)), optionalType(), next(1U << 30)) };

            const std::uint64_t MEMBERS{ next(4) };

            for (std::uint64_t member{ 0 }; member < MEMBERS; ++member) {
                const std::uint32_t TYPE_REFERENCE{ static_cast<std::uint32_t>(next(TYPE_COUNT)) };

                MAREA_addMember(module, TYPE, optionalString(), TYPE_REFERENCE, next(1U << 30) * 0x1000000001ULL);
            }
        }

        const std::uint32_t FUNCTION_COUNT{ static_cast<std::uint32_t>(next(4)) };

        for (std::uint32_t index{ 0 }; index < FUNCTION_COUNT; ++index) {
            const std::uint32_t FUNCTION{ MAREA_addFunction(module, optionalString(), optionalType(), static_cast<std::uint32_t>(next(1U << 16)), next(1U << 20), static_cast<std::uint32_t>(next(8))) };

            const std::uint64_t PARAMETERS{ (TYPE_COUNT == 0) ? 0 : next(3) };

            for (std::uint64_t parameter{ 0 }; parameter < PARAMETERS; ++parameter) {
                MAREA_addParameter(module, FUNCTION, static_cast<std::uint32_t>(next(TYPE_COUNT)), static_cast<std::uint32_t>(next(2)));
            }

            const std::uint64_t INSTRUCTIONS{ next(5) };

            for (std::uint64_t instruction{ 0 }; instruction < INSTRUCTIONS; ++instruction) {
                MAREA_addInstruction(module, FUNCTION, static_cast<std::uint16_t>(next(MAREA_BYTECODE_OPCODE_COUNT)), static_cast<std::uint16_t>(next(65536)), static_cast<std::uint16_t>(next(65536)), static_cast<std::uint16_t>(next(65536)));
            }
        }

        const std::uint32_t DATA_COUNT{ static_cast<std::uint32_t>(next(4)) };

        for (std::uint32_t index{ 0 }; index < DATA_COUNT; ++index) {
            std::vector<std::uint8_t> bytes;

            const std::uint64_t LENGTH{ next(10) };

            for (std::uint64_t position{ 0 }; position < LENGTH; ++position) {
                bytes.push_back(static_cast<std::uint8_t>(next(256)));
            }

            MAREA_IGNORE(MAREA_addData(module, bytes));
        }

        return module;
    }

    std::vector<std::uint8_t> MAREA_serialize(const MAREA_BytecodeModule *module)
    {
        const MAREA_Byte *bytes{ nullptr };

        MAREA_IntegerSize size{ 0 };

        MAREA_IGNORE(MAREA_BytecodeSerialize(module, &bytes, &size));

        return std::vector<std::uint8_t>(bytes, (bytes + size));
    }

    std::string MAREA_disassemble(const MAREA_BytecodeModule *module)
    {
        const MAREA_Character8 *text{ nullptr };

        MAREA_IntegerSize length{ 0 };

        MAREA_IGNORE(MAREA_BytecodeDisassemble(module, &text, &length));

        return std::string(text, length);
    }

    MAREA_BytecodeModule *MAREA_deserialize(const std::vector<std::uint8_t> &bytes)
    {
        MAREA_BytecodeModule *module{ nullptr };

        MAREA_IGNORE(MAREA_BytecodeDeserialize(context, bytes.data(), bytes.size(), &module));

        return module;
    }

    MAREA_BytecodeModule *MAREA_assemble(const std::string &text)
    {
        MAREA_BytecodeModule *module{ nullptr };

        MAREA_IGNORE(MAREA_BytecodeAssemble(context, text.data(), text.size(), &module));

        return module;
    }

    MAREA_IntegerSize MAREA_errors() const
    {
        return MAREA_DiagnosticSinkGetErrorCount(sink);
    }

    std::uint32_t MAREA_lastCode() const
    {
        const MAREA_IntegerSize COUNT{ MAREA_DiagnosticSinkGetCount(sink) };

        if (COUNT == 0) {
            return 0;
        }

        const MAREA_Diagnostic *diagnostic{ nullptr };

        MAREA_IGNORE(MAREA_DiagnosticSinkGetAt(sink, (COUNT - 1), &diagnostic));

        return MAREA_DiagnosticGetCode(diagnostic);
    }
};

#endif
