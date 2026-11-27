// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_TESTS_MAREAHOSTSIGNATUREHELPERS_HPP)
    #define MAREA_TESTS_MAREAHOSTSIGNATUREHELPERS_HPP

    #include "tests/MAREABytecodeGenerateHelpers.hpp"

    #include <cstring>
    #include <map>
    #include <string>
    #include <vector>

struct MAREA_HostSignatureRow
{
    std::vector<MAREA_SemaSignatureParameter> parameters;
    MAREA_SemaSignature signature{};

    MAREA_HostSignatureRow(const std::vector<const char *> &parameterTexts, const char *resultText)
    {
        for (const char *text : parameterTexts) {
            parameters.push_back({ text, std::strlen(text) });
        }

        signature = { (parameters.empty() ? nullptr : parameters.data()), parameters.size(), resultText, ((resultText == nullptr) ? 0 : std::strlen(resultText)) };
    }

    MAREA_HostSignatureRow(const MAREA_HostSignatureRow &) = delete;
    MAREA_HostSignatureRow &operator=(const MAREA_HostSignatureRow &) = delete;
};

inline const MAREA_SemaSignature *MAREA_hostSignature(const std::string &member)
{
    static const std::map<std::string, MAREA_HostSignatureRow> *const ROWS{ [] {
        auto *rows{ new std::map<std::string, MAREA_HostSignatureRow>() };

        auto add = [rows](const char *name, const std::vector<const char *> &parameters, const char *result) {
            rows->emplace(std::piecewise_construct, std::forward_as_tuple(name), std::forward_as_tuple(parameters, result));
        };

        add("add", { "sinteger32", "sinteger32" }, "sinteger32");
        add("wide", { "uinteger64", "sinteger8", "float32" }, "float64");
        add("narrow", { "sinteger32" }, "sinteger8");
        add("flag", { "sinteger32" }, "boolean");
        add("third", {}, "float32");
        add("negate", { "boolean" }, "boolean");
        add("sum", { "Pair", "Pair" }, "Pair");
        add("square", { "Pair" }, "sinteger32");
        add("bump", { "Pair" }, nullptr);
        add("address", { "Pair" }, "uinteger64");
        add("record", { "sinteger32" }, nullptr);
        add("missing", {}, "sinteger32");
        add("text", { "string" }, "sinteger32");
        add("twice", { "sinteger32" }, "sinteger32");
        add("swap", { "Wide" }, "Wide");
        add("reverse", { "Block" }, "Block");
        add("shift", { "Pair", "sinteger32" }, nullptr);
        add("block", { "uinteger64" }, "boolean");
        add("raw", { "sinteger32" }, nullptr);
        add("alloc", { "uinteger64", "uinteger64" }, "uinteger64");
        add("free", { "uinteger64" }, nullptr);
        add("offset", { "uinteger64", "sinteger64", "uinteger64" }, "uinteger64");
        add("is_null", { "uinteger64" }, "boolean");
        add("copy_bytes", { "uinteger64", "uinteger64", "uinteger64" }, nullptr);
        add("fill_bytes", { "uinteger64", "uinteger8", "uinteger64" }, nullptr);
        add("thirtytwo", std::vector<const char *>(32, "sinteger32"), "sinteger64");

        return rows;
    }() };

    const auto FOUND{ ROWS->find(member) };

    return (FOUND == ROWS->end()) ? nullptr : &FOUND->second.signature;
}

inline void MAREA_CALL MAREA_hostSignatureThunk(MAREA_Null, const MAREA_Character8 *, MAREA_IntegerSize, const MAREA_Character8 *memberText, MAREA_IntegerSize memberLength, MAREA_SemaImportResult *outResult, const MAREA_SemaSignature **outSignature)
{
    *outSignature = MAREA_hostSignature(std::string(memberText, memberLength));

    *outResult = (*outSignature == nullptr) ? MAREA_SEMA_IMPORT_RESULT_NOT_FOUND : MAREA_SEMA_IMPORT_RESULT_FOUND;
}

inline MAREA_ResolveCallback MAREA_hostResolver()
{
    return [](const std::string &text, const MAREA_AstNode **) { return ((text.rfind("c.", 0) == 0) || (text.rfind("marea.", 0) == 0)) ? MAREA_SEMA_IMPORT_RESULT_EXTERNAL : MAREA_SEMA_IMPORT_RESULT_NOT_FOUND; };
}

struct MAREA_HostGenerateFixture : MAREA_GenerateFixture
{
    MAREA_HostGenerateFixture()
        : MAREA_GenerateFixture(MAREA_hostResolver())
    {
        MAREA_IGNORE(MAREA_SemaImportResolverSetSignatureFunction(ir.sema.resolver, &MAREA_hostSignatureThunk));
    }

    bool MAREA_build(const std::string &body)
    {
        return MAREA_generate("namespace main; require demo.host as host; structure Pair x: sinteger32; y: sinteger32; end " + body);
    }
};

#endif
