// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/ir/MAREAIrInternal.h"

#include <string.h>

// INFO: a chain of type aliases is followed this many steps; a longer chain, or a cycle, stops at the last name.
#define MAREA_IR_ALIAS_DEPTH_LIMIT MAREA_INTEGER_SIZE(8)

// INFO: the segments of a QUALIFIED_NAME as one text. One segment points into the source; more are joined with a dot in the arena.
static MAREA_IrExcept MAREA_CALL MAREA_G_JoinSegments(MAREA_IrContext *context, const MAREA_AstNode *name, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength)
{
    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(name);

    MAREA_IntegerSize total = MAREA_INTEGER_SIZE(0);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_AstNode *segment = MAREA_NULLPTR;

        if (MAREA_AstNodeGetChildAt(name, index, &segment) != MAREA_AST_EXCEPT_SUCCESS) {
            return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
        }

        total = (total + MAREA_AstNodeGetTextLength(segment) + ((index == MAREA_INTEGER_SIZE(0)) ? MAREA_INTEGER_SIZE(0) : MAREA_INTEGER_SIZE(1)));
    }

    const MAREA_AstNode *first = MAREA_NULLPTR;

    if ((COUNT == MAREA_INTEGER_SIZE(0)) || (MAREA_AstNodeGetChildAt(name, MAREA_INTEGER_SIZE(0), &first) != MAREA_AST_EXCEPT_SUCCESS)) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if (COUNT == MAREA_INTEGER_SIZE(1)) {
        *outText = MAREA_AstNodeGetText(first);
        *outLength = MAREA_AstNodeGetTextLength(first);

        return MAREA_IR_EXCEPT_SUCCESS;
    }

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(context->arena, total, MAREA_INTEGER_SIZE(1), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_IR_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_Character8 *joined = memory;

    MAREA_IntegerSize used = MAREA_INTEGER_SIZE(0);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_AstNode *segment = MAREA_NULLPTR;

        if (MAREA_AstNodeGetChildAt(name, index, &segment) != MAREA_AST_EXCEPT_SUCCESS) {
            return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
        }

        if (index != MAREA_INTEGER_SIZE(0)) {
            joined[used] = '.';
            used = (used + MAREA_INTEGER_SIZE(1));
        }

        memcpy((joined + used), MAREA_AstNodeGetText(segment), MAREA_AstNodeGetTextLength(segment));
        used = (used + MAREA_AstNodeGetTextLength(segment));
    }

    *outText = joined;
    *outLength = total;

    return MAREA_IR_EXCEPT_SUCCESS;
}

static MAREA_Boolean MAREA_CALL MAREA_G_TextIs(const MAREA_Character8 *left, MAREA_IntegerSize leftLength, const MAREA_Character8 *right, MAREA_IntegerSize rightLength)
{
    if (leftLength != rightLength) {
        return MAREA_BOOLEAN_FALSE;
    }

    return (memcmp(left, right, leftLength) == 0) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: how many alias entries the unit makes: one per aliased import or member, one per plain type alias.
static MAREA_IntegerSize MAREA_CALL MAREA_G_CountAliases(const MAREA_SemaTypedNode *unit)
{
    MAREA_IntegerSize count = MAREA_INTEGER_SIZE(0);

    const MAREA_IntegerSize COUNT = MAREA_SemaTypedNodeGetChildCount(unit);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_SemaTypedNode *item = MAREA_IrTypedChildAt(unit, index);

        const MAREA_AstKind KIND = MAREA_IrTypedKind(item);

        if ((KIND == MAREA_AST_KIND_IMPORT_DECLARATION) || (KIND == MAREA_AST_KIND_REQUIRE_DECLARATION)) {
            count = (count + MAREA_SemaTypedNodeGetChildCount(item));
        } else if (KIND == MAREA_AST_KIND_TYPE_DECLARATION) {
            count = (count + MAREA_INTEGER_SIZE(1));
        }
    }

    return count;
}

