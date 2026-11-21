// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/base/MAREADiagnostic.h"

#include <string.h>

#if !defined(MAREA_DIAGNOSTIC_SINK_INITIAL_CAPACITY)
    #define MAREA_DIAGNOSTIC_SINK_INITIAL_CAPACITY MAREA_INTEGER_SIZE(16)
#endif

typedef struct MAREA_Diagnostic
{
    MAREA_SourcePosition position;
    const MAREA_Character8 *message;
    MAREA_Uinteger32 code;
    MAREA_DiagnosticSeverity severity;
} MAREA_Diagnostic;

typedef struct MAREA_DiagnosticSink
{
    MAREA_Arena *arena;
    MAREA_Diagnostic *items;
    MAREA_IntegerSize count;
    MAREA_IntegerSize capacity;
} MAREA_DiagnosticSink;

MAREA_DiagnosticExcept MAREA_CALL MAREA_DiagnosticSinkCreate(MAREA_Arena *arena, MAREA_DiagnosticSink **outSink)
{
    if ((arena == MAREA_NULLPTR) || (outSink == MAREA_NULLPTR)) {
        return MAREA_DIAGNOSTIC_EXCEPT_ERROR_NULL_POINTER;
    }

    MAREA_Null sinkMemory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(arena, sizeof(MAREA_DiagnosticSink), MAREA_ALIGNOF(MAREA_DiagnosticSink), &sinkMemory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_DIAGNOSTIC_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_DiagnosticSink *sink = sinkMemory;

    sink->arena = arena;
    sink->items = MAREA_NULLPTR;
    sink->count = MAREA_INTEGER_SIZE(0);
    sink->capacity = MAREA_INTEGER_SIZE(0);

    *outSink = sink;

    return MAREA_DIAGNOSTIC_EXCEPT_SUCCESS;
}

static MAREA_DiagnosticExcept MAREA_CALL MAREA_G_DiagnosticSinkGrow(MAREA_DiagnosticSink *sink)
{
    const MAREA_IntegerSize newCapacity = (sink->capacity == MAREA_INTEGER_SIZE(0)) ? MAREA_DIAGNOSTIC_SINK_INITIAL_CAPACITY : (sink->capacity * MAREA_INTEGER_SIZE(2));

    MAREA_Null newItemsMemory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(sink->arena, (newCapacity * sizeof(MAREA_Diagnostic)), MAREA_ALIGNOF(MAREA_Diagnostic), &newItemsMemory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_DIAGNOSTIC_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_Diagnostic *newItems = newItemsMemory;

    if (sink->count > MAREA_INTEGER_SIZE(0)) {
        memcpy(newItems, sink->items, (sink->count * sizeof(MAREA_Diagnostic)));
    }

    sink->items = newItems;
    sink->capacity = newCapacity;

    return MAREA_DIAGNOSTIC_EXCEPT_SUCCESS;
}

MAREA_DiagnosticExcept MAREA_CALL MAREA_DiagnosticSinkAdd(MAREA_DiagnosticSink *sink, MAREA_Uinteger32 code, MAREA_DiagnosticSeverity severity, MAREA_SourcePosition position, const MAREA_Character8 *message)
{
    if ((sink == MAREA_NULLPTR) || (message == MAREA_NULLPTR)) {
        return MAREA_DIAGNOSTIC_EXCEPT_ERROR_NULL_POINTER;
    }

    if (sink->count == sink->capacity) {
        const MAREA_DiagnosticExcept growResult = MAREA_G_DiagnosticSinkGrow(sink);

        if (growResult != MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) {
            return growResult;
        }
    }

    const MAREA_IntegerSize messageLength = strlen(message);

    MAREA_Null messageMemory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(sink->arena, (messageLength + MAREA_INTEGER_SIZE(1)), MAREA_ALIGNOF(MAREA_Character8), &messageMemory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_DIAGNOSTIC_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    memcpy(messageMemory, message, (messageLength + MAREA_INTEGER_SIZE(1)));

    MAREA_Diagnostic *diagnostic = &sink->items[sink->count];

    diagnostic->position = position;
    diagnostic->message = messageMemory;
    diagnostic->code = code;
    diagnostic->severity = severity;

    sink->count = sink->count + MAREA_INTEGER_SIZE(1);

    return MAREA_DIAGNOSTIC_EXCEPT_SUCCESS;
}

MAREA_IntegerSize MAREA_CALL MAREA_DiagnosticSinkGetCount(const MAREA_DiagnosticSink *sink)
{
    if (sink == MAREA_NULLPTR) {
        return MAREA_INTEGER_SIZE(0);
    }

    return sink->count;
}

MAREA_IntegerSize MAREA_CALL MAREA_DiagnosticSinkGetErrorCount(const MAREA_DiagnosticSink *sink)
{
    if (sink == MAREA_NULLPTR) {
        return MAREA_INTEGER_SIZE(0);
    }

    MAREA_IntegerSize errorCount = MAREA_INTEGER_SIZE(0);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < sink->count; index = index + MAREA_INTEGER_SIZE(1)) {
        if (sink->items[index].severity == MAREA_DIAGNOSTIC_SEVERITY_ERROR) {
            errorCount = errorCount + MAREA_INTEGER_SIZE(1);
        }
    }

    return errorCount;
}

MAREA_DiagnosticExcept MAREA_CALL MAREA_DiagnosticSinkGetAt(const MAREA_DiagnosticSink *sink, MAREA_IntegerSize index, const MAREA_Diagnostic **out)
{
    if ((sink == MAREA_NULLPTR) || (out == MAREA_NULLPTR)) {
        return MAREA_DIAGNOSTIC_EXCEPT_ERROR_NULL_POINTER;
    }

    if (index >= sink->count) {
        return MAREA_DIAGNOSTIC_EXCEPT_ERROR_OUT_OF_RANGE;
    }

    *out = &sink->items[index];

    return MAREA_DIAGNOSTIC_EXCEPT_SUCCESS;
}

MAREA_Uinteger32 MAREA_CALL MAREA_DiagnosticGetCode(const MAREA_Diagnostic *diagnostic)
{
    if (diagnostic == MAREA_NULLPTR) {
        return MAREA_UINTEGER_32(0);
    }

    return diagnostic->code;
}

MAREA_DiagnosticSeverity MAREA_CALL MAREA_DiagnosticGetSeverity(const MAREA_Diagnostic *diagnostic)
{
    if (diagnostic == MAREA_NULLPTR) {
        return MAREA_DIAGNOSTIC_SEVERITY_ERROR;
    }

    return diagnostic->severity;
}

MAREA_SourcePosition MAREA_CALL MAREA_DiagnosticGetPosition(const MAREA_Diagnostic *diagnostic)
{
    if (diagnostic == MAREA_NULLPTR) {
        MAREA_SourcePosition zeroed = { MAREA_NULLPTR, MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0) };

        return zeroed;
    }

    return diagnostic->position;
}

const MAREA_Character8 *MAREA_CALL MAREA_DiagnosticGetMessage(const MAREA_Diagnostic *diagnostic)
{
    if (diagnostic == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    return diagnostic->message;
}
