// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREABytecodeHelpers.hpp"

namespace
{
    const char *const EMPTY_TEXT = ".marea-bytecode 0.1\n.strings\n.types\n.functions\n.data\n";

    const char *const SAMPLE_TEXT =
        ".marea-bytecode 0.1\n"
        ".strings\n"
        "s0 \"Point\"\n"
        "s1 \"x\"\n"
        "s2 \"q\\\"\\\\\\x0a\\xff\"\n"
        ".types\n"
        "t0 primitive name none size 4 align 4 element none count 0\n"
        "t1 structure name s0 size 4 align 4 element none count 0\n"
        "  member name s1 type t0 value 0\n"
        "t2 array name none size 16 align 4 element t0 count 4\n"
        ".functions\n"
        "f0 name s2 result t0 registers 3 area 16 flags 1\n"
        "  param type t1 flags 1\n"
        "  ins nop 1 2 3\n"
        "  ins return_void 0 0 0\n"
        "f1 name none result none registers 0 area 0 flags 0\n"
        ".data\n"
        "d0 00ff10\n"
        "d1 -\n";

    MAREA_BytecodeModule *MAREA_sampleModule(MAREA_BytecodeFixture &fixture)
    {
        MAREA_BytecodeModule *module{ fixture.MAREA_newModule() };

        const std::uint32_t POINT{ fixture.MAREA_addString(module, "Point") };

        const std::uint32_t X{ fixture.MAREA_addString(module, "x") };

        const std::uint32_t QUOTED{ fixture.MAREA_addString(module, std::string("q\"\\\n\xff", 5)) };

        const std::uint32_t INTEGER{ fixture.MAREA_addType(module, MAREA_BYTECODE_TYPE_KIND_PRIMITIVE, MAREA_BYTECODE_NONE, 4, 4) };

        const std::uint32_t STRUCTURE{ fixture.MAREA_addType(module, MAREA_BYTECODE_TYPE_KIND_STRUCTURE, POINT, 4, 4) };

        fixture.MAREA_addMember(module, STRUCTURE, X, INTEGER, 0);

        MAREA_IGNORE(fixture.MAREA_addType(module, MAREA_BYTECODE_TYPE_KIND_ARRAY, MAREA_BYTECODE_NONE, 16, 4, INTEGER, 4));

        const std::uint32_t FUNCTION{ fixture.MAREA_addFunction(module, QUOTED, INTEGER, 3, 16, 1) };

        fixture.MAREA_addParameter(module, FUNCTION, STRUCTURE, 1);

        fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_NOP, 1, 2, 3);

        fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_RETURN_VOID, 0, 0, 0);

        MAREA_IGNORE(fixture.MAREA_addFunction(module, MAREA_BYTECODE_NONE, MAREA_BYTECODE_NONE, 0, 0, 0));

        MAREA_IGNORE(fixture.MAREA_addData(module, { 0x00, 0xff, 0x10 }));

        MAREA_IGNORE(fixture.MAREA_addData(module, {}));

        return module;
    }

    std::string MAREA_replace(std::string text, const std::string &from, const std::string &to)
    {
        const std::size_t POSITION{ text.find(from) };

        if (POSITION != std::string::npos) {
            text.replace(POSITION, from.size(), to);
        }

        return text;
    }
} // namespace

