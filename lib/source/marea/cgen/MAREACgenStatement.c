// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/cgen/MAREACgenInternal.h"

void MAREA_CALL MAREA_CgenWriteTrapGuard(MAREA_CgenGenerator *generator)
{
    MAREA_CgenWriteText(generator->buffer, "if(*marea_trap){return");

    if (generator->currentResultKind != MAREA_CGEN_KIND_VOID) {
        MAREA_CgenWriteText(generator->buffer, " ");
        MAREA_CgenWriteDefaultValue(generator->buffer, generator->currentResultKind);
    }

    MAREA_CgenWriteText(generator->buffer, ";}\n");
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteLocal(MAREA_CgenGenerator *generator, const MAREA_IrNode *node)
{
    const MAREA_IrNode *value = MAREA_NULLPTR;

    MAREA_IGNORE(MAREA_IrNodeGetChildAt(node, MAREA_INTEGER_SIZE(0), &value));

    if (value == MAREA_NULLPTR) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A local has no initial value.");
    }

    const MAREA_Uinteger8 KIND = MAREA_CgenKindOfNode(node);

    if ((KIND >= MAREA_CGEN_KIND_NUMBER_COUNT) && (KIND != MAREA_CGEN_KIND_BOOLEAN)) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A local has a type that is not a scalar.");
    }

    MAREA_CgenWriteCType(generator->buffer, KIND);
    MAREA_CgenWriteText(generator->buffer, " ");
    MAREA_CgenWriteMangled(generator->buffer, "v_", MAREA_IrNodeGetText(node), MAREA_IrNodeGetTextLength(node));
    MAREA_CgenWriteText(generator->buffer, "=");

    MAREA_Uinteger8 valueKind = MAREA_CGEN_KIND_NONE;

    MAREA_CGEN_TRY(MAREA_CgenWriteExpression(generator, value, &valueKind));

    MAREA_CgenWriteText(generator->buffer, ";\n");

    // INFO: the name is bound after its value, so the value cannot read the local that it makes.
    return MAREA_CgenPushSymbol(generator, MAREA_IrNodeGetText(node), MAREA_IrNodeGetTextLength(node), KIND);
}

