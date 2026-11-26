// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_TESTS_MAREAPARITYHELPERS_HPP)
    #define MAREA_TESTS_MAREAPARITYHELPERS_HPP

    #include "marea/cgen/MAREACgen.h"
    #include "marea/vm/MAREAVm.h"

    #include "tests/MAREAHostSignatureHelpers.hpp"
    #include "tests/MAREALibraryRegistryHelpers.hpp"
    #include "tests/MAREAParityHost.h"
    #include "tests/MAREAScratchDirectory.hpp"

    #include <cmath>
    #include <cstdint>
    #include <cstdio>
    #include <cstring>
    #include <fstream>
    #include <map>
    #include <set>
    #include <sstream>
    #include <string>
    #include <utility>
    #include <vector>

    #include <sys/wait.h>

// INFO: the engine of the parity tests. One program is lowered once to the ir; the vm runs its bytecode and every C11
// compiler of this machine runs the text that cgen writes from the same module. Then the two are compared: the trap of
// each call, the value when there is no trap and the log of the host functions.

MAREA_HOST_FUNCTION(marea_parity_record, MAREA_HOST_RETURNS_NOTHING, MAREA_HOST_VALUE(sinteger32));
MAREA_HOST_FUNCTION(marea_parity_add, MAREA_HOST_RETURNS(sinteger32), MAREA_HOST_VALUE(sinteger32), MAREA_HOST_VALUE(sinteger32));
MAREA_HOST_FUNCTION(marea_parity_subtract, MAREA_HOST_RETURNS(sinteger32), MAREA_HOST_VALUE(sinteger32), MAREA_HOST_VALUE(sinteger32));
MAREA_HOST_FUNCTION(marea_parity_wide_sum, MAREA_HOST_RETURNS(float64), MAREA_HOST_VALUE(uinteger64), MAREA_HOST_VALUE(sinteger8), MAREA_HOST_VALUE(float32));
MAREA_HOST_FUNCTION(marea_parity_identity, MAREA_HOST_RETURNS(sinteger32), MAREA_HOST_VALUE(sinteger32));
MAREA_HOST_FUNCTION(marea_parity_third, MAREA_HOST_RETURNS(float64));
MAREA_HOST_FUNCTION(marea_parity_negate, MAREA_HOST_RETURNS(boolean), MAREA_HOST_VALUE(boolean));
MAREA_HOST_FUNCTION(marea_parity_sum, MAREA_HOST_RETURNS_STRUCTURE(marea_parity_pair), MAREA_HOST_BORROW(marea_parity_pair), MAREA_HOST_BORROW(marea_parity_pair));
MAREA_HOST_FUNCTION(marea_parity_square, MAREA_HOST_RETURNS(sinteger32), MAREA_HOST_BORROW(marea_parity_pair));
MAREA_HOST_FUNCTION(marea_parity_bump, MAREA_HOST_RETURNS_NOTHING, MAREA_HOST_BORROW_MUTABLE(marea_parity_pair));
MAREA_HOST_FUNCTION(marea_parity_shift, MAREA_HOST_RETURNS_NOTHING, MAREA_HOST_BORROW_MUTABLE(marea_parity_pair), MAREA_HOST_VALUE(sinteger32));
MAREA_HOST_FUNCTION(marea_parity_address, MAREA_HOST_RETURNS(uinteger64), MAREA_HOST_BORROW(marea_parity_pair));
MAREA_HOST_FUNCTION(marea_parity_swap, MAREA_HOST_RETURNS_STRUCTURE(marea_parity_wide), MAREA_HOST_BORROW(marea_parity_wide));
MAREA_HOST_FUNCTION(marea_parity_reverse, MAREA_HOST_RETURNS_STRUCTURE(marea_parity_five), MAREA_HOST_BORROW(marea_parity_five));
MAREA_HOST_FUNCTION(marea_parity_twice, MAREA_HOST_RETURNS(sinteger32), MAREA_HOST_VALUE(sinteger32));
MAREA_HOST_FUNCTION(marea_parity_block, MAREA_HOST_RETURNS(boolean), MAREA_HOST_CONTEXT, MAREA_HOST_VALUE(uinteger64));

    #define MAREA_PARITY_VALUE MAREA_HOST_VALUE(sinteger32)

MAREA_HOST_FUNCTION(marea_parity_thirtytwo, MAREA_HOST_RETURNS(sinteger64), MAREA_PARITY_VALUE, MAREA_PARITY_VALUE, MAREA_PARITY_VALUE, MAREA_PARITY_VALUE, MAREA_PARITY_VALUE, MAREA_PARITY_VALUE, MAREA_PARITY_VALUE, MAREA_PARITY_VALUE, MAREA_PARITY_VALUE, MAREA_PARITY_VALUE, MAREA_PARITY_VALUE, MAREA_PARITY_VALUE, MAREA_PARITY_VALUE, MAREA_PARITY_VALUE, MAREA_PARITY_VALUE, MAREA_PARITY_VALUE, MAREA_PARITY_VALUE, MAREA_PARITY_VALUE, MAREA_PARITY_VALUE, MAREA_PARITY_VALUE, MAREA_PARITY_VALUE, MAREA_PARITY_VALUE, MAREA_PARITY_VALUE, MAREA_PARITY_VALUE, MAREA_PARITY_VALUE, MAREA_PARITY_VALUE, MAREA_PARITY_VALUE, MAREA_PARITY_VALUE, MAREA_PARITY_VALUE, MAREA_PARITY_VALUE, MAREA_PARITY_VALUE, MAREA_PARITY_VALUE);

    #undef MAREA_PARITY_VALUE