TEST(MAREABytecodeText, DisassembleRejectsNullArguments)
{
    GTEST_COUT << "Validating MAREA_BytecodeDisassemble rejects null arguments." << std::endl;

    MAREA_BytecodeFixture fixture;

    MAREA_BytecodeModule *module{ fixture.MAREA_newModule() };

    const MAREA_Character8 *text{ nullptr };

    MAREA_IntegerSize length{ 0 };

    EXPECT_EQ(MAREA_BytecodeDisassemble(nullptr, &text, &length), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null module must be rejected.";

    EXPECT_EQ(MAREA_BytecodeDisassemble(module, nullptr, &length), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null text pointer must be rejected.";

    EXPECT_EQ(MAREA_BytecodeDisassemble(module, &text, nullptr), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null length pointer must be rejected.";
}

TEST(MAREABytecodeText, AssembleRejectsNullArguments)
{
    GTEST_COUT << "Validating MAREA_BytecodeAssemble rejects null arguments." << std::endl;

    MAREA_BytecodeFixture fixture;

    MAREA_BytecodeModule *module{ nullptr };

    EXPECT_EQ(MAREA_BytecodeAssemble(nullptr, EMPTY_TEXT, 5, &module), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null context must be rejected.";

    EXPECT_EQ(MAREA_BytecodeAssemble(fixture.context, nullptr, 5, &module), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null text with a length must be rejected.";

    EXPECT_EQ(MAREA_BytecodeAssemble(fixture.context, EMPTY_TEXT, 5, nullptr), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null out pointer must be rejected.";

    EXPECT_EQ(MAREA_BytecodeAssemble(fixture.context, nullptr, 0, &module), MAREA_BYTECODE_EXCEPT_SUCCESS) << "A null text with no length is an empty input, which is a diagnostic.";

    EXPECT_EQ(module, nullptr) << "An empty input gives no module.";

    EXPECT_EQ(fixture.MAREA_lastCode(), MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_SYNTAX) << "An empty input has no header line.";
}

TEST(MAREABytecodeText, EmptyModuleHasTheExactText)
{
    GTEST_COUT << "Validating the exact text of the empty module, with a NUL after the last character." << std::endl;

    MAREA_BytecodeFixture fixture;

    const MAREA_Character8 *text{ nullptr };

    MAREA_IntegerSize length{ 0 };

    ASSERT_EQ(MAREA_BytecodeDisassemble(fixture.MAREA_newModule(), &text, &length), MAREA_BYTECODE_EXCEPT_SUCCESS) << "The empty module must be disassembled.";

    EXPECT_EQ(std::string(text, length), std::string(EMPTY_TEXT)) << "The empty module must give the header and the four section lines.";

    EXPECT_EQ(text[length], '\0') << "A NUL must follow the last character.";
}

TEST(MAREABytecodeText, SampleModuleHasTheExactText)
{
    GTEST_COUT << "Validating the exact text of a module with every section, escapes and an empty blob." << std::endl;

    MAREA_BytecodeFixture fixture;

    EXPECT_EQ(fixture.MAREA_disassemble(MAREA_sampleModule(fixture)), std::string(SAMPLE_TEXT)) << "The text must follow the canonical form.";
}

TEST(MAREABytecodeText, DisassembleIsAscii)
{
    GTEST_COUT << "Validating the text of a module with any byte is ASCII." << std::endl;

    MAREA_BytecodeFixture fixture;

    const std::string TEXT{ fixture.MAREA_disassemble(fixture.MAREA_richModule()) };

    for (const char CHARACTER : TEXT) {
        const unsigned char VALUE{ static_cast<unsigned char>(CHARACTER) };

        EXPECT_TRUE(((VALUE == '\n') || ((VALUE >= 0x20) && (VALUE < 0x7F)))) << "Every character must be printable ASCII or a line feed.";
    }
}

TEST(MAREABytecodeText, AssembleReadsTheSampleText)
{
    GTEST_COUT << "Validating Assemble reads the canonical text of the sample module." << std::endl;

    MAREA_BytecodeFixture fixture;

    MAREA_BytecodeModule *module{ fixture.MAREA_assemble(SAMPLE_TEXT) };

    ASSERT_NE(module, nullptr) << "The sample text must be assembled.";

    EXPECT_TRUE(MAREA_BytecodeModuleEquals(module, MAREA_sampleModule(fixture))) << "The assembled module must equal the module built by hand.";

    EXPECT_EQ(fixture.MAREA_errors(), 0U) << "A valid input gives no diagnostic.";
}

TEST(MAREABytecodeText, AssembleDisassembleRoundTrips)
{
    GTEST_COUT << "Validating Assemble(Disassemble(m)) equals m for the empty, the sample and the rich module." << std::endl;

    MAREA_BytecodeFixture fixture;

    MAREA_BytecodeModule *modules[3]{ fixture.MAREA_newModule(), MAREA_sampleModule(fixture), fixture.MAREA_richModule() };

    for (MAREA_BytecodeModule *module : modules) {
        MAREA_BytecodeModule *back{ fixture.MAREA_assemble(fixture.MAREA_disassemble(module)) };

        ASSERT_NE(back, nullptr) << "The text must be assembled.";

        EXPECT_TRUE(MAREA_BytecodeModuleEquals(module, back)) << "The module must equal its text round trip.";
    }

    EXPECT_EQ(fixture.MAREA_errors(), 0U) << "A valid input gives no diagnostic.";
}

TEST(MAREABytecodeText, RandomModulesRoundTripThroughTextAndBinary)
{
    GTEST_COUT << "Validating the text law and the text to binary to text law over 200 generated modules." << std::endl;

    for (std::uint64_t seed{ 1 }; seed <= 200; ++seed) {
        MAREA_BytecodeFixture fixture;

        MAREA_BytecodeModule *module{ fixture.MAREA_randomModule(seed) };

        const std::string TEXT{ fixture.MAREA_disassemble(module) };

        MAREA_BytecodeModule *fromText{ fixture.MAREA_assemble(TEXT) };

        ASSERT_NE(fromText, nullptr) << "The text of seed " << seed << " must be assembled.";

        EXPECT_TRUE(MAREA_BytecodeModuleEquals(module, fromText)) << "The module of seed " << seed << " must equal its text round trip.";

        EXPECT_EQ(fixture.MAREA_serialize(fromText), fixture.MAREA_serialize(module)) << "Serialize(Assemble(Disassemble(m))) must equal Serialize(m) for seed " << seed << ".";

        MAREA_BytecodeModule *fromBinary{ fixture.MAREA_deserialize(fixture.MAREA_serialize(fromText)) };

        ASSERT_NE(fromBinary, nullptr) << "The binary of seed " << seed << " must be read.";

        EXPECT_EQ(fixture.MAREA_disassemble(fromBinary), TEXT) << "The text to binary to text path must be stable for seed " << seed << ".";
    }
}

TEST(MAREABytecodeText, DuplicateStringsAreKept)
{
    GTEST_COUT << "Validating a pool with two equal strings is read as it is written." << std::endl;

    MAREA_BytecodeFixture fixture;

    MAREA_BytecodeModule *module{ fixture.MAREA_assemble(".marea-bytecode 0.1\n.strings\ns0 \"a\"\ns1 \"a\"\n.types\n.functions\n.data\n") };

    ASSERT_NE(module, nullptr) << "A pool with equal strings must be assembled.";

    EXPECT_EQ(MAREA_BytecodeModuleGetStringCount(module), 2U) << "The two strings must be kept.";

    EXPECT_EQ(fixture.MAREA_disassemble(module), std::string(".marea-bytecode 0.1\n.strings\ns0 \"a\"\ns1 \"a\"\n.types\n.functions\n.data\n")) << "The two strings must be written again.";
}

TEST(MAREABytecodeText, CommentsBlankLinesAndIndentAreIgnored)
{
    GTEST_COUT << "Validating comments, blank lines, any indent, CRLF line ends and a missing last line feed." << std::endl;

    MAREA_BytecodeFixture fixture;

    const std::string TEXT{ "; first comment\n\n   .marea-bytecode   0.1  ; version\r\n.strings ; none\ns0   \"a;b\"   ; the semicolon is inside\n\n.types\n\t.functions\n.data\n  d0   0A ; upper case hex\n; last comment" };

    MAREA_BytecodeModule *module{ fixture.MAREA_assemble(TEXT) };

    ASSERT_NE(module, nullptr) << "The text with comments must be assembled.";

    EXPECT_EQ(fixture.MAREA_errors(), 0U) << "No diagnostic is expected.";

    const MAREA_Character8 *string{ nullptr };

    MAREA_IntegerSize length{ 0 };

    ASSERT_EQ(MAREA_BytecodeModuleGetString(module, 0, &string, &length), MAREA_BYTECODE_EXCEPT_SUCCESS) << "The string must exist.";

    EXPECT_EQ(std::string(string, length), "a;b") << "A semicolon inside a string is not a comment.";

    const MAREA_Byte *bytes{ nullptr };

    MAREA_IntegerSize size{ 0 };

    ASSERT_EQ(MAREA_BytecodeModuleGetData(module, 0, &bytes, &size), MAREA_BYTECODE_EXCEPT_SUCCESS) << "The blob must exist.";

    ASSERT_EQ(size, 1U) << "The blob has one byte.";

    EXPECT_EQ(bytes[0], 0x0AU) << "Upper case hex digits are accepted.";

    EXPECT_EQ(fixture.MAREA_disassemble(module), std::string(".marea-bytecode 0.1\n.strings\ns0 \"a;b\"\n.types\n.functions\n.data\nd0 0a\n")) << "The canonical form has no comment and lower case hex.";
}

TEST(MAREABytecodeText, EveryKindNameRoundTrips)
{
    GTEST_COUT << "Validating every type kind has its own text name." << std::endl;

    MAREA_BytecodeFixture fixture;

    MAREA_BytecodeModule *module{ fixture.MAREA_newModule() };

    for (int kind{ 0 }; kind <= 9; ++kind) {
        MAREA_IGNORE(fixture.MAREA_addType(module, static_cast<MAREA_BytecodeTypeKind>(kind), MAREA_BYTECODE_NONE, 1, 1));
    }

    const std::string TEXT{ fixture.MAREA_disassemble(module) };

    const char *const NAMES[10]{ "primitive", "structure", "enum", "array", "vector", "pointer", "owned", "slice", "task", "function" };

    for (int kind{ 0 }; kind <= 9; ++kind) {
        EXPECT_NE(TEXT.find(std::string("t") + std::to_string(kind) + " " + NAMES[kind] + " "), std::string::npos) << "The kind " << NAMES[kind] << " must be written.";
    }

    MAREA_BytecodeModule *back{ fixture.MAREA_assemble(TEXT) };

    ASSERT_NE(back, nullptr) << "The text must be assembled.";

    EXPECT_TRUE(MAREA_BytecodeModuleEquals(module, back)) << "Every kind must round trip.";
}

namespace
{
    struct MAREA_BadTextCase
    {
        std::string name;
        std::string text;
        MAREA_BytecodeDiagnosticCode code;
        std::uint32_t line;
    };
} // namespace

TEST(MAREABytecodeText, BadTextGivesADiagnosticWithItsLine)
{
    GTEST_COUT << "Validating each kind of bad text gives its code, a line and no module." << std::endl;

    const std::string GOOD{ SAMPLE_TEXT };

    const std::vector<MAREA_BadTextCase> CASES{
        { "empty input", "", MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_SYNTAX, 0 },
        { "only comments", "; nothing\n", MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_SYNTAX, 0 },
        { "no header", ".strings\n.types\n.functions\n.data\n", MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_SYNTAX, 1 },
        { "header with no version", ".marea-bytecode\n.strings\n.types\n.functions\n.data\n", MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_SYNTAX, 1 },
        { "major version", ".marea-bytecode 1.1\n.strings\n.types\n.functions\n.data\n", MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_VERSION, 1 },
        { "minor version", ".marea-bytecode 0.2\n.strings\n.types\n.functions\n.data\n", MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_VERSION, 1 },
        { "version number", ".marea-bytecode a.1\n.strings\n.types\n.functions\n.data\n", MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_BAD_NUMBER, 1 },
        { "unknown section", ".marea-bytecode 0.1\n.strings\n.types\n.functions\n.data\n.extra\n", MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_BAD_SECTION, 6 },
        { "missing section", ".marea-bytecode 0.1\n.strings\n.types\n.functions\n", MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_BAD_SECTION, 0 },
        { "sections out of order", ".marea-bytecode 0.1\n.types\n.strings\n.functions\n.data\n", MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_BAD_SECTION, 2 },
        { "repeated section", ".marea-bytecode 0.1\n.strings\n.strings\n.types\n.functions\n.data\n", MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_BAD_SECTION, 3 },
        { "line before a section", ".marea-bytecode 0.1\ns0 \"a\"\n.strings\n.types\n.functions\n.data\n", MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_BAD_SECTION, 2 },
        { "string index skipped", MAREA_replace(GOOD, "s1 \"x\"", "s5 \"x\""), MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_BAD_INDEX, 4 },
        { "type index skipped", MAREA_replace(GOOD, "t1 structure", "t7 structure"), MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_BAD_INDEX, 8 },
        { "function index skipped", MAREA_replace(GOOD, "f1 name", "f4 name"), MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_BAD_INDEX, 16 },
        { "data index skipped", MAREA_replace(GOOD, "d1 -", "d2 -"), MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_BAD_INDEX, 19 },
        { "wrong index letter", MAREA_replace(GOOD, "t2 array", "x2 array"), MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_BAD_INDEX, 10 },
        { "number too large", MAREA_replace(GOOD, "size 16", "size 18446744073709551616"), MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_BAD_NUMBER, 10 },
        { "number not a number", MAREA_replace(GOOD, "align 4 element t0 count 4", "align 4x element t0 count 4"), MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_BAD_NUMBER, 10 },
        { "32-bit number too large", MAREA_replace(GOOD, "registers 3", "registers 4294967296"), MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_BAD_NUMBER, 12 },
        { "16-bit operand too large", MAREA_replace(GOOD, "ins nop 1 2 3", "ins nop 1 2 65536"), MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_BAD_NUMBER, 14 },
        { "unterminated string", MAREA_replace(GOOD, "s1 \"x\"", "s1 \"x"), MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_BAD_STRING, 4 },
        { "unknown escape", MAREA_replace(GOOD, "s1 \"x\"", "s1 \"\\q\""), MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_BAD_STRING, 4 },
        { "short hex escape", MAREA_replace(GOOD, "s1 \"x\"", "s1 \"\\x1\""), MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_BAD_STRING, 4 },
        { "text after a string", MAREA_replace(GOOD, "s1 \"x\"", "s1 \"x\" junk"), MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_SYNTAX, 4 },
        { "no string quote", MAREA_replace(GOOD, "s1 \"x\"", "s1 x"), MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_BAD_STRING, 4 },
        { "unknown kind", MAREA_replace(GOOD, "t0 primitive", "t0 matrix"), MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_SYNTAX, 7 },
        { "missing key", MAREA_replace(GOOD, "t0 primitive name none", "t0 primitive none"), MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_SYNTAX, 7 },
        { "missing field", MAREA_replace(GOOD, " count 0\n  member", "\n  member"), MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_SYNTAX, 8 },
        { "extra word", MAREA_replace(GOOD, "count 4\n", "count 4 extra\n"), MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_SYNTAX, 10 },
        { "member before a type", ".marea-bytecode 0.1\n.strings\n.types\n  member name none type t0 value 0\n.functions\n.data\n", MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_SYNTAX, 4 },
        { "param before a function", ".marea-bytecode 0.1\n.strings\n.types\n.functions\n  param type t0 flags 0\n.data\n", MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_SYNTAX, 5 },
        { "member in the functions section", MAREA_replace(GOOD, "  param type t1 flags 1", "  member name none type t0 value 0"), MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_SYNTAX, 13 },
        { "unknown opcode", MAREA_replace(GOOD, "ins nop 1 2 3", "ins bogus 1 2 3"), MAREA_BYTECODE_DIAGNOSTIC_CODE_UNKNOWN_OPCODE, 14 },
        { "odd hex length", MAREA_replace(GOOD, "d0 00ff10", "d0 00ff1"), MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_SYNTAX, 18 },
        { "bad hex digit", MAREA_replace(GOOD, "d0 00ff10", "d0 00fg10"), MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_SYNTAX, 18 },
        { "string reference past the pool", MAREA_replace(GOOD, "name s0 size 4", "name s9 size 4"), MAREA_BYTECODE_DIAGNOSTIC_CODE_BAD_REFERENCE, 0 },
        { "type reference past the table", MAREA_replace(GOOD, "element t0 count 4", "element t8 count 4"), MAREA_BYTECODE_DIAGNOSTIC_CODE_BAD_REFERENCE, 0 },
        { "control character", MAREA_replace(GOOD, "s1 \"x\"", std::string("s1 \"\x01\"")), MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_BAD_STRING, 4 },
    };

    for (const MAREA_BadTextCase &CASE : CASES) {
        MAREA_BytecodeFixture fixture;

        EXPECT_EQ(fixture.MAREA_assemble(CASE.text), nullptr) << CASE.name << " must give no module.";

        EXPECT_GT(fixture.MAREA_errors(), 0U) << CASE.name << " must give a diagnostic.";

        EXPECT_EQ(fixture.MAREA_lastCode(), static_cast<std::uint32_t>(CASE.code)) << CASE.name << " must give its code.";

        const MAREA_Diagnostic *diagnostic{ nullptr };

        ASSERT_EQ(MAREA_DiagnosticSinkGetAt(fixture.sink, (MAREA_DiagnosticSinkGetCount(fixture.sink) - 1), &diagnostic), MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) << CASE.name << " must keep its diagnostic.";

        EXPECT_EQ(MAREA_DiagnosticGetPosition(diagnostic).line, CASE.line) << CASE.name << " must name its line.";
    }
}

TEST(MAREABytecodeText, DocumentedExampleIsCanonical)
{
    GTEST_COUT << "Validating the example of the stage doc assembles and is written back unchanged." << std::endl;

    MAREA_BytecodeFixture fixture;

    const std::string EXAMPLE{ ".marea-bytecode 0.1\n"
                               ".strings\n"
                               "s0 \"Point\"\n"
                               "s1 \"q\\\"\\\\\\x0a\\xff\"\n"
                               ".types\n"
                               "t0 primitive name none size 4 align 4 element none count 0\n"
                               "t1 structure name s0 size 4 align 4 element none count 0\n"
                               "  member name s1 type t0 value 0\n"
                               ".functions\n"
                               "f0 name s1 result t0 registers 3 area 16 flags 1\n"
                               "  param type t1 flags 1\n"
                               "  ins nop 1 2 3\n"
                               "  ins return_void 0 0 0\n"
                               ".data\n"
                               "d0 00ff10\n"
                               "d1 -\n" };

    MAREA_BytecodeModule *module{ fixture.MAREA_assemble(EXAMPLE) };

    ASSERT_NE(module, nullptr) << "The documented example must be assembled.";

    EXPECT_EQ(fixture.MAREA_disassemble(module), EXAMPLE) << "The documented example must be canonical.";
}

TEST(MAREABytecodeText, EveryTruncationNeverCrashes)
{
    GTEST_COUT << "Validating every proper prefix of the sample text gives a module or a diagnostic, never a crash." << std::endl;

    const std::string TEXT{ SAMPLE_TEXT };

    for (std::size_t length{ 0 }; length < TEXT.size(); ++length) {
        MAREA_BytecodeFixture fixture;

        MAREA_BytecodeModule *module{ fixture.MAREA_assemble(TEXT.substr(0, length)) };

        if (module == nullptr) {
            EXPECT_GT(fixture.MAREA_errors(), 0U) << "A rejected prefix of " << length << " characters must give a diagnostic.";
        } else {
            EXPECT_NE(fixture.MAREA_assemble(fixture.MAREA_disassemble(module)), nullptr) << "An accepted prefix of " << length << " characters must be written again.";
        }
    }
}

TEST(MAREABytecodeText, MutatedInputNeverCrashes)
{
    GTEST_COUT << "Validating random character changes of the sample text never crash the assembler." << std::endl;

    const std::string GOOD{ SAMPLE_TEXT };

    std::uint64_t state{ 0x9E3779B97F4A7C15ULL };

    for (std::size_t round{ 0 }; round < 3000; ++round) {
        MAREA_BytecodeFixture fixture;

        std::string text{ GOOD };

        const std::size_t CHANGES{ 1 + (round % 3) };

        for (std::size_t change{ 0 }; change < CHANGES; ++change) {
            state ^= (state << 13);
            state ^= (state >> 7);
            state ^= (state << 17);

            text[(state % text.size())] = static_cast<char>((state >> 40) & 0xFF);
        }

        MAREA_BytecodeModule *module{ fixture.MAREA_assemble(text) };

        if (module == nullptr) {
            EXPECT_GT(fixture.MAREA_errors(), 0U) << "A rejected input must give a diagnostic in round " << round << ".";
        } else {
            MAREA_BytecodeModule *back{ fixture.MAREA_assemble(fixture.MAREA_disassemble(module)) };

            ASSERT_NE(back, nullptr) << "A module that was read must be written and read again in round " << round << ".";

            EXPECT_TRUE(MAREA_BytecodeModuleEquals(module, back)) << "A module that was read must keep its content in round " << round << ".";
        }
    }
}
