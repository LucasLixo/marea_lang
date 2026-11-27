// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <string.h>

typedef enum MAREA_G_AttributeShape
{
    MAREA_G_ATTRIBUTE_SHAPE_FLAG = MAREA_UINTEGER_8(0),
    MAREA_G_ATTRIBUTE_SHAPE_MEMBER = MAREA_UINTEGER_8(1),
    MAREA_G_ATTRIBUTE_SHAPE_INTEGER = MAREA_UINTEGER_8(2),
    MAREA_G_ATTRIBUTE_SHAPE_ALIGNMENT = MAREA_UINTEGER_8(3),
    MAREA_G_ATTRIBUTE_SHAPE_VOCABULARY = MAREA_UINTEGER_8(4),
    MAREA_G_ATTRIBUTE_SHAPE_STRING = MAREA_UINTEGER_8(5),
    MAREA_G_ATTRIBUTE_SHAPE_C_IDENTIFIER = MAREA_UINTEGER_8(6),
} MAREA_G_AttributeShape;

typedef struct MAREA_G_AttributeRule
{
    const MAREA_Character8 *key;
    const MAREA_Character8 *const *members;
    MAREA_G_AttributeShape shape;
} MAREA_G_AttributeRule;

static const MAREA_Character8 *const MAREA_G_VisibilityMembers[] = { "public", "private", MAREA_NULLPTR };
static const MAREA_Character8 *const MAREA_G_InlineMembers[] = { "force", "never", "auto", MAREA_NULLPTR };
static const MAREA_Character8 *const MAREA_G_BuildMembers[] = { "debug", "release", MAREA_NULLPTR };
static const MAREA_Character8 *const MAREA_G_LayoutMembers[] = { "c", "marea", MAREA_NULLPTR };
static const MAREA_Character8 *const MAREA_G_SemanticsMembers[] = { "value_read_only", "reference_read_only", "reference_mutable", MAREA_NULLPTR };
static const MAREA_Character8 *const MAREA_G_BackendMembers[] = { "auto", "scalar", "vector_extension", "intrinsics", MAREA_NULLPTR };

// INFO: the values of the key operator: the operator tokens, then the index, call and lifecycle hooks.
static const MAREA_Character8 *const MAREA_G_OperatorVocabulary[] = {
    "+",
    "-",
    "*",
    "/",
    "%",
    "**",
    "+%",
    "-%",
    "*%",
    "+|",
    "-|",
    "*|",
    "~",
    "&",
    "|",
    "^",
    "<<",
    ">>",
    "==",
    "!=",
    "<",
    "<=",
    ">",
    ">=",
    "=",
    "+=",
    "-=",
    "*=",
    "/=",
    "%=",
    "**=",
    "<<=",
    ">>=",
    "&=",
    "|=",
    "^=",
    "[]",
    "()",
    "convert",
    "construct",
    "destroy",
    MAREA_NULLPTR,
};

// INFO: the index of each key in MAREA_G_RULES, and the bit that MAREA_G_PLACE_KEYS uses for it.
enum
{
    MAREA_G_RULE_VISIBILITY = 0,
    MAREA_G_RULE_INLINE = 1,
    MAREA_G_RULE_BUILD = 2,
    MAREA_G_RULE_LAYOUT = 3,
    MAREA_G_RULE_ALIGN = 4,
    MAREA_G_RULE_PACK = 5,
    MAREA_G_RULE_MOVE_ONLY = 6,
    MAREA_G_RULE_OPERATOR = 7,
    MAREA_G_RULE_EXPORT = 8,
    MAREA_G_RULE_SEMANTICS = 9,
    MAREA_G_RULE_BACKEND = 10,
    MAREA_G_RULE_COUNT = 11,
};

// INFO: the order is fixed because the MAREA_G_RULE_ indexes above name entries of this table.
static const MAREA_G_AttributeRule MAREA_G_RULES[MAREA_G_RULE_COUNT] = {
    { .key = "visibility", .members = MAREA_G_VisibilityMembers, .shape = MAREA_G_ATTRIBUTE_SHAPE_MEMBER },
    { .key = "inline", .members = MAREA_G_InlineMembers, .shape = MAREA_G_ATTRIBUTE_SHAPE_MEMBER },
    { .key = "build", .members = MAREA_G_BuildMembers, .shape = MAREA_G_ATTRIBUTE_SHAPE_MEMBER },
    { .key = "layout", .members = MAREA_G_LayoutMembers, .shape = MAREA_G_ATTRIBUTE_SHAPE_MEMBER },
    { .key = "align", .members = MAREA_NULLPTR, .shape = MAREA_G_ATTRIBUTE_SHAPE_ALIGNMENT },
    { .key = "pack", .members = MAREA_NULLPTR, .shape = MAREA_G_ATTRIBUTE_SHAPE_INTEGER },
    // INFO: "value" is retired outright; MAREA_G_IsByValue in MAREASemaOwnership.c names the one by-value exception left.
    { .key = "move_only", .members = MAREA_NULLPTR, .shape = MAREA_G_ATTRIBUTE_SHAPE_FLAG },
    { .key = "operator", .members = MAREA_G_OperatorVocabulary, .shape = MAREA_G_ATTRIBUTE_SHAPE_VOCABULARY },
    // INFO: the label of an export is the name of a C function, so it must be a valid C identifier (decision 20).
    { .key = "export", .members = MAREA_NULLPTR, .shape = MAREA_G_ATTRIBUTE_SHAPE_C_IDENTIFIER },
    // INFO: the one and only parameter-passing mechanism; MAREA_SemaGetParameterSemantics is the one place the rest of sema reads it.
    { .key = "semantics", .members = MAREA_G_SemanticsMembers, .shape = MAREA_G_ATTRIBUTE_SHAPE_MEMBER },
    // INFO: the backend of marea.math (decision 21). The place and the namespace limit are checked by MAREA_G_CheckItem.
    { .key = "backend", .members = MAREA_G_BackendMembers, .shape = MAREA_G_ATTRIBUTE_SHAPE_MEMBER },
};