// INFO: how the host environment is given to both backends: the vm gets a registry, cgen gets a resolver that names the
// C function of each name. The two must say the same thing, so a test changes both at once.
struct MAREA_ParityHost
{
    std::map<std::string, MAREA_HostFunction> vmFunctions;
    std::map<std::string, std::pair<std::string, MAREA_IntegerSize>> cFunctions;
    MAREA_RuntimeAllocateFunction vmAllocate{ nullptr };
    MAREA_RuntimeFreeFunction vmRelease{ nullptr };
    std::string cAllocate;
    std::string cRelease;
    std::string header{ "tests/MAREAParityHost.h" };

    void MAREA_bind(const std::string &name, MAREA_HostFunction wrapper, const std::string &symbol, MAREA_IntegerSize contextPosition = MAREA_CGEN_HOST_NO_CONTEXT)
    {
        vmFunctions["demo.host." + name] = wrapper;
        cFunctions["demo.host." + name] = { symbol, contextPosition };
    }
};

inline MAREA_ParityHost MAREA_parityHost()
{
    MAREA_ParityHost host;

    #define MAREA_PARITY_BIND(name, function) host.MAREA_bind(name, &MAREA_HOST_WRAPPER(function), #function)

    MAREA_PARITY_BIND("record", marea_parity_record);
    MAREA_PARITY_BIND("add", marea_parity_add);
    MAREA_PARITY_BIND("wide", marea_parity_wide_sum);
    MAREA_PARITY_BIND("narrow", marea_parity_identity);
    MAREA_PARITY_BIND("flag", marea_parity_identity);
    MAREA_PARITY_BIND("third", marea_parity_third);
    MAREA_PARITY_BIND("negate", marea_parity_negate);
    MAREA_PARITY_BIND("sum", marea_parity_sum);
    MAREA_PARITY_BIND("square", marea_parity_square);
    MAREA_PARITY_BIND("bump", marea_parity_bump);
    MAREA_PARITY_BIND("shift", marea_parity_shift);
    MAREA_PARITY_BIND("address", marea_parity_address);
    MAREA_PARITY_BIND("swap", marea_parity_swap);
    MAREA_PARITY_BIND("reverse", marea_parity_reverse);
    MAREA_PARITY_BIND("twice", marea_parity_twice);
    MAREA_PARITY_BIND("thirtytwo", marea_parity_thirtytwo);

    #undef MAREA_PARITY_BIND

    host.MAREA_bind("block", &MAREA_HOST_WRAPPER(marea_parity_block), "marea_parity_block", 0);

    return host;
}

// INFO: cgen asks this for the C function of a name that the host registered.
inline MAREA_Boolean MAREA_CALL MAREA_parityResolveHost(MAREA_Null userData, const MAREA_Character8 *name, MAREA_IntegerSize nameLength, MAREA_CgenHostFunction *outFunction)
{
    const auto *host{ static_cast<const MAREA_ParityHost *>(userData) };

    const auto FOUND{ host->cFunctions.find(std::string(name, nameLength)) };

    if (FOUND == host->cFunctions.end()) {
        return MAREA_BOOLEAN_FALSE;
    }

    outFunction->symbol = FOUND->second.first.c_str();
    outFunction->symbolLength = FOUND->second.first.size();
    outFunction->contextPosition = FOUND->second.second;

    return MAREA_BOOLEAN_TRUE;
}

// INFO: the fixture of a program that calls the host functions: sema types the calls from the signatures of the host.
inline MAREA_ResolveCallback MAREA_parityHostResolverCallback()
{
    return MAREA_hostResolver();
}

inline const std::string MAREA_PARITY_HOST_PRELUDE{ "namespace main; require demo.host as host; structure Pair x: sinteger32; y: sinteger32; end " };

struct MAREA_ParityArgument
{
    std::uint64_t bits{ 0 };
    std::string text;
};

struct MAREA_ParityCall
{
    std::string function;
    std::vector<MAREA_ParityArgument> arguments;
    std::string result;
};

struct MAREA_ParityOutcome
{
    int trap{ 0 };
    std::uint64_t bits{ 0 };
    std::vector<std::int64_t> log;
};