// INFO: left, a dot, then right, as one text in the arena.
static MAREA_IrExcept MAREA_CALL MAREA_G_JoinWithDot(MAREA_IrContext *context, const MAREA_Character8 *left, MAREA_IntegerSize leftLength, const MAREA_Character8 *right, MAREA_IntegerSize rightLength, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength)
{
    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(context->arena, (leftLength + MAREA_INTEGER_SIZE(1) + rightLength), MAREA_INTEGER_SIZE(1), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_IR_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_Character8 *joined = memory;

    memcpy(joined, left, leftLength);
    joined[leftLength] = '.';
    memcpy((joined + leftLength + MAREA_INTEGER_SIZE(1)), right, rightLength);

    *outText = joined;
    *outLength = (leftLength + MAREA_INTEGER_SIZE(1) + rightLength);

    return MAREA_IR_EXCEPT_SUCCESS;
}

// INFO: one alias entry. The original is the full path that the alias stands for: the path of the import for a namespace or a one-symbol alias, and the namespace of the import then the member path for a member of a list, so two members of one name in two namespaces never share a lowered name. namespacePath is null for a local type alias, whose original is the path as written.
static MAREA_IrExcept MAREA_CALL MAREA_G_AppendAlias(MAREA_IrContext *context, const MAREA_AstNode *alias, const MAREA_AstNode *original, const MAREA_AstNode *namespacePath, MAREA_Boolean isImport)
{
    MAREA_IrAlias *entry = &context->aliases[context->aliasCount];

    entry->local = MAREA_AstNodeGetText(alias);
    entry->localLength = MAREA_AstNodeGetTextLength(alias);
    entry->isImport = isImport;
    entry->isFunction = MAREA_BOOLEAN_FALSE;
    entry->hasArguments = MAREA_BOOLEAN_FALSE;

    MAREA_IrExcept result = MAREA_G_JoinSegments(context, original, &entry->original, &entry->originalLength);

    if ((result == MAREA_IR_EXCEPT_SUCCESS) && (namespacePath != MAREA_NULLPTR)) {
        const MAREA_Character8 *prefix = MAREA_NULLPTR;
        MAREA_IntegerSize prefixLength = MAREA_INTEGER_SIZE(0);

        result = MAREA_G_JoinSegments(context, namespacePath, &prefix, &prefixLength);

        if (result == MAREA_IR_EXCEPT_SUCCESS) {
            result = MAREA_G_JoinWithDot(context, prefix, prefixLength, entry->original, entry->originalLength, &entry->original, &entry->originalLength);
        }
    }

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    context->aliasCount = (context->aliasCount + MAREA_INTEGER_SIZE(1));

    return MAREA_IR_EXCEPT_SUCCESS;
}

static MAREA_IrExcept MAREA_CALL MAREA_G_AddImport(MAREA_IrContext *context, const MAREA_SemaTypedNode *item)
{
    const MAREA_AstNode *ast = MAREA_SemaTypedNodeGetAstNode(item);

    const MAREA_Uinteger32 FLAGS = MAREA_AstNodeGetFlags(ast);

    const MAREA_AstNode *path = MAREA_SemaTypedNodeGetAstNode(MAREA_IrTypedChildAt(item, MAREA_INTEGER_SIZE(0)));

    if ((FLAGS & (MAREA_Uinteger32)MAREA_AST_FLAG_HAS_MEMBERS) != 0) {
        const MAREA_IntegerSize COUNT = MAREA_SemaTypedNodeGetChildCount(item);

        for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(1); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
            const MAREA_SemaTypedNode *member = MAREA_IrTypedChildAt(item, index);

            // INFO: a member without "as" binds no name, so it has no alias to lower.
            if ((MAREA_IrTypedKind(member) != MAREA_AST_KIND_IMPORT_MEMBER) || (MAREA_SemaTypedNodeGetChildCount(member) < MAREA_INTEGER_SIZE(2))) {
                continue;
            }

            const MAREA_IrExcept result = MAREA_G_AppendAlias(context, MAREA_SemaTypedNodeGetAstNode(MAREA_IrTypedChildAt(member, MAREA_INTEGER_SIZE(1))), MAREA_SemaTypedNodeGetAstNode(MAREA_IrTypedChildAt(member, MAREA_INTEGER_SIZE(0))), path, MAREA_BOOLEAN_TRUE);

            if (result != MAREA_IR_EXCEPT_SUCCESS) {
                return result;
            }
        }

        return MAREA_IR_EXCEPT_SUCCESS;
    }

    if (MAREA_SemaTypedNodeGetChildCount(item) < MAREA_INTEGER_SIZE(2)) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    // INFO: an alias of a whole path stands for the full path, namespace or symbol, never for its last segment alone.
    return MAREA_G_AppendAlias(context, MAREA_SemaTypedNodeGetAstNode(MAREA_IrTypedChildAt(item, MAREA_INTEGER_SIZE(1))), path, MAREA_NULLPTR, MAREA_BOOLEAN_TRUE);
}

