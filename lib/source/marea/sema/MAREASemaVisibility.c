// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <string.h>

MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaNamespaceOf(MAREA_SemaContext *context, const MAREA_SemaName *entry, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength)
{
    if ((context == MAREA_NULLPTR) || (entry == MAREA_NULLPTR) || (outText == MAREA_NULLPTR) || (outLength == MAREA_NULLPTR)) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    *outText = MAREA_NULLPTR;
    *outLength = 0;

    const MAREA_Character8 *name = (entry->original != MAREA_NULLPTR) ? entry->original : entry->text;

    const MAREA_IntegerSize NAME_LENGTH = (entry->original != MAREA_NULLPTR) ? entry->originalLength : entry->length;

    if (entry->kind == MAREA_SEMA_NAME_KIND_IMPORTED_SYMBOL) {
        *outText = entry->qualifier;
        *outLength = entry->qualifierLength;

        return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
    }

    if (entry->qualifierLength == 0) {
        *outText = name;
        *outLength = NAME_LENGTH;

        return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
    }

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(context->arena, (entry->qualifierLength + NAME_LENGTH + MAREA_INTEGER_SIZE(1)), MAREA_INTEGER_SIZE(1), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_Character8 *joined = memory;

    memcpy(joined, entry->qualifier, entry->qualifierLength);

    joined[entry->qualifierLength] = '.';

    memcpy((joined + entry->qualifierLength + MAREA_INTEGER_SIZE(1)), name, NAME_LENGTH);

    *outText = joined;
    *outLength = (entry->qualifierLength + NAME_LENGTH + MAREA_INTEGER_SIZE(1));

    return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
}

const MAREA_AstNode *MAREA_CALL MAREA_SemaFoundRoot(const MAREA_SemaContext *context, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    if ((context == MAREA_NULLPTR) || (context->resolver == MAREA_NULLPTR) || (text == MAREA_NULLPTR) || (length == 0)) {
        return MAREA_NULLPTR;
    }

    const MAREA_AstNode *root = MAREA_NULLPTR;

    const MAREA_SemaImportResult RESULT = MAREA_SemaResolveNamespace(context, text, length, &root);

    return (RESULT == MAREA_SEMA_IMPORT_RESULT_FOUND) ? root : MAREA_NULLPTR;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsKind(const MAREA_AstNode *node, MAREA_AstKind kind)
{
    return ((node != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(node) == kind)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsTypeDeclaration(const MAREA_AstNode *declaration)
{
    return ((MAREA_G_IsKind(declaration, MAREA_AST_KIND_STRUCTURE_DECLARATION) == MAREA_BOOLEAN_TRUE) || (MAREA_G_IsKind(declaration, MAREA_AST_KIND_ENUM_DECLARATION) == MAREA_BOOLEAN_TRUE) || (MAREA_G_IsKind(declaration, MAREA_AST_KIND_OPAQUE_DECLARATION) == MAREA_BOOLEAN_TRUE) || (MAREA_G_IsKind(declaration, MAREA_AST_KIND_TYPE_DECLARATION) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: the top-level declaration of a namespace tree with a name, or null. A member that the tree does not declare is not a rule of this file.
static const MAREA_AstNode *MAREA_CALL MAREA_G_FindDeclaration(const MAREA_AstNode *root, const MAREA_Character8 *name, MAREA_IntegerSize nameLength)
{
    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(root);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *declaration = MAREA_SemaChildAt(root, index);

        const MAREA_Character8 *declarationName = MAREA_NULLPTR;

        MAREA_IntegerSize declarationLength = 0;

        if ((MAREA_SemaDeclarationName(declaration, &declarationName, &declarationLength) == MAREA_BOOLEAN_TRUE) && (MAREA_SemaTextEquals(declarationName, declarationLength, name, nameLength) == MAREA_BOOLEAN_TRUE)) {
            return declaration;
        }
    }

    return MAREA_NULLPTR;
}

// INFO: the field of a structure with a name, or null. A structure holds no other IDENTIFIER-named child than its fields and its own name.
static const MAREA_AstNode *MAREA_CALL MAREA_G_FindField(const MAREA_AstNode *structure, const MAREA_Character8 *name, MAREA_IntegerSize nameLength)
{
    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(structure);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *field = MAREA_SemaChildAt(structure, index);

        if (MAREA_G_IsKind(field, MAREA_AST_KIND_FIELD_DECLARATION) == MAREA_BOOLEAN_FALSE) {
            continue;
        }

        const MAREA_IntegerSize FIELD_COUNT = MAREA_AstNodeGetChildCount(field);

        for (MAREA_IntegerSize fieldIndex = 0; fieldIndex < FIELD_COUNT; ++fieldIndex) {
            const MAREA_AstNode *child = MAREA_SemaChildAt(field, fieldIndex);

            if (MAREA_G_IsKind(child, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_FALSE) {
                continue;
            }

            if (MAREA_SemaTextEquals(MAREA_AstNodeGetText(child), MAREA_AstNodeGetTextLength(child), name, nameLength) == MAREA_BOOLEAN_TRUE) {
                return field;
            }

            break;
        }
    }

    return MAREA_NULLPTR;
}

// INFO: the FOUND tree of the namespace that a bare name stands for, or null. A local hides a namespace only where the name is read as a value, and a generic parameter is never one.
static MAREA_SemaExcept MAREA_CALL MAREA_G_NamespaceRoot(MAREA_SemaContext *context, const MAREA_AstNode *identifier, MAREA_Boolean localHides, const MAREA_AstNode **outRoot)
{
    *outRoot = MAREA_NULLPTR;

    if (MAREA_G_IsKind(identifier, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *text = MAREA_AstNodeGetText(identifier);

    const MAREA_IntegerSize LENGTH = MAREA_AstNodeGetTextLength(identifier);

    if (((localHides == MAREA_BOOLEAN_TRUE) && (MAREA_SemaLocalFind(context, text, LENGTH) != MAREA_NULLPTR)) || (MAREA_SemaIsGenericInScope(context, text, LENGTH) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, text, LENGTH);

    if ((entry == MAREA_NULLPTR) || (entry->kind != MAREA_SEMA_NAME_KIND_NAMESPACE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *namespaceText = MAREA_NULLPTR;

    MAREA_IntegerSize namespaceLength = 0;

    if (MAREA_SemaNamespaceOf(context, entry, &namespaceText, &namespaceLength) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
        return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    *outRoot = MAREA_SemaFoundRoot(context, namespaceText, namespaceLength);

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

#define MAREA_G_TARGET_DEPTH MAREA_UINTEGER_32(16)

// INFO: the first child of a kind, or null.
static const MAREA_AstNode *MAREA_CALL MAREA_G_FindChild(const MAREA_AstNode *node, MAREA_AstKind kind)
{
    const MAREA_IntegerSize COUNT = (node == MAREA_NULLPTR) ? 0 : MAREA_AstNodeGetChildCount(node);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *child = MAREA_SemaChildAt(node, index);

        if (MAREA_G_IsKind(child, kind) == MAREA_BOOLEAN_TRUE) {
            return child;
        }
    }

    return MAREA_NULLPTR;
}

// INFO: the declaration that an imported symbol names, in the tree of its namespace.
static const MAREA_AstNode *MAREA_CALL MAREA_G_ImportedDeclaration(const MAREA_SemaContext *context, const MAREA_SemaName *entry, const MAREA_AstNode **outRoot)
{
    *outRoot = MAREA_SemaFoundRoot(context, entry->qualifier, entry->qualifierLength);

    return ((*outRoot == MAREA_NULLPTR) || (entry->original == MAREA_NULLPTR)) ? MAREA_NULLPTR : MAREA_G_FindDeclaration(*outRoot, entry->original, entry->originalLength);
}

// INFO: the tree of the namespace that all the segments of a qualified type name but the last one stand for.
// The file of the run reads its own aliases; another tree is read by a full path only.
static const MAREA_AstNode *MAREA_CALL MAREA_G_PrefixRoot(const MAREA_SemaContext *context, const MAREA_AstNode *name, const MAREA_AstNode *root)
{
    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(name);

    const MAREA_AstNode *first = MAREA_SemaChildAt(name, 0);

    const MAREA_AstNode *last = MAREA_SemaChildAt(name, (COUNT - MAREA_INTEGER_SIZE(1)));

    // INFO: the join and the namespace of an entry only allocate from the arena, so the cast is safe.
    MAREA_SemaContext *mutableContext = (MAREA_SemaContext *)context;

    if (root == MAREA_NULLPTR) {
        const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, MAREA_AstNodeGetText(first), MAREA_AstNodeGetTextLength(first));

        if (entry != MAREA_NULLPTR) {
            const MAREA_Character8 *text = MAREA_NULLPTR;

            MAREA_IntegerSize length = 0;

            if ((entry->kind != MAREA_SEMA_NAME_KIND_NAMESPACE) || (COUNT != MAREA_INTEGER_SIZE(2)) || (MAREA_SemaNamespaceOf(mutableContext, entry, &text, &length) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS)) {
                return MAREA_NULLPTR;
            }

            return MAREA_SemaFoundRoot(context, text, length);
        }
    }

    const MAREA_Character8 *joined = MAREA_NULLPTR;

    MAREA_IntegerSize joinedLength = 0;

    if (MAREA_SemaQualifiedNameJoin(mutableContext, name, &joined, &joinedLength) != MAREA_SEMA_EXCEPT_SUCCESS) {
        return MAREA_NULLPTR;
    }

    return MAREA_SemaFoundRoot(context, joined, (joinedLength - MAREA_AstNodeGetTextLength(last) - MAREA_INTEGER_SIZE(1)));
}

// INFO: the declaration that one written type names, with no alias followed.
static const MAREA_AstNode *MAREA_CALL MAREA_G_DeclarationOfType(const MAREA_SemaContext *context, const MAREA_AstNode *type, const MAREA_AstNode *root, const MAREA_AstNode **outRoot)
{
    *outRoot = root;

    const MAREA_TypeDescriptor *descriptor = MAREA_NULLPTR;

    const MAREA_AstNode *name = (MAREA_G_IsKind(type, MAREA_AST_KIND_NAMED_TYPE) == MAREA_BOOLEAN_TRUE) ? MAREA_SemaChildAt(type, 0) : MAREA_NULLPTR;

    if ((MAREA_G_IsKind(name, MAREA_AST_KIND_QUALIFIED_NAME) == MAREA_BOOLEAN_FALSE) || (MAREA_SemaDescriptorOfType(context, type, &descriptor) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_NULLPTR;
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(name);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        if (MAREA_G_IsKind(MAREA_SemaChildAt(name, index), MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_FALSE) {
            return MAREA_NULLPTR;
        }
    }

    if (COUNT == 0) {
        return MAREA_NULLPTR;
    }

    const MAREA_AstNode *last = MAREA_SemaChildAt(name, (COUNT - MAREA_INTEGER_SIZE(1)));

    const MAREA_Character8 *text = MAREA_AstNodeGetText(last);

    const MAREA_IntegerSize LENGTH = MAREA_AstNodeGetTextLength(last);

    if (COUNT > MAREA_INTEGER_SIZE(1)) {
        *outRoot = MAREA_G_PrefixRoot(context, name, root);

        return (*outRoot == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_G_FindDeclaration(*outRoot, text, LENGTH);
    }

    if (root != MAREA_NULLPTR) {
        return MAREA_G_FindDeclaration(root, text, LENGTH);
    }

    const MAREA_SemaName *entry = (MAREA_SemaIsGenericInScope(context, text, LENGTH) == MAREA_BOOLEAN_TRUE) ? MAREA_NULLPTR : MAREA_SemaNameTableFind(context, text, LENGTH);

    if (entry == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    if (entry->kind == MAREA_SEMA_NAME_KIND_IMPORTED_SYMBOL) {
        return MAREA_G_ImportedDeclaration(context, entry, outRoot);
    }

    return (entry->kind == MAREA_SEMA_NAME_KIND_DECLARATION) ? entry->declaration : MAREA_NULLPTR;
}

MAREA_Boolean MAREA_CALL MAREA_SemaFindTypeTarget(const MAREA_SemaContext *context, const MAREA_AstNode *type, const MAREA_AstNode *root, MAREA_SemaTypeTarget *outTarget)
{
    if (outTarget == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    outTarget->type = type;
    outTarget->declaration = MAREA_NULLPTR;
    outTarget->root = root;

    if ((context == MAREA_NULLPTR) || (type == MAREA_NULLPTR)) {
        return MAREA_BOOLEAN_FALSE;
    }

    // INFO: the bound ends a cycle of aliases, which its own check reports.
    for (MAREA_Uinteger32 depth = 0; depth < MAREA_G_TARGET_DEPTH; ++depth) {
        const MAREA_AstNode *declarationRoot = MAREA_NULLPTR;

        const MAREA_AstNode *declaration = MAREA_G_DeclarationOfType(context, outTarget->type, outTarget->root, &declarationRoot);

        if (MAREA_G_IsKind(declaration, MAREA_AST_KIND_TYPE_DECLARATION) == MAREA_BOOLEAN_FALSE) {
            outTarget->declaration = declaration;

            if (declaration != MAREA_NULLPTR) {
                outTarget->root = declarationRoot;
            }

            break;
        }

        outTarget->type = MAREA_SemaChildAt(declaration, (MAREA_AstNodeGetChildCount(declaration) - MAREA_INTEGER_SIZE(1)));
        outTarget->root = declarationRoot;
    }

    return (outTarget->declaration != MAREA_NULLPTR) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: the tree of the namespace that the base of a member access stands for, or null: a bare name, or a full path that a cover reaches.
static MAREA_SemaExcept MAREA_CALL MAREA_G_BaseRoot(MAREA_SemaContext *context, const MAREA_AstNode *base, const MAREA_AstNode *member, const MAREA_AstNode **outRoot)
{
    *outRoot = MAREA_NULLPTR;

    if (MAREA_G_IsKind(base, MAREA_AST_KIND_MEMBER_ACCESS) == MAREA_BOOLEAN_FALSE) {
        return MAREA_G_NamespaceRoot(context, base, MAREA_BOOLEAN_TRUE, outRoot);
    }

    const MAREA_SemaName *entry = MAREA_NULLPTR;

    const MAREA_SemaExcept ENTRY_RESULT = MAREA_SemaBaseEntry(context, base, member, &entry);

    if ((ENTRY_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) || (entry == MAREA_NULLPTR) || (entry->kind != MAREA_SEMA_NAME_KIND_NAMESPACE)) {
        return ENTRY_RESULT;
    }

    const MAREA_Character8 *text = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    if (MAREA_SemaNamespaceOf(context, entry, &text, &length) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
        return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    *outRoot = MAREA_SemaFoundRoot(context, text, length);

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

// INFO: the function of an implementation block of a structure or an enum, and that block.
static const MAREA_AstNode *MAREA_CALL MAREA_G_FindMethod(const MAREA_AstNode *root, const MAREA_AstNode *declaration, const MAREA_Character8 *name, MAREA_IntegerSize nameLength, const MAREA_AstNode **outBlock)
{
    *outBlock = MAREA_NULLPTR;

    const MAREA_Character8 *typeName = MAREA_NULLPTR;

    MAREA_IntegerSize typeLength = 0;

    if (MAREA_SemaDeclarationName(declaration, &typeName, &typeLength) == MAREA_BOOLEAN_FALSE) {
        return MAREA_NULLPTR;
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(root);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *block = MAREA_SemaChildAt(root, index);

        if (MAREA_G_IsKind(block, MAREA_AST_KIND_IMPLEMENTATION_DECLARATION) == MAREA_BOOLEAN_FALSE) {
            continue;
        }

        const MAREA_AstNode *segment = MAREA_SemaChildAt(MAREA_SemaChildAt(MAREA_G_FindChild(block, MAREA_AST_KIND_NAMED_TYPE), 0), 0);

        if ((segment == MAREA_NULLPTR) || (MAREA_SemaTextEquals(MAREA_AstNodeGetText(segment), MAREA_AstNodeGetTextLength(segment), typeName, typeLength) == MAREA_BOOLEAN_FALSE)) {
            continue;
        }

        const MAREA_IntegerSize MEMBER_COUNT = MAREA_AstNodeGetChildCount(block);

        for (MAREA_IntegerSize memberIndex = 0; memberIndex < MEMBER_COUNT; ++memberIndex) {
            const MAREA_AstNode *function = MAREA_SemaChildAt(block, memberIndex);

            const MAREA_Character8 *text = MAREA_NULLPTR;

            MAREA_IntegerSize length = 0;

            if ((MAREA_G_IsKind(function, MAREA_AST_KIND_FUNCTION_DECLARATION) == MAREA_BOOLEAN_TRUE) && (MAREA_SemaDeclarationName(function, &text, &length) == MAREA_BOOLEAN_TRUE) && (MAREA_SemaTextEquals(text, length, name, nameLength) == MAREA_BOOLEAN_TRUE)) {
                *outBlock = block;

                return function;
            }
        }
    }

    return MAREA_NULLPTR;
}

// INFO: the structure or enum that an expression names as a type: Box, ns.Box or the full path of one.
static MAREA_SemaExcept MAREA_CALL MAREA_G_NamedTarget(MAREA_SemaContext *context, const MAREA_AstNode *expression, MAREA_SemaTypeTarget *outTarget)
{
    outTarget->type = MAREA_NULLPTR;
    outTarget->declaration = MAREA_NULLPTR;
    outTarget->root = MAREA_NULLPTR;

    if (MAREA_G_IsKind(expression, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_TRUE) {
        const MAREA_Character8 *text = MAREA_AstNodeGetText(expression);

        const MAREA_IntegerSize LENGTH = MAREA_AstNodeGetTextLength(expression);

        const MAREA_SemaName *entry = (MAREA_SemaLocalFind(context, text, LENGTH) != MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_SemaNameTableFind(context, text, LENGTH);

        if ((entry != MAREA_NULLPTR) && (entry->kind == MAREA_SEMA_NAME_KIND_DECLARATION)) {
            outTarget->declaration = entry->declaration;
        } else if ((entry != MAREA_NULLPTR) && (entry->kind == MAREA_SEMA_NAME_KIND_IMPORTED_SYMBOL)) {
            outTarget->declaration = MAREA_G_ImportedDeclaration(context, entry, &outTarget->root);
        }

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    if (MAREA_G_IsKind(expression, MAREA_AST_KIND_MEMBER_ACCESS) == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *member = MAREA_SemaChildAt(expression, 1);

    const MAREA_SemaExcept RESULT = MAREA_G_BaseRoot(context, MAREA_SemaChildAt(expression, 0), member, &outTarget->root);

    if ((RESULT == MAREA_SEMA_EXCEPT_SUCCESS) && (outTarget->root != MAREA_NULLPTR)) {
        outTarget->declaration = MAREA_G_FindDeclaration(outTarget->root, MAREA_AstNodeGetText(member), MAREA_AstNodeGetTextLength(member));
    }

    return RESULT;
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_ValueType(MAREA_SemaContext *context, const MAREA_AstNode *expression, MAREA_Uinteger32 depth, MAREA_SemaTypeTarget *outTarget);

// INFO: the structure or enum behind the base of a member: the type of a value, or the type that the base names.
static MAREA_SemaExcept MAREA_CALL MAREA_G_BaseTarget(MAREA_SemaContext *context, const MAREA_AstNode *base, MAREA_Uinteger32 depth, MAREA_SemaTypeTarget *outTarget)
{
    MAREA_SemaTypeTarget value = { .type = MAREA_NULLPTR, .declaration = MAREA_NULLPTR, .root = MAREA_NULLPTR };

    const MAREA_SemaExcept RESULT = MAREA_G_ValueType(context, base, (depth + 1), &value);

    if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
        return RESULT;
    }

    if (value.type == MAREA_NULLPTR) {
        return MAREA_G_NamedTarget(context, base, outTarget);
    }

    MAREA_IGNORE(MAREA_SemaFindTypeTarget(context, value.type, value.root, outTarget));

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

// INFO: the result type of a function of a tree. Self stands for the type that holds the function.
static void MAREA_CALL MAREA_G_ResultType(const MAREA_AstNode *function, const MAREA_AstNode *root, const MAREA_SemaTypeTarget *receiver, MAREA_SemaTypeTarget *outTarget)
{
    const MAREA_AstNode *result = MAREA_SemaReturnType(function);

    const MAREA_AstNode *segment = MAREA_SemaChildAt(MAREA_SemaChildAt(result, 0), 0);

    const MAREA_Character8 *text = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    if ((receiver != MAREA_NULLPTR) && (segment != MAREA_NULLPTR) && (MAREA_SemaTextEquals(MAREA_AstNodeGetText(segment), MAREA_AstNodeGetTextLength(segment), "Self", MAREA_INTEGER_SIZE(4)) == MAREA_BOOLEAN_TRUE) && (MAREA_SemaDeclarationName(receiver->declaration, &text, &length) == MAREA_BOOLEAN_TRUE)) {
        // INFO: the written type of the receiver may be unknown (a static call), so the declaration stands for itself.
        outTarget->type = result;
        outTarget->declaration = receiver->declaration;
        outTarget->root = receiver->root;

        return;
    }

    outTarget->type = result;
    outTarget->root = root;
}

// INFO: the result type of a call of a function of a tree, of an imported function, of a method or of a static function.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CallType(MAREA_SemaContext *context, const MAREA_AstNode *callee, MAREA_Uinteger32 depth, MAREA_SemaTypeTarget *outTarget)
{
    if (MAREA_G_IsKind(callee, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_TRUE) {
        MAREA_SemaTypeTarget named = { .type = MAREA_NULLPTR, .declaration = MAREA_NULLPTR, .root = MAREA_NULLPTR };

        const MAREA_SemaExcept RESULT = MAREA_G_NamedTarget(context, callee, &named);

        if ((RESULT == MAREA_SEMA_EXCEPT_SUCCESS) && (MAREA_G_IsKind(named.declaration, MAREA_AST_KIND_FUNCTION_DECLARATION) == MAREA_BOOLEAN_TRUE)) {
            MAREA_G_ResultType(named.declaration, named.root, MAREA_NULLPTR, outTarget);
        }

        return RESULT;
    }

    if (MAREA_G_IsKind(callee, MAREA_AST_KIND_MEMBER_ACCESS) == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *base = MAREA_SemaChildAt(callee, 0);

    const MAREA_AstNode *member = MAREA_SemaChildAt(callee, 1);

    const MAREA_Character8 *text = MAREA_AstNodeGetText(member);

    const MAREA_IntegerSize LENGTH = MAREA_AstNodeGetTextLength(member);

    const MAREA_AstNode *root = MAREA_NULLPTR;

    const MAREA_SemaExcept ROOT_RESULT = MAREA_G_BaseRoot(context, base, member, &root);

    if (ROOT_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
        return ROOT_RESULT;
    }

    if (root != MAREA_NULLPTR) {
        const MAREA_AstNode *function = MAREA_G_FindDeclaration(root, text, LENGTH);

        if (MAREA_G_IsKind(function, MAREA_AST_KIND_FUNCTION_DECLARATION) == MAREA_BOOLEAN_TRUE) {
            MAREA_G_ResultType(function, root, MAREA_NULLPTR, outTarget);
        }

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    MAREA_SemaTypeTarget receiver = { .type = MAREA_NULLPTR, .declaration = MAREA_NULLPTR, .root = MAREA_NULLPTR };

    const MAREA_SemaExcept RESULT = MAREA_G_BaseTarget(context, base, depth, &receiver);

    if ((RESULT != MAREA_SEMA_EXCEPT_SUCCESS) || (receiver.declaration == MAREA_NULLPTR)) {
        return RESULT;
    }

    const MAREA_AstNode *block = MAREA_NULLPTR;

    const MAREA_AstNode *function = MAREA_G_FindMethod(((receiver.root != MAREA_NULLPTR) ? receiver.root : context->root), receiver.declaration, text, LENGTH, &block);

    if (function != MAREA_NULLPTR) {
        MAREA_G_ResultType(function, receiver.root, &receiver, outTarget);
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

// INFO: the written type of a value with the tree that wrote it, read segment by segment: each segment is looked up in the declaration of the segment before it. A shape that sema does not see gives a null type.
static MAREA_SemaExcept MAREA_CALL MAREA_G_ValueType(MAREA_SemaContext *context, const MAREA_AstNode *expression, MAREA_Uinteger32 depth, MAREA_SemaTypeTarget *outTarget)
{
    outTarget->type = MAREA_NULLPTR;
    outTarget->declaration = MAREA_NULLPTR;
    outTarget->root = MAREA_NULLPTR;

    if ((expression == MAREA_NULLPTR) || (depth >= MAREA_G_TARGET_DEPTH)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    switch (MAREA_AstNodeGetKind(expression)) {
    case MAREA_AST_KIND_PARENTHESIZED_EXPRESSION:
        return MAREA_G_ValueType(context, MAREA_SemaChildAt(expression, 0), (depth + 1), outTarget);
    case MAREA_AST_KIND_LITERAL_VALUE:
        outTarget->type = MAREA_SemaChildAt(expression, 0);

        return MAREA_SEMA_EXCEPT_SUCCESS;
    case MAREA_AST_KIND_IDENTIFIER:
    {
        const MAREA_SemaName *local = MAREA_SemaLocalFind(context, MAREA_AstNodeGetText(expression), MAREA_AstNodeGetTextLength(expression));

        outTarget->type = (local != MAREA_NULLPTR) ? local->declaration : MAREA_G_FindChild(MAREA_SemaFindConstant(context, expression), MAREA_AST_KIND_NAMED_TYPE);

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }
    case MAREA_AST_KIND_CALL_EXPRESSION:
        return MAREA_G_CallType(context, MAREA_SemaChildAt(expression, 0), depth, outTarget);
    case MAREA_AST_KIND_INDEX_ACCESS:
    {
        MAREA_SemaTypeTarget base = { .type = MAREA_NULLPTR, .declaration = MAREA_NULLPTR, .root = MAREA_NULLPTR };

        const MAREA_SemaExcept RESULT = MAREA_G_ValueType(context, MAREA_SemaChildAt(expression, 0), (depth + 1), &base);

        if ((RESULT != MAREA_SEMA_EXCEPT_SUCCESS) || (base.type == MAREA_NULLPTR)) {
            return RESULT;
        }

        MAREA_SemaTypeTarget followed = { .type = MAREA_NULLPTR, .declaration = MAREA_NULLPTR, .root = MAREA_NULLPTR };

        MAREA_IGNORE(MAREA_SemaFindTypeTarget(context, base.type, base.root, &followed));

        const MAREA_TypeDescriptor *descriptor = MAREA_NULLPTR;

        if ((MAREA_SemaDescriptorOfType(context, followed.type, &descriptor) == MAREA_BOOLEAN_TRUE) && ((descriptor->hasIndexProtocol == MAREA_BOOLEAN_TRUE) || ((descriptor->roles & MAREA_TYPE_ROLE_RAW_POINTER) != MAREA_TYPE_ROLE_NONE))) {
            outTarget->type = MAREA_SemaChildAt(MAREA_G_FindChild(followed.type, MAREA_AST_KIND_TYPE_ARGUMENTS), 0);
            outTarget->root = followed.root;
        }

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }
    case MAREA_AST_KIND_MEMBER_ACCESS:
    {
        const MAREA_AstNode *base = MAREA_SemaChildAt(expression, 0);

        const MAREA_AstNode *member = MAREA_SemaChildAt(expression, 1);

        const MAREA_AstNode *root = MAREA_NULLPTR;

        const MAREA_SemaExcept ROOT_RESULT = MAREA_G_BaseRoot(context, base, member, &root);

        if (ROOT_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return ROOT_RESULT;
        }

        if (root != MAREA_NULLPTR) {
            const MAREA_AstNode *constant = MAREA_G_FindDeclaration(root, MAREA_AstNodeGetText(member), MAREA_AstNodeGetTextLength(member));

            if (MAREA_G_IsKind(constant, MAREA_AST_KIND_CONSTANT_DECLARATION) == MAREA_BOOLEAN_TRUE) {
                outTarget->type = MAREA_G_FindChild(constant, MAREA_AST_KIND_NAMED_TYPE);
                outTarget->root = root;
            }

            return MAREA_SEMA_EXCEPT_SUCCESS;
        }

        MAREA_SemaTypeTarget owner = { .type = MAREA_NULLPTR, .declaration = MAREA_NULLPTR, .root = MAREA_NULLPTR };

        const MAREA_SemaExcept RESULT = MAREA_G_ValueType(context, base, (depth + 1), &owner);

        if ((RESULT != MAREA_SEMA_EXCEPT_SUCCESS) || (owner.type == MAREA_NULLPTR)) {
            return RESULT;
        }

        MAREA_SemaTypeTarget structure = { .type = MAREA_NULLPTR, .declaration = MAREA_NULLPTR, .root = MAREA_NULLPTR };

        if ((owner.declaration != MAREA_NULLPTR) || (MAREA_SemaFindTypeTarget(context, owner.type, owner.root, &structure) == MAREA_BOOLEAN_TRUE)) {
            const MAREA_SemaTypeTarget *known = (owner.declaration != MAREA_NULLPTR) ? &owner : &structure;

            if (MAREA_G_IsKind(known->declaration, MAREA_AST_KIND_STRUCTURE_DECLARATION) == MAREA_BOOLEAN_TRUE) {
                outTarget->type = MAREA_G_FindChild(MAREA_G_FindField(known->declaration, MAREA_AstNodeGetText(member), MAREA_AstNodeGetTextLength(member)), MAREA_AST_KIND_NAMED_TYPE);
                outTarget->root = known->root;
            }
        }

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }
    default:
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }
}

// INFO: a structure of another namespace that has an explicitly private field. A private structure that the file of the run names is the business of the type rule.
static MAREA_Boolean MAREA_CALL MAREA_G_IsPrivateField(const MAREA_SemaTypeTarget *structure, MAREA_Boolean writtenHere, const MAREA_Character8 *fieldName, MAREA_IntegerSize fieldLength)
{
    if ((structure->root == MAREA_NULLPTR) || (MAREA_G_IsKind(structure->declaration, MAREA_AST_KIND_STRUCTURE_DECLARATION) == MAREA_BOOLEAN_FALSE) || ((writtenHere == MAREA_BOOLEAN_TRUE) && (MAREA_SemaIsPrivate(structure->root, structure->declaration) == MAREA_BOOLEAN_TRUE))) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *field = MAREA_G_FindField(structure->declaration, fieldName, fieldLength);

    // INFO: only the own attribute of a field counts, so a file attribute that hides the symbols of a file never hides a field.
    return (field != MAREA_NULLPTR) ? MAREA_SemaIsPrivate(MAREA_NULLPTR, field) : MAREA_BOOLEAN_FALSE;
}

// INFO: a function of an implementation block of another namespace is private by its own attribute, or by the attribute of its block when it has none.
static MAREA_Boolean MAREA_CALL MAREA_G_IsPrivateMethod(const MAREA_SemaTypeTarget *type, const MAREA_Character8 *name, MAREA_IntegerSize nameLength)
{
    if ((type->root == MAREA_NULLPTR) || (type->declaration == MAREA_NULLPTR)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *block = MAREA_NULLPTR;

    const MAREA_AstNode *function = MAREA_G_FindMethod(type->root, type->declaration, name, nameLength, &block);

    if (function == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    return MAREA_SemaIsPrivate(MAREA_NULLPTR, (MAREA_SemaAttributeFind(MAREA_NULLPTR, function, "visibility") != MAREA_NULLPTR) ? function : block);
}

// INFO: rules 1 and 3. A member of a namespace or a function of a type that is explicitly private is PRIVATE_SYMBOL_ACCESS; a field that is explicitly private, read through any value, is PRIVATE_FIELD_ACCESS.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckMemberAccess(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    const MAREA_AstNode *base = MAREA_SemaChildAt(node, 0);

    const MAREA_AstNode *member = MAREA_SemaChildAt(node, 1);

    if (MAREA_G_IsKind(member, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *memberText = MAREA_AstNodeGetText(member);

    const MAREA_IntegerSize MEMBER_LENGTH = MAREA_AstNodeGetTextLength(member);

    const MAREA_AstNode *root = MAREA_NULLPTR;

    const MAREA_SemaExcept ROOT_RESULT = MAREA_G_BaseRoot(context, base, member, &root);

    if (ROOT_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
        return ROOT_RESULT;
    }

    MAREA_Boolean privateSymbol = MAREA_BOOLEAN_FALSE;

    if (root != MAREA_NULLPTR) {
        const MAREA_AstNode *declaration = MAREA_G_FindDeclaration(root, memberText, MEMBER_LENGTH);

        privateSymbol = ((declaration != MAREA_NULLPTR) && (MAREA_SemaIsPrivate(root, declaration) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    } else {
        MAREA_SemaTypeTarget value = { .type = MAREA_NULLPTR, .declaration = MAREA_NULLPTR, .root = MAREA_NULLPTR };

        const MAREA_SemaExcept VALUE_RESULT = MAREA_G_ValueType(context, base, 0, &value);

        if (VALUE_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return VALUE_RESULT;
        }

        MAREA_SemaTypeTarget target = value;

        if (value.type == MAREA_NULLPTR) {
            const MAREA_SemaExcept NAMED_RESULT = MAREA_G_NamedTarget(context, base, &target);

            if (NAMED_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
                return NAMED_RESULT;
            }
        } else if (value.declaration == MAREA_NULLPTR) {
            MAREA_IGNORE(MAREA_SemaFindTypeTarget(context, value.type, value.root, &target));
        }

        const MAREA_Boolean WRITTEN_HERE = ((value.type != MAREA_NULLPTR) && (value.root == MAREA_NULLPTR)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

        if (MAREA_G_IsPrivateField(&target, WRITTEN_HERE, memberText, MEMBER_LENGTH) == MAREA_BOOLEAN_TRUE) {
            return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_PRIVATE_FIELD_ACCESS, MAREA_AstNodeGetPosition(member), "field is private:", memberText, MEMBER_LENGTH);
        }

        privateSymbol = MAREA_G_IsPrivateMethod(&target, memberText, MEMBER_LENGTH);
    }

    if (privateSymbol == MAREA_BOOLEAN_TRUE) {
        return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_PRIVATE_SYMBOL_ACCESS, MAREA_AstNodeGetPosition(member), "symbol is private:", memberText, MEMBER_LENGTH);
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

// INFO: rule 2. A type written ns.T, or with its full path, whose declaration is explicitly private is PRIVATE_TYPE_ACCESS.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckNamedType(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    const MAREA_AstNode *name = MAREA_SemaChildAt(node, 0);

    if ((MAREA_G_IsKind(name, MAREA_AST_KIND_QUALIFIED_NAME) == MAREA_BOOLEAN_FALSE) || (MAREA_AstNodeGetChildCount(name) < MAREA_INTEGER_SIZE(2))) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *root = MAREA_NULLPTR;

    const MAREA_AstNode *declaration = MAREA_G_DeclarationOfType(context, node, MAREA_NULLPTR, &root);

    if ((MAREA_G_IsTypeDeclaration(declaration) == MAREA_BOOLEAN_FALSE) || (MAREA_SemaIsPrivate(root, declaration) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *last = MAREA_SemaChildAt(name, (MAREA_AstNodeGetChildCount(name) - MAREA_INTEGER_SIZE(1)));

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_PRIVATE_TYPE_ACCESS, MAREA_AstNodeGetPosition(node), "type is private:", MAREA_AstNodeGetText(last), MAREA_AstNodeGetTextLength(last));
}

// INFO: rule 3 for a struct literal: each FIELD_ENTRY that sets an explicitly private field of a type of another namespace is PRIVATE_FIELD_ACCESS.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckLiteral(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    MAREA_SemaTypeTarget structure = { .type = MAREA_NULLPTR, .declaration = MAREA_NULLPTR, .root = MAREA_NULLPTR };

    if (MAREA_SemaFindTypeTarget(context, MAREA_SemaChildAt(node, 0), MAREA_NULLPTR, &structure) == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(node);

    for (MAREA_IntegerSize index = 1; index < COUNT; ++index) {
        const MAREA_AstNode *entry = MAREA_SemaChildAt(node, index);

        const MAREA_AstNode *name = MAREA_SemaChildAt(entry, 0);

        if ((MAREA_G_IsKind(entry, MAREA_AST_KIND_FIELD_ENTRY) == MAREA_BOOLEAN_FALSE) || (MAREA_G_IsKind(name, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_FALSE)) {
            continue;
        }

        if (MAREA_G_IsPrivateField(&structure, MAREA_BOOLEAN_TRUE, MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name)) == MAREA_BOOLEAN_TRUE) {
            const MAREA_SemaExcept REPORT_RESULT = MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_PRIVATE_FIELD_ACCESS, MAREA_AstNodeGetPosition(name), "field is private:", MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name));

            if (REPORT_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
                return REPORT_RESULT;
            }
        }
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckVisibility(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    switch (MAREA_AstNodeGetKind(node)) {
    case MAREA_AST_KIND_MEMBER_ACCESS:
        return MAREA_G_CheckMemberAccess(context, node);
    case MAREA_AST_KIND_NAMED_TYPE:
        return MAREA_G_CheckNamedType(context, node);
    case MAREA_AST_KIND_LITERAL_VALUE:
        return MAREA_G_CheckLiteral(context, node);
    default:
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }
}
