// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/parse/MAREAParseInternal.h"

MAREA_AstNode *MAREA_CALL MAREA_ParseBlock(MAREA_ParseState *state)
{
    MAREA_AstNode *block = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_BLOCK);

    MAREA_ParseNodeEnd(state, block);

    return block;
}
