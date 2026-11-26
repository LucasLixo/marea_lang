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
    MAREA_G_MACRO_SHAPE_BUILD_VALUE = MAREA_UINTEGER_8(5),
} MAREA_G_MacroShape;

// INFO: the one table of the built-in macros. examples/macros/builtin.marea is its source of truth.
typedef struct MAREA_G_MacroRule
{
    const MAREA_Character8 *name;
    MAREA_G_MacroShape shape;
    MAREA_SemaMacroValue value;
} MAREA_G_MacroRule;

// INFO: line gives a uinteger32. include_bytes takes its type from the macro that the registry holds for marea.array, so its value is unknown here.
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
    { "line", MAREA_G_MACRO_SHAPE_NONE, MAREA_SEMA_MACRO_VALUE_UINTEGER_32 },
    { "file", MAREA_G_MACRO_SHAPE_NONE, MAREA_SEMA_MACRO_VALUE_STRING },
    { "namespace", MAREA_G_MACRO_SHAPE_NONE, MAREA_SEMA_MACRO_VALUE_STRING },
    // INFO: build_value takes its type from its second argument, so MAREA_SemaMacroTypeOf types it and the value here is unknown.
    { "build_value", MAREA_G_MACRO_SHAPE_BUILD_VALUE, MAREA_SEMA_MACRO_VALUE_UNKNOWN },
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

// =========================================== build_value! ===========================================

#define MAREA_G_BUILD_ARGUMENT_COUNT MAREA_INTEGER_SIZE(3)

typedef enum MAREA_G_BuildKind
{
    MAREA_G_BUILD_KIND_BOOLEAN = MAREA_UINTEGER_8(0),
    MAREA_G_BUILD_KIND_SIGNED = MAREA_UINTEGER_8(1),
    MAREA_G_BUILD_KIND_UNSIGNED = MAREA_UINTEGER_8(2),
    MAREA_G_BUILD_KIND_FLOAT = MAREA_UINTEGER_8(3),
    MAREA_G_BUILD_KIND_STRING = MAREA_UINTEGER_8(4),
    MAREA_G_BUILD_KIND_CHARACTER = MAREA_UINTEGER_8(5),
    MAREA_G_BUILD_KIND_ENUM = MAREA_UINTEGER_8(6),
} MAREA_G_BuildKind;

// INFO: the type argument of a call: its kind, its name, the width of an integer in bits, and the enum declaration.
typedef struct MAREA_G_BuildType
{
    const MAREA_AstNode *enumeration;
    const MAREA_Character8 *name;
    MAREA_IntegerSize length;
    MAREA_G_BuildKind kind;
    MAREA_Uinteger32 bits;
} MAREA_G_BuildType;

// INFO: INVALID is a type that the macro does not accept. UNCHECKED is a qualified name, which sema does not resolve to a declaration here, so it is accepted and nothing about it is checked.
typedef enum MAREA_G_BuildRead
{
    MAREA_G_BUILD_READ_INVALID = MAREA_UINTEGER_8(0),
    MAREA_G_BUILD_READ_UNCHECKED = MAREA_UINTEGER_8(1),
    MAREA_G_BUILD_READ_OK = MAREA_UINTEGER_8(2),
} MAREA_G_BuildRead;

typedef struct MAREA_G_BuildPrimitive
{
    const MAREA_Character8 *name;
    MAREA_G_BuildKind kind;
    MAREA_Uinteger32 bits;
} MAREA_G_BuildPrimitive;

static const MAREA_G_BuildPrimitive MAREA_G_BUILD_PRIMITIVES[] = {
    { "boolean", MAREA_G_BUILD_KIND_BOOLEAN, 0 },
    { "sinteger8", MAREA_G_BUILD_KIND_SIGNED, 8 },
    { "sinteger16", MAREA_G_BUILD_KIND_SIGNED, 16 },
    { "sinteger32", MAREA_G_BUILD_KIND_SIGNED, 32 },
    { "sinteger64", MAREA_G_BUILD_KIND_SIGNED, 64 },
    { "uinteger8", MAREA_G_BUILD_KIND_UNSIGNED, 8 },
    { "uinteger16", MAREA_G_BUILD_KIND_UNSIGNED, 16 },
    { "uinteger32", MAREA_G_BUILD_KIND_UNSIGNED, 32 },
    { "uinteger64", MAREA_G_BUILD_KIND_UNSIGNED, 64 },
    { "float32", MAREA_G_BUILD_KIND_FLOAT, 32 },
    { "float64", MAREA_G_BUILD_KIND_FLOAT, 64 },
    { "string", MAREA_G_BUILD_KIND_STRING, 0 },
    { "character", MAREA_G_BUILD_KIND_CHARACTER, 0 },
};

// INFO: every variant field of the enum needs a default, because no text from the build can give a value for a field (decision 53). A field has the name, the type and an optional default.
static MAREA_Boolean MAREA_CALL MAREA_G_EnumHasOnlyDefaults(const MAREA_AstNode *enumeration)
{
    for (MAREA_IntegerSize variantIndex = 0; variantIndex < MAREA_AstNodeGetChildCount(enumeration); ++variantIndex) {
        const MAREA_AstNode *variant = MAREA_SemaChildAt(enumeration, variantIndex);

        if ((variant == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(variant) != MAREA_AST_KIND_VARIANT_DECLARATION)) {
            continue;
        }

        for (MAREA_IntegerSize fieldIndex = 0; fieldIndex < MAREA_AstNodeGetChildCount(variant); ++fieldIndex) {
            const MAREA_AstNode *field = MAREA_SemaChildAt(variant, fieldIndex);

            if ((field != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(field) == MAREA_AST_KIND_VARIANT_FIELD) && (MAREA_AstNodeGetChildCount(field) < 3)) {
                return MAREA_BOOLEAN_FALSE;
            }
        }
    }

    return MAREA_BOOLEAN_TRUE;
}

