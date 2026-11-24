// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <string.h>

// INFO: the argument shape of a built-in macro. A text is a string literal, a condition is any expression, a value is any expression.
typedef enum MAREA_G_MacroShape
{
    MAREA_G_MACRO_SHAPE_CONDITION_AND_TEXT = MAREA_UINTEGER_8(0),
    MAREA_G_MACRO_SHAPE_NONE = MAREA_UINTEGER_8(1),
    MAREA_G_MACRO_SHAPE_TEXT = MAREA_UINTEGER_8(2),
    MAREA_G_MACRO_SHAPE_FORMAT = MAREA_UINTEGER_8(3),
    MAREA_G_MACRO_SHAPE_LEVEL_AND_FORMAT = MAREA_UINTEGER_8(4),
} MAREA_G_MacroShape;

// INFO: the one table of the built-in macros. examples/macros/builtin.marea is its source of truth.
typedef struct MAREA_G_MacroRule
{
    const MAREA_Character8 *name;
    MAREA_G_MacroShape shape;
    MAREA_SemaMacroValue value;
} MAREA_G_MacroRule;

// INFO: include_bytes gives c.array<uinteger8> and line gives a number, but sema does not type them yet, so their value is unknown.
static const MAREA_G_MacroRule MAREA_G_MACROS[] = {
    { "assert", MAREA_G_MACRO_SHAPE_CONDITION_AND_TEXT, MAREA_SEMA_MACRO_VALUE_NONE },
    { "debug_assert", MAREA_G_MACRO_SHAPE_CONDITION_AND_TEXT, MAREA_SEMA_MACRO_VALUE_NONE },
    { "unreachable", MAREA_G_MACRO_SHAPE_NONE, MAREA_SEMA_MACRO_VALUE_NONE },
    { "compile_error", MAREA_G_MACRO_SHAPE_TEXT, MAREA_SEMA_MACRO_VALUE_NONE },
    { "static_assert", MAREA_G_MACRO_SHAPE_CONDITION_AND_TEXT, MAREA_SEMA_MACRO_VALUE_NONE },
    { "include_bytes", MAREA_G_MACRO_SHAPE_TEXT, MAREA_SEMA_MACRO_VALUE_UNKNOWN },
    { "include_text", MAREA_G_MACRO_SHAPE_TEXT, MAREA_SEMA_MACRO_VALUE_STRING },
    { "format", MAREA_G_MACRO_SHAPE_FORMAT, MAREA_SEMA_MACRO_VALUE_STRING },
    { "print", MAREA_G_MACRO_SHAPE_FORMAT, MAREA_SEMA_MACRO_VALUE_NONE },
    { "log", MAREA_G_MACRO_SHAPE_LEVEL_AND_FORMAT, MAREA_SEMA_MACRO_VALUE_NONE },
    { "line", MAREA_G_MACRO_SHAPE_NONE, MAREA_SEMA_MACRO_VALUE_UNKNOWN },
    { "file", MAREA_G_MACRO_SHAPE_NONE, MAREA_SEMA_MACRO_VALUE_STRING },
    { "namespace", MAREA_G_MACRO_SHAPE_NONE, MAREA_SEMA_MACRO_VALUE_STRING },
};

// INFO: the log levels. The language docs do not list them, so this list is an assumption that the user must confirm.
static const MAREA_Character8 *const MAREA_G_LOG_LEVELS[] = { "trace", "debug", "info", "warning", "error" };

static const MAREA_G_MacroRule *MAREA_CALL MAREA_G_FindRule(const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    for (MAREA_IntegerSize index = 0; index < MAREA_ARRAY_SIZE(MAREA_G_MACROS); ++index) {
        if (MAREA_SemaTextEquals(text, length, MAREA_G_MACROS[index].name, strlen(MAREA_G_MACROS[index].name)) == MAREA_BOOLEAN_TRUE) {
            return &MAREA_G_MACROS[index];
        }
    }

    return MAREA_NULLPTR;
}

