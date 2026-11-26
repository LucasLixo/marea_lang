// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <string.h>

typedef struct MAREA_G_MemberRow
{
    const MAREA_Character8 *name;
    MAREA_IntegerSize argumentCount;
    MAREA_SemaInlineRunMember member;
    MAREA_Boolean isStatic;
} MAREA_G_MemberRow;

static const MAREA_G_MemberRow MAREA_G_MEMBERS[] = {
    { "front", 0, MAREA_SEMA_INLINE_RUN_MEMBER_FRONT, MAREA_BOOLEAN_FALSE },
    { "back", 0, MAREA_SEMA_INLINE_RUN_MEMBER_BACK, MAREA_BOOLEAN_FALSE },
    { "size", 0, MAREA_SEMA_INLINE_RUN_MEMBER_SIZE, MAREA_BOOLEAN_FALSE },
    { "max_size", 0, MAREA_SEMA_INLINE_RUN_MEMBER_MAX_SIZE, MAREA_BOOLEAN_FALSE },
    { "empty", 0, MAREA_SEMA_INLINE_RUN_MEMBER_EMPTY, MAREA_BOOLEAN_FALSE },
    { "fill", 1, MAREA_SEMA_INLINE_RUN_MEMBER_FILL, MAREA_BOOLEAN_FALSE },
    { "swap", 1, MAREA_SEMA_INLINE_RUN_MEMBER_SWAP, MAREA_BOOLEAN_FALSE },
    { "filled", 1, MAREA_SEMA_INLINE_RUN_MEMBER_FILLED, MAREA_BOOLEAN_TRUE },
};

