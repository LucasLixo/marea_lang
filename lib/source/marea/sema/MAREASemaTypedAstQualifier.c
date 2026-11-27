// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <string.h>

MAREA_Boolean MAREA_CALL MAREA_SemaTypedNodeIsQualifier(const MAREA_SemaTypedNode *node)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    return node->isQualifier;
}

MAREA_Boolean MAREA_CALL MAREA_SemaTypedNodeIsExternalCall(const MAREA_SemaTypedNode *node)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    return node->isExternalCall;
}

const MAREA_SemaTypeFrame *MAREA_CALL MAREA_SemaTypeFrameFindForeign(const MAREA_SemaTypeFrame *frame)
{
    for (const MAREA_SemaTypeFrame *current = frame; current != MAREA_NULLPTR; current = current->next) {
        if (current->foreignRoot != MAREA_NULLPTR) {
            return current;
        }
    }

    return MAREA_NULLPTR;
}

static MAREA_SemaExprType MAREA_CALL MAREA_G_NoType(void)
{
    return (MAREA_SemaExprType){ .node = MAREA_NULLPTR, .declaration = MAREA_NULLPTR, .arguments = MAREA_NULLPTR, .frame = MAREA_NULLPTR, .fixed = MAREA_NULLPTR, .fixedLength = 0 };
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsKind(const MAREA_AstNode *node, MAREA_AstKind kind)
{
    return ((node != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(node) == kind)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsTypeDeclaration(const MAREA_AstNode *declaration)
{
    return ((MAREA_G_IsKind(declaration, MAREA_AST_KIND_STRUCTURE_DECLARATION) == MAREA_BOOLEAN_TRUE) || (MAREA_G_IsKind(declaration, MAREA_AST_KIND_ENUM_DECLARATION) == MAREA_BOOLEAN_TRUE) || (MAREA_G_IsKind(declaration, MAREA_AST_KIND_OPAQUE_DECLARATION) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsWrittenType(const MAREA_AstNode *node)
{
    return ((MAREA_G_IsKind(node, MAREA_AST_KIND_NAMED_TYPE) == MAREA_BOOLEAN_TRUE) || (MAREA_G_IsKind(node, MAREA_AST_KIND_FUNCTION_TYPE) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static const MAREA_AstNode *MAREA_CALL MAREA_G_FindChild(const MAREA_AstNode *node, MAREA_AstKind kind)
{
    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(node);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *child = MAREA_SemaChildAt(node, index);

        if (MAREA_G_IsKind(child, kind) == MAREA_BOOLEAN_TRUE) {
            return child;
        }
    }

    return MAREA_NULLPTR;
}

// INFO: the top-level function or constant of a namespace tree that is not explicitly private, or null. A type is not one.
static const MAREA_AstNode *MAREA_CALL MAREA_G_FindVisibleValue(const MAREA_AstNode *root, const MAREA_Character8 *name, MAREA_IntegerSize nameLength)
{
    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(root);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *declaration = MAREA_SemaChildAt(root, index);

        const MAREA_Character8 *declarationName = MAREA_NULLPTR;

        MAREA_IntegerSize declarationLength = 0;

        if ((MAREA_SemaDeclarationName(declaration, &declarationName, &declarationLength) == MAREA_BOOLEAN_FALSE) || (MAREA_SemaTextEquals(declarationName, declarationLength, name, nameLength) == MAREA_BOOLEAN_FALSE)) {
            continue;
        }

        if ((MAREA_SemaIsPrivate(root, declaration) == MAREA_BOOLEAN_TRUE) || ((MAREA_G_IsKind(declaration, MAREA_AST_KIND_FUNCTION_DECLARATION) == MAREA_BOOLEAN_FALSE) && (MAREA_G_IsKind(declaration, MAREA_AST_KIND_CONSTANT_DECLARATION) == MAREA_BOOLEAN_FALSE))) {
            continue;
        }

        return declaration;
    }

    return MAREA_NULLPTR;
}

// INFO: true when a namespace tree has a top-level declaration of a name, private or not.
static MAREA_Boolean MAREA_CALL MAREA_G_Declares(const MAREA_AstNode *root, const MAREA_Character8 *name, MAREA_IntegerSize nameLength)
{
    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(root);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_Character8 *declarationName = MAREA_NULLPTR;

        MAREA_IntegerSize declarationLength = 0;

        if ((MAREA_SemaDeclarationName(MAREA_SemaChildAt(root, index), &declarationName, &declarationLength) == MAREA_BOOLEAN_TRUE) && (MAREA_SemaTextEquals(declarationName, declarationLength, name, nameLength) == MAREA_BOOLEAN_TRUE)) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

static MAREA_SemaTypeFrame *MAREA_CALL MAREA_G_NewFrame(MAREA_SemaContext *context)
{
    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(context->arena, sizeof(MAREA_SemaTypeFrame), MAREA_ALIGNOF(MAREA_SemaTypeFrame), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_NULLPTR;
    }

    MAREA_SemaTypeFrame *frame = memory;

    *frame = (MAREA_SemaTypeFrame){ .parameters = MAREA_NULLPTR, .arguments = MAREA_NULLPTR, .selfDeclaration = MAREA_NULLPTR, .selfArguments = MAREA_NULLPTR, .argumentFrame = MAREA_NULLPTR, .next = MAREA_NULLPTR, .foreignRoot = MAREA_NULLPTR, .foreignName = MAREA_NULLPTR, .foreignNameLength = 0 };

    return frame;
}

// INFO: the declared type of a public function or constant of a FOUND tree. A function with generic parameters gets a frame with the type arguments written at the call, or none, on top of the frame that marks the other file.
static MAREA_SemaTypedAstExcept MAREA_CALL MAREA_G_ValueType(MAREA_SemaContext *context, const MAREA_AstNode *root, const MAREA_Character8 *namespaceText, MAREA_IntegerSize namespaceLength, const MAREA_Character8 *name, MAREA_IntegerSize nameLength, const MAREA_AstNode *callTypeArguments, MAREA_SemaExprType *outType)
{
    const MAREA_AstNode *declaration = MAREA_G_FindVisibleValue(root, name, nameLength);

    if (declaration == MAREA_NULLPTR) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
    }

    MAREA_SemaTypeFrame *foreign = MAREA_G_NewFrame(context);

    if (foreign == MAREA_NULLPTR) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    // INFO: the name that this file gives a type of the other file is the last segment of the namespace, as in structures.Shape.
    MAREA_IntegerSize last = namespaceLength;

    while ((last > 0) && (namespaceText[(last - MAREA_INTEGER_SIZE(1))] != '.')) {
        last -= 1;
    }

    foreign->foreignRoot = root;
    foreign->foreignName = (namespaceText + last);
    foreign->foreignNameLength = (namespaceLength - last);

    if (MAREA_G_IsKind(declaration, MAREA_AST_KIND_CONSTANT_DECLARATION) == MAREA_BOOLEAN_TRUE) {
        const MAREA_AstNode *type = MAREA_SemaChildAt(declaration, (MAREA_AstNodeGetChildCount(declaration) - MAREA_INTEGER_SIZE(2)));

        if (MAREA_G_IsWrittenType(type) == MAREA_BOOLEAN_TRUE) {
            outType->node = type;
            outType->frame = foreign;
        }

        return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *parameters = MAREA_G_FindChild(declaration, MAREA_AST_KIND_GENERIC_PARAMETERS);

    const MAREA_SemaTypeFrame *frame = foreign;

    if (parameters != MAREA_NULLPTR) {
        MAREA_SemaTypeFrame *generic = MAREA_G_NewFrame(context);

        if (generic == MAREA_NULLPTR) {
            return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_OUT_OF_MEMORY;
        }

        generic->parameters = parameters;
        generic->arguments = callTypeArguments;
        generic->next = foreign;

        frame = generic;
    }

    outType->node = declaration;
    outType->frame = frame;

    return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
}

MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypedAstFoundMemberType(MAREA_SemaContext *context, const MAREA_AstNode *base, const MAREA_AstNode *member, const MAREA_AstNode *callTypeArguments, MAREA_SemaExprType *outType)
{
    if ((context == MAREA_NULLPTR) || (outType == MAREA_NULLPTR)) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    *outType = MAREA_G_NoType();

    if (MAREA_G_IsKind(member, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
    }

    // INFO: a local or a parameter hides a namespace, a generic parameter is no namespace, and a full path counts only when a cover reaches it.
    const MAREA_SemaName *entry = MAREA_NULLPTR;

    if (MAREA_SemaBaseEntry(context, base, member, &entry) != MAREA_SEMA_EXCEPT_SUCCESS) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    if ((entry == MAREA_NULLPTR) || (entry->kind != MAREA_SEMA_NAME_KIND_NAMESPACE)) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *namespaceText = MAREA_NULLPTR;

    MAREA_IntegerSize namespaceLength = 0;

    const MAREA_SemaTypedAstExcept JOINED = MAREA_SemaNamespaceOf(context, entry, &namespaceText, &namespaceLength);

    if (JOINED != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
        return JOINED;
    }

    const MAREA_AstNode *root = MAREA_SemaFoundRoot(context, namespaceText, namespaceLength);

    if (root == MAREA_NULLPTR) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
    }

    return MAREA_G_ValueType(context, root, namespaceText, namespaceLength, MAREA_AstNodeGetText(member), MAREA_AstNodeGetTextLength(member), callTypeArguments, outType);
}

MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypedAstFoundNameType(MAREA_SemaContext *context, const MAREA_AstNode *identifier, const MAREA_AstNode *callTypeArguments, MAREA_SemaExprType *outType)
{
    if ((context == MAREA_NULLPTR) || (outType == MAREA_NULLPTR)) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    *outType = MAREA_G_NoType();

    if (MAREA_G_IsKind(identifier, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
    }

    const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, MAREA_AstNodeGetText(identifier), MAREA_AstNodeGetTextLength(identifier));

    if ((entry == MAREA_NULLPTR) || (entry->kind != MAREA_SEMA_NAME_KIND_IMPORTED_SYMBOL) || (entry->qualifierLength == 0)) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *name = (entry->original != MAREA_NULLPTR) ? entry->original : entry->text;

    const MAREA_IntegerSize NAME_LENGTH = (entry->original != MAREA_NULLPTR) ? entry->originalLength : entry->length;

    // INFO: a member path with a dot (Color.Red) names a variant, not a function or a constant.
    if (memchr(name, '.', NAME_LENGTH) != MAREA_NULLPTR) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *root = MAREA_SemaFoundRoot(context, entry->qualifier, entry->qualifierLength);

    if (root == MAREA_NULLPTR) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
    }

    return MAREA_G_ValueType(context, root, entry->qualifier, entry->qualifierLength, name, NAME_LENGTH, callTypeArguments, outType);
}

// INFO: true when a resolver answers EXTERNAL for a namespace text; the signature function is never asked for a namespace that no resolver calls EXTERNAL.
static MAREA_Boolean MAREA_CALL MAREA_G_IsExternalNamespace(const MAREA_SemaContext *context, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    if ((context->resolver == MAREA_NULLPTR) || (text == MAREA_NULLPTR) || (length == 0)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *root = MAREA_NULLPTR;

    return (MAREA_SemaResolveNamespace(context, text, length, &root) == MAREA_SEMA_IMPORT_RESULT_EXTERNAL) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: a name that an import or a require bound into a namespace the resolver answers EXTERNAL for.
static MAREA_Boolean MAREA_CALL MAREA_G_IsExternalImport(const MAREA_SemaContext *context, const MAREA_SemaName *entry)
{
    return ((entry != MAREA_NULLPTR) && (entry->kind != MAREA_SEMA_NAME_KIND_DECLARATION) && (entry->qualifierLength > 0) && (MAREA_G_IsExternalNamespace(context, entry->qualifier, entry->qualifierLength) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: a member chain callee: its head is never a local, a generic parameter, Self or a declaration of the file; a namespace alias head is read as the path it stands for, and the longest namespace of the path decides.
static MAREA_SemaTypedAstExcept MAREA_CALL MAREA_G_IsExternalChain(MAREA_SemaContext *context, const MAREA_AstNode *callee, MAREA_Boolean *outExternal)
{
    const MAREA_Character8 *text = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    const MAREA_AstNode *head = MAREA_NULLPTR;

    if (MAREA_SemaChainJoin(context, callee, &text, &length, &head) != MAREA_SEMA_EXCEPT_SUCCESS) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    if (text == MAREA_NULLPTR) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *headText = MAREA_AstNodeGetText(head);

    const MAREA_IntegerSize HEAD_LENGTH = MAREA_AstNodeGetTextLength(head);

    if ((MAREA_SemaTextEquals(headText, HEAD_LENGTH, "Self", MAREA_INTEGER_SIZE(4)) == MAREA_BOOLEAN_TRUE) || (MAREA_SemaLocalFind(context, headText, HEAD_LENGTH) != MAREA_NULLPTR) || (MAREA_SemaIsGenericInScope(context, headText, HEAD_LENGTH) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
    }

    const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, headText, HEAD_LENGTH);

    if ((entry != MAREA_NULLPTR) && (entry->kind != MAREA_SEMA_NAME_KIND_NAMESPACE)) {
        *outExternal = MAREA_G_IsExternalImport(context, entry);

        return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
    }

    MAREA_SemaPathStatus status = MAREA_SEMA_PATH_STATUS_NOT_A_PATH;

    const MAREA_Character8 *namespaceText = MAREA_NULLPTR;

    MAREA_IntegerSize namespaceLength = 0;

    if (MAREA_SemaClassifyPath(context, text, length, MAREA_BOOLEAN_TRUE, &status, &namespaceText, &namespaceLength) != MAREA_SEMA_EXCEPT_SUCCESS) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    *outExternal = ((status == MAREA_SEMA_PATH_STATUS_COVERED) && (MAREA_G_IsExternalNamespace(context, namespaceText, namespaceLength) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
}

MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypedAstIsExternalCallee(MAREA_SemaContext *context, const MAREA_AstNode *callee, MAREA_Boolean *outExternal)
{
    if ((context == MAREA_NULLPTR) || (outExternal == MAREA_NULLPTR)) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    *outExternal = MAREA_BOOLEAN_FALSE;

    const MAREA_AstNode *inner = callee;

    // INFO: the ir erases the parentheses of a callee, so they are read through here too.
    while (MAREA_G_IsKind(inner, MAREA_AST_KIND_PARENTHESIZED_EXPRESSION) == MAREA_BOOLEAN_TRUE) {
        inner = MAREA_SemaChildAt(inner, 0);
    }

    if (MAREA_G_IsKind(inner, MAREA_AST_KIND_MEMBER_ACCESS) == MAREA_BOOLEAN_TRUE) {
        const MAREA_SemaTypedAstExcept CHAIN = MAREA_G_IsExternalChain(context, inner, outExternal);

        if ((CHAIN != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) || (*outExternal == MAREA_BOOLEAN_TRUE)) {
            return CHAIN;
        }

        // INFO: a registered function of a shipped library that has a source file is external too.
        const MAREA_SemaSignature *signature = MAREA_NULLPTR;

        const MAREA_SemaTypedAstExcept REGISTERED = MAREA_SemaTypedAstExternalMemberSignature(context, MAREA_SemaChildAt(inner, 0), MAREA_SemaChildAt(inner, 1), &signature);

        *outExternal = (signature != MAREA_NULLPTR) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

        return REGISTERED;
    }

    if ((MAREA_G_IsKind(inner, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_FALSE) || (MAREA_SemaLocalFind(context, MAREA_AstNodeGetText(inner), MAREA_AstNodeGetTextLength(inner)) != MAREA_NULLPTR)) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
    }

    *outExternal = MAREA_G_IsExternalImport(context, MAREA_SemaNameTableFind(context, MAREA_AstNodeGetText(inner), MAREA_AstNodeGetTextLength(inner)));

    return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
}

// INFO: asks the optional signature function of the resolver that knows the namespace for one member; a resolver with none, or an answer other than FOUND, leaves outSignature null.
static void MAREA_CALL MAREA_G_QuerySignature(const MAREA_SemaContext *context, const MAREA_Character8 *namespaceText, MAREA_IntegerSize namespaceLength, const MAREA_Character8 *memberText, MAREA_IntegerSize memberLength, const MAREA_SemaSignature **outSignature)
{
    *outSignature = MAREA_NULLPTR;

    const MAREA_SemaImportResolver *owner = MAREA_SemaResolverOf(context, namespaceText, namespaceLength);

    // INFO: no resolver knows the namespace of the file by name alone, so the C side answers for it.
    if (owner == MAREA_NULLPTR) {
        owner = (context->requireResolver != MAREA_NULLPTR) ? context->requireResolver : context->resolver;
    }

    if ((owner == MAREA_NULLPTR) || (owner->signature == MAREA_NULLPTR)) {
        return;
    }

    MAREA_SemaImportResult result = MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;

    const MAREA_SemaSignature *signature = MAREA_NULLPTR;

    owner->signature(owner->userData, namespaceText, namespaceLength, memberText, memberLength, &result, &signature);

    if (result == MAREA_SEMA_IMPORT_RESULT_FOUND) {
        *outSignature = signature;
    }
}

// INFO: the root of the file when a text is the namespace that the file declares, else null.
static const MAREA_AstNode *MAREA_CALL MAREA_G_OwnRoot(MAREA_SemaContext *context, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    const MAREA_IntegerSize COUNT = (context->root == MAREA_NULLPTR) ? MAREA_INTEGER_SIZE(0) : MAREA_AstNodeGetChildCount(context->root);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *declaration = MAREA_SemaChildAt(context->root, index);

        if (MAREA_G_IsKind(declaration, MAREA_AST_KIND_NAMESPACE_DECLARATION) == MAREA_BOOLEAN_FALSE) {
            continue;
        }

        const MAREA_Character8 *own = MAREA_NULLPTR;

        MAREA_IntegerSize ownLength = 0;

        const MAREA_Boolean SAME = ((MAREA_SemaQualifiedNameJoin(context, MAREA_SemaChildAt(declaration, 0), &own, &ownLength) == MAREA_SEMA_EXCEPT_SUCCESS) && (MAREA_SemaTextEquals(own, ownLength, text, length) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

        return (SAME == MAREA_BOOLEAN_TRUE) ? context->root : MAREA_NULLPTR;
    }

    return MAREA_NULLPTR;
}

// INFO: the namespace that a member access reads a member from: an alias or a covered path, or the full path of the namespace of the file, which needs no cover. Null for any other base.
static MAREA_SemaTypedAstExcept MAREA_CALL MAREA_G_MemberNamespace(MAREA_SemaContext *context, const MAREA_AstNode *base, const MAREA_AstNode *member, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength)
{
    *outText = MAREA_NULLPTR;
    *outLength = 0;

    // INFO: the base is read exactly as MAREA_SemaTypedAstFoundMemberType reads it.
    const MAREA_SemaName *entry = MAREA_NULLPTR;

    if (MAREA_SemaBaseEntry(context, base, member, &entry) != MAREA_SEMA_EXCEPT_SUCCESS) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    if (entry != MAREA_NULLPTR) {
        return (entry->kind == MAREA_SEMA_NAME_KIND_NAMESPACE) ? MAREA_SemaNamespaceOf(context, entry, outText, outLength) : MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *text = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    const MAREA_AstNode *head = MAREA_NULLPTR;

    if (MAREA_SemaChainJoin(context, base, &text, &length, &head) != MAREA_SEMA_EXCEPT_SUCCESS) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    if ((text == MAREA_NULLPTR) || (MAREA_SemaLocalFind(context, MAREA_AstNodeGetText(head), MAREA_AstNodeGetTextLength(head)) != MAREA_NULLPTR) || (MAREA_G_OwnRoot(context, text, length) == MAREA_NULLPTR)) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
    }

    *outText = text;
    *outLength = length;

    return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
}

// INFO: only a library that the language ships, under c or marea, has a source file and registered C functions in one namespace.
static MAREA_Boolean MAREA_CALL MAREA_G_IsShippedNamespace(const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    return (((length > MAREA_INTEGER_SIZE(2)) && (memcmp(text, "c.", MAREA_INTEGER_SIZE(2)) == 0)) || ((length > MAREA_INTEGER_SIZE(6)) && (memcmp(text, "marea.", MAREA_INTEGER_SIZE(6)) == 0))) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypedAstExternalMemberSignature(MAREA_SemaContext *context, const MAREA_AstNode *base, const MAREA_AstNode *member, const MAREA_SemaSignature **outSignature)
{
    if ((context == MAREA_NULLPTR) || (outSignature == MAREA_NULLPTR)) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    *outSignature = MAREA_NULLPTR;

    if (MAREA_G_IsKind(member, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *namespaceText = MAREA_NULLPTR;

    MAREA_IntegerSize namespaceLength = 0;

    const MAREA_SemaTypedAstExcept JOINED = MAREA_G_MemberNamespace(context, base, member, &namespaceText, &namespaceLength);

    if ((JOINED != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) || (namespaceText == MAREA_NULLPTR)) {
        return JOINED;
    }

    const MAREA_Character8 *name = MAREA_AstNodeGetText(member);

    const MAREA_IntegerSize NAME_LENGTH = MAREA_AstNodeGetTextLength(member);

    // INFO: a member of a shipped library that its source file does not declare is a C function that the library registered.
    if (MAREA_G_IsExternalNamespace(context, namespaceText, namespaceLength) == MAREA_BOOLEAN_FALSE) {
        const MAREA_AstNode *root = MAREA_G_OwnRoot(context, namespaceText, namespaceLength);

        if (root == MAREA_NULLPTR) {
            root = MAREA_SemaFoundRoot(context, namespaceText, namespaceLength);
        }

        if ((root == MAREA_NULLPTR) || (MAREA_G_IsShippedNamespace(namespaceText, namespaceLength) == MAREA_BOOLEAN_FALSE) || (MAREA_G_Declares(root, name, NAME_LENGTH) == MAREA_BOOLEAN_TRUE)) {
            return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
        }
    }

    MAREA_G_QuerySignature(context, namespaceText, namespaceLength, name, NAME_LENGTH, outSignature);

    return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
}

MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypedAstExternalNameSignature(MAREA_SemaContext *context, const MAREA_AstNode *identifier, const MAREA_SemaSignature **outSignature)
{
    if ((context == MAREA_NULLPTR) || (outSignature == MAREA_NULLPTR)) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    *outSignature = MAREA_NULLPTR;

    if (MAREA_G_IsKind(identifier, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
    }

    const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, MAREA_AstNodeGetText(identifier), MAREA_AstNodeGetTextLength(identifier));

    // INFO: an import of a full path into an EXTERNAL namespace with no tree (import c.math.pow as power;) never
    // resolves its own full joined path as NOT_FOUND, because every depth of it answers EXTERNAL, so the name stays
    // bound as a NAMESPACE with the member kept as its original, never becoming an IMPORTED_SYMBOL the way a
    // deeper member of a FOUND namespace does (MAREA_G_CheckImport, the "unresolved import" branch). Both shapes
    // are read the same way here: the qualifier is the namespace, the original (or the name itself) is the member.
    if ((entry == MAREA_NULLPTR) || (entry->kind == MAREA_SEMA_NAME_KIND_DECLARATION) || (entry->qualifierLength == 0)) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *name = (entry->original != MAREA_NULLPTR) ? entry->original : entry->text;

    const MAREA_IntegerSize NAME_LENGTH = (entry->original != MAREA_NULLPTR) ? entry->originalLength : entry->length;

    // INFO: a member path with a dot (Color.Red) names a variant, never a function or a constant, exactly as MAREA_SemaTypedAstFoundNameType reads the name.
    if (memchr(name, '.', NAME_LENGTH) != MAREA_NULLPTR) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
    }

    if (MAREA_G_IsExternalNamespace(context, entry->qualifier, entry->qualifierLength) == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
    }

    MAREA_G_QuerySignature(context, entry->qualifier, entry->qualifierLength, name, NAME_LENGTH, outSignature);

    return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
}

// INFO: an imported symbol is a qualifier only when it is a type of a FOUND tree: a function or a constant is a value, and the tree of an EXTERNAL namespace is unknown.
static MAREA_Boolean MAREA_CALL MAREA_G_ImportedSymbolIsType(const MAREA_SemaContext *context, const MAREA_SemaName *entry)
{
    if (MAREA_G_IsTypeDeclaration(entry->declaration) == MAREA_BOOLEAN_TRUE) {
        return MAREA_BOOLEAN_TRUE;
    }

    const MAREA_AstNode *root = MAREA_SemaFoundRoot(context, entry->qualifier, entry->qualifierLength);

    if (root == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_Character8 *name = (entry->original != MAREA_NULLPTR) ? entry->original : entry->text;

    const MAREA_IntegerSize NAME_LENGTH = (entry->original != MAREA_NULLPTR) ? entry->originalLength : entry->length;

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(root);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *declaration = MAREA_SemaChildAt(root, index);

        const MAREA_Character8 *declarationName = MAREA_NULLPTR;

        MAREA_IntegerSize declarationLength = 0;

        if ((MAREA_G_IsTypeDeclaration(declaration) == MAREA_BOOLEAN_TRUE) && (MAREA_SemaDeclarationName(declaration, &declarationName, &declarationLength) == MAREA_BOOLEAN_TRUE) && (MAREA_SemaTextEquals(declarationName, declarationLength, name, NAME_LENGTH) == MAREA_BOOLEAN_TRUE)) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

// INFO: a chain such as demo.io before the member of demo.io.puts is the written path of a namespace or of a type of one, so it qualifies the member and is no value.
static MAREA_Boolean MAREA_CALL MAREA_G_IsNamespaceChain(MAREA_SemaContext *context, const MAREA_AstNode *chain)
{
    const MAREA_Character8 *text = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    if ((MAREA_SemaChainJoin(context, chain, &text, &length, MAREA_NULLPTR) != MAREA_SEMA_EXCEPT_SUCCESS) || (text == MAREA_NULLPTR)) {
        return MAREA_BOOLEAN_FALSE;
    }

    // INFO: the namespace of the file needs no cover, so its full path qualifies a member too.
    if (MAREA_G_OwnRoot(context, text, length) != MAREA_NULLPTR) {
        return MAREA_BOOLEAN_TRUE;
    }

    MAREA_SemaPathStatus status = MAREA_SEMA_PATH_STATUS_NOT_A_PATH;

    const MAREA_Character8 *namespaceText = MAREA_NULLPTR;

    MAREA_IntegerSize namespaceLength = 0;

    if ((MAREA_SemaClassifyPath(context, text, length, MAREA_BOOLEAN_FALSE, &status, &namespaceText, &namespaceLength) != MAREA_SEMA_EXCEPT_SUCCESS) || (status != MAREA_SEMA_PATH_STATUS_COVERED)) {
        return MAREA_BOOLEAN_FALSE;
    }

    // INFO: the chain is covered and some prefix of it is a namespace, so it is a namespace or a type of one, never a value.
    return ((namespaceLength > 0) && (namespaceText != MAREA_NULLPTR)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

MAREA_Boolean MAREA_CALL MAREA_SemaTypedAstIsQualifier(MAREA_SemaContext *context, const MAREA_AstNode *node, const MAREA_AstNode *parent, MAREA_IntegerSize index)
{
    if ((context != MAREA_NULLPTR) && (MAREA_G_IsKind(node, MAREA_AST_KIND_MEMBER_ACCESS) == MAREA_BOOLEAN_TRUE) && (MAREA_G_IsKind(parent, MAREA_AST_KIND_MEMBER_ACCESS) == MAREA_BOOLEAN_TRUE) && (index == 0)) {
        return MAREA_G_IsNamespaceChain(context, node);
    }

    if ((context == MAREA_NULLPTR) || (MAREA_G_IsKind(node, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_FALSE) || (MAREA_G_IsKind(parent, MAREA_AST_KIND_MEMBER_ACCESS) == MAREA_BOOLEAN_FALSE) || (index != 0)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_Character8 *text = MAREA_AstNodeGetText(node);

    const MAREA_IntegerSize LENGTH = MAREA_AstNodeGetTextLength(node);

    if ((MAREA_SemaLocalFind(context, text, LENGTH) != MAREA_NULLPTR) || (MAREA_SemaIsGenericInScope(context, text, LENGTH) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, text, LENGTH);

    // INFO: a name with no declaration is taken for a namespace, as the checker does for the head of a member chain.
    if (entry == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_TRUE;
    }

    switch (entry->kind) {
    case MAREA_SEMA_NAME_KIND_NAMESPACE:
        return MAREA_BOOLEAN_TRUE;
    case MAREA_SEMA_NAME_KIND_DECLARATION:
        return MAREA_G_IsTypeDeclaration(entry->declaration);
    case MAREA_SEMA_NAME_KIND_IMPORTED_SYMBOL:
        return MAREA_G_ImportedSymbolIsType(context, entry);
    default:
        return MAREA_BOOLEAN_FALSE;
    }
}
