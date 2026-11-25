// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_TESTS_MAREACGENHELPERS_HPP)
    #define MAREA_TESTS_MAREACGENHELPERS_HPP

    #include "marea/cgen/MAREACgen.h"

    #include "tests/MAREAIrHelpers.hpp"

    #include <array>
    #include <cstdio>
    #include <fstream>
    #include <sstream>
    #include <string>
    #include <vector>

    #include <sys/wait.h>

// INFO: one cgen fixture lowers source through lex/parse/sema/ir, as MAREA_GenerateFixture does for bytecode,
// then asks cgen to write the C11 text for the resulting module.
struct MAREA_CgenFixture
{
    MAREA_IrFixture ir;
    MAREA_CgenContext *context{ nullptr };
    const MAREA_Character8 *text{ nullptr };
    MAREA_IntegerSize length{ 0 };
    MAREA_CgenExcept except{ MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER };

    explicit MAREA_CgenFixture(MAREA_ResolveCallback resolve = nullptr)
        : ir(std::move(resolve))
    {
        MAREA_IGNORE(MAREA_CgenContextCreate(ir.sema.parser.lexer.arena, ir.sema.parser.lexer.sink, &context));
    }

    MAREA_CgenFixture(const MAREA_CgenFixture &) = delete;
    MAREA_CgenFixture &operator=(const MAREA_CgenFixture &) = delete;

    bool MAREA_generate(const std::string &source, const std::string &filePath = "main.marea")
    {
        if (ir.MAREA_lower(source, filePath) != MAREA_IR_EXCEPT_SUCCESS) {
            return false;
        }

        except = MAREA_CgenGenerateModule(context, ir.module, &text, &length);

        return ((except == MAREA_CGEN_EXCEPT_SUCCESS) && (text != nullptr));
    }

    std::string MAREA_text() const
    {
        return std::string(text, static_cast<std::size_t>(length));
    }

    std::vector<MAREA_Uinteger32> MAREA_codes() const
    {
        std::vector<MAREA_Uinteger32> result;

        for (MAREA_Uinteger32 code : ir.sema.parser.MAREA_codes()) {
            if ((code >= 7000) && (code < 8000)) {
                result.push_back(code);
            }
        }

        return result;
    }
};

// INFO: a mangled plain identifier is "f_" or "v_" plus itself unchanged, because MAREA_CgenWriteMangled only
// rewrites a byte that is not a letter, a digit or an underscore, and a plain source name has none of those.
inline std::string MAREA_cgenFunctionName(const std::string &name)
{
    return "f_" + name;
}

struct MAREA_CgenRunResult
{
    bool compiled{ false };
    bool ran{ false };
    int exitCode{ -1 };
    std::string output;
};

// INFO: writes moduleText plus a small main() to a scratch .c file, compiles it with compilerPath under the
// acceptance flags of this apply, and runs it. Returns compiled=false when the compiler itself fails, so a
// test can tell a cgen defect (would not have compiled) from a runtime assertion failure.
inline MAREA_CgenRunResult MAREA_cgenCompileAndRun(const std::string &compilerPath, const std::string &moduleText, const std::string &mainText)
{
    MAREA_CgenRunResult result;

    static int marea_g_cgenRunCounter{ 0 };

    const std::string BASE{ std::string(::testing::TempDir()) + "marea_cgen_" + std::to_string(::testing::UnitTest::GetInstance()->random_seed()) + "_" + std::to_string(marea_g_cgenRunCounter++) };

    const std::string SOURCE_PATH{ BASE + ".c" };
    const std::string BINARY_PATH{ BASE + ".bin" };

    {
        std::ofstream stream(SOURCE_PATH, std::ios::binary | std::ios::trunc);

        stream << moduleText << "\n"
               << mainText;
    }

    const std::string COMPILE_COMMAND{ "\"" + compilerPath + "\" -std=c11 -Wall -Wextra -Werror -O0 -o \"" + BINARY_PATH + "\" \"" + SOURCE_PATH + "\" 2>&1" };

    std::string compileOutput;

    {
        FILE *pipe{ popen(COMPILE_COMMAND.c_str(), "r") };

        if (pipe == nullptr) {
            return result;
        }

        char chunk[256];

        while (fgets(chunk, sizeof(chunk), pipe) != nullptr) {
            compileOutput += chunk;
        }

        const int STATUS{ pclose(pipe) };

        if ((WIFEXITED(STATUS) == 0) || (WEXITSTATUS(STATUS) != 0)) {
            result.output = compileOutput;

            return result;
        }
    }

    result.compiled = true;

    const std::string RUN_COMMAND{ "\"" + BINARY_PATH + "\" 2>&1" };

    FILE *pipe{ popen(RUN_COMMAND.c_str(), "r") };

    if (pipe == nullptr) {
        return result;
    }

    char chunk[256];

    while (fgets(chunk, sizeof(chunk), pipe) != nullptr) {
        result.output += chunk;
    }

    const int STATUS{ pclose(pipe) };

    result.ran = true;
    result.exitCode = (WIFEXITED(STATUS) != 0) ? WEXITSTATUS(STATUS) : -1;

    std::remove(SOURCE_PATH.c_str());
    std::remove(BINARY_PATH.c_str());

    return result;
}

#endif