inline std::string MAREA_parityDescribe(const MAREA_ParityCall &call)
{
    std::string text{ call.function + "(" };

    for (std::size_t index{ 0 }; index < call.arguments.size(); ++index) {
        text += ((index == 0) ? "" : ", ") + call.arguments[index].text;
    }

    return text + ")";
}

inline std::string MAREA_parityDescribe(const MAREA_ParityOutcome &outcome)
{
    std::string text{ "trap " + std::to_string(outcome.trap) + " bits " + std::to_string(outcome.bits) + " log [" };

    for (std::size_t index{ 0 }; index < outcome.log.size(); ++index) {
        text += ((index == 0) ? "" : ", ") + std::to_string(outcome.log[index]);
    }

    return text + "]";
}

inline bool MAREA_parityIsSigned(const std::string &type)
{
    return type.rfind("sinteger", 0) == 0;
}

inline bool MAREA_parityIsUnsigned(const std::string &type)
{
    return type.rfind("uinteger", 0) == 0;
}

inline bool MAREA_parityIsFloat(const std::string &type)
{
    return (type == "float32") || (type == "float64");
}

inline bool MAREA_parityIsScalar(const std::string &type)
{
    return MAREA_parityIsSigned(type) || MAREA_parityIsUnsigned(type) || MAREA_parityIsFloat(type) || (type == "boolean");
}

inline std::string MAREA_parityCType(const std::string &type)
{
    if (type == "boolean") {
        return "_Bool";
    }

    if (type == "float32") {
        return "float";
    }

    if (type == "float64") {
        return "double";
    }

    const std::string WIDTH{ type.substr(type.find_first_of("0123456789")) };

    return (MAREA_parityIsSigned(type) ? "int" : "uint") + WIDTH + "_t";
}

inline MAREA_ParityArgument MAREA_paritySigned(const std::string &type, std::int64_t value)
{
    const std::string LITERAL{ (value == INT64_MIN) ? std::string("-9223372036854775807LL - 1") : std::to_string(value) + "LL" };

    return { static_cast<std::uint64_t>(value), "(" + MAREA_parityCType(type) + ")(" + LITERAL + ")" };
}

inline MAREA_ParityArgument MAREA_parityUnsigned(const std::string &type, std::uint64_t value)
{
    return { value, "(" + MAREA_parityCType(type) + ")(" + std::to_string(value) + "ULL)" };
}

inline MAREA_ParityArgument MAREA_parityBoolean(bool value)
{
    return { value ? 1U : 0U, value ? "(_Bool)1" : "(_Bool)0" };
}

inline MAREA_ParityArgument MAREA_parityFloat(const std::string &type, double value)
{
    const double HELD{ (type == "float32") ? static_cast<double>(static_cast<float>(value)) : value };

    std::uint64_t bits{ 0 };

    std::memcpy(&bits, &HELD, sizeof(bits));

    char text[64];

    std::snprintf(text, sizeof(text), "(%s)(%a)", MAREA_parityCType(type).c_str(), value);

    return { bits, text };
}

// INFO: the values that every scalar parameter takes. They are small, so a loop or a recursion over one of them ends.
inline std::vector<MAREA_ParityArgument> MAREA_paritySamples(const std::string &type)
{
    std::vector<MAREA_ParityArgument> samples;

    if (MAREA_parityIsSigned(type)) {
        for (const std::int64_t VALUE : { 0, 1, 2, 3, 7, -1, -3, 100 }) {
            samples.push_back(MAREA_paritySigned(type, VALUE));
        }
    } else if (MAREA_parityIsUnsigned(type)) {
        for (const std::uint64_t VALUE : { 0U, 1U, 2U, 3U, 7U, 10U, 100U, 255U }) {
            samples.push_back(MAREA_parityUnsigned(type, VALUE));
        }
    } else if (MAREA_parityIsFloat(type)) {
        for (const double VALUE : { 0.0, 1.0, -1.5, 2.5, 100.0, 0.1, -0.0, 3.0 }) {
            samples.push_back(MAREA_parityFloat(type, VALUE));
        }
    } else {
        samples.push_back(MAREA_parityBoolean(false));
        samples.push_back(MAREA_parityBoolean(true));
    }

    return samples;
}

// INFO: the C name that cgen gives a function: f_ and the name, and for a member of a type the length of the type name.
inline std::string MAREA_parityCName(const std::string &function)
{
    const std::size_t DOT{ function.find('.') };

    if (DOT == std::string::npos) {
        return "f_" + function;
    }

    return "f_" + std::to_string(DOT) + "_" + function.substr(0, DOT) + "_" + function.substr(DOT + 1);
}

inline std::string MAREA_parityResolvedType(const MAREA_IrNode *node)
{
    const MAREA_Character8 *text{ MAREA_IrNodeGetResolvedType(node) };

    return (text == nullptr) ? std::string() : std::string(text, MAREA_IrNodeGetResolvedTypeLength(node));
}

