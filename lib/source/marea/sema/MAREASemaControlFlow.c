// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <string.h>

static const MAREA_AstNode *MAREA_CALL MAREA_G_LastChild(const MAREA_AstNode *node)
{
    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(node);

    return (COUNT == 0) ? MAREA_NULLPTR : MAREA_SemaChildAt(node, (COUNT - 1));
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsError(const MAREA_AstNode *node)
{
    return ((node == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(node) == MAREA_AST_KIND_ERROR_NODE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

typedef enum MAREA_G_ScrutineeClass
{
    MAREA_G_SCRUTINEE_CLASS_UNKNOWN = MAREA_UINTEGER_8(0),
    MAREA_G_SCRUTINEE_CLASS_ENUM = MAREA_UINTEGER_8(1),
    MAREA_G_SCRUTINEE_CLASS_OTHER = MAREA_UINTEGER_8(2),
} MAREA_G_ScrutineeClass;

static MAREA_G_ScrutineeClass MAREA_CALL MAREA_G_ClassifyScrutinee(const MAREA_SemaContext *context, const MAREA_AstNode *scrutinee, const MAREA_AstNode **outEnum)
{
    *outEnum = MAREA_NULLPTR;

    const MAREA_AstNode *type = MAREA_SemaInferType(context, scrutinee);

    if (type == MAREA_NULLPTR) {
        return MAREA_G_SCRUTINEE_CLASS_UNKNOWN;
    }

    const MAREA_AstNode *declaration = MAREA_NULLPTR;

    const MAREA_Character8 *name = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    const MAREA_SemaTypeClass CLASS = MAREA_SemaClassifyType(context, type, &declaration, &name, &length);

    if (CLASS == MAREA_SEMA_TYPE_CLASS_EXTERNAL) {
        return MAREA_G_SCRUTINEE_CLASS_UNKNOWN;
    }

    if ((CLASS == MAREA_SEMA_TYPE_CLASS_USER) && (MAREA_AstNodeGetKind(declaration) == MAREA_AST_KIND_ENUM_DECLARATION)) {
        *outEnum = declaration;

        return MAREA_G_SCRUTINEE_CLASS_ENUM;
    }

    return MAREA_G_SCRUTINEE_CLASS_OTHER;
}

static MAREA_Boolean MAREA_CALL MAREA_G_Terminates(const MAREA_SemaContext *context, const MAREA_AstNode *statement);

static MAREA_Boolean MAREA_CALL MAREA_G_HasError(const MAREA_AstNode *node)
{
    if (MAREA_G_IsError(node) == MAREA_BOOLEAN_TRUE) {
        return MAREA_BOOLEAN_TRUE;
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(node); ++index) {
        if (MAREA_G_HasError(MAREA_SemaChildAt(node, index)) == MAREA_BOOLEAN_TRUE) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_BlockTerminates(const MAREA_SemaContext *context, const MAREA_AstNode *block)
{
    if (MAREA_G_IsError(block) == MAREA_BOOLEAN_TRUE) {
        return MAREA_BOOLEAN_TRUE;
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(block); ++index) {
        if (MAREA_G_Terminates(context, MAREA_SemaChildAt(block, index)) == MAREA_BOOLEAN_TRUE) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IfTerminates(const MAREA_SemaContext *context, const MAREA_AstNode *statement)
{
    MAREA_Boolean hasElse = MAREA_BOOLEAN_FALSE;

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(statement); ++index) {
        const MAREA_AstNode *child = MAREA_SemaChildAt(statement, index);

        if (MAREA_G_IsError(child) == MAREA_BOOLEAN_TRUE) {
            return MAREA_BOOLEAN_TRUE;
        }

        const MAREA_AstKind KIND = MAREA_AstNodeGetKind(child);

        if ((KIND == MAREA_AST_KIND_BLOCK) && (MAREA_G_BlockTerminates(context, child) == MAREA_BOOLEAN_FALSE)) {
            return MAREA_BOOLEAN_FALSE;
        }

        if (KIND == MAREA_AST_KIND_ELSEIF_CLAUSE) {
            if (MAREA_G_BlockTerminates(context, MAREA_G_LastChild(child)) == MAREA_BOOLEAN_FALSE) {
                return MAREA_BOOLEAN_FALSE;
            }
        }

        if (KIND == MAREA_AST_KIND_ELSE_CLAUSE) {
            hasElse = MAREA_BOOLEAN_TRUE;

            if (MAREA_G_BlockTerminates(context, MAREA_SemaChildAt(child, 0)) == MAREA_BOOLEAN_FALSE) {
                return MAREA_BOOLEAN_FALSE;
            }
        }
    }

    return hasElse;
}

// INFO: a switch over an enum that names too few variants counts as ending, because NON_EXHAUSTIVE_SWITCH already reports it.
static MAREA_Boolean MAREA_CALL MAREA_G_SwitchTerminates(const MAREA_SemaContext *context, const MAREA_AstNode *statement)
{
    const MAREA_AstNode *enumeration = MAREA_NULLPTR;

    const MAREA_G_ScrutineeClass CLASS = MAREA_G_ClassifyScrutinee(context, MAREA_SemaChildAt(statement, 0), &enumeration);

    MAREA_Boolean hasDefault = MAREA_BOOLEAN_FALSE;

    for (MAREA_IntegerSize index = 1; index < MAREA_AstNodeGetChildCount(statement); ++index) {
        const MAREA_AstNode *arm = MAREA_SemaChildAt(statement, index);

        if (MAREA_G_IsError(arm) == MAREA_BOOLEAN_TRUE) {
            return MAREA_BOOLEAN_TRUE;
        }

        if (MAREA_G_BlockTerminates(context, MAREA_G_LastChild(arm)) == MAREA_BOOLEAN_FALSE) {
            return MAREA_BOOLEAN_FALSE;
        }

        if (MAREA_AstNodeGetKind(arm) == MAREA_AST_KIND_DEFAULT_ARM) {
            hasDefault = MAREA_BOOLEAN_TRUE;
        }
    }

    return ((hasDefault == MAREA_BOOLEAN_TRUE) || (CLASS != MAREA_G_SCRUTINEE_CLASS_OTHER)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_MacroDiverges(const MAREA_AstNode *statement)
{
    const MAREA_AstNode *call = MAREA_G_LastChild(statement);

    const MAREA_AstNode *name = ((call != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(call) == MAREA_AST_KIND_MACRO_CALL)) ? MAREA_SemaChildAt(call, 0) : MAREA_NULLPTR;

    if (name == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_IntegerSize LENGTH = MAREA_AstNodeGetTextLength(name);

    // INFO: unreachable!() and compile_error!() never give control back, so they end a path like a return.
    return (((LENGTH == 11) && (memcmp(MAREA_AstNodeGetText(name), "unreachable", 11) == 0)) || ((LENGTH == 13) && (memcmp(MAREA_AstNodeGetText(name), "compile_error", 13) == 0))) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_Terminates(const MAREA_SemaContext *context, const MAREA_AstNode *statement)
{
    // INFO: a statement that the parser could not build is given the benefit of the doubt, so no diagnostic lands on it.
    if (MAREA_G_HasError(statement) == MAREA_BOOLEAN_TRUE) {
        return MAREA_BOOLEAN_TRUE;
    }

    switch (MAREA_AstNodeGetKind(statement)) {
    case MAREA_AST_KIND_RETURN_STATEMENT:
        return (MAREA_AstNodeGetChildCount(statement) > 0) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    case MAREA_AST_KIND_MACRO_STATEMENT:
        return MAREA_G_MacroDiverges(statement);
    case MAREA_AST_KIND_IF_STATEMENT:
        return MAREA_G_IfTerminates(context, statement);
    case MAREA_AST_KIND_SWITCH_STATEMENT:
        return MAREA_G_SwitchTerminates(context, statement);
    case MAREA_AST_KIND_SCOPE_BLOCK:
    case MAREA_AST_KIND_UNSAFE_BLOCK:
    case MAREA_AST_KIND_FORK_JOIN_BLOCK:
        return MAREA_G_BlockTerminates(context, MAREA_SemaChildAt(statement, 0));
    case MAREA_AST_KIND_WITH_STATEMENT:
        // INFO: a with statement ends a path the same way a scope block does, through its own body, the last child here (the value and the bound name come first).
        return MAREA_G_BlockTerminates(context, MAREA_G_LastChild(statement));
    default:
        return MAREA_BOOLEAN_FALSE;
    }
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckFunction(MAREA_SemaContext *context, const MAREA_AstNode *function)
{
    if (MAREA_SemaReturnType(function) == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *body = MAREA_G_LastChild(function);

    if ((body == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(body) != MAREA_AST_KIND_BLOCK) || (MAREA_G_BlockTerminates(context, body) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *name = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    if (MAREA_SemaDeclarationName(function, &name, &length) == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_MISSING_RETURN, MAREA_AstNodeGetPosition(function), "not every path returns a value in", name, length);
}

static const MAREA_AstNode *MAREA_CALL MAREA_G_CaseName(const MAREA_AstNode *value)
{
    if (MAREA_G_IsError(value) == MAREA_BOOLEAN_TRUE) {
        return MAREA_NULLPTR;
    }

    const MAREA_AstKind KIND = MAREA_AstNodeGetKind(value);

    if (KIND == MAREA_AST_KIND_IDENTIFIER) {
        return value;
    }

    if (KIND == MAREA_AST_KIND_MEMBER_ACCESS) {
        return MAREA_SemaChildAt(value, 1);
    }

    return MAREA_NULLPTR;
}

static MAREA_Boolean MAREA_CALL MAREA_G_SameText(const MAREA_AstNode *left, const MAREA_AstNode *right)
{
    return ((MAREA_AstNodeGetTextLength(left) == MAREA_AstNodeGetTextLength(right)) && (memcmp(MAREA_AstNodeGetText(left), MAREA_AstNodeGetText(right), MAREA_AstNodeGetTextLength(left)) == 0)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsCovered(const MAREA_AstNode *statement, const MAREA_AstNode *variant)
{
    for (MAREA_IntegerSize index = 1; index < MAREA_AstNodeGetChildCount(statement); ++index) {
        const MAREA_AstNode *arm = MAREA_SemaChildAt(statement, index);

        if ((MAREA_G_IsError(arm) == MAREA_BOOLEAN_TRUE) || (MAREA_AstNodeGetKind(arm) != MAREA_AST_KIND_SWITCH_ARM)) {
            continue;
        }

        for (MAREA_IntegerSize valueIndex = 0; (valueIndex + 1) < MAREA_AstNodeGetChildCount(arm); ++valueIndex) {
            const MAREA_AstNode *name = MAREA_G_CaseName(MAREA_SemaChildAt(arm, valueIndex));

            if ((name != MAREA_NULLPTR) && (MAREA_G_SameText(name, variant) == MAREA_BOOLEAN_TRUE)) {
                return MAREA_BOOLEAN_TRUE;
            }
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

#define MAREA_G_CONSTANT_DEPTH_LIMIT MAREA_INTEGER_SIZE(16)

// INFO: reads an integer literal of any base (decimal, 0x, 0b, with "_" separators); false when it does not fit 64 bits.
static MAREA_Boolean MAREA_CALL MAREA_G_ReadInteger(const MAREA_AstNode *node, MAREA_Uinteger64 *outValue)
{
    const MAREA_Character8 *text = MAREA_AstNodeGetText(node);

    const MAREA_IntegerSize LENGTH = MAREA_AstNodeGetTextLength(node);

    MAREA_IntegerSize index = 0;

    MAREA_Uinteger64 base = 10;

    if ((LENGTH > 2) && (text[0] == '0') && ((text[1] == 'x') || (text[1] == 'X'))) {
        base = 16;
        index = 2;
    } else if ((LENGTH > 2) && (text[0] == '0') && ((text[1] == 'b') || (text[1] == 'B'))) {
        base = 2;
        index = 2;
    }

    MAREA_Uinteger64 value = 0;

    MAREA_Boolean any = MAREA_BOOLEAN_FALSE;

    for (; index < LENGTH; ++index) {
        const MAREA_Character8 LETTER = text[index];

        if (LETTER == '_') {
            continue;
        }

        MAREA_Uinteger64 digit = 0;

        if ((LETTER >= '0') && (LETTER <= '9')) {
            digit = (MAREA_Uinteger64)(LETTER - '0');
        } else if ((base == 16) && (LETTER >= 'a') && (LETTER <= 'f')) {
            digit = (MAREA_Uinteger64)((LETTER - 'a') + 10);
        } else if ((base == 16) && (LETTER >= 'A') && (LETTER <= 'F')) {
            digit = (MAREA_Uinteger64)((LETTER - 'A') + 10);
        } else {
            return MAREA_BOOLEAN_FALSE;
        }

        if ((digit >= base) || (value > ((MAREA_UINTEGER_64_MAXIMUM - digit) / base))) {
            return MAREA_BOOLEAN_FALSE;
        }

        value = ((value * base) + digit);
        any = MAREA_BOOLEAN_TRUE;
    }

    *outValue = value;

    return any;
}

// INFO: a name of a constant of this file stands for the value it is declared with; a local of the same name hides it.
static const MAREA_AstNode *MAREA_CALL MAREA_G_ResolveConstant(const MAREA_SemaContext *context, const MAREA_AstNode *value)
{
    const MAREA_AstNode *current = value;

    for (MAREA_IntegerSize depth = 0; depth < MAREA_G_CONSTANT_DEPTH_LIMIT; ++depth) {
        if ((MAREA_G_IsError(current) == MAREA_BOOLEAN_TRUE) || (MAREA_AstNodeGetKind(current) != MAREA_AST_KIND_IDENTIFIER)) {
            return current;
        }

        if (MAREA_SemaLocalFind(context, MAREA_AstNodeGetText(current), MAREA_AstNodeGetTextLength(current)) != MAREA_NULLPTR) {
            return current;
        }

        const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, MAREA_AstNodeGetText(current), MAREA_AstNodeGetTextLength(current));

        if ((entry == MAREA_NULLPTR) || (entry->kind != MAREA_SEMA_NAME_KIND_DECLARATION) || (entry->declaration == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(entry->declaration) != MAREA_AST_KIND_CONSTANT_DECLARATION)) {
            return current;
        }

        const MAREA_AstNode *initializer = MAREA_SemaChildAt(entry->declaration, 2);

        if (MAREA_G_IsError(initializer) == MAREA_BOOLEAN_TRUE) {
            return current;
        }

        current = initializer;
    }

    return current;
}

// INFO: a qualified name is compared by its path, because its last name belongs to another namespace and never to a constant of this file.
static MAREA_Boolean MAREA_CALL MAREA_G_SamePath(const MAREA_AstNode *left, const MAREA_AstNode *right)
{
    if ((MAREA_G_IsError(left) == MAREA_BOOLEAN_TRUE) || (MAREA_G_IsError(right) == MAREA_BOOLEAN_TRUE) || (MAREA_AstNodeGetKind(left) != MAREA_AstNodeGetKind(right))) {
        return MAREA_BOOLEAN_FALSE;
    }

    switch (MAREA_AstNodeGetKind(left)) {
    case MAREA_AST_KIND_IDENTIFIER:
        return MAREA_G_SameText(left, right);
    case MAREA_AST_KIND_MEMBER_ACCESS:
        return ((MAREA_G_SamePath(MAREA_SemaChildAt(left, 0), MAREA_SemaChildAt(right, 0)) == MAREA_BOOLEAN_TRUE) && (MAREA_G_SamePath(MAREA_SemaChildAt(left, 1), MAREA_SemaChildAt(right, 1)) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    default:
        return MAREA_BOOLEAN_FALSE;
    }
}

// INFO: two case values are the same when they hold one value: integer literals by number in any base, a constant of this file by the value it is declared with, any other literal, name or unary operator by text.
static MAREA_Boolean MAREA_CALL MAREA_G_SameValue(const MAREA_SemaContext *context, const MAREA_AstNode *left, const MAREA_AstNode *right)
{
    if ((MAREA_G_IsError(left) == MAREA_BOOLEAN_TRUE) || (MAREA_G_IsError(right) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *leftValue = MAREA_G_ResolveConstant(context, left);

    const MAREA_AstNode *rightValue = MAREA_G_ResolveConstant(context, right);

    const MAREA_AstKind KIND = MAREA_AstNodeGetKind(leftValue);

    if (KIND != MAREA_AstNodeGetKind(rightValue)) {
        return MAREA_BOOLEAN_FALSE;
    }

    switch (KIND) {
    case MAREA_AST_KIND_INTEGER_LITERAL:
    {
        MAREA_Uinteger64 leftNumber = 0;

        MAREA_Uinteger64 rightNumber = 0;

        if ((MAREA_G_ReadInteger(leftValue, &leftNumber) == MAREA_BOOLEAN_TRUE) && (MAREA_G_ReadInteger(rightValue, &rightNumber) == MAREA_BOOLEAN_TRUE)) {
            return (leftNumber == rightNumber) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
        }

        return MAREA_G_SameText(leftValue, rightValue);
    }
    case MAREA_AST_KIND_IDENTIFIER:
    case MAREA_AST_KIND_FLOAT_LITERAL:
    case MAREA_AST_KIND_STRING_LITERAL:
    case MAREA_AST_KIND_CHARACTER_LITERAL:
    case MAREA_AST_KIND_BOOLEAN_LITERAL:
        return MAREA_G_SameText(leftValue, rightValue);
    case MAREA_AST_KIND_UNARY_EXPRESSION:
        return ((MAREA_AstNodeGetOperator(leftValue) == MAREA_AstNodeGetOperator(rightValue)) && (MAREA_G_SameValue(context, MAREA_SemaChildAt(leftValue, 0), MAREA_SemaChildAt(rightValue, 0)) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    case MAREA_AST_KIND_MEMBER_ACCESS:
        return MAREA_G_SamePath(leftValue, rightValue);
    default:
        return MAREA_BOOLEAN_FALSE;
    }
}

// INFO: over an enum, "red" and "Color.red" name one variant, the same way the coverage check reads them.
static MAREA_Boolean MAREA_CALL MAREA_G_SameCase(const MAREA_SemaContext *context, const MAREA_AstNode *left, const MAREA_AstNode *right, MAREA_Boolean isEnum)
{
    if (MAREA_G_SameValue(context, left, right) == MAREA_BOOLEAN_TRUE) {
        return MAREA_BOOLEAN_TRUE;
    }

    const MAREA_AstNode *leftName = (isEnum == MAREA_BOOLEAN_TRUE) ? MAREA_G_CaseName(left) : MAREA_NULLPTR;

    const MAREA_AstNode *rightName = (isEnum == MAREA_BOOLEAN_TRUE) ? MAREA_G_CaseName(right) : MAREA_NULLPTR;

    return ((leftName != MAREA_NULLPTR) && (rightName != MAREA_NULLPTR)) ? MAREA_G_SameText(leftName, rightName) : MAREA_BOOLEAN_FALSE;
}

// INFO: an arm and its values are numbered in source order; only a value or a default that comes before the one in hand counts as seen.
static MAREA_Boolean MAREA_CALL MAREA_G_ValueSeenBefore(const MAREA_SemaContext *context, const MAREA_AstNode *statement, MAREA_IntegerSize armIndex, MAREA_IntegerSize valueIndex, MAREA_Boolean isEnum)
{
    const MAREA_AstNode *value = MAREA_SemaChildAt(MAREA_SemaChildAt(statement, armIndex), valueIndex);

    for (MAREA_IntegerSize index = 1; index <= armIndex; ++index) {
        const MAREA_AstNode *arm = MAREA_SemaChildAt(statement, index);

        if ((MAREA_G_IsError(arm) == MAREA_BOOLEAN_TRUE) || (MAREA_AstNodeGetKind(arm) != MAREA_AST_KIND_SWITCH_ARM)) {
            continue;
        }

        const MAREA_IntegerSize LIMIT = (index == armIndex) ? valueIndex : (MAREA_AstNodeGetChildCount(arm) - 1);

        for (MAREA_IntegerSize earlier = 0; earlier < LIMIT; ++earlier) {
            if (MAREA_G_SameCase(context, MAREA_SemaChildAt(arm, earlier), value, isEnum) == MAREA_BOOLEAN_TRUE) {
                return MAREA_BOOLEAN_TRUE;
            }
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

static void MAREA_CALL MAREA_G_CheckDuplicateArms(MAREA_SemaContext *context, const MAREA_AstNode *statement, MAREA_Boolean isEnum)
{
    MAREA_Boolean hasDefault = MAREA_BOOLEAN_FALSE;

    for (MAREA_IntegerSize index = 1; index < MAREA_AstNodeGetChildCount(statement); ++index) {
        const MAREA_AstNode *arm = MAREA_SemaChildAt(statement, index);

        if (MAREA_G_IsError(arm) == MAREA_BOOLEAN_TRUE) {
            continue;
        }

        if (MAREA_AstNodeGetKind(arm) == MAREA_AST_KIND_DEFAULT_ARM) {
            if (hasDefault == MAREA_BOOLEAN_TRUE) {
                MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_DUPLICATE_SWITCH_ARM, MAREA_AstNodeGetPosition(arm), "switch has more than one default arm");
            }

            hasDefault = MAREA_BOOLEAN_TRUE;

            continue;
        }

        for (MAREA_IntegerSize valueIndex = 0; (valueIndex + 1) < MAREA_AstNodeGetChildCount(arm); ++valueIndex) {
            if (MAREA_G_ValueSeenBefore(context, statement, index, valueIndex, isEnum) == MAREA_BOOLEAN_TRUE) {
                MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_DUPLICATE_SWITCH_ARM, MAREA_AstNodeGetPosition(MAREA_SemaChildAt(arm, valueIndex)), "switch repeats a case value");
            }
        }
    }
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckSwitch(MAREA_SemaContext *context, const MAREA_AstNode *statement)
{
    const MAREA_AstNode *enumeration = MAREA_NULLPTR;

    const MAREA_G_ScrutineeClass CLASS = MAREA_G_ClassifyScrutinee(context, MAREA_SemaChildAt(statement, 0), &enumeration);

    MAREA_G_CheckDuplicateArms(context, statement, (CLASS == MAREA_G_SCRUTINEE_CLASS_ENUM) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE);

    if (CLASS != MAREA_G_SCRUTINEE_CLASS_ENUM) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    for (MAREA_IntegerSize index = 1; index < MAREA_AstNodeGetChildCount(statement); ++index) {
        const MAREA_AstNode *arm = MAREA_SemaChildAt(statement, index);

        if (MAREA_G_IsError(arm) == MAREA_BOOLEAN_TRUE) {
            return MAREA_SEMA_EXCEPT_SUCCESS;
        }

        if (MAREA_AstNodeGetKind(arm) == MAREA_AST_KIND_DEFAULT_ARM) {
            return MAREA_SEMA_EXCEPT_SUCCESS;
        }
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(enumeration); ++index) {
        const MAREA_AstNode *variant = MAREA_SemaChildAt(enumeration, index);

        const MAREA_AstNode *name = ((variant != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(variant) == MAREA_AST_KIND_VARIANT_DECLARATION)) ? MAREA_SemaChildAt(variant, 0) : MAREA_NULLPTR;

        if ((MAREA_G_IsError(name) == MAREA_BOOLEAN_TRUE) || (MAREA_G_IsCovered(statement, name) == MAREA_BOOLEAN_TRUE)) {
            continue;
        }

        return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_NON_EXHAUSTIVE_SWITCH, MAREA_AstNodeGetPosition(statement), "switch does not handle the variant", MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name));
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckControlFlow(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    switch (MAREA_AstNodeGetKind(node)) {
    case MAREA_AST_KIND_FUNCTION_DECLARATION:
        return MAREA_G_CheckFunction(context, node);
    case MAREA_AST_KIND_SWITCH_STATEMENT:
        return MAREA_G_CheckSwitch(context, node);
    default:
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }
}