static MAREA_TokenKind MAREA_CALL MAREA_G_PlainOperatorOf(MAREA_TokenKind compound)
{
    switch (compound) {
    case MAREA_TOKEN_KIND_PLUS_EQUAL:
        return MAREA_TOKEN_KIND_PLUS;
    case MAREA_TOKEN_KIND_MINUS_EQUAL:
        return MAREA_TOKEN_KIND_MINUS;
    case MAREA_TOKEN_KIND_STAR_EQUAL:
        return MAREA_TOKEN_KIND_STAR;
    case MAREA_TOKEN_KIND_SLASH_EQUAL:
        return MAREA_TOKEN_KIND_SLASH;
    case MAREA_TOKEN_KIND_PERCENT_EQUAL:
        return MAREA_TOKEN_KIND_PERCENT;
    case MAREA_TOKEN_KIND_AMPERSAND_EQUAL:
        return MAREA_TOKEN_KIND_AMPERSAND;
    case MAREA_TOKEN_KIND_PIPE_EQUAL:
        return MAREA_TOKEN_KIND_PIPE;
    case MAREA_TOKEN_KIND_CARET_EQUAL:
        return MAREA_TOKEN_KIND_CARET;
    case MAREA_TOKEN_KIND_SHIFT_LEFT_EQUAL:
        return MAREA_TOKEN_KIND_SHIFT_LEFT;
    case MAREA_TOKEN_KIND_SHIFT_RIGHT_EQUAL:
        return MAREA_TOKEN_KIND_SHIFT_RIGHT;
    default:
        return MAREA_TOKEN_KIND_ERROR;
    }
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteAssignment(MAREA_CgenGenerator *generator, const MAREA_IrNode *node)
{
    const MAREA_IrNode *target = MAREA_NULLPTR;
    const MAREA_IrNode *value = MAREA_NULLPTR;

    MAREA_IGNORE(MAREA_IrNodeGetChildAt(node, MAREA_INTEGER_SIZE(0), &target));
    MAREA_IGNORE(MAREA_IrNodeGetChildAt(node, MAREA_INTEGER_SIZE(1), &value));

    if ((target == MAREA_NULLPTR) || (value == MAREA_NULLPTR) || (MAREA_IrNodeGetKind(target) != MAREA_IR_KIND_IDENTIFIER)) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "Only a local or a parameter can be assigned yet.");
    }

    const MAREA_CgenSymbol *symbol = MAREA_CgenFindSymbol(generator, MAREA_IrNodeGetText(target), MAREA_IrNodeGetTextLength(target));

    if (symbol == MAREA_NULLPTR) {
        return MAREA_CgenGeneratorFail(generator, target, MAREA_CGEN_DIAGNOSTIC_CODE_UNRESOLVED_NAME, "An assigned name is not a local or a parameter.");
    }

    const MAREA_TokenKind OPERATOR = MAREA_IrNodeGetOperator(node);

    MAREA_CgenWriteMangled(generator->buffer, "v_", symbol->name, symbol->length);
    MAREA_CgenWriteText(generator->buffer, "=");

    if (OPERATOR == MAREA_TOKEN_KIND_EQUAL) {
        MAREA_Uinteger8 valueKind = MAREA_CGEN_KIND_NONE;

        MAREA_CGEN_TRY(MAREA_CgenWriteExpression(generator, value, &valueKind));
        MAREA_CgenWriteText(generator->buffer, ";\n");

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    const MAREA_TokenKind PLAIN = MAREA_G_PlainOperatorOf(OPERATOR);

    if (PLAIN == MAREA_TOKEN_KIND_ERROR) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "An assignment operator is not lowered.");
    }

    const MAREA_Uinteger8 TARGET_KIND = symbol->kind;
    const MAREA_Boolean FLOAT = ((TARGET_KIND == MAREA_CGEN_KIND_FLOAT_32) || (TARGET_KIND == MAREA_CGEN_KIND_FLOAT_64));
    const MAREA_Boolean BITWISE = ((PLAIN == MAREA_TOKEN_KIND_AMPERSAND) || (PLAIN == MAREA_TOKEN_KIND_PIPE) || (PLAIN == MAREA_TOKEN_KIND_CARET));
    const MAREA_Boolean NEEDS_TRAP = ((PLAIN == MAREA_TOKEN_KIND_SLASH) || (PLAIN == MAREA_TOKEN_KIND_PERCENT) || (PLAIN == MAREA_TOKEN_KIND_SHIFT_LEFT) || (PLAIN == MAREA_TOKEN_KIND_SHIFT_RIGHT));

    if ((FLOAT == MAREA_BOOLEAN_TRUE) || (BITWISE == MAREA_BOOLEAN_TRUE)) {
        const MAREA_Character8 *symbolText = "+";

        if (PLAIN == MAREA_TOKEN_KIND_MINUS) {
            symbolText = "-";
        } else if (PLAIN == MAREA_TOKEN_KIND_STAR) {
            symbolText = "*";
        } else if (PLAIN == MAREA_TOKEN_KIND_SLASH) {
            symbolText = "/";
        } else if (PLAIN == MAREA_TOKEN_KIND_AMPERSAND) {
            symbolText = "&";
        } else if (PLAIN == MAREA_TOKEN_KIND_PIPE) {
            symbolText = "|";
        } else if (PLAIN == MAREA_TOKEN_KIND_CARET) {
            symbolText = "^";
        }

        MAREA_CgenWriteText(generator->buffer, "(");
        MAREA_CgenWriteMangled(generator->buffer, "v_", symbol->name, symbol->length);
        MAREA_CgenWriteText(generator->buffer, ")");
        MAREA_CgenWriteText(generator->buffer, symbolText);

        MAREA_Uinteger8 valueKind = MAREA_CGEN_KIND_NONE;

        MAREA_CGEN_TRY(MAREA_CgenWriteExpression(generator, value, &valueKind));
        MAREA_CgenWriteText(generator->buffer, ";\n");

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *name = "add";

    if (PLAIN == MAREA_TOKEN_KIND_MINUS) {
        name = "sub";
    } else if (PLAIN == MAREA_TOKEN_KIND_STAR) {
        name = "mul";
    } else if (PLAIN == MAREA_TOKEN_KIND_SLASH) {
        name = "div";
    } else if (PLAIN == MAREA_TOKEN_KIND_PERCENT) {
        name = "rem";
    } else if (PLAIN == MAREA_TOKEN_KIND_SHIFT_LEFT) {
        name = "shl";
    } else if (PLAIN == MAREA_TOKEN_KIND_SHIFT_RIGHT) {
        name = "shr";
    }

    MAREA_CgenWriteText(generator->buffer, "marea_rt_");
    MAREA_CgenWriteText(generator->buffer, name);
    MAREA_CgenWriteText(generator->buffer, "_");
    MAREA_CgenWriteText(generator->buffer, MAREA_CgenKindSuffix(TARGET_KIND));
    MAREA_CgenWriteText(generator->buffer, "(");
    MAREA_CgenWriteMangled(generator->buffer, "v_", symbol->name, symbol->length);
    MAREA_CgenWriteText(generator->buffer, ",");

    MAREA_Uinteger8 valueKind = MAREA_CGEN_KIND_NONE;

    MAREA_CGEN_TRY(MAREA_CgenWriteExpression(generator, value, &valueKind));

    if (NEEDS_TRAP == MAREA_BOOLEAN_TRUE) {
        MAREA_CgenWriteText(generator->buffer, ",marea_trap");
    }

    MAREA_CgenWriteText(generator->buffer, ");\n");

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteExpressionStatement(MAREA_CgenGenerator *generator, const MAREA_IrNode *node)
{
    const MAREA_IrNode *expression = MAREA_NULLPTR;

    MAREA_IGNORE(MAREA_IrNodeGetChildAt(node, MAREA_INTEGER_SIZE(0), &expression));

    MAREA_Uinteger8 kind = MAREA_CGEN_KIND_NONE;

    MAREA_CGEN_TRY(MAREA_CgenWriteExpression(generator, expression, &kind));

    MAREA_CgenWriteText(generator->buffer, ";\n");

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteReturn(MAREA_CgenGenerator *generator, const MAREA_IrNode *node)
{
    if (MAREA_IrNodeGetChildCount(node) == MAREA_INTEGER_SIZE(0)) {
        MAREA_CgenWriteText(generator->buffer, "return;\n");

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    const MAREA_IrNode *value = MAREA_NULLPTR;

    MAREA_IGNORE(MAREA_IrNodeGetChildAt(node, MAREA_INTEGER_SIZE(0), &value));

    MAREA_CgenWriteText(generator->buffer, "return");

    MAREA_Uinteger8 kind = MAREA_CGEN_KIND_NONE;

    MAREA_CGEN_TRY(MAREA_CgenWriteExpression(generator, value, &kind));

    MAREA_CgenWriteText(generator->buffer, ";\n");

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteIf(MAREA_CgenGenerator *generator, const MAREA_IrNode *node)
{
    const MAREA_IntegerSize COUNT = MAREA_IrNodeGetChildCount(node);
    const MAREA_IrNode *arm = node;
    MAREA_IntegerSize position = MAREA_INTEGER_SIZE(2);

    for (;;) {
        const MAREA_IrNode *condition = MAREA_NULLPTR;
        const MAREA_IrNode *body = MAREA_NULLPTR;

        MAREA_IGNORE(MAREA_IrNodeGetChildAt(arm, MAREA_INTEGER_SIZE(0), &condition));
        MAREA_IGNORE(MAREA_IrNodeGetChildAt(arm, MAREA_INTEGER_SIZE(1), &body));

        MAREA_CgenWriteText(generator->buffer, "if");

        MAREA_Uinteger8 kind = MAREA_CGEN_KIND_NONE;

        MAREA_CGEN_TRY(MAREA_CgenWriteExpression(generator, condition, &kind));

        MAREA_CGEN_TRY(MAREA_CgenWriteBlock(generator, body));

        if (position >= COUNT) {
            break;
        }

        const MAREA_IrNode *clause = MAREA_NULLPTR;

        MAREA_IGNORE(MAREA_IrNodeGetChildAt(node, position, &clause));

        position = (position + MAREA_INTEGER_SIZE(1));

        MAREA_CgenWriteText(generator->buffer, "else ");

        if (MAREA_IrNodeGetKind(clause) == MAREA_IR_KIND_ELSE_CLAUSE) {
            const MAREA_IrNode *elseBody = MAREA_NULLPTR;

            MAREA_IGNORE(MAREA_IrNodeGetChildAt(clause, MAREA_INTEGER_SIZE(0), &elseBody));

            MAREA_CGEN_TRY(MAREA_CgenWriteBlock(generator, elseBody));

            break;
        }

        arm = clause;
    }

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteWhile(MAREA_CgenGenerator *generator, const MAREA_IrNode *node)
{
    const MAREA_IrNode *condition = MAREA_NULLPTR;
    const MAREA_IrNode *body = MAREA_NULLPTR;

    MAREA_IGNORE(MAREA_IrNodeGetChildAt(node, MAREA_INTEGER_SIZE(0), &condition));
    MAREA_IGNORE(MAREA_IrNodeGetChildAt(node, MAREA_INTEGER_SIZE(1), &body));

    MAREA_CgenWriteText(generator->buffer, "while");

    MAREA_Uinteger8 kind = MAREA_CGEN_KIND_NONE;

    MAREA_CGEN_TRY(MAREA_CgenWriteExpression(generator, condition, &kind));

    return MAREA_CgenWriteBlock(generator, body);
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteStatement(MAREA_CgenGenerator *generator, const MAREA_IrNode *node)
{
    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    switch (MAREA_IrNodeGetKind(node)) {
    case MAREA_IR_KIND_LOCAL_DECLARATION:
        return MAREA_G_WriteLocal(generator, node);
    case MAREA_IR_KIND_BLOCK:
        return MAREA_CgenWriteBlock(generator, node);
    case MAREA_IR_KIND_SCOPE_BLOCK:
    {
        const MAREA_IrNode *inner = MAREA_NULLPTR;

        MAREA_IGNORE(MAREA_IrNodeGetChildAt(node, MAREA_INTEGER_SIZE(0), &inner));

        return MAREA_CgenWriteBlock(generator, inner);
    }
    case MAREA_IR_KIND_ASSIGNMENT:
        return MAREA_G_WriteAssignment(generator, node);
    case MAREA_IR_KIND_EXPRESSION_STATEMENT:
        return MAREA_G_WriteExpressionStatement(generator, node);
    case MAREA_IR_KIND_RETURN_STATEMENT:
        return MAREA_G_WriteReturn(generator, node);
    case MAREA_IR_KIND_IF_STATEMENT:
        return MAREA_G_WriteIf(generator, node);
    case MAREA_IR_KIND_WHILE_STATEMENT:
        return MAREA_G_WriteWhile(generator, node);
    case MAREA_IR_KIND_BREAK_STATEMENT:
        MAREA_CgenWriteText(generator->buffer, "break;\n");

        return MAREA_CGEN_EXCEPT_SUCCESS;
    case MAREA_IR_KIND_CONTINUE_STATEMENT:
        MAREA_CgenWriteText(generator->buffer, "continue;\n");

        return MAREA_CGEN_EXCEPT_SUCCESS;
    case MAREA_IR_KIND_UNREACHABLE_STATEMENT:
        MAREA_CgenWriteText(generator->buffer, "__builtin_trap();\n");

        return MAREA_CGEN_EXCEPT_SUCCESS;
    default:
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A statement is not generated yet.");
    }
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteBlock(MAREA_CgenGenerator *generator, const MAREA_IrNode *node)
{
    const MAREA_IntegerSize SYMBOL_MARK = generator->symbolCount;
    const MAREA_IntegerSize COUNT = MAREA_IrNodeGetChildCount(node);

    MAREA_CgenWriteText(generator->buffer, "{\n");

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_IrNode *statement = MAREA_NULLPTR;

        MAREA_IGNORE(MAREA_IrNodeGetChildAt(node, index, &statement));

        MAREA_CGEN_TRY(MAREA_CgenWriteStatement(generator, statement));

        if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
            break;
        }

        const MAREA_IrKind KIND = MAREA_IrNodeGetKind(statement);

        // INFO: a trap can only come from an expression, so the guard follows only the statements that write one.
        if ((KIND == MAREA_IR_KIND_LOCAL_DECLARATION) || (KIND == MAREA_IR_KIND_ASSIGNMENT) || (KIND == MAREA_IR_KIND_EXPRESSION_STATEMENT) || ((KIND == MAREA_IR_KIND_RETURN_STATEMENT) && (MAREA_IrNodeGetChildCount(statement) > MAREA_INTEGER_SIZE(0)))) {
            MAREA_CgenWriteTrapGuard(generator);
        }
    }

    generator->symbolCount = SYMBOL_MARK;

    MAREA_CgenWriteText(generator->buffer, "}\n");

    return MAREA_CGEN_EXCEPT_SUCCESS;
}