MAREA_Boolean MAREA_CALL MAREA_SemaInlineRunOfType(const MAREA_SemaContext *context, const MAREA_AstNode *type, MAREA_SemaInlineRun *outRun)
{
    *outRun = (MAREA_SemaInlineRun){ .type = type, .element = MAREA_NULLPTR, .count = 0, .hasCount = MAREA_BOOLEAN_FALSE };

    const MAREA_TypeDescriptor *descriptor = MAREA_NULLPTR;

    // INFO: sema knows no library type name: an inline run is the shape that the descriptor gives.
    if ((MAREA_SemaDescriptorOfType(context, type, &descriptor) == MAREA_BOOLEAN_FALSE) || (descriptor->hasIndexProtocol == MAREA_BOOLEAN_FALSE) || (descriptor->roles != MAREA_TYPE_ROLE_NONE) || (descriptor->ownershipClass != MAREA_TYPE_OWNERSHIP_CLASS_COPY) || (descriptor->typeParameterCount != MAREA_INTEGER_SIZE(2))) {
        return MAREA_BOOLEAN_FALSE;
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(type); ++index) {
        const MAREA_AstNode *arguments = MAREA_SemaChildAt(type, index);

        if ((arguments == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(arguments) != MAREA_AST_KIND_TYPE_ARGUMENTS)) {
            continue;
        }

        outRun->element = MAREA_SemaChildAt(arguments, 0);
        outRun->hasCount = MAREA_SemaReadIntegerLiteral(MAREA_SemaChildAt(arguments, 1), &outRun->count);
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

    for (MAREA_IntegerSize index = 0; index < MAREA_ARRAY_SIZE(MAREA_G_MEMBERS); ++index) {
        const MAREA_G_MemberRow *row = &MAREA_G_MEMBERS[index];

        if ((row->isStatic == outCall->isStatic) && (MAREA_SemaTextEquals(MAREA_AstNodeGetText(outCall->name), MAREA_AstNodeGetTextLength(outCall->name), row->name, strlen(row->name)) == MAREA_BOOLEAN_TRUE)) {
            outCall->member = row->member;
            outCall->argumentCount = row->argumentCount;
        }
    }

    return MAREA_BOOLEAN_TRUE;
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_Report(MAREA_SemaContext *context, MAREA_SemaDiagnosticCode code, const MAREA_AstNode *at, const MAREA_Character8 *prefix, const MAREA_AstNode *name)
{
    return MAREA_SemaReportNamed(context, code, MAREA_AstNodeGetPosition(at), prefix, MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name));
}

// INFO: the member writes its receiver or its argument, so the place needs storage of this function that is not a read-only borrow.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckWritable(MAREA_SemaContext *context, const MAREA_AstNode *place, const MAREA_AstNode *name)
{
    const MAREA_AstNode *root = place;

    while ((root != MAREA_NULLPTR) && ((MAREA_AstNodeGetKind(root) == MAREA_AST_KIND_MEMBER_ACCESS) || (MAREA_AstNodeGetKind(root) == MAREA_AST_KIND_INDEX_ACCESS))) {
        root = MAREA_SemaChildAt(root, 0);
    }

    const MAREA_Boolean IS_LOCAL = ((root != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(root) == MAREA_AST_KIND_IDENTIFIER) && (MAREA_SemaLocalFind(context, MAREA_AstNodeGetText(root), MAREA_AstNodeGetTextLength(root)) != MAREA_NULLPTR)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    if ((IS_LOCAL == MAREA_BOOLEAN_TRUE) && (MAREA_SemaIsReadOnly(context, place) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_G_Report(context, MAREA_SEMA_DIAGNOSTIC_CODE_WRITE_THROUGH_READ_ONLY_BORROW, place, "an array that cannot be written is changed by", name);
}

// INFO: a value argument of the element type: the same fit and range rules as any other value of a written type.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckElementValue(MAREA_SemaContext *context, const MAREA_SemaInlineRunCall *call, const MAREA_AstNode *value)
{
    MAREA_SemaWrittenType target = call->written;

    target.type = call->run.element;

    const MAREA_SemaExcept FIT = MAREA_SemaCheckWrittenFit(context, &target, value);

    return (FIT != MAREA_SEMA_EXCEPT_SUCCESS) ? FIT : MAREA_SemaCheckLiteralRange(context, call->run.element, value);
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckSwapArgument(MAREA_SemaContext *context, const MAREA_SemaInlineRunCall *call, const MAREA_AstNode *node, const MAREA_AstNode *argument)
{
    MAREA_SemaWrittenType written = { .type = MAREA_NULLPTR, .bindings = { .count = 0 } };

    MAREA_SemaInlineRun other = { 0 };

    if (MAREA_SemaInlineRunOfValue(context, argument, &written, &other) == MAREA_BOOLEAN_FALSE) {
        return (written.type == MAREA_NULLPTR) ? MAREA_SEMA_EXCEPT_SUCCESS : MAREA_G_Report(context, MAREA_SEMA_DIAGNOSTIC_CODE_TYPE_MISMATCH, argument, "an array is needed by", call->name);
    }

    if (((call->run.hasCount == MAREA_BOOLEAN_TRUE) && (other.hasCount == MAREA_BOOLEAN_TRUE) && (call->run.count != other.count)) || (MAREA_SemaTypeArgumentsDiffer(context, &call->written, argument) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_G_Report(context, MAREA_SEMA_DIAGNOSTIC_CODE_GENERIC_ARGUMENT_MISMATCH, argument, "the two arrays of a swap differ in their type arguments:", call->name);
    }

    if (MAREA_SemaPlacesAlias(call->receiver, argument) == MAREA_BOOLEAN_TRUE) {
        return MAREA_G_Report(context, MAREA_SEMA_DIAGNOSTIC_CODE_EXCLUSIVE_BORROW_CONFLICT, node, "two write borrows of one value are passed to", call->name);
    }

    return MAREA_G_CheckWritable(context, argument, call->name);
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsShipped(const MAREA_SemaContext *context, const MAREA_AstNode *type)
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
    if ((call.member == MAREA_SEMA_INLINE_RUN_MEMBER_NONE) && (MAREA_G_IsShipped(context, call.run.type) == MAREA_BOOLEAN_FALSE)) {
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

    const MAREA_Boolean COPIES = ((call.member == MAREA_SEMA_INLINE_RUN_MEMBER_FRONT) || (call.member == MAREA_SEMA_INLINE_RUN_MEMBER_BACK) || (call.member == MAREA_SEMA_INLINE_RUN_MEMBER_FILL) || (call.member == MAREA_SEMA_INLINE_RUN_MEMBER_FILLED)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    if ((COPIES == MAREA_BOOLEAN_TRUE) && (call.run.element != MAREA_NULLPTR) && (call.written.bindings.count == 0) && (MAREA_SemaIsCopyable(context, call.run.element) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_G_Report(context, MAREA_SEMA_DIAGNOSTIC_CODE_CONTRACT_NOT_SATISFIED, node, "an element type that is not Copyable is copied by", call.name);
    }

    if ((call.member == MAREA_SEMA_INLINE_RUN_MEMBER_FILL) || (call.member == MAREA_SEMA_INLINE_RUN_MEMBER_SWAP)) {
        const MAREA_SemaExcept WRITABLE = MAREA_G_CheckWritable(context, call.receiver, call.name);

        if (WRITABLE != MAREA_SEMA_EXCEPT_SUCCESS) {
            return WRITABLE;
        }
    }

    if (call.member == MAREA_SEMA_INLINE_RUN_MEMBER_SWAP) {
        return MAREA_G_CheckSwapArgument(context, &call, node, MAREA_SemaChildAt(arguments, 0));
    }

    return (call.argumentCount == 1) ? MAREA_G_CheckElementValue(context, &call, MAREA_SemaChildAt(arguments, 0)) : MAREA_SEMA_EXCEPT_SUCCESS;
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