// INFO: the calls of one module: each function that has a scalar result and only scalar parameters, once for each row of
// sample values (once when it takes no value). The same values go to both backends.
inline std::vector<MAREA_ParityCall> MAREA_parityCalls(const MAREA_IrNode *module)
{
    std::vector<MAREA_ParityCall> calls;

    for (MAREA_IntegerSize index{ 0 }; index < MAREA_IrNodeGetChildCount(module); ++index) {
        const MAREA_IrNode *function{ MAREA_irChild(module, index) };

        if (MAREA_IrNodeGetKind(function) != MAREA_IR_KIND_FUNCTION_DECLARATION) {
            continue;
        }

        const std::string RESULT{ MAREA_parityResolvedType(function) };

        if (!MAREA_parityIsScalar(RESULT)) {
            continue;
        }

        const MAREA_IrNode *parameters{ MAREA_irChild(function, 0) };

        std::vector<std::vector<MAREA_ParityArgument>> columns;

        bool callable{ true };

        for (MAREA_IntegerSize parameter{ 0 }; parameter < MAREA_IrNodeGetChildCount(parameters); ++parameter) {
            const std::string TYPE{ MAREA_parityResolvedType(MAREA_irChild(parameters, parameter)) };

            callable = callable && MAREA_parityIsScalar(TYPE);

            if (callable) {
                columns.push_back(MAREA_paritySamples(TYPE));
            }
        }

        if (!callable) {
            continue;
        }

        const std::size_t ROWS{ columns.empty() ? 1U : 8U };

        for (std::size_t row{ 0 }; row < ROWS; ++row) {
            MAREA_ParityCall call{ MAREA_irText(function), {}, RESULT };

            for (std::size_t column{ 0 }; column < columns.size(); ++column) {
                call.arguments.push_back(columns[column][(row + (column * 3)) % columns[column].size()]);
            }

            calls.push_back(call);
        }
    }

    return calls;
}

// INFO: both backends from one lowered module. A backend that cannot lower it gives its diagnostic codes.
struct MAREA_ParityModules
{
    MAREA_BytecodeModule *bytecode{ nullptr };
    std::string cText;
    bool vmOk{ false };
    bool cgenOk{ false };
    std::set<MAREA_Uinteger32> vmCodes;
    std::set<MAREA_Uinteger32> cgenCodes;
};

inline std::set<MAREA_Uinteger32> MAREA_parityNewCodes(MAREA_IrFixture &fixture, MAREA_IntegerSize first)
{
    std::set<MAREA_Uinteger32> codes;

    MAREA_DiagnosticSink *sink{ fixture.sema.parser.lexer.sink };

    for (MAREA_IntegerSize index{ first }; index < MAREA_DiagnosticSinkGetCount(sink); ++index) {
        const MAREA_Diagnostic *diagnostic{ nullptr };

        MAREA_IGNORE(MAREA_DiagnosticSinkGetAt(sink, index, &diagnostic));

        codes.insert(MAREA_DiagnosticGetCode(diagnostic));
    }

    return codes;
}

inline std::string MAREA_parityCodes(const std::set<MAREA_Uinteger32> &codes)
{
    std::string text;

    for (const MAREA_Uinteger32 CODE : codes) {
        text += (text.empty() ? "" : " ") + std::to_string(CODE);
    }

    return text;
}

inline MAREA_CgenContext *MAREA_parityCgenContext(MAREA_IrFixture &fixture, const MAREA_ParityHost *host)
{
    MAREA_Arena *arena{ fixture.sema.parser.lexer.arena };

    MAREA_CgenContext *context{ nullptr };

    MAREA_IGNORE(MAREA_CgenContextCreate(arena, fixture.sema.parser.lexer.sink, &context));
    MAREA_IGNORE(MAREA_CgenContextSetTypeRegistry(context, MAREA_buildLibraryRegistry(arena)));

    if (host != nullptr) {
        MAREA_IGNORE(MAREA_CgenContextSetHostResolver(context, &MAREA_parityResolveHost, const_cast<MAREA_ParityHost *>(host)));
        MAREA_IGNORE(MAREA_CgenContextAddInclude(context, host->header.c_str(), host->header.size()));

        if (!host->cAllocate.empty()) {
            MAREA_IGNORE(MAREA_CgenContextSetAllocator(context, host->cAllocate.c_str(), host->cAllocate.size(), host->cRelease.c_str(), host->cRelease.size()));
        }
    }

    return context;
}

