// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <string.h>

MAREA_SemaExcept MAREA_CALL MAREA_SemaContextCreate(MAREA_Arena *arena, MAREA_DiagnosticSink *sink, MAREA_SemaImportResolver *resolver, MAREA_SemaContext **outContext)
{
    if (arena == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    if (sink == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    if (resolver == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outContext == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(arena, sizeof(MAREA_SemaContext), MAREA_ALIGNOF(MAREA_SemaContext), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_SemaContext *context = memory;

    context->arena = arena;
    context->sink = sink;
    context->resolver = resolver;
    context->names = (MAREA_SemaNameTable){ 0 };
    context->generics = (MAREA_SemaNameTable){ 0 };
    context->locals = (MAREA_SemaNameTable){ 0 };
    context->references = (MAREA_SemaNameTable){ 0 };
    context->localStart = 0;
    context->functionLocalStart = 0;
    context->root = MAREA_NULLPTR;
    context->function = MAREA_NULLPTR;
    context->implementation = MAREA_NULLPTR;
    context->selfDepth = 0;
    context->instantiationDepth = 0;
    context->forkDepth = 0;
    context->partitionDepth = 0;
    context->loopDepth = 0;
    context->unsafeDepth = 0;

    *outContext = context;

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

MAREA_Arena *MAREA_CALL MAREA_SemaContextGetArena(const MAREA_SemaContext *context)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    return context->arena;
}

MAREA_DiagnosticSink *MAREA_CALL MAREA_SemaContextGetDiagnostics(const MAREA_SemaContext *context)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    return context->sink;
}

MAREA_SemaImportResolver *MAREA_CALL MAREA_SemaContextGetImportResolver(const MAREA_SemaContext *context)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    return context->resolver;
}

void MAREA_CALL MAREA_SemaReport(MAREA_SemaContext *context, MAREA_SemaDiagnosticCode code, MAREA_SourcePosition position, const MAREA_Character8 *message)
{
    MAREA_IGNORE(MAREA_DiagnosticSinkAdd(context->sink, (MAREA_Uinteger32)code, MAREA_DIAGNOSTIC_SEVERITY_ERROR, position, message));
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaReportNamed(MAREA_SemaContext *context, MAREA_SemaDiagnosticCode code, MAREA_SourcePosition position, const MAREA_Character8 *prefix, const MAREA_Character8 *name, MAREA_IntegerSize nameLength)
{
    const MAREA_IntegerSize prefixLength = strlen(prefix);

    // ERROR: a total that does not fit in MAREA_IntegerSize would allocate a small block and then copy past it.
    if (nameLength > (MAREA_INTEGER_SIZE_MAXIMUM - prefixLength - MAREA_INTEGER_SIZE(4))) {
        return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(context->arena, (prefixLength + nameLength + MAREA_INTEGER_SIZE(4)), MAREA_ALIGNOF(MAREA_Character8), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_Character8 *message = memory;

    memcpy(message, prefix, prefixLength);
    message[prefixLength] = ' ';
    message[(prefixLength + MAREA_INTEGER_SIZE(1))] = '\'';
    memcpy((message + prefixLength + MAREA_INTEGER_SIZE(2)), name, nameLength);
    message[(prefixLength + nameLength + MAREA_INTEGER_SIZE(2))] = '\'';
    message[(prefixLength + nameLength + MAREA_INTEGER_SIZE(3))] = '\0';

    MAREA_SemaReport(context, code, position, message);

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaVisit(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    // INFO: a node the parser could not build is skipped whole, so no check reads a broken shape.
    if (MAREA_AstNodeGetKind(node) == MAREA_AST_KIND_ERROR_NODE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_SemaExcept attributesResult = MAREA_SemaCheckAttributes(context, node);

    if (attributesResult != MAREA_SEMA_EXCEPT_SUCCESS) {
        return attributesResult;
    }

    MAREA_SemaScopeMark mark = { 0 };

    const MAREA_SemaExcept scopeResult = MAREA_SemaScopeEnter(context, node, &mark);

    if (scopeResult != MAREA_SEMA_EXCEPT_SUCCESS) {
        return scopeResult;
    }

    MAREA_SemaExcept result = MAREA_SEMA_EXCEPT_SUCCESS;

    const MAREA_AstKind kind = MAREA_AstNodeGetKind(node);

    // INFO: a fork_join block is a jump barrier too, the same as a partition body, so a loop outside it is not a target of a break or continue that stands directly in the block, and MAREA_SemaScopeLeave restores the saved mark once this block ends.
    if (kind == MAREA_AST_KIND_FORK_JOIN_BLOCK) {
        context->loopDepth = 0;
    }

    MAREA_Boolean entered = MAREA_BOOLEAN_FALSE;

    if (result == MAREA_SEMA_EXCEPT_SUCCESS) {
        result = MAREA_SemaCheckDuplicateNames(context, node);
    }

    if (result == MAREA_SEMA_EXCEPT_SUCCESS) {
        result = MAREA_SemaCheckOwnership(context, node);
    }

    if (result == MAREA_SEMA_EXCEPT_SUCCESS) {
        result = MAREA_SemaCheckLifecycle(context, node);
    }

    if (result == MAREA_SEMA_EXCEPT_SUCCESS) {
        result = MAREA_SemaCheckControlFlow(context, node);
    }

    if (result == MAREA_SEMA_EXCEPT_SUCCESS) {
        result = MAREA_SemaCheckExpressions(context, node);
    }

    if (result != MAREA_SEMA_EXCEPT_SUCCESS) {
        // INFO: an out-of-memory result from the ownership check ends the visit of this node.
    } else if (kind == MAREA_AST_KIND_NAMED_TYPE) {
        result = MAREA_SemaCheckNamedType(context, node);

        if (result == MAREA_SEMA_EXCEPT_SUCCESS) {
            result = MAREA_SemaInstantiationEnter(context, node, &entered);
        }
    } else if (kind == MAREA_AST_KIND_WHERE_CLAUSE) {
        result = MAREA_SemaCheckWhereClause(context, node);
    } else if (kind == MAREA_AST_KIND_CALL_EXPRESSION) {
        result = MAREA_SemaCheckLifecycleCall(context, node);

        if (result == MAREA_SEMA_EXCEPT_SUCCESS) {
            result = MAREA_SemaCheckCall(context, node);
        }
    } else if (kind == MAREA_AST_KIND_MACRO_CALL) {
        result = MAREA_SemaCheckMacro(context, node);
    } else if ((kind == MAREA_AST_KIND_UNARY_EXPRESSION) && (MAREA_AstNodeGetOperator(node) == MAREA_TOKEN_KIND_KEYWORD_SPAWN)) {
        result = MAREA_SemaCheckSpawn(context, node);
    } else if ((kind == MAREA_AST_KIND_BREAK_STATEMENT) || (kind == MAREA_AST_KIND_CONTINUE_STATEMENT) || (kind == MAREA_AST_KIND_RETURN_STATEMENT)) {
        result = MAREA_SemaCheckJump(context, node);
    }

    const MAREA_IntegerSize count = MAREA_AstNodeGetChildCount(node);

    for (MAREA_IntegerSize index = 0; ((index < count) && (result == MAREA_SEMA_EXCEPT_SUCCESS)); ++index) {
        const MAREA_AstNode *child = MAREA_NULLPTR;

        if (MAREA_AstNodeGetChildAt(node, index, &child) != MAREA_AST_EXCEPT_SUCCESS) {
            continue;
        }

        result = MAREA_SemaVisit(context, child);
    }

    MAREA_SemaInstantiationLeave(context, entered);

    MAREA_SemaScopeLeave(context, mark);

    // INFO: a local lives in the block that holds it, so it is recorded after the scope of its own node ends.
    if ((result == MAREA_SEMA_EXCEPT_SUCCESS) && (kind == MAREA_AST_KIND_LOCAL_DECLARATION)) {
        result = MAREA_SemaDeclareLocal(context, node);
    }

    return result;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckCompilationUnit(MAREA_SemaContext *context, const MAREA_AstNode *root)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    if (root == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    context->names = (MAREA_SemaNameTable){ 0 };
    context->generics = (MAREA_SemaNameTable){ 0 };
    context->locals = (MAREA_SemaNameTable){ 0 };
    context->references = (MAREA_SemaNameTable){ 0 };
    context->root = root;
    context->function = MAREA_NULLPTR;
    context->implementation = MAREA_NULLPTR;
    context->selfDepth = 0;
    context->instantiationDepth = 0;
    context->forkDepth = 0;
    context->partitionDepth = 0;
    context->loopDepth = 0;
    context->unsafeDepth = 0;

    const MAREA_SemaExcept namesResult = MAREA_SemaCheckNamespaceAndNames(context, root);

    if (namesResult != MAREA_SEMA_EXCEPT_SUCCESS) {
        return namesResult;
    }

    const MAREA_SemaExcept declarationsResult = MAREA_SemaCheckDeclarations(context, root);

    if (declarationsResult != MAREA_SEMA_EXCEPT_SUCCESS) {
        return declarationsResult;
    }

    const MAREA_SemaExcept cyclesResult = MAREA_SemaCheckConstantCycles(context, root);

    if (cyclesResult != MAREA_SEMA_EXCEPT_SUCCESS) {
        return cyclesResult;
    }

    return MAREA_SemaVisit(context, root);
}
