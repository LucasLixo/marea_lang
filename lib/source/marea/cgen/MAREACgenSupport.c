// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/cgen/MAREACgenInternal.h"

#include <string.h>

const MAREA_IrNode *MAREA_CALL MAREA_CgenChildAt(const MAREA_IrNode *node, MAREA_IntegerSize index)
{
    const MAREA_IrNode *child = MAREA_NULLPTR;

    if ((node == MAREA_NULLPTR) || (MAREA_IrNodeGetChildAt(node, index, &child) != MAREA_IR_NODE_EXCEPT_SUCCESS)) {
        return MAREA_NULLPTR;
    }

    return child;
}

MAREA_Boolean MAREA_CALL MAREA_CgenSameText(const MAREA_Character8 *left, MAREA_IntegerSize leftLength, const MAREA_Character8 *right, MAREA_IntegerSize rightLength)
{
    return ((leftLength == rightLength) && ((leftLength == MAREA_INTEGER_SIZE(0)) || (memcmp(left, right, leftLength) == 0)));
}

MAREA_Boolean MAREA_CALL MAREA_CgenTextIs(const MAREA_IrNode *node, const MAREA_Character8 *expected)
{
    return MAREA_CgenSameText(MAREA_IrNodeGetText(node), MAREA_IrNodeGetTextLength(node), expected, (MAREA_IntegerSize)strlen(expected));
}

