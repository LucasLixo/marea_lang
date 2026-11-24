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
    if ((context == MAREA_NULLPTR) || (context->resolver == MAREA_NULLPTR) || (context->resolver->resolve == MAREA_NULLPTR) || (text == MAREA_NULLPTR) || (length == 0)) {
        return MAREA_NULLPTR;
    }

    MAREA_SemaImportResult result = MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;

    const MAREA_AstNode *root = MAREA_NULLPTR;

    context->resolver->resolve(context->resolver->userData, text, length, &result, &root);

    return (result == MAREA_SEMA_IMPORT_RESULT_FOUND) ? root : MAREA_NULLPTR;
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

// INFO: reads a type written ns.T: the first two segments of its name, and how many segments the name has. Any other shape gives a count of zero.
static MAREA_IntegerSize MAREA_CALL MAREA_G_QualifiedParts(const MAREA_AstNode *type, const MAREA_AstNode **outFirst, const MAREA_AstNode **outSecond)
{
    *outFirst = MAREA_NULLPTR;
    *outSecond = MAREA_NULLPTR;

    if (MAREA_G_IsKind(type, MAREA_AST_KIND_NAMED_TYPE) == MAREA_BOOLEAN_FALSE) {
        return 0;
    }

    const MAREA_AstNode *name = MAREA_SemaChildAt(type, 0);

    if ((MAREA_G_IsKind(name, MAREA_AST_KIND_QUALIFIED_NAME) == MAREA_BOOLEAN_FALSE) || (MAREA_AstNodeGetChildCount(name) < MAREA_INTEGER_SIZE(2))) {
        return 0;
    }

    const MAREA_AstNode *first = MAREA_SemaChildAt(name, 0);

    const MAREA_AstNode *second = MAREA_SemaChildAt(name, 1);

    if ((MAREA_G_IsKind(first, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_FALSE) || (MAREA_G_IsKind(second, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_FALSE)) {
        return 0;
    }

    *outFirst = first;
    *outSecond = second;

    return MAREA_AstNodeGetChildCount(name);
}

// INFO: a type written ns.T whose structure is in a FOUND tree and is not private, and a field of that structure that is explicitly private. A private structure is the business of the type rule.
static MAREA_SemaExcept MAREA_CALL MAREA_G_IsPrivateField(MAREA_SemaContext *context, const MAREA_AstNode *type, const MAREA_Character8 *fieldName, MAREA_IntegerSize fieldLength, MAREA_Boolean *outPrivate)
{
    *outPrivate = MAREA_BOOLEAN_FALSE;

    const MAREA_AstNode *first = MAREA_NULLPTR;

    const MAREA_AstNode *second = MAREA_NULLPTR;

    if (MAREA_G_QualifiedParts(type, &first, &second) != MAREA_INTEGER_SIZE(2)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *root = MAREA_NULLPTR;

    const MAREA_SemaExcept ROOT_RESULT = MAREA_G_NamespaceRoot(context, first, MAREA_BOOLEAN_FALSE, &root);

    if ((ROOT_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) || (root == MAREA_NULLPTR)) {
        return ROOT_RESULT;
    }

    const MAREA_AstNode *structure = MAREA_G_FindDeclaration(root, MAREA_AstNodeGetText(second), MAREA_AstNodeGetTextLength(second));

    if ((MAREA_G_IsKind(structure, MAREA_AST_KIND_STRUCTURE_DECLARATION) == MAREA_BOOLEAN_FALSE) || (MAREA_SemaIsPrivate(root, structure) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *field = MAREA_G_FindField(structure, fieldName, fieldLength);

    // INFO: only the own attribute of a field counts, so a file attribute that hides the symbols of a file never hides a field.
    *outPrivate = (field != MAREA_NULLPTR) ? MAREA_SemaIsPrivate(MAREA_NULLPTR, field) : MAREA_BOOLEAN_FALSE;

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

// INFO: rules 1 and 3. A member of a namespace that is explicitly private is PRIVATE_SYMBOL_ACCESS; a field that is explicitly private, read through a name of type ns.T, is PRIVATE_FIELD_ACCESS.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckMemberAccess(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    const MAREA_AstNode *base = MAREA_SemaChildAt(node, 0);

    const MAREA_AstNode *member = MAREA_SemaChildAt(node, 1);

    if ((MAREA_G_IsKind(base, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_FALSE) || (MAREA_G_IsKind(member, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *memberText = MAREA_AstNodeGetText(member);

    const MAREA_IntegerSize MEMBER_LENGTH = MAREA_AstNodeGetTextLength(member);

    const MAREA_AstNode *root = MAREA_NULLPTR;

    const MAREA_SemaExcept ROOT_RESULT = MAREA_G_NamespaceRoot(context, base, MAREA_BOOLEAN_TRUE, &root);

    if (ROOT_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
        return ROOT_RESULT;
    }

    if (root != MAREA_NULLPTR) {
        const MAREA_AstNode *declaration = MAREA_G_FindDeclaration(root, memberText, MEMBER_LENGTH);

        if ((declaration != MAREA_NULLPTR) && (MAREA_SemaIsPrivate(root, declaration) == MAREA_BOOLEAN_TRUE)) {
            return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_PRIVATE_SYMBOL_ACCESS, MAREA_AstNodeGetPosition(member), "symbol is private:", memberText, MEMBER_LENGTH);
        }

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_SemaName *local = MAREA_SemaLocalFind(context, MAREA_AstNodeGetText(base), MAREA_AstNodeGetTextLength(base));

    if ((local == MAREA_NULLPTR) || (local->declaration == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    MAREA_Boolean isPrivate = MAREA_BOOLEAN_FALSE;

    const MAREA_SemaExcept FIELD_RESULT = MAREA_G_IsPrivateField(context, local->declaration, memberText, MEMBER_LENGTH, &isPrivate);

    if ((FIELD_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) || (isPrivate == MAREA_BOOLEAN_FALSE)) {
        return FIELD_RESULT;
    }

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_PRIVATE_FIELD_ACCESS, MAREA_AstNodeGetPosition(member), "field is private:", memberText, MEMBER_LENGTH);
}

// INFO: rule 2. A type written ns.T whose declaration is explicitly private is PRIVATE_TYPE_ACCESS.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckNamedType(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    const MAREA_AstNode *first = MAREA_NULLPTR;

    const MAREA_AstNode *second = MAREA_NULLPTR;

    if (MAREA_G_QualifiedParts(node, &first, &second) == 0) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *root = MAREA_NULLPTR;

    const MAREA_SemaExcept ROOT_RESULT = MAREA_G_NamespaceRoot(context, first, MAREA_BOOLEAN_FALSE, &root);

    if ((ROOT_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) || (root == MAREA_NULLPTR)) {
        return ROOT_RESULT;
    }

    const MAREA_Character8 *typeText = MAREA_AstNodeGetText(second);

    const MAREA_IntegerSize TYPE_LENGTH = MAREA_AstNodeGetTextLength(second);

    const MAREA_AstNode *declaration = MAREA_G_FindDeclaration(root, typeText, TYPE_LENGTH);

    if ((MAREA_G_IsTypeDeclaration(declaration) == MAREA_BOOLEAN_FALSE) || (MAREA_SemaIsPrivate(root, declaration) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_PRIVATE_TYPE_ACCESS, MAREA_AstNodeGetPosition(node), "type is private:", typeText, TYPE_LENGTH);
}

// INFO: rule 3 for a struct literal: each FIELD_ENTRY that sets an explicitly private field of ns.T is PRIVATE_FIELD_ACCESS.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckLiteral(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    const MAREA_AstNode *type = MAREA_SemaChildAt(node, 0);

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(node);

    for (MAREA_IntegerSize index = 1; index < COUNT; ++index) {
        const MAREA_AstNode *entry = MAREA_SemaChildAt(node, index);

        const MAREA_AstNode *name = MAREA_SemaChildAt(entry, 0);

        if ((MAREA_G_IsKind(entry, MAREA_AST_KIND_FIELD_ENTRY) == MAREA_BOOLEAN_FALSE) || (MAREA_G_IsKind(name, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_FALSE)) {
            continue;
        }

        MAREA_Boolean isPrivate = MAREA_BOOLEAN_FALSE;

        const MAREA_SemaExcept FIELD_RESULT = MAREA_G_IsPrivateField(context, type, MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name), &isPrivate);

        if (FIELD_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return FIELD_RESULT;
        }

        if (isPrivate == MAREA_BOOLEAN_TRUE) {
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