inline MAREA_ParityModules MAREA_parityGenerate(MAREA_IrFixture &fixture, const MAREA_ParityHost *host)
{
    MAREA_ParityModules modules;

    MAREA_BytecodeContext *bytecodeContext{ nullptr };

    MAREA_IGNORE(MAREA_BytecodeContextCreate(fixture.sema.parser.lexer.arena, fixture.sema.parser.lexer.sink, &bytecodeContext));

    MAREA_IntegerSize first{ MAREA_DiagnosticSinkGetCount(fixture.sema.parser.lexer.sink) };

    modules.vmOk = (MAREA_BytecodeGenerate(bytecodeContext, fixture.module, &modules.bytecode) == MAREA_BYTECODE_EXCEPT_SUCCESS) && (modules.bytecode != nullptr);
    modules.vmCodes = MAREA_parityNewCodes(fixture, first);

    first = MAREA_DiagnosticSinkGetCount(fixture.sema.parser.lexer.sink);

    const MAREA_Character8 *text{ nullptr };

    MAREA_IntegerSize length{ 0 };

    const MAREA_CgenExcept EXCEPT{ MAREA_CgenGenerateModule(MAREA_parityCgenContext(fixture, host), fixture.module, &text, &length) };

    modules.cgenCodes = MAREA_parityNewCodes(fixture, first);
    modules.cgenOk = (EXCEPT == MAREA_CGEN_EXCEPT_SUCCESS) && modules.cgenCodes.empty();

    if (text != nullptr) {
        modules.cText.assign(text, length);
    }

    return modules;
}

// INFO: runs the calls in the vm, one new run each, and reads the log of the host functions after each one.
inline std::vector<MAREA_ParityOutcome> MAREA_parityRunVm(MAREA_IrFixture &fixture, const MAREA_BytecodeModule *module, const std::vector<MAREA_ParityCall> &calls, const MAREA_ParityHost *host)
{
    MAREA_Arena *arena{ fixture.sema.parser.lexer.arena };

    MAREA_VmContext *context{ nullptr };

    MAREA_IGNORE(MAREA_VmContextCreate(arena, fixture.sema.parser.lexer.sink, &context));

    if (host != nullptr) {
        MAREA_TypeRegistry *registry{ nullptr };

        MAREA_IGNORE(MAREA_TypeRegistryCreate(arena, MAREA_LIBRARY_MASK_DEFAULT, &registry));

        for (const auto &entry : host->vmFunctions) {
            const std::size_t DOT{ entry.first.rfind('.') };

            const std::string NAME_SPACE{ entry.first.substr(0, DOT) };

            const std::string NAME{ entry.first.substr(DOT + 1) };

            MAREA_IGNORE(MAREA_TypeRegistryRegisterPublicFunction(registry, entry.first.c_str(), NAME_SPACE.size(), entry.first.c_str() + DOT + 1, NAME.size(), entry.second));
        }

        if (host->vmAllocate != nullptr) {
            MAREA_IGNORE(MAREA_TypeRegistryReplaceRuntimeFunction(registry, "alloc", 5, reinterpret_cast<MAREA_Null>(host->vmAllocate)));
            MAREA_IGNORE(MAREA_TypeRegistryReplaceRuntimeFunction(registry, "free", 4, reinterpret_cast<MAREA_Null>(host->vmRelease)));
        }

        MAREA_IGNORE(MAREA_VmContextSetTypeRegistry(context, registry));
    }

    std::vector<MAREA_ParityOutcome> outcomes;

    const std::string NAME_SPACE{ MAREA_irText(fixture.module) };

    for (const MAREA_ParityCall &call : calls) {
        const std::string FULL{ NAME_SPACE + "." + call.function };

        std::vector<std::uint64_t> arguments;

        for (const MAREA_ParityArgument &argument : call.arguments) {
            arguments.push_back(argument.bits);
        }

        marea_parity_log_count = 0;

        MAREA_VmResult result{};

        const MAREA_VmExcept EXCEPT{ MAREA_VmCallByName(context, module, FULL.c_str(), FULL.size(), arguments.empty() ? nullptr : reinterpret_cast<const MAREA_Uinteger64 *>(arguments.data()), static_cast<MAREA_Uinteger32>(arguments.size()), &result) };

        EXPECT_EQ(EXCEPT, MAREA_VM_EXCEPT_SUCCESS) << "The vm must run " << MAREA_parityDescribe(call);

        MAREA_ParityOutcome outcome;

        outcome.trap = result.trapped ? static_cast<int>(result.trap) : 0;
        outcome.bits = (result.trapped || call.result.empty() || (call.result == "void")) ? 0 : result.value;
        outcome.log.assign(marea_parity_log, marea_parity_log + marea_parity_log_count);

        outcomes.push_back(outcome);
    }

    return outcomes;
}

