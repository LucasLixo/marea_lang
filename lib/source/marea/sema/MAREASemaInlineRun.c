// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <string.h>

#define MAREA_G_ARRAY  MAREA_UINTEGER_8(1)
#define MAREA_G_VECTOR MAREA_UINTEGER_8(2)
#define MAREA_G_BOTH   MAREA_UINTEGER_8(3)

typedef struct MAREA_G_MemberRow
{
    const MAREA_Character8 *name;
    MAREA_IntegerSize argumentCount;
    MAREA_SemaInlineRunMember member;
    MAREA_Boolean isStatic;
    MAREA_Uinteger8 runs;
} MAREA_G_MemberRow;

static const MAREA_G_MemberRow MAREA_G_MEMBERS[] = {
    { "front", 0, MAREA_SEMA_INLINE_RUN_MEMBER_FRONT, MAREA_BOOLEAN_FALSE, MAREA_G_BOTH },
    { "back", 0, MAREA_SEMA_INLINE_RUN_MEMBER_BACK, MAREA_BOOLEAN_FALSE, MAREA_G_BOTH },
    { "size", 0, MAREA_SEMA_INLINE_RUN_MEMBER_SIZE, MAREA_BOOLEAN_FALSE, MAREA_G_BOTH },
    { "max_size", 0, MAREA_SEMA_INLINE_RUN_MEMBER_MAX_SIZE, MAREA_BOOLEAN_FALSE, MAREA_G_BOTH },
    { "empty", 0, MAREA_SEMA_INLINE_RUN_MEMBER_EMPTY, MAREA_BOOLEAN_FALSE, MAREA_G_BOTH },
    { "fill", 1, MAREA_SEMA_INLINE_RUN_MEMBER_FILL, MAREA_BOOLEAN_FALSE, MAREA_G_BOTH },
    { "swap", 1, MAREA_SEMA_INLINE_RUN_MEMBER_SWAP, MAREA_BOOLEAN_FALSE, MAREA_G_BOTH },
    { "filled", 1, MAREA_SEMA_INLINE_RUN_MEMBER_FILLED, MAREA_BOOLEAN_TRUE, MAREA_G_ARRAY },
    { "capacity", 0, MAREA_SEMA_INLINE_RUN_MEMBER_CAPACITY, MAREA_BOOLEAN_FALSE, MAREA_G_VECTOR },
    { "reserve", 1, MAREA_SEMA_INLINE_RUN_MEMBER_RESERVE, MAREA_BOOLEAN_FALSE, MAREA_G_VECTOR },
    { "shrink_to_fit", 0, MAREA_SEMA_INLINE_RUN_MEMBER_SHRINK_TO_FIT, MAREA_BOOLEAN_FALSE, MAREA_G_VECTOR },
    { "append", 1, MAREA_SEMA_INLINE_RUN_MEMBER_APPEND, MAREA_BOOLEAN_FALSE, MAREA_G_VECTOR },
    { "pop", 0, MAREA_SEMA_INLINE_RUN_MEMBER_POP, MAREA_BOOLEAN_FALSE, MAREA_G_VECTOR },
    { "insert", 2, MAREA_SEMA_INLINE_RUN_MEMBER_INSERT, MAREA_BOOLEAN_FALSE, MAREA_G_VECTOR },
    { "erase", 1, MAREA_SEMA_INLINE_RUN_MEMBER_ERASE, MAREA_BOOLEAN_FALSE, MAREA_G_VECTOR },
    { "clear", 0, MAREA_SEMA_INLINE_RUN_MEMBER_CLEAR, MAREA_BOOLEAN_FALSE, MAREA_G_VECTOR },
    { "resize", 2, MAREA_SEMA_INLINE_RUN_MEMBER_RESIZE, MAREA_BOOLEAN_FALSE, MAREA_G_VECTOR },
    { "data", 0, MAREA_SEMA_INLINE_RUN_MEMBER_DATA, MAREA_BOOLEAN_FALSE, MAREA_G_VECTOR },
};

