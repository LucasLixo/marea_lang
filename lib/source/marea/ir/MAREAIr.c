// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/ir/MAREAIrInternal.h"

#include <string.h>

#if !defined(MAREA_IR_MESSAGE_CAPACITY)
    #define MAREA_IR_MESSAGE_CAPACITY MAREA_INTEGER_SIZE(160)
#endif

MAREA_IrExcept MAREA_CALL MAREA_IrContextCreate(MAREA_Arena *arena, MAREA_DiagnosticSink *sink, MAREA_IrContext **outContext)
{
    if (arena == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if (sink == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outContext == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(arena, sizeof(MAREA_IrContext), MAREA_ALIGNOF(MAREA_IrContext), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_IR_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_IrContext *context = memory;

    context->arena = arena;
    context->sink = sink;
    context->unit = MAREA_NULLPTR;
    context->function = MAREA_NULLPTR;
    context->aliases = MAREA_NULLPTR;
    context->aliasCount = MAREA_INTEGER_SIZE(0);
    context->loopCount = MAREA_INTEGER_SIZE(0);
    context->switchCount = MAREA_INTEGER_SIZE(0);
    context->returnCount = MAREA_INTEGER_SIZE(0);

    *outContext = context;

    return MAREA_IR_EXCEPT_SUCCESS;
}

MAREA_Arena *MAREA_CALL MAREA_IrContextGetArena(const MAREA_IrContext *context)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    return context->arena;
}

MAREA_DiagnosticSink *MAREA_CALL MAREA_IrContextGetDiagnostics(const MAREA_IrContext *context)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    return context->sink;
}

static MAREA_IrExcept MAREA_CALL MAREA_G_RaiseAtPosition(MAREA_IrContext *context, MAREA_IrDiagnosticCode code, MAREA_SourcePosition position, const MAREA_Character8 *message, const MAREA_Character8 *name, MAREA_IntegerSize nameLength)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if (message == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    MAREA_Character8 buffer[MAREA_IR_MESSAGE_CAPACITY];

    MAREA_IntegerSize length = strlen(message);

    if (length > (MAREA_IR_MESSAGE_CAPACITY - MAREA_INTEGER_SIZE(1))) {
        length = (MAREA_IR_MESSAGE_CAPACITY - MAREA_INTEGER_SIZE(1));
    }

    memcpy(buffer, message, length);

    // INFO: the name is added as 'name' and cut when it does not fit; the message always ends with a NUL.
    if ((name != MAREA_NULLPTR) && ((length + MAREA_INTEGER_SIZE(4)) < MAREA_IR_MESSAGE_CAPACITY)) {
        MAREA_IntegerSize room = (MAREA_IR_MESSAGE_CAPACITY - length - MAREA_INTEGER_SIZE(4));

        if (nameLength < room) {
            room = nameLength;
        }

        buffer[length] = ' ';
        buffer[(length + MAREA_INTEGER_SIZE(1))] = '\'';
        memcpy((buffer + length + MAREA_INTEGER_SIZE(2)), name, room);
        buffer[(length + MAREA_INTEGER_SIZE(2) + room)] = '\'';
        length = (length + MAREA_INTEGER_SIZE(3) + room);
    }

    buffer[length] = '\0';

    if (MAREA_DiagnosticSinkAdd(context->sink, (MAREA_Uinteger32)code, MAREA_DIAGNOSTIC_SEVERITY_ERROR, position, buffer) != MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) {
        return MAREA_IR_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    return MAREA_IR_EXCEPT_SUCCESS;
}

MAREA_IrExcept MAREA_CALL MAREA_IrRaise(MAREA_IrContext *context, MAREA_IrDiagnosticCode code, const MAREA_AstNode *at, const MAREA_Character8 *message, const MAREA_Character8 *name, MAREA_IntegerSize nameLength)
{
    return MAREA_G_RaiseAtPosition(context, code, MAREA_AstNodeGetPosition(at), message, name, nameLength);
}

// INFO: the name of a function that lowering left out of the module. The text points into the source.
typedef struct MAREA_G_DroppedName
{
    const MAREA_Character8 *text;
    MAREA_IntegerSize length;
} MAREA_G_DroppedName;

static MAREA_Boolean MAREA_CALL MAREA_G_IsDropped(const MAREA_G_DroppedName *dropped, MAREA_IntegerSize count, const MAREA_IrNode *name)
{
    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < count; index = (index + MAREA_INTEGER_SIZE(1))) {
        if ((dropped[index].length == (MAREA_IntegerSize)name->textLength) && (memcmp(dropped[index].text, name->text, dropped[index].length) == 0)) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

// INFO: raises one diagnostic for each call in the subtree whose callee names a dropped function. A call is a CALL_EXPRESSION whose first child is a bare IDENTIFIER.
static MAREA_IrExcept MAREA_CALL MAREA_G_CheckCalls(MAREA_IrContext *context, const MAREA_IrNode *node, const MAREA_G_DroppedName *dropped, MAREA_IntegerSize count, MAREA_Boolean *outFound)
{
    if ((node->kind == MAREA_IR_KIND_CALL_EXPRESSION) && (node->childCount > MAREA_INTEGER_SIZE(0)) && (node->children[0]->kind == MAREA_IR_KIND_IDENTIFIER) && (MAREA_G_IsDropped(dropped, count, node->children[0]) == MAREA_BOOLEAN_TRUE)) {
        const MAREA_IrNode *callee = node->children[0];

        const MAREA_IrExcept raised = MAREA_G_RaiseAtPosition(context, MAREA_IR_DIAGNOSTIC_CODE_UNINSTANTIATED_GENERIC, node->position, "A call to a function that was left out of the module is not supported by the IR:", callee->text, callee->textLength);

        if (raised != MAREA_IR_EXCEPT_SUCCESS) {
            return raised;
        }

        *outFound = MAREA_BOOLEAN_TRUE;
    }

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < node->childCount; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_IrExcept result = MAREA_G_CheckCalls(context, node->children[index], dropped, count, outFound);

        if (result != MAREA_IR_EXCEPT_SUCCESS) {
            return result;
        }
    }

    return MAREA_IR_EXCEPT_SUCCESS;
}

// INFO: a call to a dropped function would lower to a name that points at nothing. Each such call raises, and its top-level declaration is dropped whole; when that declaration is a function, its own callers are checked in turn.
static MAREA_IrExcept MAREA_CALL MAREA_G_DropDanglingCalls(MAREA_IrContext *context, MAREA_IrNode *module, MAREA_G_DroppedName *dropped, MAREA_IntegerSize droppedCount, MAREA_IntegerSize droppedCapacity)
{
    MAREA_Boolean changed = (droppedCount > MAREA_INTEGER_SIZE(0)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    while (changed == MAREA_BOOLEAN_TRUE) {
        changed = MAREA_BOOLEAN_FALSE;

        MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0);

        while (index < module->childCount) {
            MAREA_IrNode *child = module->children[index];

            MAREA_Boolean found = MAREA_BOOLEAN_FALSE;

            const MAREA_IrExcept checked = MAREA_G_CheckCalls(context, child, dropped, droppedCount, &found);

            if (checked != MAREA_IR_EXCEPT_SUCCESS) {
                return checked;
            }

            if (found == MAREA_BOOLEAN_FALSE) {
                index = (index + MAREA_INTEGER_SIZE(1));

                continue;
            }

            // INFO: a member of an implementation is named Type.name, so a bare callee never targets it.
            if ((child->kind == MAREA_IR_KIND_FUNCTION_DECLARATION) && (memchr(child->text, '.', child->textLength) == MAREA_NULLPTR) && (droppedCount < droppedCapacity)) {
                dropped[droppedCount].text = child->text;
                dropped[droppedCount].length = (MAREA_IntegerSize)child->textLength;
                droppedCount = (droppedCount + MAREA_INTEGER_SIZE(1));
            }

            for (MAREA_IntegerSize shift = index; (shift + MAREA_INTEGER_SIZE(1)) < module->childCount; shift = (shift + MAREA_INTEGER_SIZE(1))) {
                module->children[shift] = module->children[(shift + MAREA_INTEGER_SIZE(1))];
            }

            module->childCount = (module->childCount - MAREA_INTEGER_SIZE(1));

            changed = MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_IR_EXCEPT_SUCCESS;
}

static MAREA_IrExcept MAREA_CALL MAREA_G_LowerUnitDeclarations(MAREA_IrContext *context, const MAREA_SemaTypedNode *typedRoot, const MAREA_IrNode **outModule)
{
    const MAREA_AstNode *astRoot = MAREA_SemaTypedNodeGetAstNode(typedRoot);

    MAREA_IrNode *module = MAREA_NULLPTR;

    const MAREA_IrExcept created = MAREA_IrNodeCreate(context, MAREA_IR_KIND_MODULE, astRoot, &module);

    if (created != MAREA_IR_EXCEPT_SUCCESS) {
        return created;
    }

    // INFO: the module carries the full namespace of the unit, so the name of each of its top-level declarations is the namespace and the name. The text points into the source.
    const MAREA_SemaTypedNode *namespaceItem = MAREA_IrTypedFindChild(typedRoot, MAREA_AST_KIND_NAMESPACE_DECLARATION);

    const MAREA_SemaTypedNode *namespaceName = (namespaceItem == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_IrTypedFindChild(namespaceItem, MAREA_AST_KIND_QUALIFIED_NAME);

    if (namespaceName != MAREA_NULLPTR) {
        const MAREA_Character8 *namespaceText = MAREA_NULLPTR;

        MAREA_IntegerSize namespaceLength = MAREA_INTEGER_SIZE(0);

        if (MAREA_IrAstQualifiedNameSpan(MAREA_SemaTypedNodeGetAstNode(namespaceName), &namespaceText, &namespaceLength) == MAREA_BOOLEAN_TRUE) {
            module->text = namespaceText;
            module->textLength = (MAREA_Uinteger32)namespaceLength;
        }
    }

    const MAREA_IntegerSize COUNT = MAREA_SemaTypedNodeGetChildCount(typedRoot);

    // INFO: room for one name per top-level item, so the list of dropped functions never grows.
    MAREA_G_DroppedName *dropped = MAREA_NULLPTR;

    MAREA_IntegerSize droppedCount = MAREA_INTEGER_SIZE(0);

    if (COUNT > MAREA_INTEGER_SIZE(0)) {
        MAREA_Null memory = MAREA_NULLPTR;

        if (MAREA_ArenaAllocate(context->arena, (COUNT * sizeof(MAREA_G_DroppedName)), MAREA_ALIGNOF(MAREA_G_DroppedName), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
            return MAREA_IR_EXCEPT_ERROR_OUT_OF_MEMORY;
        }

        dropped = memory;
    }

    // INFO: room for one source per top-level item, for the layout table that is built once the declarations are final.
    MAREA_IrLayoutSource *sources = MAREA_NULLPTR;

    MAREA_IntegerSize sourceCount = MAREA_INTEGER_SIZE(0);

    if (COUNT > MAREA_INTEGER_SIZE(0)) {
        MAREA_Null memory = MAREA_NULLPTR;

        if (MAREA_ArenaAllocate(context->arena, (COUNT * sizeof(MAREA_IrLayoutSource)), MAREA_ALIGNOF(MAREA_IrLayoutSource), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
            return MAREA_IR_EXCEPT_ERROR_OUT_OF_MEMORY;
        }

        sources = memory;
    }

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_SemaTypedNode *item = MAREA_IrTypedChildAt(typedRoot, index);

        const MAREA_IntegerSize BEFORE = module->childCount;

        const MAREA_IrExcept lowered = MAREA_IrLowerDeclaration(context, item, MAREA_NULLPTR, MAREA_INTEGER_SIZE(0), module);

        if (lowered != MAREA_IR_EXCEPT_SUCCESS) {
            return lowered;
        }

        if (module->childCount > BEFORE) {
            const MAREA_AstKind ITEM_KIND = MAREA_IrTypedKind(item);

            // INFO: a structure, an opaque structure and an enum each lower to one node, the last child, and get one entry of the layout table.
            if ((ITEM_KIND == MAREA_AST_KIND_STRUCTURE_DECLARATION) || (ITEM_KIND == MAREA_AST_KIND_OPAQUE_DECLARATION) || (ITEM_KIND == MAREA_AST_KIND_ENUM_DECLARATION)) {
                sources[sourceCount].typed = item;
                sources[sourceCount].node = module->children[(module->childCount - MAREA_INTEGER_SIZE(1))];
                sourceCount = (sourceCount + MAREA_INTEGER_SIZE(1));
            }
        }

        if ((module->childCount == BEFORE) && (MAREA_IrTypedKind(item) == MAREA_AST_KIND_FUNCTION_DECLARATION)) {
            const MAREA_AstNode *name = MAREA_SemaTypedNodeGetAstNode(MAREA_IrTypedFindChild(item, MAREA_AST_KIND_IDENTIFIER));

            if (name != MAREA_NULLPTR) {
                dropped[droppedCount].text = MAREA_AstNodeGetText(name);
                dropped[droppedCount].length = (MAREA_IntegerSize)MAREA_AstNodeGetTextLength(name);
                droppedCount = (droppedCount + MAREA_INTEGER_SIZE(1));
            }
        }
    }

    const MAREA_IrExcept swept = MAREA_G_DropDanglingCalls(context, module, dropped, droppedCount, COUNT);

    if (swept != MAREA_IR_EXCEPT_SUCCESS) {
        return swept;
    }

    const MAREA_IrExcept TABLE = MAREA_IrLayoutTableBuild(context, module, sources, sourceCount);

    if (TABLE != MAREA_IR_EXCEPT_SUCCESS) {
        return TABLE;
    }

    *outModule = module;

    return MAREA_IR_EXCEPT_SUCCESS;
}

MAREA_IrExcept MAREA_CALL MAREA_IrLowerCompilationUnit(MAREA_IrContext *context, const MAREA_SemaTypedNode *typedRoot, const MAREA_IrNode **outModule)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if (typedRoot == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outModule == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if (MAREA_AstNodeGetKind(MAREA_SemaTypedNodeGetAstNode(typedRoot)) != MAREA_AST_KIND_COMPILATION_UNIT) {
        return MAREA_IR_EXCEPT_ERROR_BAD_ROOT;
    }

    // INFO: the root and the alias table stay in the context only while the unit is lowered, so that a call can ask which names the unit binds and which names are aliases.
    context->unit = typedRoot;

    MAREA_IrExcept result = MAREA_IrAliasBuild(context, typedRoot);

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_G_LowerUnitDeclarations(context, typedRoot, outModule);
    }

    context->unit = MAREA_NULLPTR;
    context->function = MAREA_NULLPTR;
    context->aliases = MAREA_NULLPTR;
    context->aliasCount = MAREA_INTEGER_SIZE(0);

    return result;
}
