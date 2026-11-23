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

enum
{
    MAREA_G_RULE_ALIGN = 4,
    MAREA_G_RULE_PACK = 5,
    MAREA_G_RULE_COUNT = 10,
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
    { .key = "export", .members = MAREA_NULLPTR, .shape = MAREA_G_ATTRIBUTE_SHAPE_STRING },
    // INFO: the one and only parameter-passing mechanism; MAREA_SemaGetParameterSemantics in
    // MAREASemaOwnership.c is the one place the rest of sema reads it.
    { .key = "semantics", .members = MAREA_G_SemanticsMembers, .shape = MAREA_G_ATTRIBUTE_SHAPE_MEMBER },
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

static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckGroup(MAREA_SemaContext *context, const MAREA_AstNode *owner, MAREA_AstKind listKind)
{
    MAREA_Boolean seen[MAREA_G_RULE_COUNT] = { MAREA_BOOLEAN_FALSE };

    const MAREA_IntegerSize LIST_COUNT = MAREA_AstNodeGetChildCount(owner);

    for (MAREA_IntegerSize listIndex = 0; listIndex < LIST_COUNT; ++listIndex) {
        const MAREA_AstNode *list = MAREA_SemaChildAt(owner, listIndex);

        if ((list == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(list) != listKind)) {
            continue;
        }

        const MAREA_IntegerSize ITEM_COUNT = MAREA_AstNodeGetChildCount(list);

        for (MAREA_IntegerSize itemIndex = 0; itemIndex < ITEM_COUNT; ++itemIndex) {
            const MAREA_AstNode *item = MAREA_SemaChildAt(list, itemIndex);

            if ((item == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(item) != MAREA_AST_KIND_ATTRIBUTE_ITEM)) {
                continue;
            }

            const MAREA_AstNode *key = MAREA_SemaChildAt(item, 0);

            if ((key == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(key) != MAREA_AST_KIND_IDENTIFIER)) {
                continue;
            }

            MAREA_IntegerSize ruleIndex = MAREA_G_RULE_COUNT;

            for (MAREA_IntegerSize candidate = 0; candidate < MAREA_G_RULE_COUNT; ++candidate) {
                if (MAREA_G_TextIs(key, MAREA_G_RULES[candidate].key) == MAREA_BOOLEAN_TRUE) {
                    ruleIndex = candidate;
                }
            }

            MAREA_SemaExcept result = MAREA_SEMA_EXCEPT_SUCCESS;

            if (ruleIndex == MAREA_G_RULE_COUNT) {
                result = MAREA_G_ReportValue(context, MAREA_SEMA_DIAGNOSTIC_CODE_UNKNOWN_ATTRIBUTE_KEY, item, "unknown attribute key:");
            } else if (seen[ruleIndex] == MAREA_BOOLEAN_TRUE) {
                result = MAREA_G_ReportValue(context, MAREA_SEMA_DIAGNOSTIC_CODE_CONFLICTING_ATTRIBUTES, item, "attribute given twice:");
            } else {
                seen[ruleIndex] = MAREA_BOOLEAN_TRUE;

                result = MAREA_G_CheckValue(context, &MAREA_G_RULES[ruleIndex], item);

                const MAREA_Boolean EXCLUDES = ((ruleIndex == MAREA_G_RULE_PACK) && (seen[MAREA_G_RULE_ALIGN] == MAREA_BOOLEAN_TRUE)) || ((ruleIndex == MAREA_G_RULE_ALIGN) && (seen[MAREA_G_RULE_PACK] == MAREA_BOOLEAN_TRUE));

                if ((result == MAREA_SEMA_EXCEPT_SUCCESS) && (EXCLUDES == MAREA_BOOLEAN_TRUE)) {
                    result = MAREA_G_ReportValue(context, MAREA_SEMA_DIAGNOSTIC_CODE_CONFLICTING_ATTRIBUTES, item, "attribute conflicts with another:");
                }
            }

            if (result != MAREA_SEMA_EXCEPT_SUCCESS) {
                return result;
            }
        }
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
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

MAREA_Boolean MAREA_CALL MAREA_SemaAttributeReadInteger(const MAREA_AstNode *item, MAREA_Uinteger64 *outValue)
{
    const MAREA_AstNode *value = MAREA_SemaChildAt(item, 1);

    if ((value == MAREA_NULLPTR) || (outValue == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(value) != MAREA_AST_KIND_INTEGER_LITERAL)) {
        return MAREA_BOOLEAN_FALSE;
    }

    return MAREA_G_ParseInteger(value, outValue);
}

MAREA_Boolean MAREA_CALL MAREA_SemaAttributeIsOn(const MAREA_AstNode *root, const MAREA_AstNode *declaration, const MAREA_Character8 *key)
{
    const MAREA_AstNode *item = MAREA_SemaAttributeFind(root, declaration, key);

    return (item == MAREA_NULLPTR) ? MAREA_BOOLEAN_FALSE : MAREA_G_IsOn(item);
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