MAREA_Boolean MAREA_CALL MAREA_SemaInlineRunOfType(const MAREA_SemaContext *context, const MAREA_AstNode *type, MAREA_SemaInlineRun *outRun)
{
    *outRun = (MAREA_SemaInlineRun){ .type = type, .element = MAREA_NULLPTR, .count = 0, .hasCount = MAREA_BOOLEAN_FALSE, .isGrowable = MAREA_BOOLEAN_FALSE };

    const MAREA_TypeDescriptor *descriptor = MAREA_NULLPTR;

    // INFO: sema knows no library type name: a run is the shape that the descriptor gives.
    if ((MAREA_SemaDescriptorOfType(context, type, &descriptor) == MAREA_BOOLEAN_FALSE) || (descriptor->hasIndexProtocol == MAREA_BOOLEAN_FALSE) || (descriptor->roles != MAREA_TYPE_ROLE_NONE)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_Boolean IS_INLINE = ((descriptor->ownershipClass == MAREA_TYPE_OWNERSHIP_CLASS_COPY) && (descriptor->typeParameterCount == MAREA_INTEGER_SIZE(2))) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    outRun->isGrowable = ((descriptor->ownershipClass == MAREA_TYPE_OWNERSHIP_CLASS_MOVE) && (descriptor->typeParameterCount == MAREA_INTEGER_SIZE(1))) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    if ((IS_INLINE == MAREA_BOOLEAN_FALSE) && (outRun->isGrowable == MAREA_BOOLEAN_FALSE)) {
        return MAREA_BOOLEAN_FALSE;
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(type); ++index) {
        const MAREA_AstNode *arguments = MAREA_SemaChildAt(type, index);

        if ((arguments == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(arguments) != MAREA_AST_KIND_TYPE_ARGUMENTS)) {
            continue;
        }

        outRun->element = MAREA_SemaChildAt(arguments, 0);
        outRun->hasCount = (outRun->isGrowable == MAREA_BOOLEAN_FALSE) ? MAREA_SemaReadIntegerLiteral(MAREA_SemaChildAt(arguments, 1), &outRun->count) : MAREA_BOOLEAN_FALSE;
    }

    return MAREA_BOOLEAN_TRUE;
}

MAREA_Boolean MAREA_CALL MAREA_SemaInlineRunOfValue(const MAREA_SemaContext *context, const MAREA_AstNode *expression, MAREA_SemaWrittenType *outWritten, MAREA_SemaInlineRun *outRun)
{
    if (MAREA_SemaWrittenTypeOf(context, expression, outWritten) == MAREA_BOOLEAN_FALSE) {
        return MAREA_BOOLEAN_FALSE;
    }

    MAREA_SemaResolveWrittenType(context, outWritten);

    return MAREA_SemaInlineRunOfType(context, outWritten->type, outRun);
}

MAREA_SemaInlineRunMember MAREA_CALL MAREA_SemaInlineRunMemberOf(const MAREA_AstNode *name, MAREA_Boolean isStatic, MAREA_Boolean isGrowable, MAREA_IntegerSize *outArgumentCount)
{
    const MAREA_Uinteger8 RUN = (isGrowable == MAREA_BOOLEAN_TRUE) ? MAREA_G_VECTOR : MAREA_G_ARRAY;

    for (MAREA_IntegerSize index = 0; index < MAREA_ARRAY_SIZE(MAREA_G_MEMBERS); ++index) {
        const MAREA_G_MemberRow *row = &MAREA_G_MEMBERS[index];

        if ((row->isStatic == isStatic) && ((row->runs & RUN) != 0) && (MAREA_SemaTextEquals(MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name), row->name, strlen(row->name)) == MAREA_BOOLEAN_TRUE)) {
            if (outArgumentCount != MAREA_NULLPTR) {
                *outArgumentCount = row->argumentCount;
            }

            return row->member;
        }
    }

    return MAREA_SEMA_INLINE_RUN_MEMBER_NONE;
}

