// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/bytecode/MAREABytecodeGeneratorInternal.h"

// INFO: a typed raw pointer, the type with the raw pointer role, is an address in a register of kind uinteger64. A
// conversion to or from it keeps the bits, and an element is loaded and stored at base + index * size with no check.

MAREA_Boolean MAREA_CALL MAREA_BytecodeGeneratorPointee(const MAREA_BytecodeGenerator *generator, const MAREA_Character8 *text, MAREA_IntegerSize length, const MAREA_Character8 **outElement, MAREA_IntegerSize *outElementLength)
{
    *outElement = MAREA_NULLPTR;
    *outElementLength = MAREA_INTEGER_SIZE(0);

    const MAREA_TypeDescriptor *descriptor = MAREA_BytecodeGeneratorDescribe(generator, text, length);

    if ((descriptor == MAREA_NULLPTR) || ((descriptor->roles & MAREA_TYPE_ROLE_RAW_POINTER) == MAREA_TYPE_ROLE_NONE) || (text[(length - MAREA_INTEGER_SIZE(1))] != '>')) {
        return MAREA_BOOLEAN_FALSE;
    }

    MAREA_IntegerSize from = MAREA_INTEGER_SIZE(0);
    MAREA_IntegerSize to = (length - MAREA_INTEGER_SIZE(1));

    while ((from < to) && (text[from] != '<')) {
        from += MAREA_INTEGER_SIZE(1);
    }

    from += MAREA_INTEGER_SIZE(1);

    while ((from < to) && (text[from] == ' ')) {
        from += MAREA_INTEGER_SIZE(1);
    }

    while ((to > from) && (text[(to - MAREA_INTEGER_SIZE(1))] == ' ')) {
        to -= MAREA_INTEGER_SIZE(1);
    }

    if (to <= from) {
        return MAREA_BOOLEAN_FALSE;
    }

    *outElement = (text + from);
    *outElementLength = (to - from);

    return MAREA_BOOLEAN_TRUE;
}

MAREA_Boolean MAREA_CALL MAREA_BytecodeGeneratorIsPointer(const MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node)
{
    const MAREA_Character8 *element = MAREA_NULLPTR;
    MAREA_IntegerSize length = MAREA_INTEGER_SIZE(0);

    return (node != MAREA_NULLPTR) && MAREA_BytecodeGeneratorPointee(generator, MAREA_IrNodeGetResolvedType(node), MAREA_IrNodeGetResolvedTypeLength(node), &element, &length);
}
