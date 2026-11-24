// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <string.h>

static MAREA_Boolean MAREA_CALL MAREA_G_IsKind(const MAREA_AstNode *node, MAREA_AstKind kind)
{
    return ((node != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(node) == kind)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsWord(const MAREA_AstNode *node, MAREA_TokenKind word)
{
    return ((MAREA_G_IsKind(node, MAREA_AST_KIND_UNARY_EXPRESSION) == MAREA_BOOLEAN_TRUE) && (MAREA_AstNodeGetOperator(node) == word)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static const MAREA_AstNode *MAREA_CALL MAREA_G_RootOf(const MAREA_AstNode *place)
{
    const MAREA_AstNode *root = place;

    while ((MAREA_G_IsKind(root, MAREA_AST_KIND_MEMBER_ACCESS) == MAREA_BOOLEAN_TRUE) || (MAREA_G_IsKind(root, MAREA_AST_KIND_INDEX_ACCESS) == MAREA_BOOLEAN_TRUE)) {
        root = MAREA_SemaChildAt(root, 0);
    }

    return (MAREA_G_IsKind(root, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_TRUE) ? root : MAREA_NULLPTR;
}

#define MAREA_G_CHAIN_LIMIT MAREA_INTEGER_SIZE(32)

static MAREA_Boolean MAREA_CALL MAREA_G_IsChain(const MAREA_AstNode *node)
{
    return ((MAREA_G_IsKind(node, MAREA_AST_KIND_MEMBER_ACCESS) == MAREA_BOOLEAN_TRUE) || (MAREA_G_IsKind(node, MAREA_AST_KIND_INDEX_ACCESS) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: the links of a place from the root outward, so the first link is the access nearest the root; returns the root, or null when the place has no name root or is too long.
static const MAREA_AstNode *MAREA_CALL MAREA_G_Links(const MAREA_AstNode *place, const MAREA_AstNode **links, MAREA_IntegerSize *outCount)
{
    MAREA_IntegerSize count = 0;

    const MAREA_AstNode *node = place;

    while (MAREA_G_IsChain(node) == MAREA_BOOLEAN_TRUE) {
        if (count == MAREA_G_CHAIN_LIMIT) {
            return MAREA_NULLPTR;
        }

        links[count] = node;
        count += 1;
        node = MAREA_SemaChildAt(node, 0);
    }

    *outCount = count;

    return (MAREA_G_IsKind(node, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_TRUE) ? node : MAREA_NULLPTR;
}

// INFO: two links that are both accesses of one kind differ only when they name two fields, or when both indices are integer literals of two values; any other index may be the same element.
static MAREA_Boolean MAREA_CALL MAREA_G_LinksDiffer(const MAREA_AstNode *left, const MAREA_AstNode *right)
{
    if (MAREA_AstNodeGetKind(left) != MAREA_AstNodeGetKind(right)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *leftKey = MAREA_SemaChildAt(left, 1);

    const MAREA_AstNode *rightKey = MAREA_SemaChildAt(right, 1);

    if ((leftKey == MAREA_NULLPTR) || (rightKey == MAREA_NULLPTR)) {
        return MAREA_BOOLEAN_FALSE;
    }

    if ((MAREA_AstNodeGetKind(left) == MAREA_AST_KIND_INDEX_ACCESS) && ((MAREA_G_IsKind(leftKey, MAREA_AST_KIND_INTEGER_LITERAL) == MAREA_BOOLEAN_FALSE) || (MAREA_G_IsKind(rightKey, MAREA_AST_KIND_INTEGER_LITERAL) == MAREA_BOOLEAN_FALSE))) {
        return MAREA_BOOLEAN_FALSE;
    }

    return (MAREA_SemaTextEquals(MAREA_AstNodeGetText(leftKey), MAREA_AstNodeGetTextLength(leftKey), MAREA_AstNodeGetText(rightKey), MAREA_AstNodeGetTextLength(rightKey)) == MAREA_BOOLEAN_TRUE) ? MAREA_BOOLEAN_FALSE : MAREA_BOOLEAN_TRUE;
}

// INFO: two places alias when they share a root and one chain is the start of the other, so a value and a field of it alias, two fields do not, and two elements alias unless both indices are literals of two values.
static MAREA_Boolean MAREA_CALL MAREA_G_Alias(const MAREA_AstNode *left, const MAREA_AstNode *right)
{
    const MAREA_AstNode *leftLinks[MAREA_G_CHAIN_LIMIT];

    const MAREA_AstNode *rightLinks[MAREA_G_CHAIN_LIMIT];

    MAREA_IntegerSize leftCount = 0;

    MAREA_IntegerSize rightCount = 0;

    const MAREA_AstNode *leftRoot = MAREA_G_Links(left, leftLinks, &leftCount);

    const MAREA_AstNode *rightRoot = MAREA_G_Links(right, rightLinks, &rightCount);

    if ((leftRoot == MAREA_NULLPTR) || (rightRoot == MAREA_NULLPTR) || (MAREA_SemaTextEquals(MAREA_AstNodeGetText(leftRoot), MAREA_AstNodeGetTextLength(leftRoot), MAREA_AstNodeGetText(rightRoot), MAREA_AstNodeGetTextLength(rightRoot)) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_IntegerSize SHARED = (leftCount < rightCount) ? leftCount : rightCount;

    for (MAREA_IntegerSize index = 0; index < SHARED; ++index) {
        if (MAREA_G_LinksDiffer(leftLinks[(leftCount - 1 - index)], rightLinks[(rightCount - 1 - index)]) == MAREA_BOOLEAN_TRUE) {
            return MAREA_BOOLEAN_FALSE;
        }
    }

    return MAREA_BOOLEAN_TRUE;
}

// INFO: the loop name of a for each is the last name before the source, and it stands for an element of the source.
static const MAREA_AstNode *MAREA_CALL MAREA_G_LoopSource(const MAREA_SemaContext *context, const MAREA_AstNode *root)
{
    for (MAREA_IntegerSize index = context->references.count; index > 0; --index) {
        const MAREA_SemaName *entry = &context->references.entries[(index - 1)];

        if (MAREA_SemaTextEquals(entry->text, entry->length, MAREA_AstNodeGetText(root), MAREA_AstNodeGetTextLength(root)) == MAREA_BOOLEAN_FALSE) {
            continue;
        }

        if ((entry->declaration == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(entry->declaration) != MAREA_AST_KIND_FOR_EACH_STATEMENT)) {
            return MAREA_NULLPTR;
        }

        const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(entry->declaration);

        const MAREA_AstNode *loopName = (COUNT >= 3) ? MAREA_SemaChildAt(entry->declaration, (COUNT - 3)) : MAREA_NULLPTR;

        if ((loopName == MAREA_NULLPTR) || (MAREA_SemaTextEquals(MAREA_AstNodeGetText(loopName), MAREA_AstNodeGetTextLength(loopName), entry->text, entry->length) == MAREA_BOOLEAN_FALSE)) {
            return MAREA_NULLPTR;
        }

        return MAREA_SemaChildAt(entry->declaration, (COUNT - 2));
    }

    return MAREA_NULLPTR;
}

static const MAREA_AstNode *MAREA_CALL MAREA_G_ArgumentPlace(const MAREA_SemaContext *context, const MAREA_AstNode *argument, MAREA_Boolean *outMoved)
{
    const MAREA_AstNode *place = argument;

    *outMoved = MAREA_BOOLEAN_FALSE;

    // INFO: a copy makes a new value, so it is no use of the place.
    if (MAREA_G_IsWord(place, MAREA_TOKEN_KIND_KEYWORD_COPY) == MAREA_BOOLEAN_TRUE) {
        return MAREA_NULLPTR;
    }

    if (MAREA_G_IsWord(place, MAREA_TOKEN_KIND_KEYWORD_MOVE) == MAREA_BOOLEAN_TRUE) {
        *outMoved = MAREA_BOOLEAN_TRUE;

        place = MAREA_SemaChildAt(place, 0);
    }

    const MAREA_AstNode *root = MAREA_G_RootOf(place);

    if (root == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    const MAREA_AstNode *source = MAREA_G_LoopSource(context, root);

    return (source != MAREA_NULLPTR) ? source : place;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckAliasedArguments(MAREA_SemaContext *context, const MAREA_AstNode *call, const MAREA_AstNode *parameters, const MAREA_AstNode *arguments)
{
    if ((context == MAREA_NULLPTR) || (call == MAREA_NULLPTR) || (parameters == MAREA_NULLPTR) || (arguments == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(arguments);

    for (MAREA_IntegerSize first = 0; first < COUNT; ++first) {
        const MAREA_AstNode *left = MAREA_SemaChildAt(arguments, first);

        MAREA_Boolean leftMoved = MAREA_BOOLEAN_FALSE;

        const MAREA_AstNode *leftPlace = MAREA_G_ArgumentPlace(context, left, &leftMoved);

        const MAREA_AstNode *leftParameter = MAREA_SemaChildAt(parameters, first);

        if (leftPlace == MAREA_NULLPTR) {
            continue;
        }

        const MAREA_Boolean LEFT_WRITE = ((leftMoved == MAREA_BOOLEAN_TRUE) || ((leftParameter != MAREA_NULLPTR) && (MAREA_SemaGetParameterSemantics(context, leftParameter) == MAREA_SEMA_PARAMETER_SEMANTICS_REFERENCE_MUTABLE))) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

        for (MAREA_IntegerSize second = (first + 1); second < COUNT; ++second) {
            const MAREA_AstNode *right = MAREA_SemaChildAt(arguments, second);

            // INFO: one bare name given twice is the single-name rule of MAREA_SemaCheckBorrowArguments.
            if ((MAREA_G_IsKind(left, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_TRUE) && (MAREA_G_IsKind(right, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_TRUE) && (MAREA_SemaTextEquals(MAREA_AstNodeGetText(left), MAREA_AstNodeGetTextLength(left), MAREA_AstNodeGetText(right), MAREA_AstNodeGetTextLength(right)) == MAREA_BOOLEAN_TRUE)) {
                continue;
            }

            MAREA_Boolean rightMoved = MAREA_BOOLEAN_FALSE;

            const MAREA_AstNode *rightPlace = MAREA_G_ArgumentPlace(context, right, &rightMoved);

            const MAREA_AstNode *rightParameter = MAREA_SemaChildAt(parameters, second);

            const MAREA_Boolean RIGHT_WRITE = ((rightMoved == MAREA_BOOLEAN_TRUE) || ((rightParameter != MAREA_NULLPTR) && (MAREA_SemaGetParameterSemantics(context, rightParameter) == MAREA_SEMA_PARAMETER_SEMANTICS_REFERENCE_MUTABLE))) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

            if ((rightPlace == MAREA_NULLPTR) || ((LEFT_WRITE == MAREA_BOOLEAN_FALSE) && (RIGHT_WRITE == MAREA_BOOLEAN_FALSE)) || (MAREA_G_Alias(leftPlace, rightPlace) == MAREA_BOOLEAN_FALSE)) {
                continue;
            }

            const MAREA_AstNode *root = MAREA_G_RootOf(leftPlace);

            return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_EXCLUSIVE_BORROW_CONFLICT, MAREA_AstNodeGetPosition(call), "a write borrow is passed together with another use of", MAREA_AstNodeGetText(root), MAREA_AstNodeGetTextLength(root));
        }
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

// INFO: the library calls that change a collection in place and that sema knows by name; any other library call that changes its receiver is not seen.
static MAREA_Boolean MAREA_CALL MAREA_G_IsChangingMethod(const MAREA_AstNode *method)
{
    static const MAREA_Character8 *const NAMES[] = { "append", "insert", "remove", "clear", "pop", "push", "resize" };

    if (MAREA_G_IsKind(method, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_FALSE) {
        return MAREA_BOOLEAN_FALSE;
    }

    for (MAREA_IntegerSize index = 0; index < (sizeof(NAMES) / sizeof(NAMES[0])); ++index) {
        if (MAREA_SemaTextEquals(MAREA_AstNodeGetText(method), MAREA_AstNodeGetTextLength(method), NAMES[index], strlen(NAMES[index])) == MAREA_BOOLEAN_TRUE) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_ReportChange(MAREA_SemaContext *context, const MAREA_AstNode *at, const MAREA_AstNode *source, const MAREA_Character8 *prefix)
{
    const MAREA_AstNode *root = MAREA_G_RootOf(source);

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_VALUE_CHANGED_WHILE_BORROWED, MAREA_AstNodeGetPosition(at), prefix, MAREA_AstNodeGetText(root), MAREA_AstNodeGetTextLength(root));
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_ScanBody(MAREA_SemaContext *context, const MAREA_AstNode *node, const MAREA_AstNode *source)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    if (MAREA_G_IsKind(node, MAREA_AST_KIND_ASSIGNMENT) == MAREA_BOOLEAN_TRUE) {
        const MAREA_AstNode *target = MAREA_SemaChildAt(node, 0);

        if (MAREA_G_Alias(target, source) == MAREA_BOOLEAN_TRUE) {
            const MAREA_SemaExcept RESULT = MAREA_G_ReportChange(context, target, source, "a value is written while a loop or a with block borrows it:");

            if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
                return RESULT;
            }
        }
    } else if (MAREA_G_IsWord(node, MAREA_TOKEN_KIND_KEYWORD_MOVE) == MAREA_BOOLEAN_TRUE) {
        if (MAREA_G_Alias(MAREA_SemaChildAt(node, 0), source) == MAREA_BOOLEAN_TRUE) {
            const MAREA_SemaExcept RESULT = MAREA_G_ReportChange(context, node, source, "a value is moved while a loop or a with block borrows it:");

            if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
                return RESULT;
            }
        }
    } else if (MAREA_G_IsKind(node, MAREA_AST_KIND_CALL_EXPRESSION) == MAREA_BOOLEAN_TRUE) {
        const MAREA_AstNode *callee = MAREA_SemaChildAt(node, 0);

        if ((MAREA_G_IsKind(callee, MAREA_AST_KIND_MEMBER_ACCESS) == MAREA_BOOLEAN_TRUE) && (MAREA_G_IsChangingMethod(MAREA_SemaChildAt(callee, 1)) == MAREA_BOOLEAN_TRUE) && (MAREA_G_Alias(MAREA_SemaChildAt(callee, 0), source) == MAREA_BOOLEAN_TRUE)) {
            const MAREA_SemaExcept RESULT = MAREA_G_ReportChange(context, node, source, "a value is changed while a loop or a with block borrows it:");

            if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
                return RESULT;
            }
        }
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(node); ++index) {
        const MAREA_SemaExcept RESULT = MAREA_G_ScanBody(context, MAREA_SemaChildAt(node, index), source);

        if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return RESULT;
        }
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckBorrowedSource(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(node);

    if ((MAREA_G_IsKind(node, MAREA_AST_KIND_FOR_EACH_STATEMENT) == MAREA_BOOLEAN_TRUE) && (COUNT >= 3)) {
        return MAREA_G_ScanBody(context, MAREA_SemaChildAt(node, (COUNT - 1)), MAREA_SemaChildAt(node, (COUNT - 2)));
    }

    if ((MAREA_G_IsKind(node, MAREA_AST_KIND_WITH_STATEMENT) == MAREA_BOOLEAN_TRUE) && (COUNT >= 3)) {
        return MAREA_G_ScanBody(context, MAREA_SemaChildAt(node, (COUNT - 1)), MAREA_SemaChildAt(node, 0));
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}