static MAREA_G_BuildRead MAREA_CALL MAREA_G_ReadBuildType(const MAREA_SemaContext *context, const MAREA_AstNode *argument, MAREA_G_BuildType *outType)
{
    if (argument == MAREA_NULLPTR) {
        return MAREA_G_BUILD_READ_INVALID;
    }

    if (MAREA_AstNodeGetKind(argument) == MAREA_AST_KIND_MEMBER_ACCESS) {
        return MAREA_G_BUILD_READ_UNCHECKED;
    }

    if (MAREA_AstNodeGetKind(argument) != MAREA_AST_KIND_IDENTIFIER) {
        return MAREA_G_BUILD_READ_INVALID;
    }

    const MAREA_Character8 *text = MAREA_AstNodeGetText(argument);

    const MAREA_IntegerSize LENGTH = MAREA_AstNodeGetTextLength(argument);

    outType->enumeration = MAREA_NULLPTR;
    outType->name = text;
    outType->length = LENGTH;
    outType->bits = 0;

    for (MAREA_IntegerSize index = 0; index < MAREA_ARRAY_SIZE(MAREA_G_BUILD_PRIMITIVES); ++index) {
        if (MAREA_SemaTextEquals(text, LENGTH, MAREA_G_BUILD_PRIMITIVES[index].name, strlen(MAREA_G_BUILD_PRIMITIVES[index].name)) == MAREA_BOOLEAN_TRUE) {
            outType->kind = MAREA_G_BUILD_PRIMITIVES[index].kind;
            outType->bits = MAREA_G_BUILD_PRIMITIVES[index].bits;

            return MAREA_G_BUILD_READ_OK;
        }
    }

    const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, text, LENGTH);

    if ((entry == MAREA_NULLPTR) || (entry->kind != MAREA_SEMA_NAME_KIND_DECLARATION) || (entry->declaration == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(entry->declaration) != MAREA_AST_KIND_ENUM_DECLARATION)) {
        return MAREA_G_BUILD_READ_INVALID;
    }

    outType->kind = MAREA_G_BUILD_KIND_ENUM;
    outType->enumeration = entry->declaration;

    return MAREA_G_BUILD_READ_OK;
}

// INFO: what a text from the build gives once it is converted to its type.
typedef struct MAREA_G_BuildValue
{
    MAREA_Sinteger64 integer;
    MAREA_Boolean valid;
    MAREA_Boolean known;
    MAREA_Boolean boolean;
} MAREA_G_BuildValue;

