// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_TESTS_MAREAIRHELPERS_HPP)
    #define MAREA_TESTS_MAREAIRHELPERS_HPP

    #include "marea/ir/MAREAIr.h"
    #include "marea/sema/MAREASemaTypedAst.h"

    #include "tests/MAREASemaHelpers.hpp"

    #include <string>
    #include <vector>

struct MAREA_IrFixture
{
    MAREA_SemaFixture sema;
    MAREA_IrContext *context{ nullptr };
    const MAREA_SemaTypedNode *typedRoot{ nullptr };
    const MAREA_IrNode *module{ nullptr };

    MAREA_IrFixture()
    {
        MAREA_IGNORE(MAREA_IrContextCreate(sema.parser.lexer.arena, sema.parser.lexer.sink, &context));
    }

    MAREA_IrFixture(const MAREA_IrFixture &) = delete;
    MAREA_IrFixture &operator=(const MAREA_IrFixture &) = delete;

    MAREA_IrExcept MAREA_lower(const std::string &source, const std::string &filePath = "main.marea")
    {
        if ((sema.MAREA_sema(source, filePath) != MAREA_SEMA_EXCEPT_SUCCESS) || (sema.MAREA_errorCount() != 0)) {
            return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
        }

        if (MAREA_SemaBuildTypedAst(sema.context, sema.parser.root, &typedRoot) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
            return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
        }

        return MAREA_IrLowerCompilationUnit(context, typedRoot, &module);
    }

    std::vector<MAREA_Uinteger32> MAREA_irCodes() const
    {
        std::vector<MAREA_Uinteger32> result;

        for (MAREA_Uinteger32 code : sema.parser.MAREA_codes()) {
            if ((code >= 4000) && (code < 5000)) {
                result.push_back(code);
            }
        }

        return result;
    }
};

#endif
