// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#define MAREA_G_CYCLE_DEPTH_LIMIT MAREA_UINTEGER_32(64)

// INFO: the visit state of one constant in the depth-first search.
typedef enum MAREA_G_CycleState
{
    MAREA_G_CYCLE_STATE_NEW = 0,
    MAREA_G_CYCLE_STATE_OPEN = 1,
    MAREA_G_CYCLE_STATE_DONE = 2,
} MAREA_G_CycleState;

// INFO: the file constants in file order, and the edges of each one in one flat array. A constant reads edgeCount[index] edges from edgeStart[index]. The count pass may count an edge twice, so the fill pass sets the real count.
typedef struct MAREA_G_CycleGraph
{
    const MAREA_AstNode **constants;
    MAREA_IntegerSize count;
    MAREA_IntegerSize *edgeStart;
    MAREA_IntegerSize *edgeCount;
    MAREA_IntegerSize *edges;
} MAREA_G_CycleGraph;

static MAREA_IntegerSize MAREA_CALL MAREA_G_ConstantIndexOf(const MAREA_G_CycleGraph *graph, const MAREA_AstNode *declaration)
{
    for (MAREA_IntegerSize index = 0; index < graph->count; ++index) {
        if (graph->constants[index] == declaration) {
            return index;
        }
    }

    return graph->count;
}

// INFO: when out is null the edges are only counted. A member name, a field key, a type and a call target are not followed. An edge to the same constant is kept once.
static MAREA_IntegerSize MAREA_CALL MAREA_G_CollectEdges(const MAREA_SemaContext *context, const MAREA_G_CycleGraph *graph, const MAREA_AstNode *expression, MAREA_Uinteger32 depth, MAREA_IntegerSize *out, MAREA_IntegerSize found, MAREA_IntegerSize base)
{
    if ((expression == MAREA_NULLPTR) || (depth >= MAREA_G_CYCLE_DEPTH_LIMIT)) {
        return found;
    }

    MAREA_IntegerSize first = 0;

    switch (MAREA_AstNodeGetKind(expression)) {
    case MAREA_AST_KIND_IDENTIFIER:
    {
        const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, MAREA_AstNodeGetText(expression), MAREA_AstNodeGetTextLength(expression));

        if ((entry == MAREA_NULLPTR) || (entry->kind != MAREA_SEMA_NAME_KIND_DECLARATION) || (entry->declaration == MAREA_NULLPTR)) {
            return found;
        }

        const MAREA_IntegerSize TARGET = MAREA_G_ConstantIndexOf(graph, entry->declaration);

        if (TARGET >= graph->count) {
            return found;
        }

        for (MAREA_IntegerSize seen = 0; seen < found; ++seen) {
            if ((out != MAREA_NULLPTR) && (out[base + seen] == TARGET)) {
                return found;
            }
        }

        if (out != MAREA_NULLPTR) {
            out[base + found] = TARGET;
        }

        return (found + 1);
    }
    case MAREA_AST_KIND_CALL_EXPRESSION:
    case MAREA_AST_KIND_LITERAL_VALUE:
    case MAREA_AST_KIND_FIELD_ENTRY:
        first = 1;
        break;
    case MAREA_AST_KIND_BINARY_EXPRESSION:
    case MAREA_AST_KIND_UNARY_EXPRESSION:
    case MAREA_AST_KIND_PARENTHESIZED_EXPRESSION:
    case MAREA_AST_KIND_INDEX_ACCESS:
    case MAREA_AST_KIND_ARGUMENT_LIST:
    case MAREA_AST_KIND_INDEX_ENTRY:
    case MAREA_AST_KIND_VALUE_ENTRY:
        break;
    case MAREA_AST_KIND_MEMBER_ACCESS:
        return MAREA_G_CollectEdges(context, graph, MAREA_SemaChildAt(expression, 0), (depth + 1), out, found, base);
    default:
        return found;
    }

    for (MAREA_IntegerSize index = first; index < MAREA_AstNodeGetChildCount(expression); ++index) {
        found = MAREA_G_CollectEdges(context, graph, MAREA_SemaChildAt(expression, index), (depth + 1), out, found, base);
    }

    return found;
}