static MAREA_Boolean MAREA_CALL MAREA_G_IsDigit(MAREA_Character8 letter, MAREA_Boolean hexadecimal)
{
    return (((letter >= '0') && (letter <= '9')) || ((hexadecimal == MAREA_BOOLEAN_TRUE) && (((letter >= 'a') && (letter <= 'f')) || ((letter >= 'A') && (letter <= 'F'))))) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Uinteger64 MAREA_CALL MAREA_G_DigitValue(MAREA_Character8 letter)
{
    if ((letter >= '0') && (letter <= '9')) {
        return (MAREA_Uinteger64)(letter - '0');
    }

    return (letter >= 'a') ? (MAREA_Uinteger64)((letter - 'a') + 10) : (MAREA_Uinteger64)((letter - 'A') + 10);
}

// INFO: an integer text is a decimal or a 0x hexadecimal number with one optional leading minus. It gives false when a character is wrong, no digit is there, or the magnitude does not fit 64 bits unsigned.
static MAREA_Boolean MAREA_CALL MAREA_G_ReadIntegerText(const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_Boolean *outNegative, MAREA_Uinteger64 *outMagnitude)
{
    MAREA_IntegerSize index = 0;

    *outNegative = MAREA_BOOLEAN_FALSE;

    if ((length > 0) && (text[0] == '-')) {
        *outNegative = MAREA_BOOLEAN_TRUE;
        index = 1;
    }

    MAREA_Uinteger64 base = 10;

    if (((length - index) > 2) && (text[index] == '0') && ((text[(index + 1)] == 'x') || (text[(index + 1)] == 'X'))) {
        base = 16;
        index += 2;
    }

    if (index >= length) {
        return MAREA_BOOLEAN_FALSE;
    }

    MAREA_Uinteger64 magnitude = 0;

    for (; index < length; ++index) {
        if (MAREA_G_IsDigit(text[index], (base == 16) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE) == MAREA_BOOLEAN_FALSE) {
            return MAREA_BOOLEAN_FALSE;
        }

        const MAREA_Uinteger64 DIGIT = MAREA_G_DigitValue(text[index]);

        if (magnitude > ((MAREA_UINTEGER_64_MAXIMUM - DIGIT) / base)) {
            return MAREA_BOOLEAN_FALSE;
        }

        magnitude = ((magnitude * base) + DIGIT);
    }

    *outMagnitude = magnitude;

    return MAREA_BOOLEAN_TRUE;
}

static MAREA_G_BuildValue MAREA_CALL MAREA_G_ConvertInteger(const MAREA_G_BuildType *type, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    MAREA_G_BuildValue value = { 0, MAREA_BOOLEAN_FALSE, MAREA_BOOLEAN_FALSE, MAREA_BOOLEAN_FALSE };

    MAREA_Boolean negative = MAREA_BOOLEAN_FALSE;

    MAREA_Uinteger64 magnitude = 0;

    if (MAREA_G_ReadIntegerText(text, length, &negative, &magnitude) == MAREA_BOOLEAN_FALSE) {
        return value;
    }

    if (type->kind == MAREA_G_BUILD_KIND_UNSIGNED) {
        const MAREA_Uinteger64 LIMIT = (type->bits == 64) ? MAREA_UINTEGER_64_MAXIMUM : (((MAREA_Uinteger64)1 << type->bits) - 1);

        // INFO: a minus sign never fits an unsigned type, except for zero.
        if (((negative == MAREA_BOOLEAN_TRUE) && (magnitude != 0)) || (magnitude > LIMIT)) {
            return value;
        }

        value.valid = MAREA_BOOLEAN_TRUE;
        value.known = (magnitude <= (MAREA_Uinteger64)MAREA_SINTEGER_64_MAXIMUM) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
        value.integer = (value.known == MAREA_BOOLEAN_TRUE) ? (MAREA_Sinteger64)magnitude : 0;

        return value;
    }

    const MAREA_Uinteger64 HALF = ((MAREA_Uinteger64)1 << (type->bits - 1));

    if (magnitude > ((negative == MAREA_BOOLEAN_TRUE) ? HALF : (HALF - 1))) {
        return value;
    }

    value.valid = MAREA_BOOLEAN_TRUE;
    value.known = MAREA_BOOLEAN_TRUE;
    value.integer = (negative == MAREA_BOOLEAN_TRUE) ? (MAREA_Sinteger64)(0 - magnitude) : (MAREA_Sinteger64)magnitude;

    return value;
}

// INFO: reads a run of decimal digits and gives how many it read.
static MAREA_IntegerSize MAREA_CALL MAREA_G_SkipDigits(const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_IntegerSize *index)
{
    MAREA_IntegerSize digits = 0;

    while (((*index) < length) && (MAREA_G_IsDigit(text[*index], MAREA_BOOLEAN_FALSE) == MAREA_BOOLEAN_TRUE)) {
        ++(*index);
        ++digits;
    }

    return digits;
}

// INFO: a float text is an optional sign, digits, an optional fraction with digits, and an optional exponent with an optional sign and digits.
static MAREA_Boolean MAREA_CALL MAREA_G_IsFloatText(const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    MAREA_IntegerSize index = 0;

    if ((index < length) && ((text[index] == '-') || (text[index] == '+'))) {
        ++index;
    }

    if (MAREA_G_SkipDigits(text, length, &index) == 0) {
        return MAREA_BOOLEAN_FALSE;
    }

    if ((index < length) && (text[index] == '.')) {
        ++index;

        if (MAREA_G_SkipDigits(text, length, &index) == 0) {
            return MAREA_BOOLEAN_FALSE;
        }
    }

    if ((index < length) && ((text[index] == 'e') || (text[index] == 'E'))) {
        ++index;

        if ((index < length) && ((text[index] == '-') || (text[index] == '+'))) {
            ++index;
        }

        if (MAREA_G_SkipDigits(text, length, &index) == 0) {
            return MAREA_BOOLEAN_FALSE;
        }
    }

    return (index == length) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: one UTF-8 character: a lead byte, then its continuation bytes. A text of one byte is one character.
static MAREA_Boolean MAREA_CALL MAREA_G_IsOneCharacter(const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    if (length == 1) {
        return MAREA_BOOLEAN_TRUE;
    }

    if ((length < 2) || (length > 4)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_Uinteger8 LEAD = (MAREA_Uinteger8)text[0];

    const MAREA_IntegerSize EXPECTED = ((LEAD & 0xE0U) == 0xC0U) ? 2 : (((LEAD & 0xF0U) == 0xE0U) ? 3 : (((LEAD & 0xF8U) == 0xF0U) ? 4 : 0));

    if (EXPECTED != length) {
        return MAREA_BOOLEAN_FALSE;
    }

    for (MAREA_IntegerSize index = 1; index < length; ++index) {
        if ((((MAREA_Uinteger8)text[index]) & 0xC0U) != 0x80U) {
            return MAREA_BOOLEAN_FALSE;
        }
    }

    return MAREA_BOOLEAN_TRUE;
}

// INFO: a character text is one character, or one character or one escape between single quotes.
static MAREA_Boolean MAREA_CALL MAREA_G_IsCharacterText(const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    if ((length >= 3) && (text[0] == '\'') && (text[(length - 1)] == '\'')) {
        const MAREA_IntegerSize INNER = (length - 2);

        return (((INNER == 2) && (text[1] == '\\')) || (MAREA_G_IsOneCharacter((text + 1), INNER) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    }

    return MAREA_G_IsOneCharacter(text, length);
}

static MAREA_G_BuildValue MAREA_CALL MAREA_G_ConvertText(const MAREA_G_BuildType *type, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    MAREA_G_BuildValue value = { 0, MAREA_BOOLEAN_FALSE, MAREA_BOOLEAN_FALSE, MAREA_BOOLEAN_FALSE };

    switch (type->kind) {
    case MAREA_G_BUILD_KIND_BOOLEAN:
        if (MAREA_SemaTextEquals(text, length, "true", 4) == MAREA_BOOLEAN_TRUE) {
            value.valid = MAREA_BOOLEAN_TRUE;
            value.known = MAREA_BOOLEAN_TRUE;
            value.boolean = MAREA_BOOLEAN_TRUE;
        } else if (MAREA_SemaTextEquals(text, length, "false", 5) == MAREA_BOOLEAN_TRUE) {
            value.valid = MAREA_BOOLEAN_TRUE;
            value.known = MAREA_BOOLEAN_TRUE;
        }

        return value;
    case MAREA_G_BUILD_KIND_SIGNED:
    case MAREA_G_BUILD_KIND_UNSIGNED:
        return MAREA_G_ConvertInteger(type, text, length);
    case MAREA_G_BUILD_KIND_FLOAT:
        value.valid = MAREA_G_IsFloatText(text, length);

        return value;
    case MAREA_G_BUILD_KIND_STRING:
        value.valid = MAREA_BOOLEAN_TRUE;

        return value;
    case MAREA_G_BUILD_KIND_CHARACTER:
        value.valid = MAREA_G_IsCharacterText(text, length);

        return value;
    case MAREA_G_BUILD_KIND_ENUM:
        value.valid = MAREA_SemaEnumHasVariant(type->enumeration, text, length);

        return value;
    }

    return value;
}

static const MAREA_SemaSetting *MAREA_CALL MAREA_G_FindSetting(const MAREA_SemaContext *context, const MAREA_Character8 *name, MAREA_IntegerSize length)
{
    if ((context->settings == MAREA_NULLPTR) || (context->settings->pairs == MAREA_NULLPTR)) {
        return MAREA_NULLPTR;
    }

    for (MAREA_IntegerSize index = 0; index < context->settings->count; ++index) {
        const MAREA_SemaSetting *pair = &context->settings->pairs[index];

        if ((pair->name != MAREA_NULLPTR) && (pair->value != MAREA_NULLPTR) && (MAREA_SemaTextEquals(pair->name, strlen(pair->name), name, length) == MAREA_BOOLEAN_TRUE)) {
            return pair;
        }
    }

    return MAREA_NULLPTR;
}

// INFO: the arguments of a well-formed build_value! call: three of them, a string literal first, then a type that the macro reads. A call of another shape is reported by MAREA_G_CheckBuildValue, never read.
static MAREA_Boolean MAREA_CALL MAREA_G_ReadBuildCall(const MAREA_SemaContext *context, const MAREA_AstNode *call, const MAREA_AstNode **outList, MAREA_G_BuildType *outType, MAREA_G_BuildRead *outRead)
{
    const MAREA_AstNode *name = (call == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_SemaChildAt(call, 0);

    const MAREA_AstNode *list = (call == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_SemaChildAt(call, 1);

    if ((name == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(name) != MAREA_AST_KIND_IDENTIFIER) || (MAREA_SemaTextEquals(MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name), "build_value", 11) == MAREA_BOOLEAN_FALSE) || (list == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(list) != MAREA_AST_KIND_ARGUMENT_LIST) || (MAREA_AstNodeGetChildCount(list) != MAREA_G_BUILD_ARGUMENT_COUNT)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *setting = MAREA_SemaChildAt(list, 0);

    if (MAREA_AstNodeGetKind(setting) != MAREA_AST_KIND_STRING_LITERAL) {
        return MAREA_BOOLEAN_FALSE;
    }

    *outList = list;
    *outRead = MAREA_G_ReadBuildType(context, MAREA_SemaChildAt(list, 1), outType);

    return MAREA_BOOLEAN_TRUE;
}

MAREA_SemaBuildGiven MAREA_CALL MAREA_SemaBuildValueGiven(const MAREA_SemaContext *context, const MAREA_AstNode *call, MAREA_Boolean *outBoolean, MAREA_Sinteger64 *outInteger)
{
    const MAREA_AstNode *list = MAREA_NULLPTR;

    MAREA_G_BuildType type = { 0 };

    MAREA_G_BuildRead read = MAREA_G_BUILD_READ_INVALID;

    if ((context == MAREA_NULLPTR) || (MAREA_G_ReadBuildCall(context, call, &list, &type, &read) == MAREA_BOOLEAN_FALSE) || (read != MAREA_G_BUILD_READ_OK)) {
        return MAREA_SEMA_BUILD_GIVEN_NOT_GIVEN;
    }

    const MAREA_AstNode *setting = MAREA_SemaChildAt(list, 0);

    if (MAREA_AstNodeGetTextLength(setting) < 2) {
        return MAREA_SEMA_BUILD_GIVEN_NOT_GIVEN;
    }

    const MAREA_SemaSetting *pair = MAREA_G_FindSetting(context, (MAREA_AstNodeGetText(setting) + 1), (MAREA_AstNodeGetTextLength(setting) - 2));

    if (pair == MAREA_NULLPTR) {
        return MAREA_SEMA_BUILD_GIVEN_NOT_GIVEN;
    }

    const MAREA_G_BuildValue VALUE = MAREA_G_ConvertText(&type, pair->value, strlen(pair->value));

    if ((VALUE.valid == MAREA_BOOLEAN_FALSE) || (VALUE.known == MAREA_BOOLEAN_FALSE)) {
        return MAREA_SEMA_BUILD_GIVEN_UNKNOWN;
    }

    if (type.kind == MAREA_G_BUILD_KIND_BOOLEAN) {
        if (outBoolean != MAREA_NULLPTR) {
            *outBoolean = VALUE.boolean;
        }

        return MAREA_SEMA_BUILD_GIVEN_BOOLEAN;
    }

    if (outInteger != MAREA_NULLPTR) {
        *outInteger = VALUE.integer;
    }

    return MAREA_SEMA_BUILD_GIVEN_INTEGER;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsLiteral(const MAREA_AstNode *argument)
{
    switch (MAREA_AstNodeGetKind(argument)) {
    case MAREA_AST_KIND_INTEGER_LITERAL:
    case MAREA_AST_KIND_FLOAT_LITERAL:
    case MAREA_AST_KIND_STRING_LITERAL:
    case MAREA_AST_KIND_CHARACTER_LITERAL:
    case MAREA_AST_KIND_BOOLEAN_LITERAL:
        return MAREA_BOOLEAN_TRUE;
    default:
        return MAREA_BOOLEAN_FALSE;
    }
}

// INFO: the default must fit the type. An enum default is a member name, so a literal never fits it.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckBuildDefault(MAREA_SemaContext *context, const MAREA_G_BuildType *type, const MAREA_AstNode *argument)
{
    if (type->kind == MAREA_G_BUILD_KIND_ENUM) {
        return (MAREA_G_IsLiteral(argument) == MAREA_BOOLEAN_TRUE) ? MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_TYPE_MISMATCH, MAREA_AstNodeGetPosition(argument), "value does not fit the type", type->name, type->length) : MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_SemaExcept FIT = MAREA_SemaCheckExpressionFitsPrimitive(context, type->name, type->length, argument);

    if ((FIT != MAREA_SEMA_EXCEPT_SUCCESS) || ((type->kind != MAREA_G_BUILD_KIND_SIGNED) && (type->kind != MAREA_G_BUILD_KIND_UNSIGNED))) {
        return FIT;
    }

    return MAREA_SemaCheckLiteralRangePrimitive(context, type->name, type->length, argument);
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_ReportBuildValue(MAREA_SemaContext *context, MAREA_SourcePosition position, const MAREA_AstNode *setting, const MAREA_G_BuildType *type, const MAREA_Character8 *value)
{
    const MAREA_Character8 *expected = "an integer (a decimal or a 0x hexadecimal number in its range)";

    switch (type->kind) {
    case MAREA_G_BUILD_KIND_BOOLEAN:
        expected = "a boolean (true or false)";
        break;
    case MAREA_G_BUILD_KIND_FLOAT:
        expected = "a float (a decimal number with an optional fraction and exponent)";
        break;
    case MAREA_G_BUILD_KIND_CHARACTER:
        expected = "a character (one character, or one character between single quotes)";
        break;
    case MAREA_G_BUILD_KIND_ENUM:
        expected = "a variant name of the enum";
        break;
    default:
        break;
    }

    const MAREA_Character8 *const PARTS[] = { "build_value! ", MAREA_AstNodeGetText(setting), ": the value \"", value, "\" given by the build is not a ", type->name, ", it needs ", expected };

    const MAREA_IntegerSize LENGTHS[] = { strlen(PARTS[0]), MAREA_AstNodeGetTextLength(setting), strlen(PARTS[2]), strlen(value), strlen(PARTS[4]), type->length, strlen(PARTS[6]), strlen(expected) };

    return MAREA_SemaReportParts(context, MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_BUILD_VALUE, position, PARTS, LENGTHS, MAREA_ARRAY_SIZE(PARTS));
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckBuildValue(MAREA_SemaContext *context, const MAREA_AstNode *call, const MAREA_AstNode *name, const MAREA_AstNode *list)
{
    if (MAREA_AstNodeGetChildCount(list) != MAREA_G_BUILD_ARGUMENT_COUNT) {
        return MAREA_G_ReportArgument(context, MAREA_AstNodeGetPosition(list), name, "takes a name, a type and a default value");
    }

    const MAREA_AstNode *setting = MAREA_SemaChildAt(list, 0);

    if (MAREA_AstNodeGetKind(setting) != MAREA_AST_KIND_STRING_LITERAL) {
        return MAREA_G_ReportArgument(context, MAREA_AstNodeGetPosition(setting), name, "needs a string literal as its first argument");
    }

    const MAREA_AstNode *typeArgument = MAREA_SemaChildAt(list, 1);

    MAREA_G_BuildType type = { 0 };

    const MAREA_G_BuildRead READ = MAREA_G_ReadBuildType(context, typeArgument, &type);

    if (READ == MAREA_G_BUILD_READ_INVALID) {
        return MAREA_G_ReportArgument(context, MAREA_AstNodeGetPosition(typeArgument), name, "needs a primitive type or an enum of the file as its second argument");
    }

    if (READ == MAREA_G_BUILD_READ_UNCHECKED) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    if ((type.kind == MAREA_G_BUILD_KIND_ENUM) && (MAREA_G_EnumHasOnlyDefaults(type.enumeration) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_G_ReportArgument(context, MAREA_AstNodeGetPosition(typeArgument), name, "needs an enum whose every variant field has a default");
    }

    // INFO: a diagnostic of the default ends the check of the call, so the call raises one diagnostic.
    const MAREA_IntegerSize BEFORE = MAREA_DiagnosticSinkGetCount(context->sink);

    const MAREA_SemaExcept DEFAULT_RESULT = MAREA_G_CheckBuildDefault(context, &type, MAREA_SemaChildAt(list, 2));

    if ((DEFAULT_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) || (MAREA_DiagnosticSinkGetCount(context->sink) != BEFORE) || (MAREA_AstNodeGetTextLength(setting) < 2)) {
        return DEFAULT_RESULT;
    }

    const MAREA_SemaSetting *pair = MAREA_G_FindSetting(context, (MAREA_AstNodeGetText(setting) + 1), (MAREA_AstNodeGetTextLength(setting) - 2));

    if ((pair == MAREA_NULLPTR) || (MAREA_G_ConvertText(&type, pair->value, strlen(pair->value)).valid == MAREA_BOOLEAN_TRUE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_G_ReportBuildValue(context, MAREA_AstNodeGetPosition(call), setting, &type, pair->value);
}

// ====================================== macros of a shipped library ======================================

// INFO: what the name of a qualified call is: a registered macro, or not.
typedef enum MAREA_G_ResolveStatus
{
    MAREA_G_RESOLVE_STATUS_UNKNOWN = MAREA_UINTEGER_8(0),
    MAREA_G_RESOLVE_STATUS_UNIMPORTED = MAREA_UINTEGER_8(1),
    MAREA_G_RESOLVE_STATUS_FOUND = MAREA_UINTEGER_8(2),
} MAREA_G_ResolveStatus;

typedef struct MAREA_G_Resolution
{
    const MAREA_MacroDescriptor *descriptor;
    const MAREA_Character8 *path;
    MAREA_IntegerSize pathLength;
    MAREA_G_ResolveStatus status;
} MAREA_G_Resolution;

// INFO: sema reads one qualified name against the imports and the requires of the file, as every other full path. The path text is built in the arena, which is the only thing that the read changes, so a const context can ask.
static MAREA_SemaExcept MAREA_CALL MAREA_G_ResolveQualified(const MAREA_SemaContext *context, const MAREA_AstNode *name, MAREA_G_Resolution *outResolution)
{
    outResolution->descriptor = MAREA_NULLPTR;
    outResolution->path = MAREA_NULLPTR;
    outResolution->pathLength = 0;
    outResolution->status = MAREA_G_RESOLVE_STATUS_UNKNOWN;

    MAREA_SemaContext *writable = (MAREA_SemaContext *)context;

    const MAREA_SemaExcept JOIN_RESULT = MAREA_SemaQualifiedNameJoin(writable, name, &outResolution->path, &outResolution->pathLength);

    if ((JOIN_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) || (outResolution->path == MAREA_NULLPTR)) {
        return JOIN_RESULT;
    }

    MAREA_SemaPathStatus status = MAREA_SEMA_PATH_STATUS_NOT_A_PATH;

    const MAREA_Character8 *namespaceText = MAREA_NULLPTR;

    MAREA_IntegerSize namespaceLength = 0;

    const MAREA_SemaExcept CLASSIFY_RESULT = MAREA_SemaClassifyPath(writable, outResolution->path, outResolution->pathLength, MAREA_BOOLEAN_TRUE, &status, &namespaceText, &namespaceLength);

    if (CLASSIFY_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
        return CLASSIFY_RESULT;
    }

    if (status == MAREA_SEMA_PATH_STATUS_UNIMPORTED) {
        outResolution->status = MAREA_G_RESOLVE_STATUS_UNIMPORTED;

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *last = MAREA_SemaChildAt(name, (MAREA_AstNodeGetChildCount(name) - 1));

    if ((status != MAREA_SEMA_PATH_STATUS_COVERED) || (namespaceText == MAREA_NULLPTR) || (last == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    // INFO: the macro name is the last segment, so it stands right after the namespace read and nothing else is between them.
    const MAREA_MacroDescriptor *descriptor = MAREA_TypeRegistryFindMacro(context->typeRegistry, namespaceText, namespaceLength, MAREA_AstNodeGetText(last), MAREA_AstNodeGetTextLength(last));

    if (descriptor != MAREA_NULLPTR) {
        outResolution->descriptor = descriptor;
        outResolution->status = MAREA_G_RESOLVE_STATUS_FOUND;
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

// INFO: a qualified name, or the bare include_bytes, which stands for the macro of that name that marea.array registers. Sema knows no library type: with marea.array off there is no such macro and the bare call has no type.
static MAREA_SemaExcept MAREA_CALL MAREA_G_ResolveMacro(const MAREA_SemaContext *context, const MAREA_AstNode *name, MAREA_G_Resolution *outResolution)
{
    if (MAREA_AstNodeGetKind(name) == MAREA_AST_KIND_QUALIFIED_NAME) {
        return MAREA_G_ResolveQualified(context, name, outResolution);
    }

    *outResolution = (MAREA_G_Resolution){ .descriptor = MAREA_NULLPTR, .path = MAREA_NULLPTR, .pathLength = 0, .status = MAREA_G_RESOLVE_STATUS_UNKNOWN };

    if ((MAREA_AstNodeGetKind(name) == MAREA_AST_KIND_IDENTIFIER) && (MAREA_SemaTextEquals(MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name), "include_bytes", strlen("include_bytes")) == MAREA_BOOLEAN_TRUE)) {
        outResolution->descriptor = MAREA_TypeRegistryFindMacro(context->typeRegistry, "marea.array", strlen("marea.array"), "include_bytes", strlen("include_bytes"));
        outResolution->status = (outResolution->descriptor != MAREA_NULLPTR) ? MAREA_G_RESOLVE_STATUS_FOUND : MAREA_G_RESOLVE_STATUS_UNKNOWN;
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

static MAREA_Boolean MAREA_CALL MAREA_G_ExpandRegistered(const MAREA_SemaContext *context, const MAREA_MacroDescriptor *descriptor, const MAREA_AstNode *list, MAREA_IntegerSize *outCount)
{
    const MAREA_AstNode *argument = MAREA_SemaChildAt(list, 0);

    if ((argument == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(argument) != MAREA_AST_KIND_STRING_LITERAL) || (MAREA_AstNodeGetTextLength(argument) < 2)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_MacroExpandRequest REQUEST = {
        .text = (MAREA_AstNodeGetText(argument) + 1),
        .textLength = (MAREA_AstNodeGetTextLength(argument) - 2),
        .includeSize = (context->includeResolver == MAREA_NULLPTR) ? MAREA_NULLPTR : context->includeResolver->size,
        .includeUserData = (context->includeResolver == MAREA_NULLPTR) ? MAREA_NULLPTR : context->includeResolver->userData,
    };

    return descriptor->expand(&REQUEST, outCount);
}

// INFO: the call has the argument kinds of the descriptor, one for each argument.
static MAREA_Boolean MAREA_CALL MAREA_G_ArgumentsMatch(const MAREA_MacroDescriptor *descriptor, const MAREA_AstNode *list)
{
    if (MAREA_AstNodeGetChildCount(list) != descriptor->argumentCount) {
        return MAREA_BOOLEAN_FALSE;
    }

    for (MAREA_IntegerSize index = 0; index < descriptor->argumentCount; ++index) {
        if ((descriptor->argumentKinds[index] == MAREA_MACRO_ARGUMENT_KIND_STRING_LITERAL) && (MAREA_AstNodeGetKind(MAREA_SemaChildAt(list, index)) != MAREA_AST_KIND_STRING_LITERAL)) {
            return MAREA_BOOLEAN_FALSE;
        }
    }

    return MAREA_BOOLEAN_TRUE;
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckRegistered(MAREA_SemaContext *context, const MAREA_AstNode *call, const MAREA_AstNode *name, const MAREA_AstNode *list)
{
    MAREA_G_Resolution resolution = { 0 };

    const MAREA_SemaExcept RESOLVE_RESULT = MAREA_G_ResolveQualified(context, name, &resolution);

    if (RESOLVE_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
        return RESOLVE_RESULT;
    }

    if (resolution.status == MAREA_G_RESOLVE_STATUS_UNIMPORTED) {
        return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_UNIMPORTED_NAMESPACE, MAREA_AstNodeGetPosition(call), "a namespace that no import or require covers:", resolution.path, resolution.pathLength);
    }

    if (resolution.status != MAREA_G_RESOLVE_STATUS_FOUND) {
        return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_UNKNOWN_MACRO, MAREA_AstNodeGetPosition(call), "unknown macro", resolution.path, resolution.pathLength);
    }

    const MAREA_AstNode *last = MAREA_SemaChildAt(name, (MAREA_AstNodeGetChildCount(name) - 1));

    if (MAREA_G_ArgumentsMatch(resolution.descriptor, list) == MAREA_BOOLEAN_FALSE) {
        return MAREA_G_ReportArgument(context, MAREA_AstNodeGetPosition(list), last, "needs the arguments that its library gives: one string literal");
    }

    MAREA_IntegerSize count = 0;

    if (MAREA_G_ExpandRegistered(context, resolution.descriptor, list, &count) == MAREA_BOOLEAN_FALSE) {
        return MAREA_G_ReportArgument(context, MAREA_AstNodeGetPosition(list), last, "cannot expand this call, a file may be missing");
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

MAREA_SemaMacroType MAREA_CALL MAREA_SemaMacroTypeOf(const MAREA_SemaContext *context, const MAREA_AstNode *call)
{
    MAREA_SemaMacroType type = { .declaration = MAREA_NULLPTR, .name = MAREA_NULLPTR, .length = 0, .kind = MAREA_SEMA_MACRO_TYPE_KIND_UNKNOWN };

    const MAREA_AstNode *name = ((context == MAREA_NULLPTR) || (call == MAREA_NULLPTR)) ? MAREA_NULLPTR : MAREA_SemaChildAt(call, 0);

    const MAREA_AstNode *list = (name == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_SemaChildAt(call, 1);

    if ((list == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(list) != MAREA_AST_KIND_ARGUMENT_LIST) || (MAREA_G_HasErrorNode(list) == MAREA_BOOLEAN_TRUE)) {
        return type;
    }

    MAREA_G_Resolution resolution = { 0 };

    if ((MAREA_G_ResolveMacro(context, name, &resolution) != MAREA_SEMA_EXCEPT_SUCCESS) || ((MAREA_AstNodeGetKind(name) == MAREA_AST_KIND_QUALIFIED_NAME) && (resolution.status != MAREA_G_RESOLVE_STATUS_FOUND))) {
        return type;
    }

    if (resolution.status == MAREA_G_RESOLVE_STATUS_FOUND) {
        if (resolution.descriptor->resultType == MAREA_NULLPTR) {
            return type;
        }

        type.kind = MAREA_SEMA_MACRO_TYPE_KIND_REGISTERED;
        type.name = resolution.descriptor->resultType->name;
        type.length = resolution.descriptor->resultType->nameLength;

        return type;
    }

    const MAREA_AstNode *callList = MAREA_NULLPTR;

    MAREA_G_BuildType buildType = { 0 };

    MAREA_G_BuildRead read = MAREA_G_BUILD_READ_INVALID;

    if ((MAREA_G_ReadBuildCall(context, call, &callList, &buildType, &read) == MAREA_BOOLEAN_FALSE) || (read != MAREA_G_BUILD_READ_OK)) {
        return type;
    }

    type.name = buildType.name;
    type.length = buildType.length;
    type.declaration = buildType.enumeration;
    type.kind = (buildType.kind == MAREA_G_BUILD_KIND_ENUM) ? MAREA_SEMA_MACRO_TYPE_KIND_ENUM : MAREA_SEMA_MACRO_TYPE_KIND_PRIMITIVE;

    return type;
}

// INFO: the integer of an INTEGER_LITERAL type argument, decimal or 0x hexadecimal. It gives false for any other text.
static MAREA_Boolean MAREA_CALL MAREA_G_ReadCount(const MAREA_AstNode *literal, MAREA_IntegerSize *outCount)
{
    MAREA_Boolean negative = MAREA_BOOLEAN_FALSE;

    MAREA_Uinteger64 magnitude = 0;

    if ((MAREA_AstNodeGetKind(literal) != MAREA_AST_KIND_INTEGER_LITERAL) || (MAREA_G_ReadIntegerText(MAREA_AstNodeGetText(literal), MAREA_AstNodeGetTextLength(literal), &negative, &magnitude) == MAREA_BOOLEAN_FALSE) || (negative == MAREA_BOOLEAN_TRUE) || (magnitude > MAREA_INTEGER_SIZE_MAXIMUM)) {
        return MAREA_BOOLEAN_FALSE;
    }

    *outCount = (MAREA_IntegerSize)magnitude;

    return MAREA_BOOLEAN_TRUE;
}

MAREA_Boolean MAREA_CALL MAREA_SemaMacroResultOf(const MAREA_SemaContext *context, const MAREA_AstNode *call, const MAREA_MacroDescriptor **outDescriptor, MAREA_IntegerSize *outCount)
{
    if ((context == MAREA_NULLPTR) || (call == MAREA_NULLPTR) || (outDescriptor == MAREA_NULLPTR) || (outCount == MAREA_NULLPTR)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *name = MAREA_SemaChildAt(call, 0);

    const MAREA_AstNode *list = MAREA_SemaChildAt(call, 1);

    MAREA_G_Resolution resolution = { 0 };

    if ((name == MAREA_NULLPTR) || (list == MAREA_NULLPTR) || (MAREA_G_ResolveMacro(context, name, &resolution) != MAREA_SEMA_EXCEPT_SUCCESS) || (resolution.status != MAREA_G_RESOLVE_STATUS_FOUND) || (resolution.descriptor->resultType == MAREA_NULLPTR) || (MAREA_G_ArgumentsMatch(resolution.descriptor, list) == MAREA_BOOLEAN_FALSE) || (MAREA_G_ExpandRegistered(context, resolution.descriptor, list, outCount) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_BOOLEAN_FALSE;
    }

    *outDescriptor = resolution.descriptor;

    return MAREA_BOOLEAN_TRUE;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckMacroResultFit(MAREA_SemaContext *context, const MAREA_AstNode *type, const MAREA_AstNode *expression)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    if ((type == MAREA_NULLPTR) || (expression == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(expression) != MAREA_AST_KIND_MACRO_CALL) || (MAREA_AstNodeGetKind(type) != MAREA_AST_KIND_NAMED_TYPE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *name = MAREA_SemaChildAt(expression, 0);

    const MAREA_AstNode *list = MAREA_SemaChildAt(expression, 1);

    const MAREA_AstNode *arguments = MAREA_SemaChildAt(type, 1);

    if ((name == MAREA_NULLPTR) || (list == MAREA_NULLPTR) || (arguments == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(arguments) != MAREA_AST_KIND_TYPE_ARGUMENTS) || (MAREA_AstNodeGetChildCount(arguments) != 2)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    MAREA_G_Resolution resolution = { 0 };

    const MAREA_SemaExcept RESOLVE_RESULT = MAREA_G_ResolveMacro(context, name, &resolution);

    const MAREA_TypeDescriptor *declared = MAREA_NULLPTR;

    MAREA_IntegerSize count = 0;

    if ((RESOLVE_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) || (resolution.status != MAREA_G_RESOLVE_STATUS_FOUND) || (MAREA_G_ArgumentsMatch(resolution.descriptor, list) == MAREA_BOOLEAN_FALSE) || (MAREA_G_ExpandRegistered(context, resolution.descriptor, list, &count) == MAREA_BOOLEAN_FALSE) || (MAREA_SemaDescriptorOfType(context, type, &declared) == MAREA_BOOLEAN_FALSE)) {
        return RESOLVE_RESULT;
    }

    if (declared != resolution.descriptor->resultType) {
        return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_TYPE_MISMATCH, MAREA_AstNodeGetPosition(expression), "value does not fit the type", declared->name, declared->nameLength);
    }

    const MAREA_AstNode *element = MAREA_SemaChildAt(arguments, 0);

    const MAREA_AstNode *elementName = ((element != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(element) == MAREA_AST_KIND_NAMED_TYPE) && (MAREA_AstNodeGetChildCount(element) > 0)) ? MAREA_SemaChildAt(MAREA_SemaChildAt(element, 0), 0) : MAREA_NULLPTR;

    MAREA_IntegerSize declaredCount = 0;

    const MAREA_Boolean ELEMENT_FITS = ((elementName != MAREA_NULLPTR) && (MAREA_SemaTextEquals(MAREA_AstNodeGetText(elementName), MAREA_AstNodeGetTextLength(elementName), resolution.descriptor->resultElementText, resolution.descriptor->resultElementLength) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    if ((ELEMENT_FITS == MAREA_BOOLEAN_TRUE) && (MAREA_G_ReadCount(MAREA_SemaChildAt(arguments, 1), &declaredCount) == MAREA_BOOLEAN_TRUE) && (declaredCount == count)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *last = (MAREA_AstNodeGetKind(name) == MAREA_AST_KIND_QUALIFIED_NAME) ? MAREA_SemaChildAt(name, (MAREA_AstNodeGetChildCount(name) - 1)) : name;

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_GENERIC_ARGUMENT_MISMATCH, MAREA_AstNodeGetPosition(expression), "type arguments differ from the result of the macro", MAREA_AstNodeGetText(last), MAREA_AstNodeGetTextLength(last));
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckMacro(MAREA_SemaContext *context, const MAREA_AstNode *call)
{
    if ((context == MAREA_NULLPTR) || (call == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_AstNode *name = MAREA_SemaChildAt(call, 0);

    const MAREA_AstNode *list = MAREA_SemaChildAt(call, 1);

    if ((name == MAREA_NULLPTR) || (list == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(list) != MAREA_AST_KIND_ARGUMENT_LIST) || (MAREA_G_HasErrorNode(list) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    if (MAREA_AstNodeGetKind(name) == MAREA_AST_KIND_QUALIFIED_NAME) {
        return MAREA_G_CheckRegistered(context, call, name, list);
    }

    if (MAREA_AstNodeGetKind(name) != MAREA_AST_KIND_IDENTIFIER) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_G_MacroRule *rule = MAREA_G_FindRule(MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name));

    if (rule == MAREA_NULLPTR) {
        return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_UNKNOWN_MACRO, MAREA_AstNodeGetPosition(call), "unknown macro", MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name));
    }

    if (rule->shape == MAREA_G_MACRO_SHAPE_BUILD_VALUE) {
        return MAREA_G_CheckBuildValue(context, call, name, list);
    }

    return MAREA_G_CheckShape(context, name, list, rule->shape);
}