// INFO: the C program that makes the same calls and prints, for each one, the trap, the bits of the result and the log.
inline std::string MAREA_parityMain(const std::vector<MAREA_ParityCall> &calls)
{
    std::string text{ "\n#include <stdint.h>\n#include <stdio.h>\n#include <string.h>\n#include \"tests/MAREAParityHost.h\"\n" };

    text += "int main(void)\n{\n";

    for (const MAREA_ParityCall &call : calls) {
        const bool HAS_RESULT{ !call.result.empty() && (call.result != "void") };

        std::string arguments;

        for (const MAREA_ParityArgument &argument : call.arguments) {
            arguments += argument.text + ", ";
        }

        const std::string CALL{ MAREA_parityCName(call.function) + "(" + arguments + "&trap)" };

        text += "{ marea_rt_trap trap = MAREA_RT_TRAP_NONE; marea_parity_log_count = 0; uint64_t bits = 0; ";

        if (!HAS_RESULT) {
            text += CALL + "; ";
        } else {
            text += MAREA_parityCType(call.result) + " r = " + CALL + "; ";

            if (MAREA_parityIsSigned(call.result)) {
                text += "bits = (uint64_t)(int64_t)r; ";
            } else if (MAREA_parityIsUnsigned(call.result)) {
                text += "bits = (uint64_t)r; ";
            } else if (call.result == "boolean") {
                text += "bits = r ? 1u : 0u; ";
            } else {
                text += "{ double d = (double)r; memcpy(&bits, &d, sizeof(bits)); } ";
            }
        }

        text += "if (trap != MAREA_RT_TRAP_NONE) { bits = 0; } printf(\"%d %llu\", (int)trap, (unsigned long long)bits); for (int i = 0; i < marea_parity_log_count; i += 1) { printf(\" %lld\", (long long)marea_parity_log[i]); } printf(\"\\n\"); }\n";
    }

    return text + "return 0;\n}\n";
}

inline std::string MAREA_parityShell(const std::string &command, int &status)
{
    std::string output;

    FILE *pipe{ popen((command + " 2>&1").c_str(), "r") };

    if (pipe == nullptr) {
        status = -1;

        return output;
    }

    char chunk[4096];

    while (fgets(chunk, sizeof(chunk), pipe) != nullptr) {
        output += chunk;
    }

    const int RAW{ pclose(pipe) };

    status = (WIFEXITED(RAW) != 0) ? WEXITSTATUS(RAW) : -1;

    return output;
}

struct MAREA_ParityCompiler
{
    std::string path;
    std::string flags;
};

inline const std::string MAREA_PARITY_SANITIZERS{ "-O1 -fsanitize=address,undefined,float-cast-overflow -fno-sanitize-recover=all" };

// INFO: a compiler that cannot link with the sanitizers runs the generated program without them.
inline bool MAREA_parityHasSanitizers(const std::string &compiler)
{
    const MAREA_ScratchDirectory SCRATCH;

    std::ofstream(SCRATCH.MAREA_file("probe.c")) << "int main(void){return 0;}\n";

    int status{ -1 };

    MAREA_parityShell("\"" + compiler + "\" " + MAREA_PARITY_SANITIZERS + " -o \"" + SCRATCH.MAREA_file("probe.bin") + "\" \"" + SCRATCH.MAREA_file("probe.c") + "\"", status);

    return status == 0;
}

// INFO: the first compiler runs with the undefined behavior and address checks, the second with the optimizer, so a
// difference that only one of them shows is still seen. A machine with one compiler runs both ways with it.
inline const std::vector<MAREA_ParityCompiler> &MAREA_parityCompilers()
{
    static const std::vector<MAREA_ParityCompiler> COMPILERS{ [] {
        std::vector<std::string> paths;

    #if defined(MAREA_TESTS_CGEN_CLANG)
        paths.push_back(MAREA_TESTS_CGEN_CLANG);
    #endif
    #if defined(MAREA_TESTS_CGEN_GCC)
        paths.push_back(MAREA_TESTS_CGEN_GCC);
    #endif

        if (paths.empty()) {
            paths.push_back(MAREA_TESTS_CGEN_C_COMPILER);
        }

        std::vector<MAREA_ParityCompiler> result;

        result.push_back({ paths.front(), MAREA_parityHasSanitizers(paths.front()) ? MAREA_PARITY_SANITIZERS : std::string("-O0") });
        result.push_back({ paths.back(), "-O2" });

        return result;
    }() };

    return COMPILERS;
}

inline std::string MAREA_parityCompileCommand(const MAREA_ParityCompiler &compiler, const std::string &source, const std::string &binary, bool link)
{
    std::string command{ "\"" + compiler.path + "\" -std=c11 -Wall -Wextra -Wpedantic -Werror " + compiler.flags };

    command += " -I\"" + std::string(MAREA_TESTS_LIB_INCLUDE_DIRECTORY) + "\" -I\"" + std::string(MAREA_TESTS_SUPPORT_DIRECTORY) + "\"";

    if (!link) {
        return command + " -c -o \"" + binary + "\" \"" + source + "\"";
    }

    return command + " -o \"" + binary + "\" \"" + source + "\" \"" + std::string(MAREA_TESTS_BASE_LIBRARY) + "\" -lm";
}

struct MAREA_ParityCRun
{
    std::string error;
    std::vector<MAREA_ParityOutcome> outcomes;
};

