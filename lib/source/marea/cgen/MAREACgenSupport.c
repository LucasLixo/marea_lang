// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/cgen/MAREACgenInternal.h"

#include <string.h>

MAREA_CgenExcept MAREA_CALL MAREA_CgenGeneratorFail(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_CgenDiagnosticCode code, const MAREA_Character8 *message)
{
    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    generator->functionFailed = MAREA_BOOLEAN_TRUE;

    const MAREA_SourcePosition POSITION = MAREA_IrNodeGetPosition(node);

    if (MAREA_DiagnosticSinkAdd(generator->sink, code, MAREA_DIAGNOSTIC_SEVERITY_ERROR, POSITION, message) != MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) {
        return MAREA_CGEN_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

const MAREA_CgenSymbol *MAREA_CALL MAREA_CgenFindSymbol(const MAREA_CgenGenerator *generator, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    for (MAREA_IntegerSize index = generator->symbolCount; index > MAREA_INTEGER_SIZE(0); index = (index - MAREA_INTEGER_SIZE(1))) {
        const MAREA_CgenSymbol *symbol = &generator->symbols[(index - MAREA_INTEGER_SIZE(1))];

        if ((symbol->length == length) && (memcmp(symbol->name, text, length) == 0)) {
            return symbol;
        }
    }

    return MAREA_NULLPTR;
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenPushSymbol(MAREA_CgenGenerator *generator, const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_Uinteger8 kind)
{
    if (generator->symbolCount == generator->symbolCapacity) {
        const MAREA_IntegerSize NEW_CAPACITY = (generator->symbolCapacity == MAREA_INTEGER_SIZE(0)) ? MAREA_INTEGER_SIZE(16) : (generator->symbolCapacity * MAREA_INTEGER_SIZE(2));

        MAREA_Null memory = MAREA_NULLPTR;

        if (MAREA_ArenaAllocate(generator->arena, (NEW_CAPACITY * sizeof(MAREA_CgenSymbol)), MAREA_ALIGNOF(MAREA_CgenSymbol), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
            return MAREA_CGEN_EXCEPT_ERROR_OUT_OF_MEMORY;
        }

        if (generator->symbolCount > MAREA_INTEGER_SIZE(0)) {
            memcpy(memory, generator->symbols, (generator->symbolCount * sizeof(MAREA_CgenSymbol)));
        }

        generator->symbols = memory;
        generator->symbolCapacity = NEW_CAPACITY;
    }

    generator->symbols[generator->symbolCount].name = text;
    generator->symbols[generator->symbolCount].length = length;
    generator->symbols[generator->symbolCount].kind = kind;
    generator->symbolCount = (generator->symbolCount + MAREA_INTEGER_SIZE(1));

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

// INFO: a plain call reads its callee by the bare name sema left it, with the namespace of this unit skipped
// when the call wrote it in full, as the bytecode stage already reads the same shape.
const MAREA_CgenFunction *MAREA_CALL MAREA_CgenFindFunction(const MAREA_CgenGenerator *generator, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    MAREA_IntegerSize skip = MAREA_INTEGER_SIZE(0);

    if ((generator->namespaceLength > MAREA_INTEGER_SIZE(0)) && (length > (generator->namespaceLength + MAREA_INTEGER_SIZE(1))) && (memcmp(text, generator->namespaceText, generator->namespaceLength) == 0) && (text[generator->namespaceLength] == '.')) {
        skip = (generator->namespaceLength + MAREA_INTEGER_SIZE(1));
    }

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < generator->functionCount; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_CgenFunction *entry = &generator->functions[index];

        if ((entry->nameLength == (length - skip)) && (memcmp(entry->name, &text[skip], (length - skip)) == 0)) {
            return entry;
        }
    }

    return MAREA_NULLPTR;
}
