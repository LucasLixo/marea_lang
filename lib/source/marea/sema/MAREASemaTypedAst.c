// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <string.h>

static MAREA_Boolean MAREA_CALL MAREA_G_TypedAstWasChecked(const MAREA_SemaContext *context, const MAREA_AstNode *root)
{
    if (context->root != root) {
        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_DiagnosticSinkGetErrorCount(context->sink) != 0) {
        return MAREA_BOOLEAN_FALSE;
    }

    return MAREA_BOOLEAN_TRUE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_TypedAstTextIs(const MAREA_Character8 *text, MAREA_IntegerSize length, const MAREA_Character8 *expected)
{
    return MAREA_SemaTextEquals(text, length, expected, strlen(expected));
}

static const MAREA_AstNode *MAREA_CALL MAREA_G_TypedAstFindChild(const MAREA_AstNode *node, MAREA_AstKind kind)
{
    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(node);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *child = MAREA_SemaChildAt(node, index);

        if ((child != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(child) == kind)) {
            return child;
        }
    }

    return MAREA_NULLPTR;
}

static MAREA_Boolean MAREA_CALL MAREA_G_TypedAstSingleName(const MAREA_AstNode *type, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength)
{
    if ((type == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(type) != MAREA_AST_KIND_NAMED_TYPE)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *name = MAREA_SemaChildAt(type, 0);

    if ((name == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(name) != MAREA_AST_KIND_QUALIFIED_NAME) || (MAREA_AstNodeGetChildCount(name) != 1)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *segment = MAREA_SemaChildAt(name, 0);

    if ((segment == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(segment) != MAREA_AST_KIND_IDENTIFIER)) {
        return MAREA_BOOLEAN_FALSE;
    }

    *outText = MAREA_AstNodeGetText(segment);
    *outLength = MAREA_AstNodeGetTextLength(segment);

    return MAREA_BOOLEAN_TRUE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_TypedAstIsType(const MAREA_AstNode *node)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstKind KIND = MAREA_AstNodeGetKind(node);

    return ((KIND == MAREA_AST_KIND_NAMED_TYPE) || (KIND == MAREA_AST_KIND_FUNCTION_TYPE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: a standalone inference path for the typed tree only; MAREA_SemaInferType stays untouched.
static const MAREA_AstNode *MAREA_CALL MAREA_G_InferTypedResultType(MAREA_SemaContext *context, const MAREA_AstNode *expression)
{
    if (expression == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    const MAREA_AstKind KIND = MAREA_AstNodeGetKind(expression);

    if (KIND == MAREA_AST_KIND_PARENTHESIZED_EXPRESSION) {
        return MAREA_G_InferTypedResultType(context, MAREA_SemaChildAt(expression, 0));
    }

    if (KIND == MAREA_AST_KIND_LITERAL_VALUE) {
        const MAREA_AstNode *type = MAREA_SemaChildAt(expression, 0);

        return (MAREA_G_TypedAstIsType(type) == MAREA_BOOLEAN_TRUE) ? type : MAREA_NULLPTR;
    }

    if (KIND == MAREA_AST_KIND_MEMBER_ACCESS) {
        const MAREA_AstNode *base = MAREA_G_InferTypedResultType(context, MAREA_SemaChildAt(expression, 0));

        const MAREA_AstNode *member = MAREA_SemaChildAt(expression, 1);

        const MAREA_AstNode *declaration = MAREA_NULLPTR;

        const MAREA_Character8 *name = MAREA_NULLPTR;

        MAREA_IntegerSize nameLength = 0;

        if ((base == MAREA_NULLPTR) || (member == MAREA_NULLPTR) || (MAREA_SemaClassifyType(context, base, &declaration, &name, &nameLength) != MAREA_SEMA_TYPE_CLASS_USER) || (MAREA_AstNodeGetKind(declaration) != MAREA_AST_KIND_STRUCTURE_DECLARATION)) {
            return MAREA_NULLPTR;
        }

        const MAREA_IntegerSize FIELD_COUNT = MAREA_AstNodeGetChildCount(declaration);

        for (MAREA_IntegerSize index = 0; index < FIELD_COUNT; ++index) {
            const MAREA_AstNode *field = MAREA_SemaChildAt(declaration, index);

            const MAREA_AstNode *fieldName = ((field != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(field) == MAREA_AST_KIND_FIELD_DECLARATION)) ? MAREA_G_TypedAstFindChild(field, MAREA_AST_KIND_IDENTIFIER) : MAREA_NULLPTR;

            if ((fieldName != MAREA_NULLPTR) && (MAREA_SemaTextEquals(MAREA_AstNodeGetText(fieldName), MAREA_AstNodeGetTextLength(fieldName), MAREA_AstNodeGetText(member), MAREA_AstNodeGetTextLength(member)) == MAREA_BOOLEAN_TRUE)) {
                const MAREA_AstNode *type = MAREA_SemaChildAt(field, (MAREA_AstNodeGetChildCount(field) - 1));

                return (MAREA_G_TypedAstIsType(type) == MAREA_BOOLEAN_TRUE) ? type : MAREA_NULLPTR;
            }
        }

        return MAREA_NULLPTR;
    }

    if (KIND == MAREA_AST_KIND_INDEX_ACCESS) {
        const MAREA_AstNode *base = MAREA_G_InferTypedResultType(context, MAREA_SemaChildAt(expression, 0));

        const MAREA_TypeDescriptor *descriptor = MAREA_NULLPTR;

        const MAREA_AstNode *part = MAREA_SemaTupleElement(context, base, MAREA_SemaChildAt(expression, 1));

        if (part != MAREA_NULLPTR) {
            return (MAREA_G_TypedAstIsType(part) == MAREA_BOOLEAN_TRUE) ? part : MAREA_NULLPTR;
        }

        const MAREA_Boolean HAS_ELEMENT = ((MAREA_SemaDescriptorOfType(context, base, &descriptor) == MAREA_BOOLEAN_TRUE) && ((descriptor->hasIndexProtocol == MAREA_BOOLEAN_TRUE) || ((descriptor->roles & MAREA_TYPE_ROLE_RAW_POINTER) != MAREA_TYPE_ROLE_NONE))) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

        if (HAS_ELEMENT == MAREA_BOOLEAN_FALSE) {
            return MAREA_NULLPTR;
        }

        const MAREA_AstNode *arguments = MAREA_G_TypedAstFindChild(base, MAREA_AST_KIND_TYPE_ARGUMENTS);

        const MAREA_AstNode *element = (arguments == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_SemaChildAt(arguments, 0);

        return (MAREA_G_TypedAstIsType(element) == MAREA_BOOLEAN_TRUE) ? element : MAREA_NULLPTR;
    }

    if (KIND != MAREA_AST_KIND_IDENTIFIER) {
        return MAREA_NULLPTR;
    }

    for (MAREA_IntegerSize index = context->locals.count; index > 0; --index) {
        const MAREA_SemaName *local = &context->locals.entries[(index - 1)];

        if (MAREA_SemaTextEquals(local->text, local->length, MAREA_AstNodeGetText(expression), MAREA_AstNodeGetTextLength(expression)) == MAREA_BOOLEAN_TRUE) {
            return local->declaration;
        }
    }

    return MAREA_NULLPTR;
}

static MAREA_SemaTypedAstExcept MAREA_CALL MAREA_G_TypedAstEnterFunctionGenerics(MAREA_SemaContext *context, const MAREA_AstNode *function, MAREA_IntegerSize *outSavedCount)
{
    *outSavedCount = context->generics.count;

    const MAREA_AstNode *parameters = (function == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_G_TypedAstFindChild(function, MAREA_AST_KIND_GENERIC_PARAMETERS);

    if (parameters == MAREA_NULLPTR) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(parameters);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *parameter = MAREA_SemaChildAt(parameters, index);

        const MAREA_AstNode *name = (parameter == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_SemaChildAt(parameter, 0);

        if ((name == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(name) != MAREA_AST_KIND_IDENTIFIER)) {
            continue;
        }

        if (MAREA_SemaTableAppend(context, &context->generics, (MAREA_SemaName){ .text = MAREA_AstNodeGetText(name), .declaration = MAREA_NULLPTR, .length = MAREA_AstNodeGetTextLength(name), .kind = MAREA_SEMA_NAME_KIND_DECLARATION }) != MAREA_SEMA_EXCEPT_SUCCESS) {
            return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_OUT_OF_MEMORY;
        }
    }

    return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
}

static void MAREA_CALL MAREA_G_TypedAstLeaveFunctionGenerics(MAREA_SemaContext *context, MAREA_IntegerSize savedCount)
{
    context->generics.count = savedCount;
}

static void MAREA_CALL MAREA_G_TypedAstApplyType(MAREA_SemaContext *context, MAREA_SemaTypedNode *built, const MAREA_AstNode *type)
{
    const MAREA_AstNode *declaration = MAREA_NULLPTR;

    const MAREA_Character8 *name = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    built->typeClass = MAREA_SemaClassifyType(context, type, &declaration, &name, &length);
    built->typeDeclaration = declaration;
    built->typeName = name;
    built->typeNameLength = length;
}

static MAREA_Boolean MAREA_CALL MAREA_G_TypedAstNameIsBound(const MAREA_SemaContext *context, const MAREA_AstNode *name)
{
    for (MAREA_IntegerSize index = context->locals.count; index > 0; --index) {
        const MAREA_SemaName *local = &context->locals.entries[(index - 1)];

        if (MAREA_SemaTextEquals(local->text, local->length, MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name)) == MAREA_BOOLEAN_TRUE) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return (MAREA_SemaNameTableFind(context, MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name)) != MAREA_NULLPTR) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: a built-in convert<T>(x) has one type argument and one argument and no name convert bound in the unit; its type is T.
static const MAREA_AstNode *MAREA_CALL MAREA_G_TypedAstConvertTarget(const MAREA_SemaContext *context, const MAREA_AstNode *callee, const MAREA_AstNode *node)
{
    if ((MAREA_G_TypedAstTextIs(MAREA_AstNodeGetText(callee), MAREA_AstNodeGetTextLength(callee), "convert") == MAREA_BOOLEAN_FALSE) || (MAREA_G_TypedAstNameIsBound(context, callee) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_NULLPTR;
    }

    const MAREA_AstNode *typeArguments = MAREA_SemaChildAt(node, 1);

    const MAREA_AstNode *arguments = MAREA_SemaChildAt(node, 2);

    if ((typeArguments == MAREA_NULLPTR) || (arguments == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(typeArguments) != MAREA_AST_KIND_TYPE_ARGUMENTS) || (MAREA_AstNodeGetKind(arguments) != MAREA_AST_KIND_ARGUMENT_LIST) || (MAREA_AstNodeGetChildCount(typeArguments) != 1) || (MAREA_AstNodeGetChildCount(arguments) != 1)) {
        return MAREA_NULLPTR;
    }

    const MAREA_AstNode *target = MAREA_SemaChildAt(typeArguments, 0);

    return (MAREA_G_TypedAstIsType(target) == MAREA_BOOLEAN_TRUE) ? target : MAREA_NULLPTR;
}

// INFO: a result type that is one generic parameter of the callee is replaced by the explicit type argument at the same place; any other shape, and a call without type arguments, keeps the raw type.
static const MAREA_AstNode *MAREA_CALL MAREA_G_TypedAstSubstituteResult(const MAREA_AstNode *function, const MAREA_AstNode *returnType, const MAREA_AstNode *node)
{
    const MAREA_AstNode *parameters = MAREA_G_TypedAstFindChild(function, MAREA_AST_KIND_GENERIC_PARAMETERS);

    const MAREA_AstNode *typeArguments = MAREA_G_TypedAstFindChild(node, MAREA_AST_KIND_TYPE_ARGUMENTS);

    const MAREA_Character8 *name = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    if ((parameters == MAREA_NULLPTR) || (typeArguments == MAREA_NULLPTR) || (MAREA_G_TypedAstSingleName(returnType, &name, &length) == MAREA_BOOLEAN_FALSE) || (MAREA_G_TypedAstFindChild(returnType, MAREA_AST_KIND_TYPE_ARGUMENTS) != MAREA_NULLPTR)) {
        return returnType;
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(parameters);

    for (MAREA_IntegerSize index = 0; ((index < COUNT) && (index < MAREA_AstNodeGetChildCount(typeArguments))); ++index) {
        const MAREA_AstNode *parameter = MAREA_SemaChildAt(parameters, index);

        const MAREA_AstNode *identifier = (parameter == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_SemaChildAt(parameter, 0);

        const MAREA_AstNode *argument = MAREA_SemaChildAt(typeArguments, index);

        if ((identifier != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(identifier) == MAREA_AST_KIND_IDENTIFIER) && (MAREA_G_TypedAstIsType(argument) == MAREA_BOOLEAN_TRUE) && (MAREA_SemaTextEquals(MAREA_AstNodeGetText(identifier), MAREA_AstNodeGetTextLength(identifier), name, length) == MAREA_BOOLEAN_TRUE)) {
            return argument;
        }
    }

    return returnType;
}

static void MAREA_CALL MAREA_G_TypedAstClassifyCall(MAREA_SemaContext *context, MAREA_SemaTypedNode *built, const MAREA_AstNode *node)
{
    const MAREA_AstNode *callee = MAREA_SemaChildAt(node, 0);

    if ((callee == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(callee) != MAREA_AST_KIND_IDENTIFIER)) {
        return;
    }

    const MAREA_AstNode *convertTarget = MAREA_G_TypedAstConvertTarget(context, callee, node);

    if (convertTarget != MAREA_NULLPTR) {
        MAREA_G_TypedAstApplyType(context, built, convertTarget);

        return;
    }

    const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, MAREA_AstNodeGetText(callee), MAREA_AstNodeGetTextLength(callee));

    if ((entry == MAREA_NULLPTR) || (entry->kind != MAREA_SEMA_NAME_KIND_DECLARATION) || (entry->declaration == MAREA_NULLPTR)) {
        return;
    }

    const MAREA_AstKind DECLARATION_KIND = MAREA_AstNodeGetKind(entry->declaration);

    // INFO: a call whose callee names a structure or an enum is construction: the typed node carries the
    // constructed type itself, never the construct function that checks its arguments.
    if ((DECLARATION_KIND == MAREA_AST_KIND_STRUCTURE_DECLARATION) || (DECLARATION_KIND == MAREA_AST_KIND_ENUM_DECLARATION)) {
        built->typeClass = MAREA_SEMA_TYPE_CLASS_USER;
        built->typeDeclaration = entry->declaration;

        return;
    }

    if (DECLARATION_KIND != MAREA_AST_KIND_FUNCTION_DECLARATION) {
        return;
    }

    const MAREA_AstNode *returnType = MAREA_SemaReturnType(entry->declaration);

    if (returnType == MAREA_NULLPTR) {
        return;
    }

    // INFO: an explicit type argument replaces a bare generic result and is read in the scope of the caller.
    const MAREA_AstNode *substituted = MAREA_G_TypedAstSubstituteResult(entry->declaration, returnType, node);

    if (substituted != returnType) {
        MAREA_G_TypedAstApplyType(context, built, substituted);

        return;
    }

    MAREA_IntegerSize savedGenericCount = 0;

    if (MAREA_G_TypedAstEnterFunctionGenerics(context, entry->declaration, &savedGenericCount) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
        return;
    }

    MAREA_G_TypedAstApplyType(context, built, returnType);

    MAREA_G_TypedAstLeaveFunctionGenerics(context, savedGenericCount);
}

static void MAREA_CALL MAREA_G_TypedAstClassify(MAREA_SemaContext *context, MAREA_SemaTypedNode *built)
{
    const MAREA_AstKind KIND = MAREA_AstNodeGetKind(built->astNode);

    if ((KIND == MAREA_AST_KIND_STRUCTURE_DECLARATION) || (KIND == MAREA_AST_KIND_ENUM_DECLARATION) || (KIND == MAREA_AST_KIND_OPAQUE_DECLARATION)) {
        built->typeClass = MAREA_SEMA_TYPE_CLASS_USER;
        built->typeDeclaration = built->astNode;

        return;
    }

    if (KIND == MAREA_AST_KIND_CALL_EXPRESSION) {
        MAREA_G_TypedAstClassifyCall(context, built, built->astNode);

        return;
    }

    const MAREA_AstNode *type = MAREA_G_InferTypedResultType(context, built->astNode);

    if (type == MAREA_NULLPTR) {
        return;
    }

    const MAREA_AstNode *declaration = MAREA_NULLPTR;

    const MAREA_Character8 *name = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    built->typeClass = MAREA_SemaClassifyType(context, type, &declaration, &name, &length);
    built->typeDeclaration = declaration;
    built->typeName = name;
    built->typeNameLength = length;
}

static MAREA_SemaTypedAstExcept MAREA_CALL MAREA_G_TypedAstBuildNode(MAREA_SemaContext *context, const MAREA_AstNode *node, const MAREA_AstNode *parent, MAREA_IntegerSize position, MAREA_SemaTypedNode **outNode)
{
    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(context->arena, sizeof(MAREA_SemaTypedNode), MAREA_ALIGNOF(MAREA_SemaTypedNode), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_SemaTypedNode *built = memory;

    built->astNode = node;
    built->typeDeclaration = MAREA_NULLPTR;
    built->layout = MAREA_NULLPTR;
    built->typeName = MAREA_NULLPTR;
    built->resolvedType = MAREA_NULLPTR;
    built->backend = MAREA_NULLPTR;
    built->backendLength = 0;
    built->exprType = (MAREA_SemaExprType){ .node = MAREA_NULLPTR, .declaration = MAREA_NULLPTR, .arguments = MAREA_NULLPTR, .frame = MAREA_NULLPTR, .fixed = MAREA_NULLPTR, .fixedLength = 0 };
    built->children = MAREA_NULLPTR;
    built->typeNameLength = 0;
    built->resolvedTypeLength = 0;
    built->childCount = MAREA_AstNodeGetChildCount(node);
    built->typeClass = MAREA_SEMA_TYPE_CLASS_PRIMITIVE;
    built->ownershipMode = MAREA_SEMA_OWNERSHIP_MODE_VALUE;
    built->isQualifier = MAREA_BOOLEAN_FALSE;
    built->isExternalCall = MAREA_BOOLEAN_FALSE;
    built->includedFile = MAREA_NULLPTR;

    // INFO: the checker's scope stack is replayed read-only, matching the first sema visit.
    MAREA_SemaScopeMark mark = { 0 };

    if (MAREA_SemaScopeEnter(context, node, &mark) == MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_G_TypedAstClassify(context, built);

    if (MAREA_SemaTypedAstResolveDeclaredType(context, built) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
        MAREA_SemaScopeLeave(context, mark);

        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_SemaTypedAstResolveLiteralDefault(built);

    const MAREA_AstKind NODE_KIND = MAREA_AstNodeGetKind(node);

    // INFO: a function carries its resolved backend, so the ir never reads the attributes or the target.
    if ((NODE_KIND == MAREA_AST_KIND_FUNCTION_DECLARATION) && (MAREA_SemaResolveBackend(context, node, &built->backend, &built->backendLength) != MAREA_SEMA_EXCEPT_SUCCESS)) {
        MAREA_SemaScopeLeave(context, mark);

        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    // INFO: a structure or an enum carries its layout, measured with the generic parameters of the declaration in scope.
    if (((NODE_KIND == MAREA_AST_KIND_STRUCTURE_DECLARATION) || (NODE_KIND == MAREA_AST_KIND_ENUM_DECLARATION)) && (MAREA_SemaMeasureDeclaration(context, node, &built->layout) != MAREA_SEMA_EXCEPT_SUCCESS)) {
        MAREA_SemaScopeLeave(context, mark);

        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    built->ownershipMode = MAREA_SemaTypedAstClassifyOwnership(context, built);

    MAREA_SemaTypedAstExcept result = MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;

    if (built->childCount > 0) {
        MAREA_Null childrenMemory = MAREA_NULLPTR;

        if (MAREA_ArenaAllocate(context->arena, (built->childCount * sizeof(MAREA_SemaTypedNode *)), MAREA_ALIGNOF(MAREA_SemaTypedNode *), &childrenMemory) != MAREA_ARENA_EXCEPT_SUCCESS) {
            MAREA_SemaScopeLeave(context, mark);

            return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_OUT_OF_MEMORY;
        }

        built->children = childrenMemory;

        for (MAREA_IntegerSize index = 0; ((index < built->childCount) && (result == MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS)); ++index) {
            const MAREA_AstNode *child = MAREA_NULLPTR;

            if (MAREA_AstNodeGetChildAt(node, index, &child) != MAREA_AST_EXCEPT_SUCCESS) {
                built->children[index] = MAREA_NULLPTR;

                continue;
            }

            MAREA_SemaTypedNode *typedChild = MAREA_NULLPTR;

            if (MAREA_SemaIsBinderHeader(node, index) == MAREA_BOOLEAN_TRUE) {
                const MAREA_SemaBinderWindow WINDOW = MAREA_SemaHideBinders(context, mark);

                result = MAREA_G_TypedAstBuildNode(context, child, node, index, &typedChild);

                MAREA_SemaShowBinders(context, WINDOW);
            } else {
                result = MAREA_G_TypedAstBuildNode(context, child, node, index, &typedChild);
            }

            built->children[index] = typedChild;

            // INFO: the name of a for, for each, partition or with statement is typed as soon as the header children that decide its type are built.
            if (result == MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
                result = MAREA_SemaTypedAstTypeScopeNames(context, built, index);
            }
        }
    }

    if (result == MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
        MAREA_SemaTypedAstResolveLiteralContext(context, built);

        // INFO: the context may change the type of a literal below, so the operations above it are typed again before this node is.
        result = MAREA_SemaTypedAstRetypeOperations(context, built);
    }

    // INFO: an expression is typed after its children, because its type is made from theirs, and before its scope ends.
    if (result == MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
        result = MAREA_SemaTypedAstResolveExpressionType(context, built, parent, position);
    }

    MAREA_SemaScopeLeave(context, mark);

    // INFO: a local is recorded after the scope of its own node ends, mirroring MAREA_SemaVisit.
    if ((result == MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) && (MAREA_AstNodeGetKind(node) == MAREA_AST_KIND_LOCAL_DECLARATION) && (MAREA_SemaDeclareLocal(context, node) == MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY)) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    if (result != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
        return result;
    }

    *outNode = built;

    return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
}

MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaBuildTypedAst(MAREA_SemaContext *context, const MAREA_AstNode *root, const MAREA_SemaTypedNode **outRoot)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    if (root == MAREA_NULLPTR) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outRoot == MAREA_NULLPTR) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    if (MAREA_G_TypedAstWasChecked(context, root) == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NOT_CHECKED;
    }

    MAREA_SemaTypedNode *built = MAREA_NULLPTR;

    const MAREA_SemaTypedAstExcept RESULT = MAREA_G_TypedAstBuildNode(context, root, MAREA_NULLPTR, 0, &built);

    if (RESULT != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
        return RESULT;
    }

    *outRoot = built;

    return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
}

const MAREA_AstNode *MAREA_CALL MAREA_SemaTypedNodeGetAstNode(const MAREA_SemaTypedNode *node)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    return node->astNode;
}

MAREA_IntegerSize MAREA_CALL MAREA_SemaTypedNodeGetChildCount(const MAREA_SemaTypedNode *node)
{
    if (node == MAREA_NULLPTR) {
        return 0;
    }

    return node->childCount;
}

MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypedNodeGetChildAt(const MAREA_SemaTypedNode *node, MAREA_IntegerSize index, const MAREA_SemaTypedNode **out)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    if (out == MAREA_NULLPTR) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    if (index >= node->childCount) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    *out = node->children[index];

    return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
}

MAREA_SemaTypeClass MAREA_CALL MAREA_SemaTypedNodeGetTypeClass(const MAREA_SemaTypedNode *node)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_SEMA_TYPE_CLASS_PRIMITIVE;
    }

    return node->typeClass;
}

MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypedNodeGetTypeDeclaration(const MAREA_SemaTypedNode *node, const MAREA_AstNode **out)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    if (out == MAREA_NULLPTR) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    *out = node->typeDeclaration;

    return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
}

MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypedNodeGetTypeName(const MAREA_SemaTypedNode *node, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outText == MAREA_NULLPTR) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outLength == MAREA_NULLPTR) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    *outText = node->typeName;
    *outLength = node->typeNameLength;

    return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
}

MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypedNodeGetResolvedType(const MAREA_SemaTypedNode *node, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outText == MAREA_NULLPTR) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outLength == MAREA_NULLPTR) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    *outText = node->resolvedType;
    *outLength = node->resolvedTypeLength;

    return (node->resolvedType == MAREA_NULLPTR) ? MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NO_TYPE : MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
}

MAREA_SemaOwnershipMode MAREA_CALL MAREA_SemaTypedNodeGetOwnershipMode(const MAREA_SemaTypedNode *node)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_SEMA_OWNERSHIP_MODE_VALUE;
    }

    return node->ownershipMode;
}

MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypedNodeGetBackend(const MAREA_SemaTypedNode *node, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength)
{
    if ((node == MAREA_NULLPTR) || (outText == MAREA_NULLPTR) || (outLength == MAREA_NULLPTR)) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    *outText = node->backend;
    *outLength = node->backendLength;

    return (node->backend == MAREA_NULLPTR) ?
MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NO_BACKEND : MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
}

const MAREA_SemaIncludedFile *MAREA_CALL MAREA_SemaTypedNodeGetIncludedFile(const MAREA_SemaTypedNode *node)
{
    return (node == MAREA_NULLPTR) ? MAREA_NULLPTR : node->includedFile;
}
