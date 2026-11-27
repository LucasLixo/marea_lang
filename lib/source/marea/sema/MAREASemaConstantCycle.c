// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <string.h>

#define MAREA_G_CYCLE_DEPTH_LIMIT MAREA_UINTEGER_32(64)

#define MAREA_G_CYCLE_FOREIGN_LIMIT MAREA_INTEGER_SIZE(64)

// INFO: the visit state of one constant in the depth-first search.
typedef enum MAREA_G_CycleState
{
    MAREA_G_CYCLE_STATE_NEW = 0,
    MAREA_G_CYCLE_STATE_OPEN = 1,
    MAREA_G_CYCLE_STATE_DONE = 2,
} MAREA_G_CycleState;

// INFO: the constants of the file and of its implementation blocks in file order, and the edges of each one in one flat array. A constant reads edgeCount[index] edges from edgeStart[index]. The count pass may count an edge twice, so the fill pass sets the real count.
typedef struct MAREA_G_CycleGraph
{
    const MAREA_AstNode **constants;
    const MAREA_AstNode **implementations;
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

static const MAREA_AstNode *MAREA_CALL MAREA_G_InitialValue(const MAREA_AstNode *declaration)
{
    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(declaration);

    return (COUNT > 1) ? MAREA_SemaChildAt(declaration, (COUNT - 1)) : MAREA_NULLPTR;
}

// INFO: one walk of the initial value of a constant. foreign is the tree of the other namespace being read, or null for this file, and visited holds the constants of other namespaces already read.
typedef struct MAREA_G_CycleWalk
{
    MAREA_SemaContext *context;
    const MAREA_G_CycleGraph *graph;
    const MAREA_AstNode *foreign;
    const MAREA_AstNode *visited[MAREA_G_CYCLE_FOREIGN_LIMIT];
    MAREA_IntegerSize *out;
    MAREA_IntegerSize visitedCount;
    MAREA_IntegerSize base;
    MAREA_IntegerSize found;
} MAREA_G_CycleWalk;

static MAREA_SemaExcept MAREA_CALL MAREA_G_CollectEdges(MAREA_G_CycleWalk *walk, const MAREA_AstNode *expression, MAREA_Uinteger32 depth);

// INFO: when out is null the edges are only counted. An edge to the same constant is kept once.
static void MAREA_CALL MAREA_G_AddEdge(MAREA_G_CycleWalk *walk, const MAREA_AstNode *declaration)
{
    const MAREA_IntegerSize TARGET = MAREA_G_ConstantIndexOf(walk->graph, declaration);

    if (TARGET >= walk->graph->count) {
        return;
    }

    for (MAREA_IntegerSize seen = 0; ((walk->out != MAREA_NULLPTR) && (seen < walk->found)); ++seen) {
        if (walk->out[walk->base + seen] == TARGET) {
            return;
        }
    }

    if (walk->out != MAREA_NULLPTR) {
        walk->out[walk->base + walk->found] = TARGET;
    }

    ++walk->found;
}

static const MAREA_AstNode *MAREA_CALL MAREA_G_FindChild(const MAREA_AstNode *node, MAREA_AstKind kind)
{
    for (MAREA_IntegerSize index = 0; ((node != MAREA_NULLPTR) && (index < MAREA_AstNodeGetChildCount(node))); ++index) {
        const MAREA_AstNode *child = MAREA_SemaChildAt(node, index);

        if ((child != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(child) == kind)) {
            return child;
        }
    }

    return MAREA_NULLPTR;
}

// INFO: the top-level constant of a tree with a name, or null.
static const MAREA_AstNode *MAREA_CALL MAREA_G_TopConstant(const MAREA_AstNode *root, const MAREA_Character8 *name, MAREA_IntegerSize length)
{
    for (MAREA_IntegerSize index = 0; ((root != MAREA_NULLPTR) && (index < MAREA_AstNodeGetChildCount(root))); ++index) {
        const MAREA_AstNode *child = MAREA_SemaChildAt(root, index);

        const MAREA_AstNode *childName = ((child != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(child) == MAREA_AST_KIND_CONSTANT_DECLARATION)) ? MAREA_G_FindChild(child, MAREA_AST_KIND_IDENTIFIER) : MAREA_NULLPTR;

        if ((childName != MAREA_NULLPTR) && (MAREA_SemaTextEquals(MAREA_AstNodeGetText(childName), MAREA_AstNodeGetTextLength(childName), name, length) == MAREA_BOOLEAN_TRUE)) {
            return child;
        }
    }

    return MAREA_NULLPTR;
}

// INFO: a constant of this file is an edge. A constant of another namespace is read through to the constants of this file that it reaches.
static MAREA_SemaExcept MAREA_CALL MAREA_G_FollowMember(MAREA_G_CycleWalk *walk, const MAREA_Character8 *namespaceText, MAREA_IntegerSize namespaceLength, const MAREA_Character8 *member, MAREA_IntegerSize memberLength, MAREA_Uinteger32 depth)
{
    MAREA_SemaContext *context = walk->context;

    const MAREA_AstNode *own = MAREA_G_FindChild(MAREA_G_FindChild(context->root, MAREA_AST_KIND_NAMESPACE_DECLARATION), MAREA_AST_KIND_QUALIFIED_NAME);

    const MAREA_Character8 *ownText = MAREA_NULLPTR;

    MAREA_IntegerSize ownLength = 0;

    if ((own != MAREA_NULLPTR) && (MAREA_SemaQualifiedNameJoin(context, own, &ownText, &ownLength) != MAREA_SEMA_EXCEPT_SUCCESS)) {
        return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    if ((ownText != MAREA_NULLPTR) && (MAREA_SemaTextEquals(ownText, ownLength, namespaceText, namespaceLength) == MAREA_BOOLEAN_TRUE)) {
        MAREA_G_AddEdge(walk, MAREA_G_TopConstant(context->root, member, memberLength));

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *root = MAREA_SemaFoundRoot(context, namespaceText, namespaceLength);

    const MAREA_AstNode *constant = MAREA_G_TopConstant(root, member, memberLength);

    if ((constant == MAREA_NULLPTR) || (walk->visitedCount >= MAREA_G_CYCLE_FOREIGN_LIMIT)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    for (MAREA_IntegerSize index = 0; index < walk->visitedCount; ++index) {
        if (walk->visited[index] == constant) {
            return MAREA_SEMA_EXCEPT_SUCCESS;
        }
    }

    walk->visited[walk->visitedCount++] = constant;

    const MAREA_AstNode *outer = walk->foreign;

    walk->foreign = root;

    const MAREA_SemaExcept RESULT = MAREA_G_CollectEdges(walk, MAREA_G_InitialValue(constant), (depth + 1));

    walk->foreign = outer;

    return RESULT;
}

// INFO: reads ns.NAME in the tree being walked: the first segment may be an import alias of that tree, and the last segment is the constant. outFollowed is false when the chain names no namespace.
static MAREA_SemaExcept MAREA_CALL MAREA_G_FollowQualified(MAREA_G_CycleWalk *walk, const MAREA_AstNode *access, MAREA_Uinteger32 depth, MAREA_Boolean *outFollowed)
{
    *outFollowed = MAREA_BOOLEAN_FALSE;

    MAREA_SemaContext *context = walk->context;

    const MAREA_Character8 *text = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    const MAREA_AstNode *head = MAREA_NULLPTR;

    if (MAREA_SemaChainJoin(context, access, &text, &length, &head) != MAREA_SEMA_EXCEPT_SUCCESS) {
        return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    if ((text == MAREA_NULLPTR) || (head == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *tree = (walk->foreign != MAREA_NULLPTR) ? walk->foreign : context->root;

    const MAREA_Character8 *prefix = MAREA_AstNodeGetText(head);

    MAREA_IntegerSize prefixLength = MAREA_AstNodeGetTextLength(head);

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(tree); ++index) {
        const MAREA_AstNode *import = MAREA_SemaChildAt(tree, index);

        if ((import == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(import) != MAREA_AST_KIND_IMPORT_DECLARATION) || ((MAREA_AstNodeGetFlags(import) & MAREA_AST_FLAG_HAS_ALIAS) == 0)) {
            continue;
        }

        const MAREA_AstNode *alias = MAREA_G_FindChild(import, MAREA_AST_KIND_IDENTIFIER);

        if ((alias == MAREA_NULLPTR) || (MAREA_SemaTextEquals(MAREA_AstNodeGetText(alias), MAREA_AstNodeGetTextLength(alias), prefix, prefixLength) == MAREA_BOOLEAN_FALSE)) {
            continue;
        }

        const MAREA_Character8 *target = MAREA_NULLPTR;

        MAREA_IntegerSize targetLength = 0;

        if (MAREA_SemaQualifiedNameJoin(context, MAREA_G_FindChild(import, MAREA_AST_KIND_QUALIFIED_NAME), &target, &targetLength) != MAREA_SEMA_EXCEPT_SUCCESS) {
            return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
        }

        const MAREA_IntegerSize REST = (length - prefixLength);

        MAREA_Null memory = MAREA_NULLPTR;

        if (MAREA_ArenaAllocate(context->arena, (targetLength + REST + MAREA_INTEGER_SIZE(1)), MAREA_ALIGNOF(MAREA_Character8), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
            return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
        }

        MAREA_Character8 *joined = memory;

        memcpy(joined, target, targetLength);

        memcpy((joined + targetLength), (text + prefixLength), REST);

        text = joined;
        length = (targetLength + REST);

        break;
    }

    MAREA_IntegerSize dot = length;

    while ((dot > 0) && (text[dot - 1] != '.')) {
        --dot;
    }

    if (dot < MAREA_INTEGER_SIZE(2)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    *outFollowed = MAREA_BOOLEAN_TRUE;

    return MAREA_G_FollowMember(walk, text, (dot - 1), (text + dot), (length - dot), depth);
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_CollectName(MAREA_G_CycleWalk *walk, const MAREA_AstNode *name, MAREA_Uinteger32 depth)
{
    const MAREA_Character8 *text = MAREA_AstNodeGetText(name);

    const MAREA_IntegerSize LENGTH = MAREA_AstNodeGetTextLength(name);

    if (walk->foreign != MAREA_NULLPTR) {
        const MAREA_AstNode *constant = MAREA_G_TopConstant(walk->foreign, text, LENGTH);

        if (constant == MAREA_NULLPTR) {
            return MAREA_SEMA_EXCEPT_SUCCESS;
        }

        for (MAREA_IntegerSize index = 0; index < walk->visitedCount; ++index) {
            if (walk->visited[index] == constant) {
                return MAREA_SEMA_EXCEPT_SUCCESS;
            }
        }

        if (walk->visitedCount >= MAREA_G_CYCLE_FOREIGN_LIMIT) {
            return MAREA_SEMA_EXCEPT_SUCCESS;
        }

        walk->visited[walk->visitedCount++] = constant;

        return MAREA_G_CollectEdges(walk, MAREA_G_InitialValue(constant), (depth + 1));
    }

    const MAREA_AstNode *constant = MAREA_SemaFindConstant(walk->context, name);

    if (constant != MAREA_NULLPTR) {
        MAREA_G_AddEdge(walk, constant);

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    // INFO: a name that a member import binds is a constant of the namespace it comes from.
    const MAREA_SemaName *entry = MAREA_SemaNameTableFind(walk->context, text, LENGTH);

    if ((entry == MAREA_NULLPTR) || (entry->kind != MAREA_SEMA_NAME_KIND_IMPORTED_SYMBOL)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *member = (entry->original != MAREA_NULLPTR) ? entry->original : entry->text;

    const MAREA_IntegerSize MEMBER_LENGTH = (entry->original != MAREA_NULLPTR) ? entry->originalLength : entry->length;

    return MAREA_G_FollowMember(walk, entry->qualifier, entry->qualifierLength, member, MEMBER_LENGTH, depth);
}

// INFO: a field key, a type and a call target are not followed. A member name is followed only when the chain names a constant of a type or of a namespace.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CollectEdges(MAREA_G_CycleWalk *walk, const MAREA_AstNode *expression, MAREA_Uinteger32 depth)
{
    if ((expression == MAREA_NULLPTR) || (depth >= MAREA_G_CYCLE_DEPTH_LIMIT)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    MAREA_IntegerSize first = 0;

    switch (MAREA_AstNodeGetKind(expression)) {
    case MAREA_AST_KIND_IDENTIFIER:
        return MAREA_G_CollectName(walk, expression, depth);
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
    {
        const MAREA_AstNode *base = MAREA_SemaChildAt(expression, 0);

        if ((base != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(base) == MAREA_AST_KIND_IDENTIFIER) && (walk->foreign == MAREA_NULLPTR)) {
            const MAREA_AstNode *constant = MAREA_SemaFindTypeConstant(walk->context, base, MAREA_SemaChildAt(expression, 1));

            if (constant != MAREA_NULLPTR) {
                MAREA_G_AddEdge(walk, constant);

                return MAREA_SEMA_EXCEPT_SUCCESS;
            }

            // INFO: a member of a constant or of a type of the file is not a name of a namespace.
            const MAREA_SemaName *entry = MAREA_SemaNameTableFind(walk->context, MAREA_AstNodeGetText(base), MAREA_AstNodeGetTextLength(base));

            if ((entry != MAREA_NULLPTR) && (entry->kind == MAREA_SEMA_NAME_KIND_DECLARATION)) {
                return MAREA_G_CollectEdges(walk, base, (depth + 1));
            }
        }

        MAREA_Boolean followed = MAREA_BOOLEAN_FALSE;

        const MAREA_SemaExcept RESULT = MAREA_G_FollowQualified(walk, expression, depth, &followed);

        if ((RESULT != MAREA_SEMA_EXCEPT_SUCCESS) || (followed == MAREA_BOOLEAN_TRUE)) {
            return RESULT;
        }

        return MAREA_G_CollectEdges(walk, base, (depth + 1));
    }
    default:
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    for (MAREA_IntegerSize index = first; index < MAREA_AstNodeGetChildCount(expression); ++index) {
        const MAREA_SemaExcept RESULT = MAREA_G_CollectEdges(walk, MAREA_SemaChildAt(expression, index), (depth + 1));

        if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return RESULT;
        }
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

// INFO: the names of an initial value are read inside the implementation block of its constant, with out as the place of the edges or null to count them.
static MAREA_SemaExcept MAREA_CALL MAREA_G_EdgesOf(MAREA_SemaContext *context, const MAREA_G_CycleGraph *graph, MAREA_IntegerSize constant, MAREA_IntegerSize *out, MAREA_IntegerSize base, MAREA_IntegerSize *outFound)
{
    MAREA_G_CycleWalk walk = { .context = context, .graph = graph, .out = out, .base = base };

    context->implementation = graph->implementations[constant];

    const MAREA_SemaExcept RESULT = MAREA_G_CollectEdges(&walk, MAREA_G_InitialValue(graph->constants[constant]), 0);

    context->implementation = MAREA_NULLPTR;

    *outFound = walk.found;

    return RESULT;
}

// INFO: counts the constants of the file and of its implementation blocks, and writes them in file order when the graph has room for them.
static MAREA_IntegerSize MAREA_CALL MAREA_G_ListConstants(const MAREA_AstNode *root, MAREA_G_CycleGraph *graph)
{
    MAREA_IntegerSize count = 0;

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(root); ++index) {
        const MAREA_AstNode *child = MAREA_SemaChildAt(root, index);

        const MAREA_Boolean BLOCK = ((child != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(child) == MAREA_AST_KIND_IMPLEMENTATION_DECLARATION)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

        const MAREA_IntegerSize MEMBERS = (BLOCK == MAREA_BOOLEAN_TRUE) ? MAREA_AstNodeGetChildCount(child) : MAREA_INTEGER_SIZE(1);

        for (MAREA_IntegerSize member = 0; member < MEMBERS; ++member) {
            const MAREA_AstNode *constant = (BLOCK == MAREA_BOOLEAN_TRUE) ? MAREA_SemaChildAt(child, member) : child;

            if ((constant == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(constant) != MAREA_AST_KIND_CONSTANT_DECLARATION)) {
                continue;
            }

            if (graph->constants != MAREA_NULLPTR) {
                graph->constants[count] = constant;
                graph->implementations[count] = (BLOCK == MAREA_BOOLEAN_TRUE) ? child : MAREA_NULLPTR;
            }

            ++count;
        }
    }

    return count;
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

    MAREA_G_CycleGraph graph = { 0 };

    graph.count = MAREA_G_ListConstants(root, &graph);

    if (graph.count == 0) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    void *memory = MAREA_NULLPTR;

    // INFO: one block holds the constants and then the implementation block of each one.
    if (MAREA_G_Allocate(context, (graph.count * 2), sizeof(const MAREA_AstNode *), MAREA_ALIGNOF(const MAREA_AstNode *), &memory) != MAREA_SEMA_EXCEPT_SUCCESS) {
        return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    graph.constants = (const MAREA_AstNode **)memory;
    graph.implementations = (graph.constants + graph.count);

    MAREA_IGNORE(MAREA_G_ListConstants(root, &graph));

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

        MAREA_IntegerSize found = 0;

        if (MAREA_G_EdgesOf(context, &graph, index, MAREA_NULLPTR, 0, &found) != MAREA_SEMA_EXCEPT_SUCCESS) {
            return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
        }

        total += found;
    }

    if (total > 0) {
        if (MAREA_G_Allocate(context, total, sizeof(MAREA_IntegerSize), MAREA_ALIGNOF(MAREA_IntegerSize), &memory) != MAREA_SEMA_EXCEPT_SUCCESS) {
            return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
        }

        graph.edges = (MAREA_IntegerSize *)memory;

        for (MAREA_IntegerSize index = 0; index < graph.count; ++index) {
            if (MAREA_G_EdgesOf(context, &graph, index, graph.edges, graph.edgeStart[index], &graph.edgeCount[index]) != MAREA_SEMA_EXCEPT_SUCCESS) {
                return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
            }
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
