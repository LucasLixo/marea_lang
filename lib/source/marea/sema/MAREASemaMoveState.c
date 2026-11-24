// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <string.h>

typedef enum MAREA_G_BranchKind
{
    MAREA_G_BRANCH_KIND_NONE = MAREA_UINTEGER_8(0),
    MAREA_G_BRANCH_KIND_IF = MAREA_UINTEGER_8(1),
    MAREA_G_BRANCH_KIND_SWITCH = MAREA_UINTEGER_8(2),
    MAREA_G_BRANCH_KIND_LOOP = MAREA_UINTEGER_8(3),
} MAREA_G_BranchKind;

static MAREA_G_BranchKind MAREA_CALL MAREA_G_BranchKindOf(const MAREA_AstNode *node)
{
    switch (MAREA_AstNodeGetKind(node)) {
    case MAREA_AST_KIND_IF_STATEMENT:
        return MAREA_G_BRANCH_KIND_IF;
    case MAREA_AST_KIND_SWITCH_STATEMENT:
        return MAREA_G_BRANCH_KIND_SWITCH;
    case MAREA_AST_KIND_WHILE_STATEMENT:
    case MAREA_AST_KIND_FOR_RANGE_STATEMENT:
    case MAREA_AST_KIND_FOR_EACH_STATEMENT:
    case MAREA_AST_KIND_PARTITION_STATEMENT:
        return MAREA_G_BRANCH_KIND_LOOP;
    default:
        return MAREA_G_BRANCH_KIND_NONE;
    }
}

