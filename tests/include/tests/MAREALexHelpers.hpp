// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_TESTS_MAREALEXHELPERS_HPP)
    #define MAREA_TESTS_MAREALEXHELPERS_HPP

    #include <string>
    #include <vector>

struct MAREA_LexFixture
{
    MAREA_Arena *arena{ nullptr };
    MAREA_DiagnosticSink *sink{ nullptr };
    MAREA_LexContext *context{ nullptr };
    MAREA_TokenList *list{ nullptr };

    MAREA_LexFixture()
    {
        MAREA_IGNORE(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(0), &arena));
        MAREA_IGNORE(MAREA_DiagnosticSinkCreate(arena, &sink));
        MAREA_IGNORE(MAREA_LexContextCreate(arena, sink, &context));
    }

    ~MAREA_LexFixture()
    {
        MAREA_ArenaDestroy(arena);
    }

    MAREA_LexFixture(const MAREA_LexFixture &) = delete;
    MAREA_LexFixture &operator=(const MAREA_LexFixture &) = delete;

    std::string ownedSource;

    MAREA_LexExcept MAREA_lex(const std::string &source)
    {
        ownedSource = source;

        return MAREA_LexTokenize(context, ownedSource.c_str(), static_cast<MAREA_IntegerSize>(ownedSource.size()), &list);
    }

    std::vector<MAREA_TokenKind> MAREA_kinds() const
    {
        std::vector<MAREA_TokenKind> result;

        const MAREA_IntegerSize COUNT{ MAREA_TokenListGetCount(list) };

        for (MAREA_IntegerSize index{ 0 }; index < COUNT; ++index) {
            const MAREA_Token *token{ nullptr };

            MAREA_IGNORE(MAREA_TokenListGetAt(list, index, &token));

            result.push_back(MAREA_TokenGetKind(token));
        }

        return result;
    }

    std::vector<std::string> MAREA_texts() const
    {
        std::vector<std::string> result;

        const MAREA_IntegerSize COUNT{ MAREA_TokenListGetCount(list) };

        for (MAREA_IntegerSize index{ 0 }; index < COUNT; ++index) {
            const MAREA_Token *token{ nullptr };

            MAREA_IGNORE(MAREA_TokenListGetAt(list, index, &token));

            result.emplace_back(MAREA_TokenGetText(token), MAREA_TokenGetLength(token));
        }

        return result;
    }

    std::vector<MAREA_SourcePosition> MAREA_positions() const
    {
        std::vector<MAREA_SourcePosition> result;

        const MAREA_IntegerSize COUNT{ MAREA_TokenListGetCount(list) };

        for (MAREA_IntegerSize index{ 0 }; index < COUNT; ++index) {
            const MAREA_Token *token{ nullptr };

            MAREA_IGNORE(MAREA_TokenListGetAt(list, index, &token));

            result.push_back(MAREA_TokenGetPosition(token));
        }

        return result;
    }

    std::vector<MAREA_Uinteger32> MAREA_diagnosticCodes() const
    {
        std::vector<MAREA_Uinteger32> result;

        const MAREA_IntegerSize COUNT{ MAREA_DiagnosticSinkGetCount(sink) };

        for (MAREA_IntegerSize index{ 0 }; index < COUNT; ++index) {
            const MAREA_Diagnostic *diagnostic{ nullptr };

            MAREA_IGNORE(MAREA_DiagnosticSinkGetAt(sink, index, &diagnostic));

            result.push_back(MAREA_DiagnosticGetCode(diagnostic));
        }

        return result;
    }
};

#endif