MAREA_Boolean MAREA_CALL MAREA_SemaInlineRunCallOf(const MAREA_SemaContext *context, const MAREA_AstNode *call, MAREA_SemaInlineRunCall *outCall)
{
    const MAREA_AstNode *callee = MAREA_SemaChildAt(call, 0);

    if ((callee == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(callee) != MAREA_AST_KIND_MEMBER_ACCESS)) {
        return MAREA_BOOLEAN_FALSE;
    }

    outCall->receiver = MAREA_SemaChildAt(callee, 0);
    outCall->name = MAREA_SemaChildAt(callee, 1);
    outCall->member = MAREA_SEMA_INLINE_RUN_MEMBER_NONE;
    outCall->argumentCount = 0;
    outCall->written.type = MAREA_NULLPTR;
    outCall->written.bindings.count = 0;

    if ((outCall->receiver == MAREA_NULLPTR) || (outCall->name == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(outCall->name) != MAREA_AST_KIND_IDENTIFIER)) {
        return MAREA_BOOLEAN_FALSE;
    }

    // INFO: a receiver that is the type itself calls a static function, any other receiver is a value.
    outCall->isStatic = MAREA_SemaInlineRunOfType(context, outCall->receiver, &outCall->run);

    if (outCall->isStatic == MAREA_BOOLEAN_TRUE) {
        outCall->written.type = outCall->receiver;
    } else if (MAREA_SemaInlineRunOfValue(context, outCall->receiver, &outCall->written, &outCall->run) == MAREA_BOOLEAN_FALSE) {
        return MAREA_BOOLEAN_FALSE;
    }

    outCall->member = MAREA_SemaInlineRunMemberOf(outCall->name, outCall->isStatic, outCall->run.isGrowable, &outCall->argumentCount);

    return MAREA_BOOLEAN_TRUE;
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_Report(MAREA_SemaContext *context, MAREA_SemaDiagnosticCode code, const MAREA_AstNode *at, const MAREA_Character8 *prefix, const MAREA_AstNode *name)
{
    return MAREA_SemaReportNamed(context, code, MAREA_AstNodeGetPosition(at), prefix, MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name));
}