// INFO: compiles and runs one C program with every compiler; all of them must print the same lines. No gtest call is
// made here, so the parity test may run this on a thread.
inline MAREA_ParityCRun MAREA_parityRunC(const std::string &moduleText, const std::vector<MAREA_ParityCall> &calls)
{
    MAREA_ParityCRun run;

    const MAREA_ScratchDirectory SCRATCH;

    const std::string SOURCE{ SCRATCH.MAREA_file("module.c") };

    std::ofstream(SOURCE, std::ios::binary) << moduleText << MAREA_parityMain(calls);

    std::string first;

    for (std::size_t index{ 0 }; index < MAREA_parityCompilers().size(); ++index) {
        const MAREA_ParityCompiler &compiler{ MAREA_parityCompilers()[index] };

        const std::string BINARY{ SCRATCH.MAREA_file("module" + std::to_string(index) + ".bin") };

        int status{ -1 };

        const std::string BUILT{ MAREA_parityShell(MAREA_parityCompileCommand(compiler, SOURCE, BINARY, true), status) };

        if (status != 0) {
            run.error = "compile failed with " + compiler.path + " " + compiler.flags + ":\n" + BUILT;

            return run;
        }

        const std::string OUTPUT{ MAREA_parityShell("\"" + BINARY + "\"", status) };

        if (status != 0) {
            run.error = "run failed with " + compiler.path + " " + compiler.flags + " (status " + std::to_string(status) + "):\n" + OUTPUT;

            return run;
        }

        if (index == 0) {
            first = OUTPUT;
        } else if (OUTPUT != first) {
            run.error = "the compilers disagree:\n" + first + "and\n" + OUTPUT;

            return run;
        }
    }

    std::istringstream lines(first);

    std::string line;

    while (std::getline(lines, line)) {
        std::istringstream stream(line);

        MAREA_ParityOutcome outcome;

        stream >> outcome.trap >> outcome.bits;

        std::int64_t entry{ 0 };

        while (stream >> entry) {
            outcome.log.push_back(entry);
        }

        run.outcomes.push_back(outcome);
    }

    if (run.outcomes.size() != calls.size()) {
        run.error = "the program printed " + std::to_string(run.outcomes.size()) + " lines for " + std::to_string(calls.size()) + " calls:\n" + first;
    }

    return run;
}

// INFO: compiles the text alone, for a program that the vm cannot run.
inline std::string MAREA_parityCompileOnly(const std::string &moduleText)
{
    const MAREA_ScratchDirectory SCRATCH;

    const std::string SOURCE{ SCRATCH.MAREA_file("module.c") };

    std::ofstream(SOURCE, std::ios::binary) << moduleText;

    for (const MAREA_ParityCompiler &compiler : MAREA_parityCompilers()) {
        int status{ -1 };

        const std::string BUILT{ MAREA_parityShell(MAREA_parityCompileCommand(compiler, SOURCE, SCRATCH.MAREA_file("module.o"), false), status) };

        if (status != 0) {
            return "compile failed with " + compiler.path + " " + compiler.flags + ":\n" + BUILT;
        }
    }

    return std::string();
}

inline std::vector<std::string> MAREA_parityDiffer(const std::vector<MAREA_ParityCall> &calls, const std::vector<MAREA_ParityOutcome> &vm, const std::vector<MAREA_ParityOutcome> &generated)
{
    std::vector<std::string> failures;

    if ((vm.size() != calls.size()) || (generated.size() != calls.size())) {
        failures.push_back("the two backends did not answer every call");

        return failures;
    }

    for (std::size_t index{ 0 }; index < calls.size(); ++index) {
        bool sameBits{ vm[index].bits == generated[index].bits };

        if (!sameBits && MAREA_parityIsFloat(calls[index].result)) {
            double left{ 0.0 };
            double right{ 0.0 };

            std::memcpy(&left, &vm[index].bits, sizeof(left));
            std::memcpy(&right, &generated[index].bits, sizeof(right));

            sameBits = std::isnan(left) && std::isnan(right);
        }

        if ((vm[index].trap != generated[index].trap) || !sameBits || (vm[index].log != generated[index].log)) {
            failures.push_back(MAREA_parityDescribe(calls[index]) + ": the vm gave " + MAREA_parityDescribe(vm[index]) + " and the C program gave " + MAREA_parityDescribe(generated[index]));
        }
    }

    return failures;
}

// INFO: one program through both backends. vm and generated hold what each backend answered, failures what differs. A
// backend that cannot lower the program is a failure here: only the parity test of the catalogue has exclusions.
struct MAREA_ParityResult
{
    std::vector<std::string> failures;
    std::vector<MAREA_ParityCall> calls;
    std::vector<MAREA_ParityOutcome> vm;
    std::vector<MAREA_ParityOutcome> generated;
    std::string cText;

    std::string MAREA_failures() const
    {
        std::string text;

        for (const std::string &failure : failures) {
            text += failure + "\n";
        }

        return text;
    }

    const MAREA_ParityOutcome &MAREA_outcome(const std::string &call) const
    {
        static const MAREA_ParityOutcome MISSING{ -1, 0, {} };

        for (std::size_t index{ 0 }; index < calls.size(); ++index) {
            if (MAREA_parityDescribe(calls[index]) == call) {
                return generated.size() > index ? generated[index] : MISSING;
            }
        }

        return MISSING;
    }
};