static const MAREA_AstNode *MAREA_CALL MAREA_G_InitialValue(const MAREA_AstNode *declaration)
{
    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(declaration);

    return (COUNT > 1) ? MAREA_SemaChildAt(declaration, (COUNT - 1)) : MAREA_NULLPTR;
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_Allocate(MAREA_SemaContext *context, MAREA_IntegerSize count, MAREA_IntegerSize size, MAREA_IntegerSize alignment, void **outMemory)
{
    if ((count == 0) || (MAREA_ArenaAllocate(context->arena, (count * size), alignment, outMemory) != MAREA_ARENA_EXCEPT_SUCCESS)) {
        return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckConstantCycles(MAREA_SemaContext *context, const MAREA_AstNode *root)
{
    if ((context == MAREA_NULLPTR) || (root == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_IntegerSize CHILDREN = MAREA_AstNodeGetChildCount(root);

    MAREA_G_CycleGraph graph = { 0 };

    for (MAREA_IntegerSize index = 0; index < CHILDREN; ++index) {
        const MAREA_AstNode *child = MAREA_SemaChildAt(root, index);

        if ((child != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(child) == MAREA_AST_KIND_CONSTANT_DECLARATION)) {
            ++graph.count;
        }
    }

    if (graph.count == 0) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    void *memory = MAREA_NULLPTR;

    if (MAREA_G_Allocate(context, graph.count, sizeof(const MAREA_AstNode *), MAREA_ALIGNOF(const MAREA_AstNode *), &memory) != MAREA_SEMA_EXCEPT_SUCCESS) {
        return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    graph.constants = (const MAREA_AstNode **)memory;

    MAREA_IntegerSize filled = 0;

    for (MAREA_IntegerSize index = 0; index < CHILDREN; ++index) {
        const MAREA_AstNode *child = MAREA_SemaChildAt(root, index);

        if ((child != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(child) == MAREA_AST_KIND_CONSTANT_DECLARATION)) {
            graph.constants[filled++] = child;
        }
    }

    // INFO: per constant the state, the next edge to take and the start of its edges, plus one extra start entry as the end mark.
    if (MAREA_G_Allocate(context, ((graph.count * 5) + 1), sizeof(MAREA_IntegerSize), MAREA_ALIGNOF(MAREA_IntegerSize), &memory) != MAREA_SEMA_EXCEPT_SUCCESS) {
        return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_IntegerSize *table = (MAREA_IntegerSize *)memory;

    MAREA_IntegerSize *state = table;
    MAREA_IntegerSize *cursor = (table + graph.count);
    MAREA_IntegerSize *stack = (table + (graph.count * 2));

    graph.edgeStart = (table + (graph.count * 3));
    graph.edgeCount = (table + (graph.count * 4) + 1);

    for (MAREA_IntegerSize index = 0; index < ((graph.count * 5) + 1); ++index) {
        table[index] = 0;
    }

    // INFO: first pass counts, second pass fills.
    MAREA_IntegerSize total = 0;

    for (MAREA_IntegerSize index = 0; index < graph.count; ++index) {
        graph.edgeStart[index] = total;

        total += MAREA_G_CollectEdges(context, &graph, MAREA_G_InitialValue(graph.constants[index]), 0, MAREA_NULLPTR, 0, 0);
    }

    if (total > 0) {
        if (MAREA_G_Allocate(context, total, sizeof(MAREA_IntegerSize), MAREA_ALIGNOF(MAREA_IntegerSize), &memory) != MAREA_SEMA_EXCEPT_SUCCESS) {
            return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
        }

        graph.edges = (MAREA_IntegerSize *)memory;

        for (MAREA_IntegerSize index = 0; index < graph.count; ++index) {
            graph.edgeCount[index] = MAREA_G_CollectEdges(context, &graph, MAREA_G_InitialValue(graph.constants[index]), 0, graph.edges, 0, graph.edgeStart[index]);
        }
    }

    // INFO: an explicit stack replaces recursion, so the length of a chain has no limit.
    for (MAREA_IntegerSize start = 0; start < graph.count; ++start) {
        if (state[start] != MAREA_G_CYCLE_STATE_NEW) {
            continue;
        }

        MAREA_IntegerSize top = 0;

        stack[top++] = start;
        state[start] = MAREA_G_CYCLE_STATE_OPEN;

        while (top > 0) {
            const MAREA_IntegerSize CURRENT = stack[top - 1];

            if (cursor[CURRENT] >= graph.edgeCount[CURRENT]) {
                state[CURRENT] = MAREA_G_CYCLE_STATE_DONE;

                --top;

                continue;
            }

            const MAREA_IntegerSize NEXT = graph.edges[graph.edgeStart[CURRENT] + cursor[CURRENT]];

            ++cursor[CURRENT];

            if (state[NEXT] == MAREA_G_CYCLE_STATE_NEW) {
                state[NEXT] = MAREA_G_CYCLE_STATE_OPEN;

                stack[top++] = NEXT;

                continue;
            }

            if (state[NEXT] != MAREA_G_CYCLE_STATE_OPEN) {
                continue;
            }

            // INFO: the cycle is the stack from NEXT to the top. It is reported at the member that comes first in the file.
            MAREA_IntegerSize at = top;

            while (stack[at - 1] != NEXT) {
                --at;
            }

            MAREA_IntegerSize first = stack[at - 1];

            for (MAREA_IntegerSize member = at; member < top; ++member) {
                if (stack[member] < first) {
                    first = stack[member];
                }
            }

            const MAREA_AstNode *name = MAREA_SemaChildAt(graph.constants[first], 0);

            const MAREA_SemaExcept REPORT = MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_CYCLIC_CONSTANT, MAREA_AstNodeGetPosition(name), "constant depends on itself:", MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name));

            if (REPORT != MAREA_SEMA_EXCEPT_SUCCESS) {
                return REPORT;
            }
        }
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}