// INFO: the member writes its receiver or its argument, so the place needs storage of this function that is not a read-only borrow.
MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckWritablePlace(MAREA_SemaContext *context, const MAREA_AstNode *place, const MAREA_AstNode *name)
{
    const MAREA_AstNode *root = place;

    while ((root != MAREA_NULLPTR) && ((MAREA_AstNodeGetKind(root) == MAREA_AST_KIND_MEMBER_ACCESS) || (MAREA_AstNodeGetKind(root) == MAREA_AST_KIND_INDEX_ACCESS))) {
        root = MAREA_SemaChildAt(root, 0);
    }

    const MAREA_Boolean IS_LOCAL = ((root != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(root) == MAREA_AST_KIND_IDENTIFIER) && (MAREA_SemaLocalFind(context, MAREA_AstNodeGetText(root), MAREA_AstNodeGetTextLength(root)) != MAREA_NULLPTR)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    if ((IS_LOCAL == MAREA_BOOLEAN_TRUE) && (MAREA_SemaIsReadOnly(context, place) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_G_Report(context, MAREA_SEMA_DIAGNOSTIC_CODE_WRITE_THROUGH_READ_ONLY_BORROW, place, "a value that cannot be written is changed by", name);
}

// INFO: a value argument of the element type: the same fit and range rules as any other value of a written type.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckElementValue(MAREA_SemaContext *context, const MAREA_SemaInlineRunCall *call, const MAREA_AstNode *value)
{
    MAREA_SemaWrittenType target = call->written;

    target.type = call->run.element;

    const MAREA_SemaExcept FIT = MAREA_SemaCheckWrittenFit(context, &target, value);

    return (FIT != MAREA_SEMA_EXCEPT_SUCCESS) ? FIT : MAREA_SemaCheckLiteralRange(context, call->run.element, value);
}

// INFO: a stored value goes into the run, so an existing value of a type that is not copied needs copy or move, as in an assignment.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckStoredValue(MAREA_SemaContext *context, const MAREA_SemaInlineRunCall *call, const MAREA_AstNode *value)
{
    const MAREA_SemaExcept RESULT = MAREA_G_CheckElementValue(context, call, value);

    const MAREA_AstKind KIND = (value == MAREA_NULLPTR) ? MAREA_AST_KIND_ERROR_NODE : MAREA_AstNodeGetKind(value);

    const MAREA_Boolean EXISTING = ((KIND == MAREA_AST_KIND_IDENTIFIER) || (KIND == MAREA_AST_KIND_MEMBER_ACCESS) || (KIND == MAREA_AST_KIND_INDEX_ACCESS)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    // INFO: a borrow that is stored is already reported as a reference that escapes.
    const MAREA_Boolean BORROW = ((KIND == MAREA_AST_KIND_IDENTIFIER) && (MAREA_SemaIsReference(context, MAREA_AstNodeGetText(value), MAREA_AstNodeGetTextLength(value)) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    if ((RESULT != MAREA_SEMA_EXCEPT_SUCCESS) || (EXISTING == MAREA_BOOLEAN_FALSE) || (BORROW == MAREA_BOOLEAN_TRUE) || (call->run.element == MAREA_NULLPTR) || (call->written.bindings.count != 0) || (MAREA_SemaTypeIsByValue(context, call->run.element, MAREA_BOOLEAN_TRUE) == MAREA_BOOLEAN_TRUE)) {
        return RESULT;
    }

    MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_ASSIGNMENT_NEEDS_COPY_OR_MOVE, MAREA_AstNodeGetPosition(value), "an existing value that is stored needs copy or move");

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

static MAREA_Boolean MAREA_CALL MAREA_G_ChangesReceiver(MAREA_SemaInlineRunMember member)
{
    switch (member) {
    case MAREA_SEMA_INLINE_RUN_MEMBER_FILL:
    case MAREA_SEMA_INLINE_RUN_MEMBER_SWAP:
    case MAREA_SEMA_INLINE_RUN_MEMBER_RESERVE:
    case MAREA_SEMA_INLINE_RUN_MEMBER_SHRINK_TO_FIT:
    case MAREA_SEMA_INLINE_RUN_MEMBER_APPEND:
    case MAREA_SEMA_INLINE_RUN_MEMBER_POP:
    case MAREA_SEMA_INLINE_RUN_MEMBER_INSERT:
    case MAREA_SEMA_INLINE_RUN_MEMBER_ERASE:
    case MAREA_SEMA_INLINE_RUN_MEMBER_CLEAR:
    case MAREA_SEMA_INLINE_RUN_MEMBER_RESIZE:
        return MAREA_BOOLEAN_TRUE;
    default:
        return MAREA_BOOLEAN_FALSE;
    }
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckSwapArgument(MAREA_SemaContext *context, const MAREA_SemaInlineRunCall *call, const MAREA_AstNode *node, const MAREA_AstNode *argument)
{
    MAREA_SemaWrittenType written = { .type = MAREA_NULLPTR, .bindings = { .count = 0 } };

    MAREA_SemaInlineRun other = { 0 };

    if (MAREA_SemaInlineRunOfValue(context, argument, &written, &other) == MAREA_BOOLEAN_FALSE) {
        return (written.type == MAREA_NULLPTR) ? MAREA_SEMA_EXCEPT_SUCCESS : MAREA_G_Report(context, MAREA_SEMA_DIAGNOSTIC_CODE_TYPE_MISMATCH, argument, "an array is needed by", call->name);
    }

    if ((call->run.isGrowable != other.isGrowable) || ((call->run.hasCount == MAREA_BOOLEAN_TRUE) && (other.hasCount == MAREA_BOOLEAN_TRUE) && (call->run.count != other.count)) || (MAREA_SemaTypeArgumentsDiffer(context, &call->written, argument) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_G_Report(context, MAREA_SEMA_DIAGNOSTIC_CODE_GENERIC_ARGUMENT_MISMATCH, argument, "the two arrays of a swap differ in their type arguments:", call->name);
    }

    if (MAREA_SemaPlacesAlias(call->receiver, argument) == MAREA_BOOLEAN_TRUE) {
        return MAREA_G_Report(context, MAREA_SEMA_DIAGNOSTIC_CODE_EXCLUSIVE_BORROW_CONFLICT, node, "two write borrows of one value are passed to", call->name);
    }

    return MAREA_SemaCheckWritablePlace(context, argument, call->name);
}

MAREA_Boolean MAREA_CALL MAREA_SemaInlineRunIsShipped(const MAREA_SemaContext *context, const MAREA_AstNode *type)
{
    const MAREA_TypeDescriptor *descriptor = MAREA_NULLPTR;

    return ((MAREA_SemaDescriptorOfType(context, type, &descriptor) == MAREA_BOOLEAN_TRUE) && (descriptor->namespaceLength > MAREA_INTEGER_SIZE(6)) && (memcmp(descriptor->namespaceText, "marea.", 6) == 0)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckInlineRunCall(MAREA_SemaContext *context, const MAREA_AstNode *node, MAREA_Boolean *outHandled)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR) || (outHandled == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    MAREA_SemaInlineRunCall call;

    *outHandled = MAREA_SemaInlineRunCallOf(context, node, &call);

    if (*outHandled == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    // INFO: the compiler gives every member of a type that the language ships, so another name does not exist. The
    // members of a type that a host registers may come from its signatures.
    if ((call.member == MAREA_SEMA_INLINE_RUN_MEMBER_NONE) && (MAREA_SemaInlineRunIsShipped(context, call.run.type) == MAREA_BOOLEAN_FALSE)) {
        *outHandled = MAREA_BOOLEAN_FALSE;

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    if (call.member == MAREA_SEMA_INLINE_RUN_MEMBER_NONE) {
        // INFO: sum is the intrinsic of the example of the vector types, which no specification describes yet.
        if (MAREA_SemaTextEquals(MAREA_AstNodeGetText(call.name), MAREA_AstNodeGetTextLength(call.name), "sum", strlen("sum")) == MAREA_BOOLEAN_TRUE) {
            return MAREA_SEMA_EXCEPT_SUCCESS;
        }

        return MAREA_G_Report(context, MAREA_SEMA_DIAGNOSTIC_CODE_UNKNOWN_TYPE_FUNCTION, node, "type has no function:", call.name);
    }

    const MAREA_AstNode *arguments = MAREA_SemaChildAt(node, 1);

    if (((arguments == MAREA_NULLPTR) ? 0 : MAREA_AstNodeGetChildCount(arguments)) != call.argumentCount) {
        return MAREA_G_Report(context, MAREA_SEMA_DIAGNOSTIC_CODE_ARGUMENT_COUNT_MISMATCH, node, "wrong number of arguments in the call of", call.name);
    }

    const MAREA_Boolean COPIES = ((call.member == MAREA_SEMA_INLINE_RUN_MEMBER_FRONT) || (call.member == MAREA_SEMA_INLINE_RUN_MEMBER_BACK) || (call.member == MAREA_SEMA_INLINE_RUN_MEMBER_FILL) || (call.member == MAREA_SEMA_INLINE_RUN_MEMBER_FILLED) || (call.member == MAREA_SEMA_INLINE_RUN_MEMBER_RESIZE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    if ((COPIES == MAREA_BOOLEAN_TRUE) && (call.run.element != MAREA_NULLPTR) && (call.written.bindings.count == 0) && (MAREA_SemaIsCopyable(context, call.run.element) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_G_Report(context, MAREA_SEMA_DIAGNOSTIC_CODE_CONTRACT_NOT_SATISFIED, node, "an element type that is not Copyable is copied by", call.name);
    }

    if ((call.member == MAREA_SEMA_INLINE_RUN_MEMBER_DATA) && (context->unsafeDepth == 0)) {
        return MAREA_G_Report(context, MAREA_SEMA_DIAGNOSTIC_CODE_RAW_POINTER_OUTSIDE_UNSAFE, node, "a raw pointer exists only inside an unsafe block, so it is not given by", call.name);
    }

    if (MAREA_G_ChangesReceiver(call.member) == MAREA_BOOLEAN_TRUE) {
        const MAREA_SemaExcept WRITABLE = MAREA_SemaCheckWritablePlace(context, call.receiver, call.name);

        if (WRITABLE != MAREA_SEMA_EXCEPT_SUCCESS) {
            return WRITABLE;
        }
    }

    if (call.member == MAREA_SEMA_INLINE_RUN_MEMBER_SWAP) {
        return MAREA_G_CheckSwapArgument(context, &call, node, MAREA_SemaChildAt(arguments, 0));
    }

    if ((call.member == MAREA_SEMA_INLINE_RUN_MEMBER_FILL) || (call.member == MAREA_SEMA_INLINE_RUN_MEMBER_FILLED)) {
        return MAREA_G_CheckElementValue(context, &call, MAREA_SemaChildAt(arguments, 0));
    }

    if (call.member == MAREA_SEMA_INLINE_RUN_MEMBER_RESIZE) {
        return MAREA_G_CheckElementValue(context, &call, MAREA_SemaChildAt(arguments, 1));
    }

    if (call.member == MAREA_SEMA_INLINE_RUN_MEMBER_APPEND) {
        return MAREA_G_CheckStoredValue(context, &call, MAREA_SemaChildAt(arguments, 0));
    }

    return (call.member == MAREA_SEMA_INLINE_RUN_MEMBER_INSERT) ? MAREA_G_CheckStoredValue(context, &call, MAREA_SemaChildAt(arguments, 1)) : MAREA_SEMA_EXCEPT_SUCCESS;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckGrowableRunCopy(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    if (MAREA_AstNodeGetOperator(node) != MAREA_TOKEN_KIND_KEYWORD_COPY) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    MAREA_SemaWrittenType written = { .type = MAREA_NULLPTR, .bindings = { .count = 0 } };

    MAREA_SemaInlineRun run = { 0 };

    const MAREA_Boolean IS_RUN = MAREA_SemaInlineRunOfValue(context, MAREA_SemaChildAt(node, 0), &written, &run);

    // INFO: a tuple is copied element by element, so each of its elements has to be Copyable.
    if ((written.type != MAREA_NULLPTR) && (written.bindings.count == 0) && (MAREA_SemaTupleOfType(context, written.type, MAREA_NULLPTR) == MAREA_BOOLEAN_TRUE)) {
        if (MAREA_SemaTupleIsCopyable(context, written.type) == MAREA_BOOLEAN_FALSE) {
            MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_CONTRACT_NOT_SATISFIED, MAREA_AstNodeGetPosition(node), "a copy of a tuple needs every element type to be Copyable");
        }

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    if ((IS_RUN == MAREA_BOOLEAN_FALSE) || (run.isGrowable == MAREA_BOOLEAN_FALSE) || (run.element == MAREA_NULLPTR) || (written.bindings.count != 0) || (MAREA_SemaIsCopyable(context, run.element) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_CONTRACT_NOT_SATISFIED, MAREA_AstNodeGetPosition(node), "a copy needs an element type that is Copyable");

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_ReportIndex(MAREA_SemaContext *context, const MAREA_AstNode *at)
{
    MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_CONSTANT_INDEX_OUT_OF_RANGE, MAREA_AstNodeGetPosition(at), "a constant index is not below the count of the array");

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

// INFO: an integer literal at or past the count, or a negative one, can never be an index.
static MAREA_Boolean MAREA_CALL MAREA_G_IsPastCount(const MAREA_AstNode *key, MAREA_Uinteger64 count)
{
    MAREA_Uinteger64 value = 0;

    if (MAREA_SemaReadIntegerLiteral(key, &value) == MAREA_BOOLEAN_TRUE) {
        return (value >= count) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    }

    return ((key != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(key) == MAREA_AST_KIND_UNARY_EXPRESSION) && (MAREA_AstNodeGetOperator(key) == MAREA_TOKEN_KIND_MINUS) && (MAREA_SemaReadIntegerLiteral(MAREA_SemaChildAt(key, 0), &value) == MAREA_BOOLEAN_TRUE) && (value > 0)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckInlineRunIndex(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    MAREA_SemaWrittenType written = { .type = MAREA_NULLPTR, .bindings = { .count = 0 } };

    MAREA_SemaInlineRun run = { 0 };

    if ((MAREA_SemaInlineRunOfValue(context, MAREA_SemaChildAt(node, 0), &written, &run) == MAREA_BOOLEAN_FALSE) || (run.hasCount == MAREA_BOOLEAN_FALSE) || (MAREA_G_IsPastCount(MAREA_SemaChildAt(node, 1), run.count) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_G_ReportIndex(context, node);
}

// INFO: the positional values of a literal fill the whole run; a literal with no positional value zero-fills it, as does one that writes its indexes.
MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckInlineRunLiteral(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    MAREA_SemaWrittenType written = { .type = MAREA_SemaChildAt(node, 0), .bindings = { .count = 0 } };

    MAREA_SemaResolveWrittenType(context, &written);

    MAREA_SemaInlineRun run = { 0 };

    if ((MAREA_SemaInlineRunOfType(context, written.type, &run) == MAREA_BOOLEAN_FALSE) || (run.hasCount == MAREA_BOOLEAN_FALSE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    MAREA_Uinteger64 positional = 0;

    MAREA_Boolean indexed = MAREA_BOOLEAN_FALSE;

    for (MAREA_IntegerSize index = 1; index < MAREA_AstNodeGetChildCount(node); ++index) {
        const MAREA_AstNode *entry = MAREA_SemaChildAt(node, index);

        if ((entry != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(entry) == MAREA_AST_KIND_VALUE_ENTRY)) {
            ++positional;
        } else if ((entry != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(entry) == MAREA_AST_KIND_INDEX_ENTRY)) {
            indexed = MAREA_BOOLEAN_TRUE;

            if (MAREA_G_IsPastCount(MAREA_SemaChildAt(entry, 0), run.count) == MAREA_BOOLEAN_TRUE) {
                const MAREA_SemaExcept RESULT = MAREA_G_ReportIndex(context, entry);

                if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
                    return RESULT;
                }
            }
        }
    }

    if ((positional > run.count) || ((indexed == MAREA_BOOLEAN_FALSE) && (positional > 0) && (positional != run.count))) {
        MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_LITERAL_COUNT_MISMATCH, MAREA_AstNodeGetPosition(node), "the values of an array literal differ from the count of the array");
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}