// INFO: an alias without generic parameters is lowered when its right side is a named type, with or without concrete type arguments, or a function type.
static MAREA_IrExcept MAREA_CALL MAREA_G_AddTypeAlias(MAREA_IrContext *context, const MAREA_SemaTypedNode *item)
{
    const MAREA_SemaTypedNode *name = MAREA_IrTypedFindChild(item, MAREA_AST_KIND_IDENTIFIER);

    if ((name == MAREA_NULLPTR) || (MAREA_IrTypedFindChild(item, MAREA_AST_KIND_GENERIC_PARAMETERS) != MAREA_NULLPTR)) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    const MAREA_SemaTypedNode *type = MAREA_IrTypedFindChild(item, MAREA_AST_KIND_NAMED_TYPE);

    if (type == MAREA_NULLPTR) {
        // INFO: a function type has no type name in the IR, so an alias of it lowers to no name, as the written function type does.
        if (MAREA_IrTypedFindChild(item, MAREA_AST_KIND_FUNCTION_TYPE) == MAREA_NULLPTR) {
            return MAREA_IR_EXCEPT_SUCCESS;
        }

        MAREA_IrAlias *entry = &context->aliases[context->aliasCount];

        const MAREA_AstNode *ast = MAREA_SemaTypedNodeGetAstNode(name);

        entry->local = MAREA_AstNodeGetText(ast);
        entry->localLength = MAREA_AstNodeGetTextLength(ast);
        entry->original = MAREA_NULLPTR;
        entry->originalLength = MAREA_INTEGER_SIZE(0);
        entry->isImport = MAREA_BOOLEAN_FALSE;
        entry->isFunction = MAREA_BOOLEAN_TRUE;
        entry->hasArguments = MAREA_BOOLEAN_FALSE;

        context->aliasCount = (context->aliasCount + MAREA_INTEGER_SIZE(1));

        return MAREA_IR_EXCEPT_SUCCESS;
    }

    const MAREA_IrExcept RESULT = MAREA_G_AppendAlias(context, MAREA_SemaTypedNodeGetAstNode(name), MAREA_SemaTypedNodeGetAstNode(MAREA_IrTypedChildAt(type, MAREA_INTEGER_SIZE(0))), MAREA_NULLPTR, MAREA_BOOLEAN_FALSE);

    if ((RESULT == MAREA_IR_EXCEPT_SUCCESS) && (MAREA_IrTypedFindChild(type, MAREA_AST_KIND_TYPE_ARGUMENTS) != MAREA_NULLPTR)) {
        context->aliases[(context->aliasCount - MAREA_INTEGER_SIZE(1))].hasArguments = MAREA_BOOLEAN_TRUE;
    }

    return RESULT;
}

MAREA_IrExcept MAREA_CALL MAREA_IrAliasBuild(MAREA_IrContext *context, const MAREA_SemaTypedNode *unit)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if (unit == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    context->aliases = MAREA_NULLPTR;
    context->aliasCount = MAREA_INTEGER_SIZE(0);

    const MAREA_IntegerSize CAPACITY = MAREA_G_CountAliases(unit);

    if (CAPACITY == MAREA_INTEGER_SIZE(0)) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(context->arena, (CAPACITY * sizeof(MAREA_IrAlias)), MAREA_ALIGNOF(MAREA_IrAlias), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_IR_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    context->aliases = memory;

    const MAREA_IntegerSize COUNT = MAREA_SemaTypedNodeGetChildCount(unit);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_SemaTypedNode *item = MAREA_IrTypedChildAt(unit, index);

        const MAREA_AstKind KIND = MAREA_IrTypedKind(item);

        MAREA_IrExcept result = MAREA_IR_EXCEPT_SUCCESS;

        if ((KIND == MAREA_AST_KIND_IMPORT_DECLARATION) || (KIND == MAREA_AST_KIND_REQUIRE_DECLARATION)) {
            result = MAREA_G_AddImport(context, item);
        } else if (KIND == MAREA_AST_KIND_TYPE_DECLARATION) {
            result = MAREA_G_AddTypeAlias(context, item);
        }

        if (result != MAREA_IR_EXCEPT_SUCCESS) {
            return result;
        }
    }

    return MAREA_IR_EXCEPT_SUCCESS;
}