MAREA_IntegerSize MAREA_CALL MAREA_CgenNamespaceSkip(const MAREA_CgenGenerator *generator, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    if ((generator->namespaceLength > MAREA_INTEGER_SIZE(0)) && (length > (generator->namespaceLength + MAREA_INTEGER_SIZE(1))) && (memcmp(text, generator->namespaceText, generator->namespaceLength) == 0) && (text[generator->namespaceLength] == '.')) {
        return (generator->namespaceLength + MAREA_INTEGER_SIZE(1));
    }

    return MAREA_INTEGER_SIZE(0);
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenAllocate(MAREA_CgenGenerator *generator, MAREA_IntegerSize size, MAREA_IntegerSize alignment, MAREA_Null *outMemory)
{
    const MAREA_IntegerSize ASKED = ((size == MAREA_INTEGER_SIZE(0)) ? MAREA_INTEGER_SIZE(1) : size);

    if (MAREA_ArenaAllocate(generator->arena, ASKED, alignment, outMemory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_CGEN_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    memset(*outMemory, 0, ASKED);

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenGrow(MAREA_CgenGenerator *generator, MAREA_Null *ioItems, MAREA_IntegerSize *ioCapacity, MAREA_IntegerSize count, MAREA_IntegerSize itemSize, MAREA_IntegerSize alignment)
{
    if (count < *ioCapacity) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    const MAREA_IntegerSize CAPACITY = ((*ioCapacity < MAREA_INTEGER_SIZE(8)) ? MAREA_INTEGER_SIZE(8) : (*ioCapacity * MAREA_INTEGER_SIZE(2)));

    MAREA_Null memory = MAREA_NULLPTR;

    MAREA_CGEN_TRY(MAREA_CgenAllocate(generator, (CAPACITY * itemSize), alignment, &memory));

    if (count > MAREA_INTEGER_SIZE(0)) {
        memcpy(memory, *ioItems, (count * itemSize));
    }

    *ioItems = memory;
    *ioCapacity = CAPACITY;

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

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

MAREA_Boolean MAREA_CALL MAREA_CgenParseInteger(const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_Uinteger64 *outValue)
{
    MAREA_Uinteger64 base = MAREA_UINTEGER_64(10);
    MAREA_IntegerSize position = MAREA_INTEGER_SIZE(0);

    if ((length > MAREA_INTEGER_SIZE(2)) && (text[0] == '0') && (text[1] == 'x')) {
        base = MAREA_UINTEGER_64(16);
        position = MAREA_INTEGER_SIZE(2);
    } else if ((length > MAREA_INTEGER_SIZE(2)) && (text[0] == '0') && (text[1] == 'b')) {
        base = MAREA_UINTEGER_64(2);
        position = MAREA_INTEGER_SIZE(2);
    }

    MAREA_Uinteger64 value = MAREA_UINTEGER_64(0);
    MAREA_Boolean seen = MAREA_BOOLEAN_FALSE;

    for (; position < length; position = (position + MAREA_INTEGER_SIZE(1))) {
        const MAREA_Character8 SYMBOL = text[position];

        if (SYMBOL == '_') {
            continue;
        }

        MAREA_Uinteger64 digit = MAREA_UINTEGER_64(16);

        if ((SYMBOL >= '0') && (SYMBOL <= '9')) {
            digit = (MAREA_Uinteger64)(SYMBOL - '0');
        } else if ((SYMBOL >= 'a') && (SYMBOL <= 'f')) {
            digit = (MAREA_Uinteger64)((SYMBOL - 'a') + 10);
        } else if ((SYMBOL >= 'A') && (SYMBOL <= 'F')) {
            digit = (MAREA_Uinteger64)((SYMBOL - 'A') + 10);
        }

        if ((digit >= base) || (value > ((MAREA_UINTEGER_64_MAXIMUM - digit) / base))) {
            return MAREA_BOOLEAN_FALSE;
        }

        value = ((value * base) + digit);
        seen = MAREA_BOOLEAN_TRUE;
    }

    *outValue = value;

    return seen;
}

const MAREA_CgenSymbol *MAREA_CALL MAREA_CgenFindSymbol(const MAREA_CgenGenerator *generator, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    for (MAREA_IntegerSize index = generator->symbolCount; index > MAREA_INTEGER_SIZE(0); index = (index - MAREA_INTEGER_SIZE(1))) {
        const MAREA_CgenSymbol *symbol = &generator->symbols[(index - MAREA_INTEGER_SIZE(1))];

        if (MAREA_CgenSameText(symbol->name, symbol->length, text, length) == MAREA_BOOLEAN_TRUE) {
            return symbol;
        }
    }

    return MAREA_NULLPTR;
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenPushSymbol(MAREA_CgenGenerator *generator, const MAREA_Character8 *text, MAREA_IntegerSize length, const MAREA_Character8 *cText, MAREA_Uinteger32 type, MAREA_Boolean stable, MAREA_Boolean unaligned)
{
    MAREA_Null items = generator->symbols;

    MAREA_CGEN_TRY(MAREA_CgenGrow(generator, &items, &generator->symbolCapacity, generator->symbolCount, sizeof(MAREA_CgenSymbol), MAREA_ALIGNOF(MAREA_CgenSymbol)));

    generator->symbols = items;

    MAREA_CgenSymbol *symbol = &generator->symbols[generator->symbolCount];

    symbol->name = text;
    symbol->length = length;
    symbol->cText = cText;
    symbol->type = type;
    symbol->stable = stable;
    symbol->unaligned = unaligned;

    generator->symbolCount = (generator->symbolCount + MAREA_INTEGER_SIZE(1));

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

MAREA_CgenFunction *MAREA_CALL MAREA_CgenFindFunction(const MAREA_CgenGenerator *generator, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    const MAREA_IntegerSize SKIP = MAREA_CgenNamespaceSkip(generator, text, length);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < generator->functionCount; index = (index + MAREA_INTEGER_SIZE(1))) {
        MAREA_CgenFunction *entry = &generator->functions[index];

        if (MAREA_CgenSameText(entry->name, entry->nameLength, (text + SKIP), (length - SKIP)) == MAREA_BOOLEAN_TRUE) {
            return entry;
        }
    }

    return MAREA_NULLPTR;
}

MAREA_CgenFunction *MAREA_CALL MAREA_CgenFindMember(const MAREA_CgenGenerator *generator, const MAREA_Character8 *owner, MAREA_IntegerSize ownerLength, const MAREA_Character8 *member, MAREA_IntegerSize memberLength)
{
    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < generator->functionCount; index = (index + MAREA_INTEGER_SIZE(1))) {
        MAREA_CgenFunction *entry = &generator->functions[index];

        if ((entry->ownerLength == ownerLength) && (entry->nameLength == (ownerLength + MAREA_INTEGER_SIZE(1) + memberLength)) && (memcmp(entry->name, owner, ownerLength) == 0) && (memcmp((entry->name + ownerLength + MAREA_INTEGER_SIZE(1)), member, memberLength) == 0)) {
            return entry;
        }
    }

    return MAREA_NULLPTR;
}

const MAREA_IrNode *MAREA_CALL MAREA_CgenFindConstant(const MAREA_CgenGenerator *generator, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    const MAREA_IntegerSize SKIP = MAREA_CgenNamespaceSkip(generator, text, length);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < generator->constantCount; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_IrNode *constant = generator->constants[index];

        if (MAREA_CgenSameText(MAREA_IrNodeGetText(constant), MAREA_IrNodeGetTextLength(constant), (text + SKIP), (length - SKIP)) == MAREA_BOOLEAN_TRUE) {
            return constant;
        }
    }

    return MAREA_NULLPTR;
}

const MAREA_IrNode *MAREA_CALL MAREA_CgenFindDeclaration(const MAREA_CgenGenerator *generator, MAREA_IrKind kind, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    const MAREA_IntegerSize COUNT = MAREA_IrNodeGetChildCount(generator->module);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_IrNode *child = MAREA_CgenChildAt(generator->module, index);

        if ((MAREA_IrNodeGetKind(child) == kind) && (MAREA_CgenSameText(MAREA_IrNodeGetText(child), MAREA_IrNodeGetTextLength(child), text, length) == MAREA_BOOLEAN_TRUE)) {
            return child;
        }
    }

    return MAREA_NULLPTR;
}