// INFO: an arm is a child that runs on one path only: a block of an if, a clause of an if, an arm of a switch, or the body of a loop. The condition, the scrutinee and the bounds run before every arm.
static MAREA_Boolean MAREA_CALL MAREA_G_IsArm(MAREA_G_BranchKind branch, const MAREA_AstNode *child)
{
    if (child == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstKind KIND = MAREA_AstNodeGetKind(child);

    switch (branch) {
    case MAREA_G_BRANCH_KIND_IF:
        return ((KIND == MAREA_AST_KIND_BLOCK) || (KIND == MAREA_AST_KIND_ELSEIF_CLAUSE) || (KIND == MAREA_AST_KIND_ELSE_CLAUSE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    case MAREA_G_BRANCH_KIND_SWITCH:
        return ((KIND == MAREA_AST_KIND_SWITCH_ARM) || (KIND == MAREA_AST_KIND_DEFAULT_ARM)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    case MAREA_G_BRANCH_KIND_LOOP:
        return (KIND == MAREA_AST_KIND_BLOCK) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    default:
        return MAREA_BOOLEAN_FALSE;
    }
}

static const MAREA_AstNode *MAREA_CALL MAREA_G_ArmBlock(const MAREA_AstNode *arm)
{
    if (MAREA_AstNodeGetKind(arm) == MAREA_AST_KIND_BLOCK) {
        return arm;
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(arm);

    return (COUNT == 0) ? MAREA_NULLPTR : MAREA_SemaChildAt(arm, (COUNT - 1));
}

static MAREA_SemaName *MAREA_CALL MAREA_G_FindLocal(MAREA_SemaContext *context, const MAREA_AstNode *name)
{
    for (MAREA_IntegerSize index = context->locals.count; index > 0; --index) {
        MAREA_SemaName *local = &context->locals.entries[(index - 1)];

        if (MAREA_SemaTextEquals(local->text, local->length, MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name)) == MAREA_BOOLEAN_TRUE) {
            return local;
        }
    }

    return MAREA_NULLPTR;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsIdentifier(const MAREA_AstNode *node)
{
    return ((node != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(node) == MAREA_AST_KIND_IDENTIFIER)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsMove(const MAREA_AstNode *node)
{
    return ((node != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(node) == MAREA_AST_KIND_UNARY_EXPRESSION) && (MAREA_AstNodeGetOperator(node) == MAREA_TOKEN_KIND_KEYWORD_MOVE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsPlainAssignmentTarget(const MAREA_AstNode *parent, MAREA_IntegerSize index)
{
    return ((parent != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(parent) == MAREA_AST_KIND_ASSIGNMENT) && (index == 0) && (MAREA_AstNodeGetOperator(parent) == MAREA_TOKEN_KIND_EQUAL)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: a name is read where the parent takes a value at that place. A declared name, a member name, a field key and a case label are not reads.
static MAREA_Boolean MAREA_CALL MAREA_G_IsValuePosition(const MAREA_AstNode *parent, MAREA_IntegerSize index)
{
    if (parent == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    switch (MAREA_AstNodeGetKind(parent)) {
    case MAREA_AST_KIND_ASSIGNMENT:
    case MAREA_AST_KIND_EXPRESSION_STATEMENT:
    case MAREA_AST_KIND_RETURN_STATEMENT:
    case MAREA_AST_KIND_BINARY_EXPRESSION:
    case MAREA_AST_KIND_UNARY_EXPRESSION:
    case MAREA_AST_KIND_PARENTHESIZED_EXPRESSION:
    case MAREA_AST_KIND_ARGUMENT_LIST:
    case MAREA_AST_KIND_INDEX_ACCESS:
    case MAREA_AST_KIND_INDEX_ENTRY:
    case MAREA_AST_KIND_VALUE_ENTRY:
        return MAREA_BOOLEAN_TRUE;
    case MAREA_AST_KIND_IF_STATEMENT:
    case MAREA_AST_KIND_ELSEIF_CLAUSE:
    case MAREA_AST_KIND_WHILE_STATEMENT:
    case MAREA_AST_KIND_SWITCH_STATEMENT:
    case MAREA_AST_KIND_WITH_STATEMENT:
    case MAREA_AST_KIND_CALL_EXPRESSION:
    case MAREA_AST_KIND_MEMBER_ACCESS:
        return (index == 0) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    case MAREA_AST_KIND_PARTITION_STATEMENT:
        return (index <= 1) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    case MAREA_AST_KIND_FOR_RANGE_STATEMENT:
        return ((index == 1) || (index == 2)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    case MAREA_AST_KIND_FOR_EACH_STATEMENT:
        // INFO: the children are the loop names, the collection and the body, so the collection is the second child from the end.
        return ((MAREA_AstNodeGetChildCount(parent) >= 2) && (index == (MAREA_AstNodeGetChildCount(parent) - 2)) && (index > 0)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    case MAREA_AST_KIND_FIELD_ENTRY:
        return (index == 1) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    case MAREA_AST_KIND_LOCAL_DECLARATION:
        return (index == 2) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    default:
        return MAREA_BOOLEAN_FALSE;
    }
}

#define MAREA_G_PATH_LIMIT MAREA_INTEGER_SIZE(16)

// INFO: a path is the chain of a field or element access from its root name outward, so a use and a moved part compare segment by segment.
typedef struct MAREA_G_Path
{
    const MAREA_AstNode *root;
    const MAREA_AstNode *segments[MAREA_G_PATH_LIMIT];
    MAREA_IntegerSize count;
} MAREA_G_Path;

static MAREA_Boolean MAREA_CALL MAREA_G_IsChain(const MAREA_AstNode *node)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstKind KIND = MAREA_AstNodeGetKind(node);

    return ((KIND == MAREA_AST_KIND_MEMBER_ACCESS) || (KIND == MAREA_AST_KIND_INDEX_ACCESS)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: a chain deeper than the limit, or one that does not start at a name, gives no path, so it is never tracked.
static MAREA_Boolean MAREA_CALL MAREA_G_PathOf(const MAREA_AstNode *chain, MAREA_G_Path *outPath)
{
    MAREA_IntegerSize depth = 0;

    const MAREA_AstNode *cursor = chain;

    while (MAREA_G_IsChain(cursor) == MAREA_BOOLEAN_TRUE) {
        depth += 1;
        cursor = MAREA_SemaChildAt(cursor, 0);
    }

    if ((MAREA_G_IsIdentifier(cursor) == MAREA_BOOLEAN_FALSE) || (depth == 0) || (depth > MAREA_G_PATH_LIMIT)) {
        return MAREA_BOOLEAN_FALSE;
    }

    outPath->root = cursor;
    outPath->count = depth;

    cursor = chain;

    for (MAREA_IntegerSize index = depth; index > 0; --index) {
        outPath->segments[(index - 1)] = cursor;
        cursor = MAREA_SemaChildAt(cursor, 0);
    }

    return MAREA_BOOLEAN_TRUE;
}

// INFO: an index is equal only when both are integer literals of one text, so a computed index never equals another one and a use is never rejected on a guess.
static MAREA_Boolean MAREA_CALL MAREA_G_SegmentEquals(const MAREA_AstNode *left, const MAREA_AstNode *right)
{
    if (MAREA_AstNodeGetKind(left) != MAREA_AstNodeGetKind(right)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *leftKey = MAREA_SemaChildAt(left, 1);

    const MAREA_AstNode *rightKey = MAREA_SemaChildAt(right, 1);

    if ((leftKey == MAREA_NULLPTR) || (rightKey == MAREA_NULLPTR)) {
        return MAREA_BOOLEAN_FALSE;
    }

    if ((MAREA_AstNodeGetKind(left) == MAREA_AST_KIND_INDEX_ACCESS) && ((MAREA_AstNodeGetKind(leftKey) != MAREA_AST_KIND_INTEGER_LITERAL) || (MAREA_AstNodeGetKind(rightKey) != MAREA_AST_KIND_INTEGER_LITERAL))) {
        return MAREA_BOOLEAN_FALSE;
    }

    return MAREA_SemaTextEquals(MAREA_AstNodeGetText(leftKey), MAREA_AstNodeGetTextLength(leftKey), MAREA_AstNodeGetText(rightKey), MAREA_AstNodeGetTextLength(rightKey));
}

// INFO: true when the first count segments of both paths are equal, so one path holds the other or both name the same part.
static MAREA_Boolean MAREA_CALL MAREA_G_SamePrefix(const MAREA_G_Path *left, const MAREA_G_Path *right, MAREA_IntegerSize count)
{
    for (MAREA_IntegerSize index = 0; index < count; ++index) {
        if (MAREA_G_SegmentEquals(left->segments[index], right->segments[index]) == MAREA_BOOLEAN_FALSE) {
            return MAREA_BOOLEAN_FALSE;
        }
    }

    return MAREA_BOOLEAN_TRUE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_PathsConflict(const MAREA_G_Path *used, const MAREA_G_Path *moved)
{
    return MAREA_G_SamePrefix(used, moved, (used->count < moved->count) ? used->count : moved->count);
}

// INFO: two moved parts agree when the same chain was moved, which is what the merge of the arms of a branch needs. A part with a computed index never agrees, so the merge rounds toward valid.
static MAREA_Boolean MAREA_CALL MAREA_G_PartsAgree(const MAREA_AstNode *left, const MAREA_AstNode *right)
{
    if ((left == MAREA_NULLPTR) || (right == MAREA_NULLPTR) || (left == right)) {
        return (left == right) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    }

    MAREA_G_Path leftPath = { 0 };

    MAREA_G_Path rightPath = { 0 };

    if ((MAREA_G_PathOf(left, &leftPath) == MAREA_BOOLEAN_FALSE) || (MAREA_G_PathOf(right, &rightPath) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_BOOLEAN_FALSE;
    }

    return ((leftPath.count == rightPath.count) && (MAREA_G_SamePrefix(&leftPath, &rightPath, leftPath.count) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static const MAREA_AstNode *MAREA_CALL MAREA_G_TypeArgument(const MAREA_AstNode *type)
{
    for (MAREA_IntegerSize index = 0; (type != MAREA_NULLPTR) && (index < MAREA_AstNodeGetChildCount(type)); ++index) {
        const MAREA_AstNode *child = MAREA_SemaChildAt(type, index);

        if ((child != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(child) == MAREA_AST_KIND_TYPE_ARGUMENTS)) {
            return MAREA_SemaChildAt(child, 0);
        }
    }

    return MAREA_NULLPTR;
}

static MAREA_Boolean MAREA_CALL MAREA_G_NameIs(const MAREA_Character8 *text, MAREA_IntegerSize length, const char *expected)
{
    return ((strlen(expected) == length) && (memcmp(expected, text, length) == 0)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: an owned wrapper is seen through, so the element of owned<vector<T>> is T. Any other shape gives null and the element is never guessed.
static const MAREA_AstNode *MAREA_CALL MAREA_G_ElementOf(const MAREA_SemaContext *context, const MAREA_AstNode *collection)
{
    const MAREA_AstNode *type = collection;

    for (MAREA_IntegerSize depth = 0; (type != MAREA_NULLPTR) && (depth < 2); ++depth) {
        const MAREA_AstNode *declaration = MAREA_NULLPTR;

        const MAREA_Character8 *name = MAREA_NULLPTR;

        MAREA_IntegerSize length = 0;

        const MAREA_SemaTypeClass CLASS = MAREA_SemaClassifyType(context, type, &declaration, &name, &length);

        if (((CLASS != MAREA_SEMA_TYPE_CLASS_MOVE_BUILTIN) && (CLASS != MAREA_SEMA_TYPE_CLASS_COPY_BUILTIN)) || (name == MAREA_NULLPTR)) {
            return MAREA_NULLPTR;
        }

        if (MAREA_G_NameIs(name, length, "owned") == MAREA_BOOLEAN_TRUE) {
            type = MAREA_G_TypeArgument(type);

            continue;
        }

        return ((MAREA_G_NameIs(name, length, "vector") == MAREA_BOOLEAN_TRUE) || (MAREA_G_NameIs(name, length, "array") == MAREA_BOOLEAN_TRUE) || (MAREA_G_NameIs(name, length, "slice") == MAREA_BOOLEAN_TRUE)) ? MAREA_G_TypeArgument(type) : MAREA_NULLPTR;
    }

    return MAREA_NULLPTR;
}

// INFO: the type of a moved part. A field is read from its structure, an element from its collection type; null when sema cannot see it.
static const MAREA_AstNode *MAREA_CALL MAREA_G_PartType(const MAREA_SemaContext *context, const MAREA_AstNode *chain)
{
    const MAREA_AstNode *type = MAREA_SemaInferType(context, chain);

    if ((type == MAREA_NULLPTR) && (MAREA_AstNodeGetKind(chain) == MAREA_AST_KIND_INDEX_ACCESS)) {
        type = MAREA_G_ElementOf(context, MAREA_SemaInferType(context, MAREA_SemaChildAt(chain, 0)));
    }

    return type;
}

// INFO: the binding of a for each name, or null for any other name. A local that reuses the name hides it, because the shadow entry has no declaration.
static const MAREA_AstNode *MAREA_CALL MAREA_G_ForEachOf(const MAREA_SemaContext *context, const MAREA_AstNode *name)
{
    for (MAREA_IntegerSize index = context->references.count; index > 0; --index) {
        const MAREA_SemaName *entry = &context->references.entries[(index - 1)];

        if (MAREA_SemaTextEquals(entry->text, entry->length, MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name)) == MAREA_BOOLEAN_TRUE) {
            return ((entry->declaration != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(entry->declaration) == MAREA_AST_KIND_FOR_EACH_STATEMENT)) ? entry->declaration : MAREA_NULLPTR;
        }
    }

    return MAREA_NULLPTR;
}

// INFO: a for each element is a part of the collection that the loop reads again on the next turn, so moving it out of a collection of a type that is not by value leaves the owner partly moved.
static MAREA_Boolean MAREA_CALL MAREA_G_MovesForEachElement(const MAREA_SemaContext *context, const MAREA_AstNode *name)
{
    const MAREA_AstNode *loop = MAREA_G_ForEachOf(context, name);

    // INFO: with a key and a value name the collection is a table, whose element type is not read here. The reference table also lists a name that is the collection, so the name must be the loop name.
    if ((loop == MAREA_NULLPTR) || (MAREA_AstNodeGetChildCount(loop) != 3)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *loopName = MAREA_SemaChildAt(loop, 0);

    if (MAREA_SemaTextEquals(MAREA_AstNodeGetText(loopName), MAREA_AstNodeGetTextLength(loopName), MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name)) == MAREA_BOOLEAN_FALSE) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *element = MAREA_G_ElementOf(context, MAREA_SemaInferType(context, MAREA_SemaChildAt(loop, 1)));

    return ((element != MAREA_NULLPTR) && (MAREA_SemaTypeIsByValue(context, element, MAREA_BOOLEAN_TRUE) == MAREA_BOOLEAN_FALSE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_ReportPart(MAREA_SemaContext *context, const MAREA_AstNode *at, const MAREA_AstNode *root)
{
    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_USE_OF_PARTIALLY_MOVED_VALUE, MAREA_AstNodeGetPosition(at), "value is used after a part of it was moved out:", MAREA_AstNodeGetText(root), MAREA_AstNodeGetTextLength(root));
}

// INFO: a chain that is the top of its access, so a name inside it is judged once, with the whole path it starts.
static MAREA_Boolean MAREA_CALL MAREA_G_IsChainTop(const MAREA_AstNode *node, const MAREA_AstNode *parent, MAREA_IntegerSize parentIndex)
{
    return ((MAREA_G_IsChain(node) == MAREA_BOOLEAN_TRUE) && !((MAREA_G_IsChain(parent) == MAREA_BOOLEAN_TRUE) && (parentIndex == 0))) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckPartUse(MAREA_SemaContext *context, const MAREA_AstNode *chain, const MAREA_AstNode *parent, MAREA_IntegerSize parentIndex)
{
    MAREA_G_Path used = { 0 };

    if (MAREA_G_PathOf(chain, &used) == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_SemaName *local = MAREA_G_FindLocal(context, used.root);

    MAREA_G_Path moved = { 0 };

    if ((local == MAREA_NULLPTR) || (local->partAt == MAREA_NULLPTR) || (MAREA_G_PathOf(local->partAt, &moved) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    // INFO: the callee of a call is the method of the receiver, so the receiver is the part that is used.
    if ((MAREA_AstNodeGetKind(chain) == MAREA_AST_KIND_MEMBER_ACCESS) && (parent != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(parent) == MAREA_AST_KIND_CALL_EXPRESSION) && (parentIndex == 0)) {
        used.count -= 1;
    }

    // INFO: a plain assignment to the moved part, or to a part that holds it, gives it a new value, so it is no use.
    if ((MAREA_G_IsPlainAssignmentTarget(parent, parentIndex) == MAREA_BOOLEAN_TRUE) && (used.count <= moved.count) && (MAREA_G_SamePrefix(&used, &moved, used.count) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return (MAREA_G_PathsConflict(&used, &moved) == MAREA_BOOLEAN_TRUE) ? MAREA_G_ReportPart(context, chain, used.root) : MAREA_SEMA_EXCEPT_SUCCESS;
}

// INFO: the move of a field or an element marks that part of its local, once, when the part is not by value. A borrow cannot be moved out of, so it is left to the borrow rules.
static void MAREA_CALL MAREA_G_RecordPart(MAREA_SemaContext *context, const MAREA_AstNode *chain)
{
    MAREA_G_Path path = { 0 };

    if (MAREA_G_PathOf(chain, &path) == MAREA_BOOLEAN_FALSE) {
        return;
    }

    MAREA_SemaName *local = MAREA_G_FindLocal(context, path.root);

    if ((local == MAREA_NULLPTR) || (local->partAt != MAREA_NULLPTR) || (MAREA_SemaIsReference(context, MAREA_AstNodeGetText(path.root), MAREA_AstNodeGetTextLength(path.root)) == MAREA_BOOLEAN_TRUE)) {
        return;
    }

    if (MAREA_SemaTypeIsByValue(context, MAREA_G_PartType(context, chain), MAREA_BOOLEAN_TRUE) == MAREA_BOOLEAN_TRUE) {
        return;
    }

    local->partAt = chain;
}

// INFO: a plain assignment to a chain that holds the moved part, or is it, makes the part live again.
static void MAREA_CALL MAREA_G_ClearPart(MAREA_SemaContext *context, const MAREA_AstNode *target)
{
    MAREA_G_Path path = { 0 };

    if (MAREA_G_PathOf(target, &path) == MAREA_BOOLEAN_FALSE) {
        return;
    }

    MAREA_SemaName *local = MAREA_G_FindLocal(context, path.root);

    MAREA_G_Path moved = { 0 };

    if ((local == MAREA_NULLPTR) || (local->partAt == MAREA_NULLPTR) || (MAREA_G_PathOf(local->partAt, &moved) == MAREA_BOOLEAN_FALSE)) {
        return;
    }

    if ((path.count <= moved.count) && (MAREA_G_SamePrefix(&path, &moved, path.count) == MAREA_BOOLEAN_TRUE)) {
        local->partAt = MAREA_NULLPTR;
    }
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaMoveCheckUse(MAREA_SemaContext *context, const MAREA_AstNode *node, const MAREA_AstNode *parent, MAREA_IntegerSize parentIndex)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    if ((MAREA_G_IsChainTop(node, parent, parentIndex) == MAREA_BOOLEAN_TRUE) && ((MAREA_G_IsValuePosition(parent, parentIndex) == MAREA_BOOLEAN_TRUE) || (MAREA_G_IsMove(parent) == MAREA_BOOLEAN_TRUE))) {
        return MAREA_G_CheckPartUse(context, node, parent, parentIndex);
    }

    if ((MAREA_G_IsIdentifier(node) == MAREA_BOOLEAN_FALSE) || (MAREA_G_IsValuePosition(parent, parentIndex) == MAREA_BOOLEAN_FALSE) || (MAREA_G_IsPlainAssignmentTarget(parent, parentIndex) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    // INFO: a name inside a chain is judged for a moved part with the whole chain, at its top.
    const MAREA_Boolean IN_CHAIN = ((MAREA_G_IsChain(parent) == MAREA_BOOLEAN_TRUE) && (parentIndex == 0)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    if ((IN_CHAIN == MAREA_BOOLEAN_FALSE) && (MAREA_G_IsMove(parent) == MAREA_BOOLEAN_TRUE) && (MAREA_G_MovesForEachElement(context, node) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_G_ReportPart(context, node, node);
    }

    const MAREA_SemaName *local = MAREA_G_FindLocal(context, node);

    if ((IN_CHAIN == MAREA_BOOLEAN_FALSE) && (local != MAREA_NULLPTR) && (local->moved == MAREA_BOOLEAN_FALSE) && (local->partAt != MAREA_NULLPTR)) {
        return MAREA_G_ReportPart(context, node, node);
    }

    if ((local == MAREA_NULLPTR) || (local->moved == MAREA_BOOLEAN_FALSE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    if (MAREA_G_IsMove(parent) == MAREA_BOOLEAN_TRUE) {
        return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_VALUE_MOVED_AGAIN, MAREA_AstNodeGetPosition(node), "value is moved a second time:", MAREA_AstNodeGetText(node), MAREA_AstNodeGetTextLength(node));
    }

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_USE_OF_MOVED_VALUE, MAREA_AstNodeGetPosition(node), "value is used after it was moved:", MAREA_AstNodeGetText(node), MAREA_AstNodeGetTextLength(node));
}

// INFO: only a local of a type that is not by value can be moved out. A primitive, an enum and a type that sema cannot see are copied, so a move of them leaves them valid.
void MAREA_CALL MAREA_SemaMoveRecord(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    if (MAREA_G_IsMove(node) == MAREA_BOOLEAN_TRUE) {
        const MAREA_AstNode *operand = MAREA_SemaChildAt(node, 0);

        if (MAREA_G_IsChain(operand) == MAREA_BOOLEAN_TRUE) {
            MAREA_G_RecordPart(context, operand);

            return;
        }

        if (MAREA_G_IsIdentifier(operand) == MAREA_BOOLEAN_FALSE) {
            return;
        }

        MAREA_SemaName *local = MAREA_G_FindLocal(context, operand);

        if ((local == MAREA_NULLPTR) || (local->declaration == MAREA_NULLPTR) || (MAREA_SemaTypeIsByValue(context, local->declaration, MAREA_BOOLEAN_TRUE) == MAREA_BOOLEAN_TRUE)) {
            return;
        }

        local->moved = MAREA_BOOLEAN_TRUE;
        local->movedAt = node;

        return;
    }

    // INFO: a plain assignment to the whole local gives it a new value, so it is live again once the right side was read.
    if ((MAREA_AstNodeGetKind(node) == MAREA_AST_KIND_ASSIGNMENT) && (MAREA_AstNodeGetOperator(node) == MAREA_TOKEN_KIND_EQUAL)) {
        const MAREA_AstNode *target = MAREA_SemaChildAt(node, 0);

        if (MAREA_G_IsChain(target) == MAREA_BOOLEAN_TRUE) {
            MAREA_G_ClearPart(context, target);

            return;
        }

        if (MAREA_G_IsIdentifier(target) == MAREA_BOOLEAN_FALSE) {
            return;
        }

        MAREA_SemaName *local = MAREA_G_FindLocal(context, target);

        if (local != MAREA_NULLPTR) {
            local->moved = MAREA_BOOLEAN_FALSE;
            local->partAt = MAREA_NULLPTR;
        }
    }
}

static void MAREA_CALL MAREA_G_Save(const MAREA_SemaContext *context, MAREA_Boolean *into, const MAREA_AstNode **parts, MAREA_IntegerSize count)
{
    for (MAREA_IntegerSize index = 0; index < count; ++index) {
        into[index] = context->locals.entries[index].moved;
        parts[index] = context->locals.entries[index].partAt;
    }
}

static void MAREA_CALL MAREA_G_Load(MAREA_SemaContext *context, const MAREA_Boolean *from, const MAREA_AstNode *const *parts, MAREA_IntegerSize count)
{
    for (MAREA_IntegerSize index = 0; index < count; ++index) {
        context->locals.entries[index].moved = from[index];
        context->locals.entries[index].partAt = parts[index];
    }
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaMoveEnter(MAREA_SemaContext *context, const MAREA_AstNode *node, MAREA_SemaMoveFrame *outFrame)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR) || (outFrame == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    *outFrame = (MAREA_SemaMoveFrame){ 0 };

    const MAREA_G_BranchKind BRANCH = MAREA_G_BranchKindOf(node);

    if ((BRANCH == MAREA_G_BRANCH_KIND_NONE) || (context->locals.count == 0)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    MAREA_Null memory = MAREA_NULLPTR;

    // INFO: the pointers come first in the block, so both arrays are aligned.
    const MAREA_IntegerSize PARTS_SIZE = (context->locals.count * MAREA_INTEGER_SIZE(2) * sizeof(const MAREA_AstNode *));

    if (MAREA_ArenaAllocate(context->arena, (PARTS_SIZE + (context->locals.count * MAREA_INTEGER_SIZE(2) * sizeof(MAREA_Boolean))), MAREA_ALIGNOF(const MAREA_AstNode *), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    outFrame->count = context->locals.count;
    outFrame->partBase = memory;
    outFrame->partMerged = (outFrame->partBase + outFrame->count);
    outFrame->base = (MAREA_Boolean *)(outFrame->partMerged + outFrame->count);
    outFrame->merged = (outFrame->base + outFrame->count);

    MAREA_G_Save(context, outFrame->base, outFrame->partBase, outFrame->count);

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

void MAREA_CALL MAREA_SemaMoveChildEnter(MAREA_SemaContext *context, const MAREA_AstNode *node, const MAREA_AstNode *child, MAREA_SemaMoveFrame *frame)
{
    if ((frame->base == MAREA_NULLPTR) || (MAREA_G_IsArm(MAREA_G_BranchKindOf(node), child) == MAREA_BOOLEAN_FALSE)) {
        return;
    }

    MAREA_G_Load(context, frame->base, frame->partBase, frame->count);
}

// INFO: a body that can leave the loop on the same pass (a break or a return among its own statements) does not run the move twice.
static MAREA_Boolean MAREA_CALL MAREA_G_BodyLeaves(const MAREA_SemaContext *context, const MAREA_AstNode *body)
{
    if (body == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_TRUE;
    }

    if (MAREA_SemaBlockTerminates(context, body) == MAREA_BOOLEAN_TRUE) {
        return MAREA_BOOLEAN_TRUE;
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(body); ++index) {
        if (MAREA_AstNodeGetKind(MAREA_SemaChildAt(body, index)) == MAREA_AST_KIND_BREAK_STATEMENT) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_ReportRepeatedMoves(MAREA_SemaContext *context, const MAREA_SemaMoveFrame *frame, const MAREA_AstNode *body)
{
    if (MAREA_G_BodyLeaves(context, body) == MAREA_BOOLEAN_TRUE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    for (MAREA_IntegerSize index = 0; index < frame->count; ++index) {
        const MAREA_SemaName *local = &context->locals.entries[index];

        if ((local->moved == MAREA_BOOLEAN_FALSE) || (frame->base[index] == MAREA_BOOLEAN_TRUE) || (local->movedAt == MAREA_NULLPTR)) {
            continue;
        }

        const MAREA_SemaExcept RESULT = MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_VALUE_MOVED_AGAIN, MAREA_AstNodeGetPosition(local->movedAt), "value moved in a loop body is moved again on the next turn:", local->text, local->length);

        if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return RESULT;
        }
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

// INFO: an arm with a break among its own statements leaves the loop before the code after the branch, so it takes no part in the merge of the branch.
static MAREA_Boolean MAREA_CALL MAREA_G_ArmBreaks(const MAREA_AstNode *block)
{
    for (MAREA_IntegerSize index = 0; (block != MAREA_NULLPTR) && (index < MAREA_AstNodeGetChildCount(block)); ++index) {
        if (MAREA_AstNodeGetKind(MAREA_SemaChildAt(block, index)) == MAREA_AST_KIND_BREAK_STATEMENT) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaMoveChildLeave(MAREA_SemaContext *context, const MAREA_AstNode *node, const MAREA_AstNode *child, MAREA_SemaMoveFrame *frame)
{
    if (frame->base == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_G_BranchKind BRANCH = MAREA_G_BranchKindOf(node);

    // INFO: a condition, a scrutinee or a bound runs before every arm, so what it moves is part of the base of the arms.
    if (MAREA_G_IsArm(BRANCH, child) == MAREA_BOOLEAN_FALSE) {
        MAREA_G_Save(context, frame->base, frame->partBase, frame->count);

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *block = MAREA_G_ArmBlock(child);

    if (BRANCH == MAREA_G_BRANCH_KIND_LOOP) {
        return MAREA_G_ReportRepeatedMoves(context, frame, block);
    }

    const MAREA_AstKind KIND = MAREA_AstNodeGetKind(child);

    if ((KIND == MAREA_AST_KIND_ELSE_CLAUSE) || (KIND == MAREA_AST_KIND_DEFAULT_ARM)) {
        frame->hasFallback = MAREA_BOOLEAN_TRUE;
    }

    // INFO: an arm that ends the function never reaches the code after the branch, so it takes no part in the merge.
    if ((block != MAREA_NULLPTR) && ((MAREA_SemaBlockTerminates(context, block) == MAREA_BOOLEAN_TRUE) || (MAREA_G_ArmBreaks(block) == MAREA_BOOLEAN_TRUE))) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    for (MAREA_IntegerSize index = 0; index < frame->count; ++index) {
        const MAREA_Boolean MOVED = context->locals.entries[index].moved;

        const MAREA_AstNode *part = context->locals.entries[index].partAt;

        if (frame->armSeen == MAREA_BOOLEAN_FALSE) {
            frame->merged[index] = MOVED;
            frame->partMerged[index] = part;
        } else {
            // INFO: the moved bit joins with OR, so a local that any arm moved is maybe moved after the branch.
            if (MOVED == MAREA_BOOLEAN_TRUE) {
                frame->merged[index] = MAREA_BOOLEAN_TRUE;
            }

            // INFO: a part counts after the branch only when every arm that reaches the end moved the same part.
            if (MAREA_G_PartsAgree(frame->partMerged[index], part) == MAREA_BOOLEAN_FALSE) {
                frame->partMerged[index] = MAREA_NULLPTR;
            }
        }
    }

    frame->armSeen = MAREA_BOOLEAN_TRUE;

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

void MAREA_CALL MAREA_SemaMoveLeave(MAREA_SemaContext *context, const MAREA_AstNode *node, MAREA_SemaMoveFrame *frame)
{
    if (frame->base == MAREA_NULLPTR) {
        return;
    }

    const MAREA_G_BranchKind BRANCH = MAREA_G_BranchKindOf(node);

    // INFO: a loop body may run zero times, and an if with no else or a switch with no default has a path that runs no arm, so that path joins the merge with the base state by OR.
    if ((BRANCH == MAREA_G_BRANCH_KIND_LOOP) || (frame->armSeen == MAREA_BOOLEAN_FALSE)) {
        MAREA_G_Load(context, frame->base, frame->partBase, frame->count);

        return;
    }

    for (MAREA_IntegerSize index = 0; index < frame->count; ++index) {
        const MAREA_Boolean MERGED = (frame->hasFallback == MAREA_BOOLEAN_TRUE) ? frame->merged[index] : (((frame->merged[index] == MAREA_BOOLEAN_TRUE) || (frame->base[index] == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE);

        context->locals.entries[index].moved = MERGED;
        context->locals.entries[index].partAt = ((frame->hasFallback == MAREA_BOOLEAN_TRUE) || (MAREA_G_PartsAgree(frame->partMerged[index], frame->partBase[index]) == MAREA_BOOLEAN_TRUE)) ? frame->partMerged[index] : MAREA_NULLPTR;
    }
}