MAREA_Boolean MAREA_CALL MAREA_SemaIsBuiltInMacroName(const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    return ((length > 0) && (text != MAREA_NULLPTR) && (MAREA_G_FindRule(text, length) != MAREA_NULLPTR)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

MAREA_SemaMacroValue MAREA_CALL MAREA_SemaMacroValueOf(const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    const MAREA_G_MacroRule *rule = ((length > 0) && (text != MAREA_NULLPTR)) ? MAREA_G_FindRule(text, length) : MAREA_NULLPTR;

    return (rule == MAREA_NULLPTR) ? MAREA_SEMA_MACRO_VALUE_UNKNOWN : rule->value;
}

// INFO: the message is "macro 'name' rest", built in the arena of the context.
static MAREA_SemaExcept MAREA_CALL MAREA_G_ReportArgument(MAREA_SemaContext *context, MAREA_SourcePosition position, const MAREA_AstNode *name, const MAREA_Character8 *rest)
{
    static const MAREA_Character8 PREFIX[] = "macro '";

    const MAREA_IntegerSize PREFIX_LENGTH = (sizeof(PREFIX) - 1);

    const MAREA_IntegerSize NAME_LENGTH = MAREA_AstNodeGetTextLength(name);

    const MAREA_IntegerSize REST_LENGTH = strlen(rest);

    if (NAME_LENGTH > (MAREA_INTEGER_SIZE_MAXIMUM - PREFIX_LENGTH - REST_LENGTH - MAREA_INTEGER_SIZE(3))) {
        return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(context->arena, (PREFIX_LENGTH + NAME_LENGTH + REST_LENGTH + MAREA_INTEGER_SIZE(3)), MAREA_ALIGNOF(MAREA_Character8), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_Character8 *message = memory;

    memcpy(message, PREFIX, PREFIX_LENGTH);
    memcpy((message + PREFIX_LENGTH), MAREA_AstNodeGetText(name), NAME_LENGTH);
    message[(PREFIX_LENGTH + NAME_LENGTH)] = '\'';
    message[(PREFIX_LENGTH + NAME_LENGTH + MAREA_INTEGER_SIZE(1))] = ' ';
    memcpy((message + PREFIX_LENGTH + NAME_LENGTH + MAREA_INTEGER_SIZE(2)), rest, (REST_LENGTH + MAREA_INTEGER_SIZE(1)));

    MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_MACRO_ARGUMENT, position, message);

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

static MAREA_Boolean MAREA_CALL MAREA_G_HasErrorNode(const MAREA_AstNode *list)
{
    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(list); ++index) {
        const MAREA_AstNode *argument = MAREA_SemaChildAt(list, index);

        if ((argument != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(argument) == MAREA_AST_KIND_ERROR_NODE)) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

// INFO: a text is rejected only when it is a literal of another kind. Any other expression has an unknown type and is accepted.
static MAREA_Boolean MAREA_CALL MAREA_G_IsWrongText(const MAREA_AstNode *argument)
{
    switch (MAREA_AstNodeGetKind(argument)) {
    case MAREA_AST_KIND_INTEGER_LITERAL:
    case MAREA_AST_KIND_FLOAT_LITERAL:
    case MAREA_AST_KIND_BOOLEAN_LITERAL:
    case MAREA_AST_KIND_CHARACTER_LITERAL:
        return MAREA_BOOLEAN_TRUE;
    default:
        return MAREA_BOOLEAN_FALSE;
    }
}

// INFO: a condition is rejected only when it is a number, string or character literal.
static MAREA_Boolean MAREA_CALL MAREA_G_IsWrongCondition(const MAREA_AstNode *argument)
{
    switch (MAREA_AstNodeGetKind(argument)) {
    case MAREA_AST_KIND_INTEGER_LITERAL:
    case MAREA_AST_KIND_FLOAT_LITERAL:
    case MAREA_AST_KIND_STRING_LITERAL:
    case MAREA_AST_KIND_CHARACTER_LITERAL:
        return MAREA_BOOLEAN_TRUE;
    default:
        return MAREA_BOOLEAN_FALSE;
    }
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsLogLevel(const MAREA_AstNode *argument)
{
    if (MAREA_AstNodeGetKind(argument) != MAREA_AST_KIND_IDENTIFIER) {
        return MAREA_BOOLEAN_FALSE;
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_ARRAY_SIZE(MAREA_G_LOG_LEVELS); ++index) {
        if (MAREA_SemaTextEquals(MAREA_AstNodeGetText(argument), MAREA_AstNodeGetTextLength(argument), MAREA_G_LOG_LEVELS[index], strlen(MAREA_G_LOG_LEVELS[index])) == MAREA_BOOLEAN_TRUE) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

// INFO: counts the two-character text {} in a string literal. A lone { or } is skipped.
static MAREA_IntegerSize MAREA_CALL MAREA_G_CountPlaceholders(const MAREA_AstNode *text)
{
    const MAREA_Character8 *characters = MAREA_AstNodeGetText(text);

    const MAREA_IntegerSize LENGTH = MAREA_AstNodeGetTextLength(text);

    MAREA_IntegerSize count = 0;

    for (MAREA_IntegerSize index = 0; ((index + 1) < LENGTH); ++index) {
        if ((characters[index] == '{') && (characters[(index + 1)] == '}')) {
            ++count;
            ++index;
        }
    }

    return count;
}

// INFO: checks the text at first and the values after it. The count of {} is compared only when the text is a string literal.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckFormat(MAREA_SemaContext *context, const MAREA_AstNode *name, const MAREA_AstNode *list, MAREA_IntegerSize first)
{
    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(list);

    if (COUNT <= first) {
        return MAREA_G_ReportArgument(context, MAREA_AstNodeGetPosition(list), name, "needs a format text");
    }

    const MAREA_AstNode *text = MAREA_SemaChildAt(list, first);

    if (MAREA_G_IsWrongText(text) == MAREA_BOOLEAN_TRUE) {
        return MAREA_G_ReportArgument(context, MAREA_AstNodeGetPosition(text), name, "needs a string literal as the format text");
    }

    if ((MAREA_AstNodeGetKind(text) == MAREA_AST_KIND_STRING_LITERAL) && (MAREA_G_CountPlaceholders(text) != (COUNT - first - 1))) {
        return MAREA_G_ReportArgument(context, MAREA_AstNodeGetPosition(text), name, "has a count of {} that differs from the count of values");
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckShape(MAREA_SemaContext *context, const MAREA_AstNode *name, const MAREA_AstNode *list, MAREA_G_MacroShape shape)
{
    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(list);

    switch (shape) {
    case MAREA_G_MACRO_SHAPE_NONE:
        return (COUNT == 0) ? MAREA_SEMA_EXCEPT_SUCCESS : MAREA_G_ReportArgument(context, MAREA_AstNodeGetPosition(MAREA_SemaChildAt(list, 0)), name, "takes no argument");
    case MAREA_G_MACRO_SHAPE_TEXT:
        if (COUNT != 1) {
            return MAREA_G_ReportArgument(context, MAREA_AstNodeGetPosition(list), name, "takes exactly one string literal");
        }

        return (MAREA_G_IsWrongText(MAREA_SemaChildAt(list, 0)) == MAREA_BOOLEAN_TRUE) ? MAREA_G_ReportArgument(context, MAREA_AstNodeGetPosition(MAREA_SemaChildAt(list, 0)), name, "takes a string literal") : MAREA_SEMA_EXCEPT_SUCCESS;
    case MAREA_G_MACRO_SHAPE_CONDITION_AND_TEXT:
        if (COUNT != 2) {
            return MAREA_G_ReportArgument(context, MAREA_AstNodeGetPosition(list), name, "takes a condition and a string literal message");
        }

        if (MAREA_G_IsWrongCondition(MAREA_SemaChildAt(list, 0)) == MAREA_BOOLEAN_TRUE) {
            return MAREA_G_ReportArgument(context, MAREA_AstNodeGetPosition(MAREA_SemaChildAt(list, 0)), name, "needs a condition that is not a literal of another type");
        }

        return (MAREA_G_IsWrongText(MAREA_SemaChildAt(list, 1)) == MAREA_BOOLEAN_TRUE) ? MAREA_G_ReportArgument(context, MAREA_AstNodeGetPosition(MAREA_SemaChildAt(list, 1)), name, "needs a string literal message") : MAREA_SEMA_EXCEPT_SUCCESS;
    case MAREA_G_MACRO_SHAPE_FORMAT:
        return MAREA_G_CheckFormat(context, name, list, 0);
    case MAREA_G_MACRO_SHAPE_LEVEL_AND_FORMAT:
        if (COUNT < 2) {
            return MAREA_G_ReportArgument(context, MAREA_AstNodeGetPosition(list), name, "needs a log level and a format text");
        }

        if (MAREA_G_IsLogLevel(MAREA_SemaChildAt(list, 0)) == MAREA_BOOLEAN_FALSE) {
            return MAREA_G_ReportArgument(context, MAREA_AstNodeGetPosition(MAREA_SemaChildAt(list, 0)), name, "needs one of the levels trace, debug, info, warning or error");
        }

        return MAREA_G_CheckFormat(context, name, list, 1);
    default:
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckMacro(MAREA_SemaContext *context, const MAREA_AstNode *call)
{
    if ((context == MAREA_NULLPTR) || (call == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_AstNode *name = MAREA_SemaChildAt(call, 0);

    const MAREA_AstNode *list = MAREA_SemaChildAt(call, 1);

    if ((name == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(name) != MAREA_AST_KIND_IDENTIFIER) || (list == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(list) != MAREA_AST_KIND_ARGUMENT_LIST) || (MAREA_G_HasErrorNode(list) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_G_MacroRule *rule = MAREA_G_FindRule(MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name));

    if (rule == MAREA_NULLPTR) {
        return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_UNKNOWN_MACRO, MAREA_AstNodeGetPosition(call), "unknown macro", MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name));
    }

    return MAREA_G_CheckShape(context, name, list, rule->shape);
}