inline MAREA_ParityResult MAREA_parityCheck(const std::string &source, const std::vector<MAREA_ParityCall> &calls, const MAREA_ParityHost *host = nullptr)
{
    MAREA_ParityResult result;

    MAREA_IrFixture fixture((host != nullptr) ? MAREA_parityHostResolverCallback() : MAREA_ResolveCallback());

    if (host != nullptr) {
        MAREA_IGNORE(MAREA_SemaImportResolverSetSignatureFunction(fixture.sema.resolver, &MAREA_hostSignatureThunk));
    }

    if ((fixture.MAREA_lower(source) != MAREA_IR_EXCEPT_SUCCESS) || !fixture.MAREA_irCodes().empty()) {
        result.failures.push_back("the program does not lower to the ir");

        return result;
    }

    const MAREA_ParityModules MODULES{ MAREA_parityGenerate(fixture, host) };

    result.cText = MODULES.cText;

    if (!MODULES.vmOk) {
        result.failures.push_back("the vm cannot lower the program: " + MAREA_parityCodes(MODULES.vmCodes));
    }

    if (!MODULES.cgenOk) {
        result.failures.push_back("cgen cannot lower the program: " + MAREA_parityCodes(MODULES.cgenCodes));
    }

    if (!result.failures.empty()) {
        return result;
    }

    result.calls = calls.empty() ? MAREA_parityCalls(fixture.module) : calls;
    result.vm = MAREA_parityRunVm(fixture, MODULES.bytecode, result.calls, host);

    const MAREA_ParityCRun GENERATED{ MAREA_parityRunC(MODULES.cText, result.calls) };

    if (!GENERATED.error.empty()) {
        result.failures.push_back(GENERATED.error);

        return result;
    }

    result.generated = GENERATED.outcomes;
    result.failures = MAREA_parityDiffer(result.calls, result.vm, result.generated);

    return result;
}

// INFO: one program through cgen alone, for what the vm cannot run yet: the C program is compiled and run, and its
// answers are checked by hand.
inline MAREA_ParityResult MAREA_parityGenerated(const std::string &source, const std::vector<MAREA_ParityCall> &calls, const MAREA_ParityHost *host = nullptr)
{
    MAREA_ParityResult result;

    MAREA_IrFixture fixture((host != nullptr) ? MAREA_parityHostResolverCallback() : MAREA_ResolveCallback());

    if (host != nullptr) {
        MAREA_IGNORE(MAREA_SemaImportResolverSetSignatureFunction(fixture.sema.resolver, &MAREA_hostSignatureThunk));
    }

    if ((fixture.MAREA_lower(source) != MAREA_IR_EXCEPT_SUCCESS) || !fixture.MAREA_irCodes().empty()) {
        result.failures.push_back("the program does not lower to the ir");

        return result;
    }

    const MAREA_ParityModules MODULES{ MAREA_parityGenerate(fixture, host) };

    result.cText = MODULES.cText;

    if (!MODULES.cgenOk) {
        result.failures.push_back("cgen cannot lower the program: " + MAREA_parityCodes(MODULES.cgenCodes));

        return result;
    }

    result.calls = calls.empty() ? MAREA_parityCalls(fixture.module) : calls;

    const MAREA_ParityCRun GENERATED{ MAREA_parityRunC(MODULES.cText, result.calls) };

    if (!GENERATED.error.empty()) {
        result.failures.push_back(GENERATED.error);

        return result;
    }

    result.generated = GENERATED.outcomes;

    return result;
}

// INFO: the codes that cgen raises for a program it cannot lower.
inline std::set<MAREA_Uinteger32> MAREA_parityRejected(const std::string &source, const MAREA_ParityHost *host = nullptr)
{
    MAREA_IrFixture fixture((host != nullptr) ? MAREA_parityHostResolverCallback() : MAREA_ResolveCallback());

    if (host != nullptr) {
        MAREA_IGNORE(MAREA_SemaImportResolverSetSignatureFunction(fixture.sema.resolver, &MAREA_hostSignatureThunk));
    }

    if (fixture.MAREA_lower(source) != MAREA_IR_EXCEPT_SUCCESS) {
        return { 0 };
    }

    return MAREA_parityGenerate(fixture, host).cgenCodes;
}

inline MAREA_ParityCall MAREA_parityCall(const std::string &function, const std::string &result, const std::vector<MAREA_ParityArgument> &arguments = {})
{
    return { function, arguments, result };
}

inline MAREA_ParityArgument MAREA_parityInt(std::int64_t value)
{
    return MAREA_paritySigned("sinteger32", value);
}

inline std::int64_t MAREA_parityValue(const MAREA_ParityOutcome &outcome)
{
    return static_cast<std::int64_t>(outcome.bits);
}

inline double MAREA_parityDouble(const MAREA_ParityOutcome &outcome)
{
    double value{ 0.0 };

    std::memcpy(&value, &outcome.bits, sizeof(value));

    return value;
}

#endif