static const MAREA_IrAlias *MAREA_CALL MAREA_G_FindAlias(const MAREA_IrContext *context, const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_Boolean importsOnly)
{
    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < context->aliasCount; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_IrAlias *entry = &context->aliases[index];

        if ((importsOnly == MAREA_BOOLEAN_TRUE) && (entry->isImport == MAREA_BOOLEAN_FALSE)) {
            continue;
        }

        if (MAREA_G_TextIs(entry->local, entry->localLength, text, length) == MAREA_BOOLEAN_TRUE) {
            return entry;
        }
    }

    return MAREA_NULLPTR;
}

MAREA_Boolean MAREA_CALL MAREA_IrFunctionDeclares(const MAREA_SemaTypedNode *typed, const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_Uinteger32 offset)
{
    const MAREA_AstKind KIND = MAREA_IrTypedKind(typed);

    const MAREA_IntegerSize COUNT = MAREA_SemaTypedNodeGetChildCount(typed);

    // INFO: the first and, for "for each", the second IDENTIFIER child names the binding; a with or partition binder is the one after its expressions.
    MAREA_IntegerSize first = MAREA_INTEGER_SIZE(0);
    MAREA_IntegerSize last = MAREA_INTEGER_SIZE(0);

    if ((KIND == MAREA_AST_KIND_PARAMETER) || (KIND == MAREA_AST_KIND_LOCAL_DECLARATION) || (KIND == MAREA_AST_KIND_FOR_RANGE_STATEMENT)) {
        last = MAREA_INTEGER_SIZE(1);
    } else if (KIND == MAREA_AST_KIND_FOR_EACH_STATEMENT) {
        last = (COUNT >= MAREA_INTEGER_SIZE(2)) ? (COUNT - MAREA_INTEGER_SIZE(2)) : MAREA_INTEGER_SIZE(0);
    } else if (KIND == MAREA_AST_KIND_WITH_STATEMENT) {
        first = MAREA_INTEGER_SIZE(1);
        last = MAREA_INTEGER_SIZE(2);
    } else if (KIND == MAREA_AST_KIND_PARTITION_STATEMENT) {
        first = MAREA_INTEGER_SIZE(2);
        last = MAREA_INTEGER_SIZE(3);
    }

    for (MAREA_IntegerSize index = first; (index < last) && (index < COUNT); index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_SemaTypedNode *binder = MAREA_IrTypedChildAt(typed, index);

        if (MAREA_IrTypedKind(binder) != MAREA_AST_KIND_IDENTIFIER) {
            continue;
        }

        const MAREA_AstNode *ast = MAREA_SemaTypedNodeGetAstNode(binder);

        if ((MAREA_G_TextIs(MAREA_AstNodeGetText(ast), MAREA_AstNodeGetTextLength(ast), text, length) == MAREA_BOOLEAN_TRUE) && (MAREA_AstNodeGetPosition(ast).offset <= offset)) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        if (MAREA_IrFunctionDeclares(MAREA_IrTypedChildAt(typed, index), text, length, offset) == MAREA_BOOLEAN_TRUE) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

MAREA_Boolean MAREA_CALL MAREA_IrAliasValue(const MAREA_IrContext *context, const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_Uinteger32 offset, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength)
{
    const MAREA_IrAlias *entry = MAREA_G_FindAlias(context, text, length, MAREA_BOOLEAN_TRUE);

    if (entry == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    // INFO: a parameter or a local of the enclosing function with the same name hides the alias from its own declaration onward.
    if ((context->function != MAREA_NULLPTR) && (MAREA_IrFunctionDeclares(context->function, text, length, offset) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_BOOLEAN_FALSE;
    }

    *outText = entry->original;
    *outLength = entry->originalLength;

    return MAREA_BOOLEAN_TRUE;
}

// INFO: follows the alias chain of a type name. lossy is set when a step carries type arguments, which the lowered name does not keep.
static MAREA_IrExcept MAREA_CALL MAREA_G_ResolveType(MAREA_IrContext *context, const MAREA_Character8 *text, MAREA_IntegerSize length, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength, MAREA_Boolean *outLossy)
{
    *outText = text;
    *outLength = length;
    *outLossy = MAREA_BOOLEAN_FALSE;

    MAREA_Boolean terminal = MAREA_BOOLEAN_FALSE;

    for (MAREA_IntegerSize depth = MAREA_INTEGER_SIZE(0); depth < MAREA_IR_ALIAS_DEPTH_LIMIT; depth = (depth + MAREA_INTEGER_SIZE(1))) {
        const MAREA_IrAlias *entry = MAREA_G_FindAlias(context, *outText, *outLength, MAREA_BOOLEAN_FALSE);

        if (entry == MAREA_NULLPTR) {
            break;
        }

        *outText = entry->original;
        *outLength = entry->originalLength;

        if (entry->hasArguments == MAREA_BOOLEAN_TRUE) {
            *outLossy = MAREA_BOOLEAN_TRUE;
        }

        // INFO: a function type has no name, so the chain ends with no name.
        if (entry->isFunction == MAREA_BOOLEAN_TRUE) {
            *outLossy = MAREA_BOOLEAN_TRUE;

            return MAREA_IR_EXCEPT_SUCCESS;
        }

        // INFO: an import original names a symbol of another namespace, so it is never looked up again here.
        if (entry->isImport == MAREA_BOOLEAN_TRUE) {
            terminal = MAREA_BOOLEAN_TRUE;

            break;
        }
    }

    if (terminal == MAREA_BOOLEAN_TRUE) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    // INFO: a dotted name keeps its tail; only its head, a namespace alias, is looked up.
    const MAREA_Character8 *dot = memchr(*outText, '.', *outLength);

    if (dot == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    const MAREA_IntegerSize HEAD = (MAREA_IntegerSize)(dot - *outText);

    const MAREA_IrAlias *head = MAREA_G_FindAlias(context, *outText, HEAD, MAREA_BOOLEAN_TRUE);

    if (head == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    const MAREA_IntegerSize TAIL = (*outLength - HEAD);

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(context->arena, (head->originalLength + TAIL), MAREA_INTEGER_SIZE(1), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_IR_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_Character8 *joined = memory;

    memcpy(joined, head->original, head->originalLength);
    memcpy((joined + head->originalLength), dot, TAIL);

    *outText = joined;
    *outLength = (head->originalLength + TAIL);

    return MAREA_IR_EXCEPT_SUCCESS;
}

MAREA_IrExcept MAREA_CALL MAREA_IrAliasType(MAREA_IrContext *context, const MAREA_Character8 *text, MAREA_IntegerSize length, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength)
{
    MAREA_Boolean lossy = MAREA_BOOLEAN_FALSE;

    return MAREA_G_ResolveType(context, text, length, outText, outLength, &lossy);
}

MAREA_Boolean MAREA_CALL MAREA_IrAliasTypeIsLossy(MAREA_IrContext *context, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    const MAREA_Character8 *lowered = MAREA_NULLPTR;
    MAREA_IntegerSize loweredLength = MAREA_INTEGER_SIZE(0);
    MAREA_Boolean lossy = MAREA_BOOLEAN_FALSE;

    if (MAREA_G_ResolveType(context, text, length, &lowered, &loweredLength, &lossy) != MAREA_IR_EXCEPT_SUCCESS) {
        return MAREA_BOOLEAN_FALSE;
    }

    return lossy;
}

MAREA_Boolean MAREA_CALL MAREA_IrAliasIsLocalType(const MAREA_IrContext *context, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    const MAREA_IrAlias *entry = MAREA_G_FindAlias(context, text, length, MAREA_BOOLEAN_FALSE);

    return ((entry != MAREA_NULLPTR) && (entry->isImport == MAREA_BOOLEAN_FALSE) && (entry->isFunction == MAREA_BOOLEAN_FALSE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}