static MAREA_Boolean MAREA_CALL MAREA_G_TextIs(const MAREA_AstNode *node, const MAREA_Character8 *text)
{
    const MAREA_IntegerSize LENGTH = strlen(text);

    if (MAREA_AstNodeGetTextLength(node) != LENGTH) {
        return MAREA_BOOLEAN_FALSE;
    }

    return (memcmp(MAREA_AstNodeGetText(node), text, LENGTH) == 0) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_ParseInteger(const MAREA_AstNode *node, MAREA_Uinteger64 *outValue)
{
    const MAREA_Character8 *text = MAREA_AstNodeGetText(node);

    const MAREA_IntegerSize LENGTH = MAREA_AstNodeGetTextLength(node);

    MAREA_IntegerSize index = 0;

    MAREA_Uinteger64 base = 10;

    if ((LENGTH > 2) && (text[0] == '0') && ((text[1] == 'x') || (text[1] == 'X'))) {
        base = 16;
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

        if (value > ((MAREA_UINTEGER_64_MAXIMUM - digit) / base)) {
            return MAREA_BOOLEAN_FALSE;
        }

        value = ((value * base) + digit);
        any = MAREA_BOOLEAN_TRUE;
    }

    *outValue = value;

    return any;
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_ReportValue(MAREA_SemaContext *context, MAREA_SemaDiagnosticCode code, const MAREA_AstNode *item, const MAREA_Character8 *prefix)
{
    const MAREA_AstNode *key = MAREA_SemaChildAt(item, 0);

    return MAREA_SemaReportNamed(context, code, MAREA_AstNodeGetPosition(item), prefix, MAREA_AstNodeGetText(key), MAREA_AstNodeGetTextLength(key));
}

// INFO: the text of a string literal keeps its two quotes.
static MAREA_Boolean MAREA_CALL MAREA_G_IsQuotedMember(const MAREA_AstNode *value, const MAREA_Character8 *const *members)
{
    const MAREA_IntegerSize LENGTH = MAREA_AstNodeGetTextLength(value);

    if (LENGTH < 2) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_Character8 *inner = (MAREA_AstNodeGetText(value) + 1);

    for (const MAREA_Character8 *const *candidate = members; *candidate != MAREA_NULLPTR; ++candidate) {
        if ((strlen(*candidate) == (LENGTH - 2)) && (memcmp(inner, *candidate, (LENGTH - 2)) == 0)) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsSingleName(const MAREA_AstNode *value)
{
    return ((MAREA_AstNodeGetKind(value) == MAREA_AST_KIND_QUALIFIED_NAME) && (MAREA_AstNodeGetChildCount(value) == 1)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: a letter or an underscore, then letters, digits and underscores. An empty text is no identifier.
static MAREA_Boolean MAREA_CALL MAREA_G_IsCIdentifier(const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_Boolean hasQuotes)
{
    if ((hasQuotes == MAREA_BOOLEAN_FALSE) || (length == 0)) {
        return MAREA_BOOLEAN_FALSE;
    }

    for (MAREA_IntegerSize index = 0; index < length; ++index) {
        const MAREA_Character8 LETTER = text[index];

        const MAREA_Boolean IS_WORD = (((LETTER >= 'a') && (LETTER <= 'z')) || ((LETTER >= 'A') && (LETTER <= 'Z')) || (LETTER == '_')) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

        const MAREA_Boolean IS_DIGIT = ((LETTER >= '0') && (LETTER <= '9')) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

        if ((IS_WORD == MAREA_BOOLEAN_FALSE) && ((IS_DIGIT == MAREA_BOOLEAN_FALSE) || (index == 0))) {
            return MAREA_BOOLEAN_FALSE;
        }
    }

    return MAREA_BOOLEAN_TRUE;
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckValue(MAREA_SemaContext *context, const MAREA_G_AttributeRule *rule, const MAREA_AstNode *item)
{
    const MAREA_AstNode *value = MAREA_SemaChildAt(item, 1);

    if ((value != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(value) == MAREA_AST_KIND_ERROR_NODE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstKind KIND = (value == MAREA_NULLPTR) ? MAREA_AST_KIND_ERROR_NODE : MAREA_AstNodeGetKind(value);

    MAREA_Boolean valid = MAREA_BOOLEAN_FALSE;

    switch (rule->shape) {
    case MAREA_G_ATTRIBUTE_SHAPE_FLAG:
        valid = ((value == MAREA_NULLPTR) || (KIND == MAREA_AST_KIND_BOOLEAN_LITERAL)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
        break;
    case MAREA_G_ATTRIBUTE_SHAPE_MEMBER:
        if ((value != MAREA_NULLPTR) && (MAREA_G_IsSingleName(value) == MAREA_BOOLEAN_TRUE)) {
            const MAREA_AstNode *member = MAREA_SemaChildAt(value, 0);

            for (const MAREA_Character8 *const *candidate = rule->members; *candidate != MAREA_NULLPTR; ++candidate) {
                if ((member != MAREA_NULLPTR) && (MAREA_G_TextIs(member, *candidate) == MAREA_BOOLEAN_TRUE)) {
                    valid = MAREA_BOOLEAN_TRUE;
                }
            }
        }
        break;
    case MAREA_G_ATTRIBUTE_SHAPE_INTEGER:
    case MAREA_G_ATTRIBUTE_SHAPE_ALIGNMENT:
    {
        MAREA_Uinteger64 number = 0;

        if ((KIND == MAREA_AST_KIND_INTEGER_LITERAL) && (MAREA_G_ParseInteger(value, &number) == MAREA_BOOLEAN_TRUE)) {
            valid = MAREA_BOOLEAN_TRUE;

            if ((rule->shape == MAREA_G_ATTRIBUTE_SHAPE_ALIGNMENT) && ((number == 0) || ((number & (number - 1)) != 0))) {
                return MAREA_G_ReportValue(context, MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_ALIGNMENT, item, "alignment is not a power of two:");
            }
        }
        break;
    }
    case MAREA_G_ATTRIBUTE_SHAPE_VOCABULARY:
        if (KIND == MAREA_AST_KIND_STRING_LITERAL) {
            valid = MAREA_G_IsQuotedMember(value, rule->members);
        }
        break;
    case MAREA_G_ATTRIBUTE_SHAPE_STRING:
        valid = (KIND == MAREA_AST_KIND_STRING_LITERAL) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
        break;
    case MAREA_G_ATTRIBUTE_SHAPE_C_IDENTIFIER:
        if (KIND == MAREA_AST_KIND_STRING_LITERAL) {
            valid = MAREA_G_IsCIdentifier((MAREA_AstNodeGetText(value) + 1), (MAREA_AstNodeGetTextLength(value) - 2), (MAREA_AstNodeGetTextLength(value) >= 2));
        }
        break;
    }

    if (valid == MAREA_BOOLEAN_TRUE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_G_ReportValue(context, MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_ATTRIBUTE_VALUE, item, "invalid value for attribute:");
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsOn(const MAREA_AstNode *item)
{
    const MAREA_AstNode *value = MAREA_SemaChildAt(item, 1);

    return ((value != MAREA_NULLPTR) && (MAREA_G_TextIs(value, "false") == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_FALSE : MAREA_BOOLEAN_TRUE;
}

// =========================================== the places of a key ===========================================

// INFO: the places the placement matrix of the attribute map (section 6) lists, one for each row.
typedef enum MAREA_G_Place
{
    MAREA_G_PLACE_FILE = MAREA_UINTEGER_8(0),
    MAREA_G_PLACE_TOP_FUNCTION = MAREA_UINTEGER_8(1),
    MAREA_G_PLACE_TOP_CONSTANT = MAREA_UINTEGER_8(2),
    MAREA_G_PLACE_STRUCTURE = MAREA_UINTEGER_8(3),
    MAREA_G_PLACE_OPAQUE_STRUCTURE = MAREA_UINTEGER_8(4),
    MAREA_G_PLACE_ENUM = MAREA_UINTEGER_8(5),
    MAREA_G_PLACE_TYPE_ALIAS = MAREA_UINTEGER_8(6),
    MAREA_G_PLACE_CONTRACT = MAREA_UINTEGER_8(7),
    MAREA_G_PLACE_IMPLEMENTATION = MAREA_UINTEGER_8(8),
    MAREA_G_PLACE_FIELD = MAREA_UINTEGER_8(9),
    MAREA_G_PLACE_IMPLEMENTATION_FUNCTION = MAREA_UINTEGER_8(10),
    MAREA_G_PLACE_IMPLEMENTATION_CONSTANT = MAREA_UINTEGER_8(11),
    MAREA_G_PLACE_PARAMETER = MAREA_UINTEGER_8(12),
    MAREA_G_PLACE_MACRO_STATEMENT = MAREA_UINTEGER_8(13),
    MAREA_G_PLACE_OTHER = MAREA_UINTEGER_8(14),
} MAREA_G_Place;

#define MAREA_G_KEY(rule) (MAREA_UINTEGER_32(1) << (rule))

#define MAREA_G_ALL_KEYS ((MAREA_UINTEGER_32(1) << MAREA_G_RULE_COUNT) - 1)

// INFO: the keys that each place allows, in the order of MAREA_G_Place. The last place is any node the grammar gives no list to, and nothing is checked there.
static const MAREA_Uinteger32 MAREA_G_PLACE_KEYS[] = {
    MAREA_G_KEY(MAREA_G_RULE_VISIBILITY) | MAREA_G_KEY(MAREA_G_RULE_INLINE) | MAREA_G_KEY(MAREA_G_RULE_BUILD) | MAREA_G_KEY(MAREA_G_RULE_LAYOUT) | MAREA_G_KEY(MAREA_G_RULE_BACKEND),
    MAREA_G_KEY(MAREA_G_RULE_VISIBILITY) | MAREA_G_KEY(MAREA_G_RULE_INLINE) | MAREA_G_KEY(MAREA_G_RULE_BUILD) | MAREA_G_KEY(MAREA_G_RULE_EXPORT) | MAREA_G_KEY(MAREA_G_RULE_BACKEND),
    MAREA_G_KEY(MAREA_G_RULE_VISIBILITY),
    MAREA_G_KEY(MAREA_G_RULE_VISIBILITY) | MAREA_G_KEY(MAREA_G_RULE_LAYOUT) | MAREA_G_KEY(MAREA_G_RULE_ALIGN) | MAREA_G_KEY(MAREA_G_RULE_PACK) | MAREA_G_KEY(MAREA_G_RULE_MOVE_ONLY),
    MAREA_G_KEY(MAREA_G_RULE_VISIBILITY),
    MAREA_G_KEY(MAREA_G_RULE_VISIBILITY) | MAREA_G_KEY(MAREA_G_RULE_LAYOUT) | MAREA_G_KEY(MAREA_G_RULE_ALIGN) | MAREA_G_KEY(MAREA_G_RULE_PACK) | MAREA_G_KEY(MAREA_G_RULE_MOVE_ONLY),
    MAREA_G_KEY(MAREA_G_RULE_VISIBILITY),
    MAREA_G_KEY(MAREA_G_RULE_VISIBILITY),
    MAREA_G_KEY(MAREA_G_RULE_VISIBILITY),
    MAREA_G_KEY(MAREA_G_RULE_VISIBILITY) | MAREA_G_KEY(MAREA_G_RULE_ALIGN),
    MAREA_G_KEY(MAREA_G_RULE_VISIBILITY) | MAREA_G_KEY(MAREA_G_RULE_INLINE) | MAREA_G_KEY(MAREA_G_RULE_BUILD) | MAREA_G_KEY(MAREA_G_RULE_OPERATOR) | MAREA_G_KEY(MAREA_G_RULE_EXPORT) | MAREA_G_KEY(MAREA_G_RULE_BACKEND),
    MAREA_G_KEY(MAREA_G_RULE_VISIBILITY),
    MAREA_G_KEY(MAREA_G_RULE_SEMANTICS),
    0,
    MAREA_G_ALL_KEYS,
};

static MAREA_Boolean MAREA_CALL MAREA_G_ParentIs(const MAREA_SemaContext *context, MAREA_AstKind kind)
{
    return ((context->parent != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(context->parent) == kind)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: the node that owns the list says the place. A function or a constant is read by its parent: the block of an implementation, or the file.
static MAREA_G_Place MAREA_CALL MAREA_G_PlaceOf(const MAREA_SemaContext *context, const MAREA_AstNode *owner)
{
    switch (MAREA_AstNodeGetKind(owner)) {
    case MAREA_AST_KIND_COMPILATION_UNIT:
        return MAREA_G_PLACE_FILE;
    case MAREA_AST_KIND_FUNCTION_DECLARATION:
        return (MAREA_G_ParentIs(context, MAREA_AST_KIND_IMPLEMENTATION_DECLARATION) == MAREA_BOOLEAN_TRUE) ? MAREA_G_PLACE_IMPLEMENTATION_FUNCTION : MAREA_G_PLACE_TOP_FUNCTION;
    case MAREA_AST_KIND_CONSTANT_DECLARATION:
        return (MAREA_G_ParentIs(context, MAREA_AST_KIND_IMPLEMENTATION_DECLARATION) == MAREA_BOOLEAN_TRUE) ? MAREA_G_PLACE_IMPLEMENTATION_CONSTANT : MAREA_G_PLACE_TOP_CONSTANT;
    case MAREA_AST_KIND_STRUCTURE_DECLARATION:
        return MAREA_G_PLACE_STRUCTURE;
    case MAREA_AST_KIND_OPAQUE_DECLARATION:
        return MAREA_G_PLACE_OPAQUE_STRUCTURE;
    case MAREA_AST_KIND_ENUM_DECLARATION:
        return MAREA_G_PLACE_ENUM;
    case MAREA_AST_KIND_TYPE_DECLARATION:
        return MAREA_G_PLACE_TYPE_ALIAS;
    case MAREA_AST_KIND_CONTRACT_DECLARATION:
        return MAREA_G_PLACE_CONTRACT;
    case MAREA_AST_KIND_IMPLEMENTATION_DECLARATION:
        return MAREA_G_PLACE_IMPLEMENTATION;
    case MAREA_AST_KIND_FIELD_DECLARATION:
        return MAREA_G_PLACE_FIELD;
    case MAREA_AST_KIND_PARAMETER:
        return MAREA_G_PLACE_PARAMETER;
    case MAREA_AST_KIND_MACRO_STATEMENT:
        return MAREA_G_PLACE_MACRO_STATEMENT;
    default:
        return MAREA_G_PLACE_OTHER;
    }
}

static MAREA_Boolean MAREA_CALL MAREA_G_HasChildOfKind(const MAREA_AstNode *node, MAREA_AstKind kind)
{
    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(node); ++index) {
        const MAREA_AstNode *child = MAREA_SemaChildAt(node, index);

        if ((child != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(child) == kind)) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

// INFO: C has no generics, so a function with a generic parameter of its own, or in a generic implementation block, is no function to export.
static MAREA_Boolean MAREA_CALL MAREA_G_IsGenericFunction(const MAREA_SemaContext *context, const MAREA_AstNode *function)
{
    if (MAREA_G_HasChildOfKind(function, MAREA_AST_KIND_GENERIC_PARAMETERS) == MAREA_BOOLEAN_TRUE) {
        return MAREA_BOOLEAN_TRUE;
    }

    return ((MAREA_G_ParentIs(context, MAREA_AST_KIND_IMPLEMENTATION_DECLARATION) == MAREA_BOOLEAN_TRUE) && (MAREA_G_HasChildOfKind(context->parent, MAREA_AST_KIND_GENERIC_PARAMETERS) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: backend is allowed in a file whose namespace is marea.math or starts with marea.math. and nowhere else.
static MAREA_SemaExcept MAREA_CALL MAREA_G_FileIsMareaMath(MAREA_SemaContext *context, MAREA_Boolean *outInMath)
{
    static const MAREA_Character8 MATH[] = "marea.math";

    *outInMath = MAREA_BOOLEAN_FALSE;

    const MAREA_AstNode *declaration = (context->root == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_SemaChildAt(context->root, 0);

    const MAREA_AstNode *qualified = ((declaration != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(declaration) == MAREA_AST_KIND_NAMESPACE_DECLARATION)) ? MAREA_SemaChildAt(declaration, 0) : MAREA_NULLPTR;

    if ((qualified == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(qualified) != MAREA_AST_KIND_QUALIFIED_NAME)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *text = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    const MAREA_SemaExcept JOIN_RESULT = MAREA_SemaQualifiedNameJoin(context, qualified, &text, &length);

    if ((JOIN_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) || (text == MAREA_NULLPTR)) {
        return JOIN_RESULT;
    }

    const MAREA_IntegerSize MATH_LENGTH = (sizeof(MATH) - 1);

    *outInMath = ((length >= MATH_LENGTH) && (memcmp(text, MATH, MATH_LENGTH) == 0) && ((length == MATH_LENGTH) || (text[MATH_LENGTH] == '.'))) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

// ================================ the target, the predicates and the backends ================================

typedef enum MAREA_G_PredicateKind
{
    MAREA_G_PREDICATE_KIND_ARCHITECTURE = MAREA_UINTEGER_8(0),
    MAREA_G_PREDICATE_KIND_OPERATING_SYSTEM = MAREA_UINTEGER_8(1),
    MAREA_G_PREDICATE_KIND_FEATURE = MAREA_UINTEGER_8(2),
    MAREA_G_PREDICATE_KIND_COMPILER = MAREA_UINTEGER_8(3),
    MAREA_G_PREDICATE_KIND_POINTER_WIDTH = MAREA_UINTEGER_8(4),
    MAREA_G_PREDICATE_KIND_ENDIANNESS = MAREA_UINTEGER_8(5),
    MAREA_G_PREDICATE_KIND_COUNT = MAREA_UINTEGER_8(6),
} MAREA_G_PredicateKind;

// INFO: the six predicate names of the plan (file 17, section 3), in the order of MAREA_G_PredicateKind. pointer_width is the only one that takes an integer.
static const MAREA_Character8 *const MAREA_G_PREDICATE_NAMES[MAREA_G_PREDICATE_KIND_COUNT] = { "architecture", "operating_system", "feature", "compiler", "pointer_width", "endianness" };

static MAREA_Boolean MAREA_CALL MAREA_G_TargetTextIs(const MAREA_Character8 *field, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    return ((field != MAREA_NULLPTR) && (strlen(field) == length) && (memcmp(field, text, length) == 0)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_TargetHasFeature(const MAREA_SemaTarget *target, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    if ((target == MAREA_NULLPTR) || (target->features == MAREA_NULLPTR)) {
        return MAREA_BOOLEAN_FALSE;
    }

    for (MAREA_IntegerSize index = 0; index < target->featureCount; ++index) {
        if (MAREA_G_TargetTextIs(target->features[index], text, length) == MAREA_BOOLEAN_TRUE) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

// INFO: a comparison that the target does not answer is false: no target, a field that the caller left empty, or a feature list without the feature.
static MAREA_Boolean MAREA_CALL MAREA_G_ComparisonHolds(const MAREA_SemaTarget *target, MAREA_G_PredicateKind kind, const MAREA_AstNode *value)
{
    if (target == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    if (kind == MAREA_G_PREDICATE_KIND_POINTER_WIDTH) {
        MAREA_Uinteger64 number = 0;

        return ((target->pointerWidth != 0) && (MAREA_G_ParseInteger(value, &number) == MAREA_BOOLEAN_TRUE) && (number == target->pointerWidth)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    }

    // INFO: the text of a string literal keeps its two quotes.
    if (MAREA_AstNodeGetTextLength(value) < 2) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_Character8 *text = (MAREA_AstNodeGetText(value) + 1);

    const MAREA_IntegerSize LENGTH = (MAREA_AstNodeGetTextLength(value) - 2);

    switch (kind) {
    case MAREA_G_PREDICATE_KIND_ARCHITECTURE:
        return MAREA_G_TargetTextIs(target->architecture, text, LENGTH);
    case MAREA_G_PREDICATE_KIND_OPERATING_SYSTEM:
        return MAREA_G_TargetTextIs(target->operatingSystem, text, LENGTH);
    case MAREA_G_PREDICATE_KIND_COMPILER:
        return MAREA_G_TargetTextIs(target->compiler, text, LENGTH);
    case MAREA_G_PREDICATE_KIND_ENDIANNESS:
        return MAREA_G_TargetTextIs(target->endianness, text, LENGTH);
    case MAREA_G_PREDICATE_KIND_FEATURE:
        return MAREA_G_TargetHasFeature(target, text, LENGTH);
    default:
        return MAREA_BOOLEAN_FALSE;
    }
}

// INFO: reads and judges one predicate. A name that is not one of the six, and a value of the wrong kind, are INVALID_ATTRIBUTE_VALUE once for each such predicate, and the predicate is false. A combinator is judged by its parts.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckPredicate(MAREA_SemaContext *context, const MAREA_AstNode *predicate, MAREA_Boolean *outHolds)
{
    *outHolds = MAREA_BOOLEAN_FALSE;

    if ((predicate == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(predicate) == MAREA_AST_KIND_ERROR_NODE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *head = MAREA_SemaChildAt(predicate, 0);

    if ((head == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(head) != MAREA_AST_KIND_IDENTIFIER)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    if (MAREA_AstNodeGetKind(predicate) == MAREA_AST_KIND_WHEN_COMBINATOR) {
        const MAREA_Boolean IS_ALL = MAREA_G_TextIs(head, "all");

        const MAREA_Boolean IS_NOT = MAREA_G_TextIs(head, "not");

        MAREA_Boolean combined = (IS_ALL == MAREA_BOOLEAN_TRUE) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

        for (MAREA_IntegerSize index = 1; index < MAREA_AstNodeGetChildCount(predicate); ++index) {
            MAREA_Boolean part = MAREA_BOOLEAN_FALSE;

            const MAREA_SemaExcept RESULT = MAREA_G_CheckPredicate(context, MAREA_SemaChildAt(predicate, index), &part);

            if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
                return RESULT;
            }

            combined = (IS_ALL == MAREA_BOOLEAN_TRUE) ? ((combined == MAREA_BOOLEAN_TRUE) && (part == MAREA_BOOLEAN_TRUE)) : ((combined == MAREA_BOOLEAN_TRUE) || (part == MAREA_BOOLEAN_TRUE));
        }

        *outHolds = (IS_NOT == MAREA_BOOLEAN_TRUE) ? ((MAREA_AstNodeGetChildCount(predicate) > 1) && (combined == MAREA_BOOLEAN_FALSE)) : combined;

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    if (MAREA_AstNodeGetKind(predicate) != MAREA_AST_KIND_WHEN_COMPARISON) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *value = MAREA_SemaChildAt(predicate, 1);

    if ((value == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(value) == MAREA_AST_KIND_ERROR_NODE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    MAREA_IntegerSize found = MAREA_G_PREDICATE_KIND_COUNT;

    for (MAREA_IntegerSize index = 0; index < MAREA_G_PREDICATE_KIND_COUNT; ++index) {
        if (MAREA_G_TextIs(head, MAREA_G_PREDICATE_NAMES[index]) == MAREA_BOOLEAN_TRUE) {
            found = index;
        }
    }

    if (found == MAREA_G_PREDICATE_KIND_COUNT) {
        return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_ATTRIBUTE_VALUE, MAREA_AstNodeGetPosition(predicate), "unknown predicate name:", MAREA_AstNodeGetText(head), MAREA_AstNodeGetTextLength(head));
    }

    const MAREA_AstKind NEEDED = (found == MAREA_G_PREDICATE_KIND_POINTER_WIDTH) ? MAREA_AST_KIND_INTEGER_LITERAL : MAREA_AST_KIND_STRING_LITERAL;

    if (MAREA_AstNodeGetKind(value) != NEEDED) {
        return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_ATTRIBUTE_VALUE, MAREA_AstNodeGetPosition(predicate), "invalid value for predicate:", MAREA_AstNodeGetText(head), MAREA_AstNodeGetTextLength(head));
    }

    *outHolds = MAREA_G_ComparisonHolds(context->target, (MAREA_G_PredicateKind)found, value);

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

// INFO: the member of the backend key that a value names, as one of the four literal names, or null for any other text.
static const MAREA_Character8 *MAREA_CALL MAREA_G_BackendMemberName(const MAREA_AstNode *member)
{
    for (MAREA_IntegerSize index = 0; MAREA_G_BackendMembers[index] != MAREA_NULLPTR; ++index) {
        if (MAREA_G_TextIs(member, MAREA_G_BackendMembers[index]) == MAREA_BOOLEAN_TRUE) {
            return MAREA_G_BackendMembers[index];
        }
    }

    return MAREA_NULLPTR;
}

// INFO: the table of the plan (file 17, section 2). scalar is always there. vector_extension needs gcc or clang and x86_64 with sse2, or aarch64. intrinsics needs gcc or clang and x86_64 with sse2, or aarch64 with neon.
static MAREA_Boolean MAREA_CALL MAREA_G_BackendNameAvailable(const MAREA_SemaTarget *target, const MAREA_Character8 *name)
{
    if ((strcmp(name, "auto") == 0) || (strcmp(name, "scalar") == 0)) {
        return MAREA_BOOLEAN_TRUE;
    }

    if ((target == MAREA_NULLPTR) || ((MAREA_G_TargetTextIs(target->compiler, "gcc", 3) == MAREA_BOOLEAN_FALSE) && (MAREA_G_TargetTextIs(target->compiler, "clang", 5) == MAREA_BOOLEAN_FALSE))) {
        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_G_TargetTextIs(target->architecture, "x86_64", 6) == MAREA_BOOLEAN_TRUE) {
        return MAREA_G_TargetHasFeature(target, "sse2", 4);
    }

    if (MAREA_G_TargetTextIs(target->architecture, "aarch64", 7) == MAREA_BOOLEAN_TRUE) {
        return (strcmp(name, "vector_extension") == 0) ? MAREA_BOOLEAN_TRUE : MAREA_G_TargetHasFeature(target, "neon", 4);
    }

    return MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_BackendAvailable(const MAREA_SemaTarget *target, const MAREA_AstNode *member)
{
    const MAREA_Character8 *name = MAREA_G_BackendMemberName(member);

    return (name == MAREA_NULLPTR) ? MAREA_BOOLEAN_FALSE : MAREA_G_BackendNameAvailable(target, name);
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_ReportUnavailable(MAREA_SemaContext *context, const MAREA_AstNode *item, const MAREA_AstNode *member)
{
    const MAREA_SemaTarget *target = context->target;

    const MAREA_Character8 *architecture = ((target != MAREA_NULLPTR) && (target->architecture != MAREA_NULLPTR)) ? target->architecture : "not given";

    const MAREA_Character8 *compiler = ((target != MAREA_NULLPTR) && (target->compiler != MAREA_NULLPTR)) ? target->compiler : "not given";

    const MAREA_Character8 *const PARTS[] = { "backend '", MAREA_AstNodeGetText(member), "' is not available, target architecture ", architecture, ", compiler ", compiler, "; it needs gcc or clang with sse2 on x86_64, or with aarch64 (neon for intrinsics), so use auto or scalar" };

    const MAREA_IntegerSize LENGTHS[] = { strlen(PARTS[0]), MAREA_AstNodeGetTextLength(member), strlen(PARTS[2]), strlen(architecture), strlen(PARTS[4]), strlen(compiler), strlen(PARTS[6]) };

    return MAREA_SemaReportParts(context, MAREA_SEMA_DIAGNOSTIC_CODE_BACKEND_UNAVAILABLE, MAREA_AstNodeGetPosition(item), PARTS, LENGTHS, MAREA_ARRAY_SIZE(PARTS));
}

// =========================================== the checks of a group ===========================================

// INFO: the state of the lists of one owner: the keys seen as plain items, the keys that a when item that holds gave, and the backend items that decide the backend (a holding when wins over the plain item).
typedef struct MAREA_G_Group
{
    MAREA_SemaContext *context;
    const MAREA_AstNode *owner;
    const MAREA_AstNode *plainBackend;
    const MAREA_AstNode *heldBackend;
    MAREA_G_Place place;
    MAREA_Boolean seen[MAREA_G_RULE_COUNT];
    MAREA_Boolean held[MAREA_G_RULE_COUNT];
} MAREA_G_Group;

// INFO: one item, plain or listed in a when item. A diagnostic of one step ends the item, so an item gives one diagnostic. A listed item is checked whether the predicate holds or not, and only a holding one takes part in the conflict and the backend rules.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckItem(MAREA_G_Group *group, const MAREA_AstNode *item, MAREA_Boolean listed, MAREA_Boolean holds)
{
    MAREA_SemaContext *context = group->context;

    const MAREA_AstNode *key = MAREA_SemaChildAt(item, 0);

    if ((key == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(key) != MAREA_AST_KIND_IDENTIFIER)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    MAREA_IntegerSize ruleIndex = MAREA_G_RULE_COUNT;

    for (MAREA_IntegerSize candidate = 0; candidate < MAREA_G_RULE_COUNT; ++candidate) {
        if (MAREA_G_TextIs(key, MAREA_G_RULES[candidate].key) == MAREA_BOOLEAN_TRUE) {
            ruleIndex = candidate;
        }
    }

    if (ruleIndex == MAREA_G_RULE_COUNT) {
        return MAREA_G_ReportValue(context, MAREA_SEMA_DIAGNOSTIC_CODE_UNKNOWN_ATTRIBUTE_KEY, item, "unknown attribute key:");
    }

    if (listed == MAREA_BOOLEAN_FALSE) {
        if (group->seen[ruleIndex] == MAREA_BOOLEAN_TRUE) {
            return MAREA_G_ReportValue(context, MAREA_SEMA_DIAGNOSTIC_CODE_CONFLICTING_ATTRIBUTES, item, "attribute given twice:");
        }

        group->seen[ruleIndex] = MAREA_BOOLEAN_TRUE;
    }

    const MAREA_IntegerSize BEFORE = MAREA_DiagnosticSinkGetCount(context->sink);

    const MAREA_SemaExcept VALUE_RESULT = MAREA_G_CheckValue(context, &MAREA_G_RULES[ruleIndex], item);

    if ((VALUE_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) || (MAREA_DiagnosticSinkGetCount(context->sink) != BEFORE)) {
        return VALUE_RESULT;
    }

    if (ruleIndex == MAREA_G_RULE_BACKEND) {
        MAREA_Boolean inMath = MAREA_BOOLEAN_FALSE;

        const MAREA_SemaExcept MATH_RESULT = MAREA_G_FileIsMareaMath(context, &inMath);

        if ((MATH_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) || (inMath == MAREA_BOOLEAN_FALSE)) {
            return (MATH_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) ? MATH_RESULT : MAREA_G_ReportValue(context, MAREA_SEMA_DIAGNOSTIC_CODE_ATTRIBUTE_NOT_ALLOWED_HERE, item, "attribute is allowed only in marea.math:");
        }
    }

    if ((MAREA_G_PLACE_KEYS[group->place] & MAREA_G_KEY(ruleIndex)) == 0) {
        return MAREA_G_ReportValue(context, MAREA_SEMA_DIAGNOSTIC_CODE_MISPLACED_ATTRIBUTE, item, "attribute is not allowed at this place:");
    }

    if ((ruleIndex == MAREA_G_RULE_EXPORT) && (MAREA_G_IsGenericFunction(context, group->owner) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_G_ReportValue(context, MAREA_SEMA_DIAGNOSTIC_CODE_MISPLACED_ATTRIBUTE, item, "a generic function cannot be exported:");
    }

    if (listed == MAREA_BOOLEAN_FALSE) {
        const MAREA_Boolean EXCLUDES = ((ruleIndex == MAREA_G_RULE_PACK) && (group->seen[MAREA_G_RULE_ALIGN] == MAREA_BOOLEAN_TRUE)) || ((ruleIndex == MAREA_G_RULE_ALIGN) && (group->seen[MAREA_G_RULE_PACK] == MAREA_BOOLEAN_TRUE));

        if (EXCLUDES == MAREA_BOOLEAN_TRUE) {
            return MAREA_G_ReportValue(context, MAREA_SEMA_DIAGNOSTIC_CODE_CONFLICTING_ATTRIBUTES, item, "attribute conflicts with another:");
        }

        group->plainBackend = (ruleIndex == MAREA_G_RULE_BACKEND) ? item : group->plainBackend;

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    if (holds == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    if (group->held[ruleIndex] == MAREA_BOOLEAN_TRUE) {
        return MAREA_G_ReportValue(context, MAREA_SEMA_DIAGNOSTIC_CODE_CONFLICTING_ATTRIBUTES, item, "attribute given twice:");
    }

    group->held[ruleIndex] = MAREA_BOOLEAN_TRUE;
    group->heldBackend = (ruleIndex == MAREA_G_RULE_BACKEND) ? item : group->heldBackend;

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

// INFO: the first child is the predicate and the rest are plain items.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckWhen(MAREA_G_Group *group, const MAREA_AstNode *when)
{
    MAREA_Boolean holds = MAREA_BOOLEAN_FALSE;

    const MAREA_SemaExcept PREDICATE_RESULT = MAREA_G_CheckPredicate(group->context, MAREA_SemaChildAt(when, 0), &holds);

    if (PREDICATE_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
        return PREDICATE_RESULT;
    }

    for (MAREA_IntegerSize index = 1; index < MAREA_AstNodeGetChildCount(when); ++index) {
        const MAREA_AstNode *item = MAREA_SemaChildAt(when, index);

        if ((item == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(item) != MAREA_AST_KIND_ATTRIBUTE_ITEM)) {
            continue;
        }

        const MAREA_SemaExcept RESULT = MAREA_G_CheckItem(group, item, MAREA_BOOLEAN_TRUE, holds);

        if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return RESULT;
        }
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

// INFO: the backend that decides is the one a holding when gives, else the plain one. An explicit backend that the target cannot give is BACKEND_UNAVAILABLE, with no silent fallback.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckBackend(MAREA_G_Group *group)
{
    const MAREA_AstNode *item = (group->heldBackend != MAREA_NULLPTR) ? group->heldBackend : group->plainBackend;

    const MAREA_AstNode *value = (item == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_SemaChildAt(item, 1);

    const MAREA_AstNode *member = ((value != MAREA_NULLPTR) && (MAREA_G_IsSingleName(value) == MAREA_BOOLEAN_TRUE)) ? MAREA_SemaChildAt(value, 0) : MAREA_NULLPTR;

    if ((member == MAREA_NULLPTR) || (MAREA_G_BackendAvailable(group->context->target, member) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_G_ReportUnavailable(group->context, item, member);
}

// INFO: the item of the key backend that decides in the lists of one owner: a holding when item wins over a plain item. Null when the owner has none. The check already judged every predicate, so none is reported here.
static const MAREA_AstNode *MAREA_CALL MAREA_G_FindBackendItem(MAREA_SemaContext *context, const MAREA_AstNode *owner, MAREA_AstKind listKind)
{
    const MAREA_AstNode *plain = MAREA_NULLPTR;
    const MAREA_AstNode *held = MAREA_NULLPTR;

    for (MAREA_IntegerSize listIndex = 0; listIndex < MAREA_AstNodeGetChildCount(owner); ++listIndex) {
        const MAREA_AstNode *list = MAREA_SemaChildAt(owner, listIndex);

        if ((list == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(list) != listKind)) {
            continue;
        }

        for (MAREA_IntegerSize itemIndex = 0; itemIndex < MAREA_AstNodeGetChildCount(list); ++itemIndex) {
            const MAREA_AstNode *item = MAREA_SemaChildAt(list, itemIndex);

            if (item == MAREA_NULLPTR) {
                continue;
            }

            if (MAREA_AstNodeGetKind(item) == MAREA_AST_KIND_ATTRIBUTE_ITEM) {
                const MAREA_AstNode *key = MAREA_SemaChildAt(item, 0);

                plain = ((key != MAREA_NULLPTR) && (MAREA_G_TextIs(key, "backend") == MAREA_BOOLEAN_TRUE)) ? item : plain;

                continue;
            }

            if (MAREA_AstNodeGetKind(item) != MAREA_AST_KIND_WHEN_ITEM) {
                continue;
            }

            MAREA_Boolean holds = MAREA_BOOLEAN_FALSE;

            if ((MAREA_G_CheckPredicate(context, MAREA_SemaChildAt(item, 0), &holds) != MAREA_SEMA_EXCEPT_SUCCESS) || (holds == MAREA_BOOLEAN_FALSE)) {
                continue;
            }

            for (MAREA_IntegerSize partIndex = 1; partIndex < MAREA_AstNodeGetChildCount(item); ++partIndex) {
                const MAREA_AstNode *part = MAREA_SemaChildAt(item, partIndex);

                const MAREA_AstNode *key = ((part != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(part) == MAREA_AST_KIND_ATTRIBUTE_ITEM)) ? MAREA_SemaChildAt(part, 0) : MAREA_NULLPTR;

                held = ((key != MAREA_NULLPTR) && (MAREA_G_TextIs(key, "backend") == MAREA_BOOLEAN_TRUE)) ? part : held;
            }
        }
    }

    return (held != MAREA_NULLPTR) ? held : plain;
}

// INFO: the member that an item of the key backend names, or null when its value is not a single name.
static const MAREA_AstNode *MAREA_CALL MAREA_G_BackendItemMember(const MAREA_AstNode *item)
{
    const MAREA_AstNode *value = (item == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_SemaChildAt(item, 1);

    return ((value != MAREA_NULLPTR) && (MAREA_G_IsSingleName(value) == MAREA_BOOLEAN_TRUE)) ? MAREA_SemaChildAt(value, 0) : MAREA_NULLPTR;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaResolveBackend(MAREA_SemaContext *context, const MAREA_AstNode *function, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength)
{
    if ((context == MAREA_NULLPTR) || (function == MAREA_NULLPTR) || (outText == MAREA_NULLPTR) || (outLength == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    *outText = MAREA_NULLPTR;
    *outLength = 0;

    MAREA_Boolean inMath = MAREA_BOOLEAN_FALSE;

    const MAREA_SemaExcept MATH_RESULT = MAREA_G_FileIsMareaMath(context, &inMath);

    if ((MATH_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) || (inMath == MAREA_BOOLEAN_FALSE)) {
        return MATH_RESULT;
    }

    // INFO: the attribute of the function wins over the attribute of the file.
    const MAREA_AstNode *item = MAREA_G_FindBackendItem(context, function, MAREA_AST_KIND_ATTRIBUTE);

    if (item == MAREA_NULLPTR) {
        item = MAREA_G_FindBackendItem(context, context->root, MAREA_AST_KIND_FILE_ATTRIBUTE);
    }

    const MAREA_AstNode *member = MAREA_G_BackendItemMember(item);

    const MAREA_Character8 *name = (member == MAREA_NULLPTR) ? "auto" : MAREA_G_BackendMemberName(member);

    if (name == MAREA_NULLPTR) {
        name = "auto";
    }

    // INFO: auto takes the best backend that the target gives and never fails: intrinsics, then vector_extension, then scalar.
    if (strcmp(name, "auto") == 0) {
        name = (MAREA_G_BackendNameAvailable(context->target, "intrinsics") == MAREA_BOOLEAN_TRUE) ? "intrinsics" : ((MAREA_G_BackendNameAvailable(context->target, "vector_extension") == MAREA_BOOLEAN_TRUE) ? "vector_extension" : "scalar");
    }

    *outText = name;
    *outLength = strlen(name);

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckGroup(MAREA_SemaContext *context, const MAREA_AstNode *owner, MAREA_AstKind listKind)
{
    MAREA_G_Group group = { .context = context, .owner = owner, .plainBackend = MAREA_NULLPTR, .heldBackend = MAREA_NULLPTR, .place = MAREA_G_PlaceOf(context, owner), .seen = { MAREA_BOOLEAN_FALSE }, .held = { MAREA_BOOLEAN_FALSE } };

    const MAREA_IntegerSize LIST_COUNT = MAREA_AstNodeGetChildCount(owner);

    for (MAREA_IntegerSize listIndex = 0; listIndex < LIST_COUNT; ++listIndex) {
        const MAREA_AstNode *list = MAREA_SemaChildAt(owner, listIndex);

        if ((list == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(list) != listKind)) {
            continue;
        }

        const MAREA_IntegerSize ITEM_COUNT = MAREA_AstNodeGetChildCount(list);

        for (MAREA_IntegerSize itemIndex = 0; itemIndex < ITEM_COUNT; ++itemIndex) {
            const MAREA_AstNode *item = MAREA_SemaChildAt(list, itemIndex);

            if (item == MAREA_NULLPTR) {
                continue;
            }

            MAREA_SemaExcept result = MAREA_SEMA_EXCEPT_SUCCESS;

            if (MAREA_AstNodeGetKind(item) == MAREA_AST_KIND_ATTRIBUTE_ITEM) {
                result = MAREA_G_CheckItem(&group, item, MAREA_BOOLEAN_FALSE, MAREA_BOOLEAN_FALSE);
            } else if (MAREA_AstNodeGetKind(item) == MAREA_AST_KIND_WHEN_ITEM) {
                result = MAREA_G_CheckWhen(&group, item);
            }

            if (result != MAREA_SEMA_EXCEPT_SUCCESS) {
                return result;
            }
        }
    }

    return MAREA_G_CheckBackend(&group);
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckAttributes(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    if (node == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    if (MAREA_AstNodeGetKind(node) == MAREA_AST_KIND_COMPILATION_UNIT) {
        return MAREA_G_CheckGroup(context, node, MAREA_AST_KIND_FILE_ATTRIBUTE);
    }

    return MAREA_G_CheckGroup(context, node, MAREA_AST_KIND_ATTRIBUTE);
}

static const MAREA_AstNode *MAREA_CALL MAREA_G_FindItem(const MAREA_AstNode *owner, MAREA_AstKind listKind, const MAREA_Character8 *key)
{
    const MAREA_IntegerSize LIST_COUNT = MAREA_AstNodeGetChildCount(owner);

    for (MAREA_IntegerSize listIndex = 0; listIndex < LIST_COUNT; ++listIndex) {
        const MAREA_AstNode *list = MAREA_SemaChildAt(owner, listIndex);

        if ((list == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(list) != listKind)) {
            continue;
        }

        const MAREA_IntegerSize ITEM_COUNT = MAREA_AstNodeGetChildCount(list);

        for (MAREA_IntegerSize itemIndex = 0; itemIndex < ITEM_COUNT; ++itemIndex) {
            const MAREA_AstNode *item = MAREA_SemaChildAt(list, itemIndex);

            const MAREA_AstNode *itemKey = (item == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_SemaChildAt(item, 0);

            if ((itemKey != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(itemKey) == MAREA_AST_KIND_IDENTIFIER) && (MAREA_G_TextIs(itemKey, key) == MAREA_BOOLEAN_TRUE)) {
                return item;
            }
        }
    }

    return MAREA_NULLPTR;
}

const MAREA_AstNode *MAREA_CALL MAREA_SemaAttributeFind(const MAREA_AstNode *root, const MAREA_AstNode *declaration, const MAREA_Character8 *key)
{
    const MAREA_AstNode *item = (declaration == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_G_FindItem(declaration, MAREA_AST_KIND_ATTRIBUTE, key);

    if ((item == MAREA_NULLPTR) && (root != MAREA_NULLPTR)) {
        item = MAREA_G_FindItem(root, MAREA_AST_KIND_FILE_ATTRIBUTE, key);
    }

    return item;
}

MAREA_Boolean MAREA_CALL MAREA_SemaReadIntegerLiteral(const MAREA_AstNode *node, MAREA_Uinteger64 *outValue)
{
    if ((node == MAREA_NULLPTR) || (outValue == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(node) != MAREA_AST_KIND_INTEGER_LITERAL)) {
        return MAREA_BOOLEAN_FALSE;
    }

    return MAREA_G_ParseInteger(node, outValue);
}

MAREA_Boolean MAREA_CALL MAREA_SemaAttributeReadInteger(const MAREA_AstNode *item, MAREA_Uinteger64 *outValue)
{
    return MAREA_SemaReadIntegerLiteral(MAREA_SemaChildAt(item, 1), outValue);
}

MAREA_Boolean MAREA_CALL MAREA_SemaAttributeIsOn(const MAREA_AstNode *root, const MAREA_AstNode *declaration, const MAREA_Character8 *key)
{
    const MAREA_AstNode *item = MAREA_SemaAttributeFind(root, declaration, key);

    return (item == MAREA_NULLPTR) ? MAREA_BOOLEAN_FALSE : MAREA_G_IsOn(item);
}

MAREA_Boolean MAREA_CALL MAREA_SemaIsPrivate(const MAREA_AstNode *root, const MAREA_AstNode *declaration)
{
    return MAREA_SemaAttributeHasMember(root, declaration, "visibility", "private");
}

MAREA_Boolean MAREA_CALL MAREA_SemaAttributeHasMember(const MAREA_AstNode *root, const MAREA_AstNode *declaration, const MAREA_Character8 *key, const MAREA_Character8 *member)
{
    const MAREA_AstNode *item = MAREA_SemaAttributeFind(root, declaration, key);

    const MAREA_AstNode *value = (item == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_SemaChildAt(item, 1);

    if ((value == MAREA_NULLPTR) || (MAREA_G_IsSingleName(value) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *name = MAREA_SemaChildAt(value, 0);

    return ((name != MAREA_NULLPTR) && (MAREA_G_TextIs(name, member) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}
