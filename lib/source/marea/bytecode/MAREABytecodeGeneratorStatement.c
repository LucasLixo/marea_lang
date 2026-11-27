// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/bytecode/MAREABytecodeGeneratorInternal.h"

// INFO: a structure local never shares its byte range with another local: MAREA_BytecodeGeneratorAllocateFrame
// gives it a fresh one, and addr_of loads that offset into the local's own register, so the register always holds
// an address, never the bytes.
static MAREA_BytecodeExcept MAREA_CALL MAREA_G_LowerAggregateLocal(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, const MAREA_IrNode *value, MAREA_Uinteger32 aggregateIndex)
{
    const MAREA_BytecodeGeneratorAggregate *aggregate = &generator->aggregates[aggregateIndex];

    MAREA_Uinteger32 offset = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocateFrame(generator, node, aggregate->size, aggregate->alignment, &offset));

    MAREA_Uinteger32 registerIndex = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), &registerIndex));

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_ADDRESS_OF, registerIndex, offset, MAREA_UINTEGER_32(0), MAREA_NULLPTR));

    // INFO: a loop builds the local again at the same bytes, whose old value its block already destroyed.
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorConstructAggregate(generator, value, registerIndex, aggregateIndex));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorPushOwned(generator, offset, aggregateIndex));

    // INFO: the name is bound after its value, so the value cannot read the local that it makes.
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorPushSymbol(generator, MAREA_IrNodeGetText(node), MAREA_IrNodeGetTextLength(node), registerIndex, MAREA_BYTECODE_GENERATOR_KIND_AGGREGATE, aggregateIndex));

    generator->nextRegister = (registerIndex + MAREA_UINTEGER_32(1));

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_LowerLocal(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node)
{
    const MAREA_IrNode *value = MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(0));

    if (value == MAREA_NULLPTR) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A local has no initial value.");
    }

    MAREA_Uinteger8 kind = MAREA_BYTECODE_GENERATOR_KIND_NONE;
    MAREA_Uinteger32 aggregateIndex = MAREA_BYTECODE_NONE;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAggregateKind(generator, node, &kind, &aggregateIndex));

    if (kind == MAREA_BYTECODE_GENERATOR_KIND_AGGREGATE) {
        return MAREA_G_LowerAggregateLocal(generator, node, value, aggregateIndex);
    }

    if (!((kind < MAREA_BYTECODE_NUMBER_KIND_COUNT) || (kind == MAREA_BYTECODE_GENERATOR_KIND_BOOLEAN))) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A local has a type that is not a scalar.");
    }

    MAREA_Uinteger32 registerIndex = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), &registerIndex));

    MAREA_Uinteger8 valueKind = MAREA_BYTECODE_GENERATOR_KIND_NONE;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLowerInto(generator, value, registerIndex, kind, &valueKind));

    // INFO: the name is bound after its value, so the value cannot read the local that it makes.
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorPushSymbol(generator, MAREA_IrNodeGetText(node), MAREA_IrNodeGetTextLength(node), registerIndex, kind, MAREA_BYTECODE_NONE));

    generator->nextRegister = (registerIndex + MAREA_UINTEGER_32(1));

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

// INFO: the new value is built in bytes of its own and copied after, because the value may read the target.
static MAREA_BytecodeExcept MAREA_CALL MAREA_G_LowerAggregateAssignment(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, const MAREA_IrNode *value, MAREA_Uinteger32 target, MAREA_Uinteger32 aggregateIndex)
{
    const MAREA_Uinteger64 SIZE = generator->aggregates[aggregateIndex].size;
    const MAREA_Uinteger64 ALIGNMENT = generator->aggregates[aggregateIndex].alignment;

    MAREA_Uinteger32 offset = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocateFrame(generator, node, SIZE, ALIGNMENT, &offset));

    MAREA_Uinteger32 temporary = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), &temporary));

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_ADDRESS_OF, temporary, offset, MAREA_UINTEGER_32(0), MAREA_NULLPTR));

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorConstructAggregate(generator, value, temporary, aggregateIndex));

    // INFO: the new value is built before the old one is destroyed, since it may read the target.
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorDrop(generator, node, target, aggregateIndex, MAREA_BOOLEAN_FALSE));

    return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_COPY_BYTES, target, temporary, (MAREA_Uinteger32)SIZE, MAREA_NULLPTR);
}

// INFO: tells whether the place of a target is inside the storage of a vector, which a call in the value may move.
static MAREA_BytecodeExcept MAREA_CALL MAREA_G_ThroughVector(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *target, MAREA_Boolean *outThrough)
{
    *outThrough = MAREA_BOOLEAN_FALSE;

    for (const MAREA_IrNode *node = target; (MAREA_IrNodeGetKind(node) == MAREA_IR_KIND_INDEX_ACCESS) || (MAREA_IrNodeGetKind(node) == MAREA_IR_KIND_MEMBER_ACCESS); node = MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(0))) {
        const MAREA_IrNode *base = MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(0));

        if (MAREA_IrNodeGetKind(node) != MAREA_IR_KIND_INDEX_ACCESS) {
            continue;
        }

        MAREA_Uinteger32 run = MAREA_BYTECODE_NONE;

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorRunKind(generator, base, MAREA_IrNodeGetResolvedType(base), MAREA_IrNodeGetResolvedTypeLength(base), &run));

        if ((run != MAREA_BYTECODE_NONE) && generator->aggregates[run].isVector) {
            *outThrough = MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

// INFO: a target in a vector: the value runs first, then the target is reached, so it is never an old address of the storage.
static MAREA_BytecodeExcept MAREA_CALL MAREA_G_LowerVectorAssignment(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, const MAREA_IrNode *target, const MAREA_IrNode *value)
{
    const MAREA_TokenKind OPERATOR = MAREA_IrNodeGetOperator(node);

    MAREA_Uinteger8 kind = MAREA_BYTECODE_GENERATOR_KIND_NONE;
    MAREA_Uinteger32 aggregate = MAREA_BYTECODE_NONE;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAggregateKind(generator, target, &kind, &aggregate));

    MAREA_Uinteger32 held = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 address = MAREA_UINTEGER_32(0);
    MAREA_Uinteger8 placeKind = MAREA_BYTECODE_GENERATOR_KIND_NONE;
    MAREA_Uinteger32 placeAggregate = MAREA_BYTECODE_NONE;
    MAREA_Uinteger8 heldKind = MAREA_BYTECODE_GENERATOR_KIND_NONE;

    if (kind == MAREA_BYTECODE_GENERATOR_KIND_AGGREGATE) {
        if (OPERATOR != MAREA_TOKEN_KIND_EQUAL) {
            return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A compound assignment needs a number.");
        }

        MAREA_Uinteger32 offset = MAREA_UINTEGER_32(0);

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocateFrame(generator, node, generator->aggregates[aggregate].size, generator->aggregates[aggregate].alignment, &offset));
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), &held));
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_ADDRESS_OF, held, offset, MAREA_UINTEGER_32(0), MAREA_NULLPTR));
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorConstructAggregate(generator, value, held, aggregate));
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorPlace(generator, target, &address, &placeKind, &placeAggregate));
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorDrop(generator, node, address, aggregate, MAREA_BOOLEAN_FALSE));

        return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_COPY_BYTES, address, held, (MAREA_Uinteger32)generator->aggregates[aggregate].size, MAREA_NULLPTR);
    }

    if (!((kind < MAREA_BYTECODE_NUMBER_KIND_COUNT) || (kind == MAREA_BYTECODE_GENERATOR_KIND_BOOLEAN))) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A value of this type is not assigned yet.");
    }

    const MAREA_Uinteger8 WIDTH_KIND = ((kind == MAREA_BYTECODE_GENERATOR_KIND_BOOLEAN) ? MAREA_BYTECODE_NUMBER_KIND_UINTEGER_8 : kind);

    MAREA_Uinteger16 opcode = MAREA_BYTECODE_OPCODE_NOP;
    MAREA_Boolean threeRegisters = MAREA_BOOLEAN_FALSE;

    if ((OPERATOR != MAREA_TOKEN_KIND_EQUAL) && (!MAREA_BytecodeGeneratorOperatorOpcode(OPERATOR, &opcode, &threeRegisters) || (kind >= MAREA_BYTECODE_NUMBER_KIND_COUNT))) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "An assignment operator is not lowered.");
    }

    const MAREA_Boolean SHIFT = ((opcode == MAREA_BYTECODE_OPCODE_SHIFT_LEFT) || (opcode == MAREA_BYTECODE_OPCODE_SHIFT_RIGHT));

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), &held));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLowerInto(generator, value, held, (SHIFT ? MAREA_BYTECODE_GENERATOR_KIND_NONE : kind), &heldKind));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorPlace(generator, target, &address, &placeKind, &placeAggregate));

    if (OPERATOR != MAREA_TOKEN_KIND_EQUAL) {
        MAREA_Uinteger32 result = MAREA_UINTEGER_32(0);

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), &result));
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_LOAD, result, address, WIDTH_KIND, MAREA_NULLPTR));
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, opcode, result, (threeRegisters ? result : held), (threeRegisters ? held : (MAREA_Uinteger32)kind), MAREA_NULLPTR));

        held = result;
    }

    return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_STORE, address, held, WIDTH_KIND, MAREA_NULLPTR);
}

// INFO: a target in memory, a field or an element, is reached first, with the check of each index, then the value
// runs; a compound assignment then reads the place, combines and writes it back.
static MAREA_BytecodeExcept MAREA_CALL MAREA_G_LowerPlaceAssignment(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, const MAREA_IrNode *target, const MAREA_IrNode *value)
{
    MAREA_Uinteger32 address = MAREA_UINTEGER_32(0);
    MAREA_Uinteger8 kind = MAREA_BYTECODE_GENERATOR_KIND_NONE;
    MAREA_Uinteger32 aggregate = MAREA_BYTECODE_NONE;

    MAREA_Boolean through = MAREA_BOOLEAN_FALSE;

    MAREA_BYTECODE_TRY(MAREA_G_ThroughVector(generator, target, &through));

    if (through) {
        return MAREA_G_LowerVectorAssignment(generator, node, target, value);
    }

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorPlace(generator, target, &address, &kind, &aggregate));

    if (generator->functionFailed) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    const MAREA_TokenKind OPERATOR = MAREA_IrNodeGetOperator(node);

    if (kind == MAREA_BYTECODE_GENERATOR_KIND_AGGREGATE) {
        if (OPERATOR != MAREA_TOKEN_KIND_EQUAL) {
            return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A compound assignment needs a number.");
        }

        return MAREA_G_LowerAggregateAssignment(generator, node, value, address, aggregate);
    }

    if (kind >= MAREA_BYTECODE_GENERATOR_KIND_VOID) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A value of this type is not assigned yet.");
    }

    const MAREA_Uinteger8 WIDTH_KIND = ((kind == MAREA_BYTECODE_GENERATOR_KIND_BOOLEAN) ? MAREA_BYTECODE_NUMBER_KIND_UINTEGER_8 : kind);

    MAREA_Uinteger32 result = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), &result));

    if (OPERATOR == MAREA_TOKEN_KIND_EQUAL) {
        MAREA_Uinteger8 valueKind = MAREA_BYTECODE_GENERATOR_KIND_NONE;

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLowerInto(generator, value, result, kind, &valueKind));

        return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_STORE, address, result, WIDTH_KIND, MAREA_NULLPTR);
    }

    MAREA_Uinteger16 opcode = MAREA_BYTECODE_OPCODE_NOP;
    MAREA_Boolean threeRegisters = MAREA_BOOLEAN_FALSE;

    if (!MAREA_BytecodeGeneratorOperatorOpcode(OPERATOR, &opcode, &threeRegisters) || (kind >= MAREA_BYTECODE_NUMBER_KIND_COUNT)) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "An assignment operator is not lowered.");
    }

    const MAREA_Boolean SHIFT = ((opcode == MAREA_BYTECODE_OPCODE_SHIFT_LEFT) || (opcode == MAREA_BYTECODE_OPCODE_SHIFT_RIGHT));

    MAREA_Uinteger32 right = MAREA_UINTEGER_32(0);
    MAREA_Uinteger8 rightKind = MAREA_BYTECODE_GENERATOR_KIND_NONE;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLowerOperand(generator, value, (SHIFT ? MAREA_BYTECODE_GENERATOR_KIND_NONE : kind), &right, &rightKind));

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_LOAD, result, address, WIDTH_KIND, MAREA_NULLPTR));

    if (threeRegisters) {
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, opcode, result, result, right, MAREA_NULLPTR));
    } else {
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, opcode, result, right, kind, MAREA_NULLPTR));
    }

    return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_STORE, address, result, WIDTH_KIND, MAREA_NULLPTR);
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_LowerAssignment(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node)
{
    const MAREA_IrNode *target = MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(0));
    const MAREA_IrNode *value = MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(1));

    if ((MAREA_IrNodeGetOperator(node) != MAREA_TOKEN_KIND_EQUAL) && MAREA_BytecodeGeneratorIsPointer(generator, target)) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A pointer has no compound assignment.");
    }

    if ((target != MAREA_NULLPTR) && (value != MAREA_NULLPTR) && ((MAREA_IrNodeGetKind(target) == MAREA_IR_KIND_INDEX_ACCESS) || (MAREA_IrNodeGetKind(target) == MAREA_IR_KIND_MEMBER_ACCESS))) {
        return MAREA_G_LowerPlaceAssignment(generator, node, target, value);
    }

    if ((target == MAREA_NULLPTR) || (value == MAREA_NULLPTR) || (MAREA_IrNodeGetKind(target) != MAREA_IR_KIND_IDENTIFIER)) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "Only a local, a parameter, a field or an element can be assigned.");
    }

    const MAREA_BytecodeGeneratorSymbol *symbol = MAREA_BytecodeGeneratorFindSymbol(generator, MAREA_IrNodeGetText(target), MAREA_IrNodeGetTextLength(target));

    if (symbol == MAREA_NULLPTR) {
        return MAREA_BytecodeGeneratorFail(generator, target, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNRESOLVED_NAME, "An assigned name is not a local or a parameter.");
    }

    if (symbol->isPlace) {
        return MAREA_G_LowerPlaceAssignment(generator, node, target, value);
    }

    const MAREA_TokenKind OPERATOR = MAREA_IrNodeGetOperator(node);
    const MAREA_Uinteger32 TARGET = symbol->registerIndex;
    const MAREA_Uinteger8 KIND = symbol->kind;

    if ((OPERATOR == MAREA_TOKEN_KIND_EQUAL) && (KIND == MAREA_BYTECODE_GENERATOR_KIND_AGGREGATE)) {
        return MAREA_G_LowerAggregateAssignment(generator, node, value, TARGET, symbol->aggregateType);
    }

    if (OPERATOR == MAREA_TOKEN_KIND_EQUAL) {
        MAREA_Uinteger32 temporary = MAREA_UINTEGER_32(0);
        MAREA_Uinteger8 valueKind = MAREA_BYTECODE_GENERATOR_KIND_NONE;

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), &temporary));

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLowerInto(generator, value, temporary, KIND, &valueKind));

        return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_MOVE, TARGET, temporary, MAREA_UINTEGER_32(0), MAREA_NULLPTR);
    }

    MAREA_Uinteger16 opcode = MAREA_BYTECODE_OPCODE_NOP;
    MAREA_Boolean threeRegisters = MAREA_BOOLEAN_FALSE;

    if (!MAREA_BytecodeGeneratorOperatorOpcode(OPERATOR, &opcode, &threeRegisters)) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "An assignment operator is not lowered.");
    }

    if (KIND >= MAREA_BYTECODE_NUMBER_KIND_COUNT) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A compound assignment needs a number.");
    }

    const MAREA_Boolean SHIFT = ((opcode == MAREA_BYTECODE_OPCODE_SHIFT_LEFT) || (opcode == MAREA_BYTECODE_OPCODE_SHIFT_RIGHT));

    MAREA_Uinteger32 right = MAREA_UINTEGER_32(0);
    MAREA_Uinteger8 rightKind = MAREA_BYTECODE_GENERATOR_KIND_NONE;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLowerOperand(generator, value, (SHIFT ? MAREA_BYTECODE_GENERATOR_KIND_NONE : KIND), &right, &rightKind));

    if (threeRegisters) {
        return MAREA_BytecodeGeneratorEmit(generator, opcode, TARGET, TARGET, right, MAREA_NULLPTR);
    }

    return MAREA_BytecodeGeneratorEmit(generator, opcode, TARGET, right, KIND, MAREA_NULLPTR);
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_LowerReturn(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node)
{
    if (MAREA_IrNodeGetChildCount(node) == MAREA_INTEGER_SIZE(0)) {
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorDrops(generator, node, MAREA_INTEGER_SIZE(0)));

        return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_RETURN_VOID, MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), MAREA_NULLPTR);
    }

    const MAREA_IrNode *value = MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(0));

    MAREA_Uinteger32 registerIndex = MAREA_UINTEGER_32(0);
    MAREA_Uinteger8 kind = MAREA_BYTECODE_GENERATOR_KIND_NONE;

    // INFO: the result is built before the values of the function are destroyed, since it may read them.
    if (generator->current->resultKind == MAREA_BYTECODE_GENERATOR_KIND_AGGREGATE) {
        registerIndex = generator->resultRegister;

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorConstructAggregate(generator, value, registerIndex, generator->current->resultAggregate));
    } else {
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLowerOperand(generator, value, generator->current->resultKind, &registerIndex, &kind));
    }

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorDrops(generator, node, MAREA_INTEGER_SIZE(0)));

    return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_RETURN, registerIndex, MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), MAREA_NULLPTR);
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_LowerExpressionStatement(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node)
{
    MAREA_Uinteger32 temporary = MAREA_UINTEGER_32(0);
    MAREA_Uinteger8 kind = MAREA_BYTECODE_GENERATOR_KIND_NONE;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), &temporary));

    const MAREA_IrNode *expression = MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(0));

    // INFO: the parts of a fork_join run one after the other, so a spawn that no one keeps is its call.
    if ((MAREA_IrNodeGetKind(expression) == MAREA_IR_KIND_UNARY_EXPRESSION) && (MAREA_IrNodeGetOperator(expression) == MAREA_TOKEN_KIND_KEYWORD_SPAWN)) {
        expression = MAREA_BytecodeGeneratorChildAt(expression, MAREA_INTEGER_SIZE(0));
    }

    if (MAREA_IrNodeGetKind(expression) == MAREA_IR_KIND_CALL_EXPRESSION) {
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLowerCall(generator, expression, temporary, MAREA_BOOLEAN_FALSE, &kind));

        if ((kind != MAREA_BYTECODE_GENERATOR_KIND_AGGREGATE) || generator->functionFailed) {
            return MAREA_BYTECODE_EXCEPT_SUCCESS;
        }

        MAREA_Uinteger32 aggregate = MAREA_BYTECODE_NONE;

        // INFO: a result that nothing takes over is destroyed at once.
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAggregateKind(generator, expression, &kind, &aggregate));

        if (aggregate == MAREA_BYTECODE_NONE) {
            return MAREA_BYTECODE_EXCEPT_SUCCESS;
        }

        return MAREA_BytecodeGeneratorDrop(generator, expression, temporary, aggregate, MAREA_BOOLEAN_TRUE);
    }

    return MAREA_BytecodeGeneratorLowerInto(generator, expression, temporary, MAREA_BYTECODE_GENERATOR_KIND_NONE, &kind);
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_PushPatch(MAREA_BytecodeGenerator *generator, MAREA_Boolean forBreak, MAREA_Uinteger32 index)
{
    MAREA_Null items = MAREA_NULLPTR;

    if (forBreak) {
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorGrow(generator, generator->patches, &generator->patchCapacity, generator->patchCount, sizeof(MAREA_Uinteger32), MAREA_ALIGNOF(MAREA_Uinteger32), &items));

        generator->patches = items;
        generator->patches[generator->patchCount] = index;
        generator->patchCount += MAREA_INTEGER_SIZE(1);
    } else {
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorGrow(generator, generator->endPatches, &generator->endPatchCapacity, generator->endPatchCount, sizeof(MAREA_Uinteger32), MAREA_ALIGNOF(MAREA_Uinteger32), &items));

        generator->endPatches = items;
        generator->endPatches[generator->endPatchCount] = index;
        generator->endPatchCount += MAREA_INTEGER_SIZE(1);
    }

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

// INFO: the test of an arm is lowered to a register that only the jump reads, so its temporaries end before the block.
static MAREA_BytecodeExcept MAREA_CALL MAREA_G_LowerArmTest(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *condition, MAREA_Uinteger32 *outJump)
{
    const MAREA_Uinteger32 MARK = generator->nextRegister;

    MAREA_Uinteger32 registerIndex = MAREA_UINTEGER_32(0);
    MAREA_Uinteger8 kind = MAREA_BYTECODE_GENERATOR_KIND_NONE;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLowerOperand(generator, condition, MAREA_BYTECODE_GENERATOR_KIND_BOOLEAN, &registerIndex, &kind));

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_JUMP_IF_NOT, registerIndex, MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), outJump));

    generator->nextRegister = MARK;

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_LowerIf(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node)
{
    const MAREA_IntegerSize COUNT = MAREA_IrNodeGetChildCount(node);
    const MAREA_IntegerSize
END_MARK
= generator->endPatchCount;

const MAREA_IrNode *arm = node;
MAREA_IntegerSize position = MAREA_INTEGER_SIZE(2);

for (;;) {
    MAREA_Uinteger32 testJump = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_G_LowerArmTest(generator, MAREA_BytecodeGeneratorChildAt(arm, MAREA_INTEGER_SIZE(0)), &testJump));

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLowerBlock(generator, MAREA_BytecodeGeneratorChildAt(arm, MAREA_INTEGER_SIZE(1))));

    if (position < COUNT) {
        MAREA_Uinteger32 endJump = MAREA_UINTEGER_32(0);

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_JUMP, MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), &endJump));

        MAREA_BYTECODE_TRY(MAREA_G_PushPatch(generator, MAREA_BOOLEAN_FALSE, endJump));
    }

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorPatchJump(generator, node, testJump, (MAREA_Uinteger32)generator->codeCount));

    if (position >= COUNT) {
        break;
    }

    const MAREA_IrNode *clause = MAREA_BytecodeGeneratorChildAt(node, position);

    position += MAREA_INTEGER_SIZE(1);

    if (MAREA_IrNodeGetKind(clause) == MAREA_IR_KIND_ELSE_CLAUSE) {
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLowerBlock(generator, MAREA_BytecodeGeneratorChildAt(clause, MAREA_INTEGER_SIZE(0))));

        break;
    }

    arm = clause;
}

for (MAREA_IntegerSize index = END_MARK; index < generator->endPatchCount; index += MAREA_INTEGER_SIZE(1)) {
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorPatchJump(generator, node, generator->endPatches[index], (MAREA_Uinteger32)generator->codeCount));
}

generator->endPatchCount =
END_MARK
;

return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_LowerWhile(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node)
{
    const MAREA_IrNode *condition = MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(0));
    const MAREA_Uinteger32 START = (MAREA_Uinteger32)generator->codeCount;

    // INFO: the do while of the IR is a while true, so a loop with a true test needs no test at all.
    const MAREA_Boolean
ENDLESS
= ((MAREA_IrNodeGetKind(condition) == MAREA_IR_KIND_BOOLEAN_LITERAL) && (MAREA_IrNodeGetTextLength(condition) == MAREA_INTEGER_SIZE(4)));

MAREA_Uinteger32 exitJump = MAREA_UINTEGER_32(0);

if (!ENDLESS) {
    MAREA_BYTECODE_TRY(MAREA_G_LowerArmTest(generator, condition, &exitJump));
}

return MAREA_BytecodeGeneratorLowerLoopBody(generator, node, MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(1)), START, (ENDLESS ? MAREA_BYTECODE_NONE : exitJump), generator->symbolCount, generator->nextRegister);
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorLowerLoopBody(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, const MAREA_IrNode *body, MAREA_Uinteger32 start, MAREA_Uinteger32 exitJump, MAREA_IntegerSize symbolMark, MAREA_Uinteger32 registerMark)
{
    const MAREA_IntegerSize BREAK_MARK = generator->patchCount;

    MAREA_Null items = MAREA_NULLPTR;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorGrow(generator, generator->loops, &generator->loopCapacity, generator->loopCount, sizeof(MAREA_BytecodeGeneratorLoop), MAREA_ALIGNOF(MAREA_BytecodeGeneratorLoop), &items));

    generator->loops = items;
    generator->loops[generator->loopCount].start = start;
    generator->loops[generator->loopCount].patchMark = BREAK_MARK;
    generator->loops[generator->loopCount].ownedMark = generator->ownedCount;
    generator->loopCount += MAREA_INTEGER_SIZE(1);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLowerBlock(generator, body));

    MAREA_Uinteger32 backJump = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_JUMP, MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), &backJump));

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorPatchJump(generator, node, backJump, start));

    const MAREA_Uinteger32 EXIT = (MAREA_Uinteger32)generator->codeCount;

    if (exitJump != MAREA_BYTECODE_NONE) {
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorPatchJump(generator, node, exitJump, EXIT));
    }

    for (MAREA_IntegerSize index = BREAK_MARK; index < generator->patchCount; index += MAREA_INTEGER_SIZE(1)) {
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorPatchJump(generator, node, generator->patches[index], EXIT));
    }

    generator->patchCount = BREAK_MARK;
    generator->loopCount -= MAREA_INTEGER_SIZE(1);
    generator->symbolCount = symbolMark;
    generator->nextRegister = registerMark;

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

// INFO: the body runs once for each part, from part 0 up. The part itself is not a value yet, so its name has no
// kind and the value that is cut is not read.
static MAREA_BytecodeExcept MAREA_CALL MAREA_G_LowerPartition(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node)
{
    const MAREA_IrNode *countNode = MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(1));

    MAREA_Uinteger32 first = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(5), &first));

    const MAREA_Uinteger32 COUNT = first;
    const MAREA_Uinteger32 ZERO = (first + MAREA_UINTEGER_32(1));
    const MAREA_Uinteger32 ONE = (first + MAREA_UINTEGER_32(2));
    const MAREA_Uinteger32 INDEX = (first + MAREA_UINTEGER_32(3));
    const MAREA_Uinteger32 TEST = (first + MAREA_UINTEGER_32(4));

    MAREA_Uinteger8 kind = MAREA_BYTECODE_GENERATOR_KIND_NONE;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLowerInto(generator, countNode, COUNT, MAREA_BYTECODE_NUMBER_KIND_SINTEGER_32, &kind));

    if (kind >= MAREA_BYTECODE_NUMBER_KIND_FLOAT_32) {
        return MAREA_BytecodeGeneratorFail(generator, countNode, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A part count is not an integer.");
    }

    const MAREA_Boolean SIGNED = (kind < MAREA_BYTECODE_NUMBER_KIND_UINTEGER_8);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmitConstant(generator, node, ZERO, MAREA_UINTEGER_64(0)));

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmitConstant(generator, node, ONE, MAREA_UINTEGER_64(1)));

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, (SIGNED ? MAREA_BYTECODE_OPCODE_LESS_EQUAL_SIGNED : MAREA_BYTECODE_OPCODE_EQUAL), TEST, COUNT, ZERO, MAREA_NULLPTR));

    MAREA_Uinteger32 validJump = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_JUMP_IF_NOT, TEST, MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), &validJump));

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_TRAP, MAREA_BYTECODE_TRAP_CODE_INVALID_PARTITION_COUNT, MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), MAREA_NULLPTR));

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorPatchJump(generator, node, validJump, (MAREA_Uinteger32)generator->codeCount));

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmitConstant(generator, node, INDEX, MAREA_UINTEGER_64(0)));

    const MAREA_Uinteger32 START = (MAREA_Uinteger32)generator->codeCount;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, (SIGNED ? MAREA_BYTECODE_OPCODE_LESS_SIGNED : MAREA_BYTECODE_OPCODE_LESS_UNSIGNED), TEST, INDEX, COUNT, MAREA_NULLPTR));

    MAREA_Uinteger32 exitJump = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_JUMP_IF_NOT, TEST, MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), &exitJump));

    const MAREA_IntegerSize SYMBOL_MARK = generator->symbolCount;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorPushSymbol(generator, MAREA_IrNodeGetText(node), MAREA_IrNodeGetTextLength(node), MAREA_UINTEGER_32(0), MAREA_BYTECODE_GENERATOR_KIND_NONE, MAREA_BYTECODE_NONE));

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLowerBlock(generator, MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(2))));

    generator->symbolCount = SYMBOL_MARK;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_ADD, INDEX, ONE, kind, MAREA_NULLPTR));

    MAREA_Uinteger32 backJump = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_JUMP, MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), &backJump));

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorPatchJump(generator, node, backJump, START));

    return MAREA_BytecodeGeneratorPatchJump(generator, node, exitJump, (MAREA_Uinteger32)generator->codeCount);
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_LowerJumpOut(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Boolean isBreak)
{
    if (generator->loopCount == MAREA_INTEGER_SIZE(0)) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A break or a continue is outside a loop.");
    }

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorDrops(generator, node, generator->loops[(generator->loopCount - MAREA_INTEGER_SIZE(1))].ownedMark));

    MAREA_Uinteger32 jump = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_JUMP, MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), &jump));

    if (isBreak) {
        return MAREA_G_PushPatch(generator, MAREA_BOOLEAN_TRUE, jump);
    }

    return MAREA_BytecodeGeneratorPatchJump(generator, node, jump, generator->loops[(generator->loopCount - MAREA_INTEGER_SIZE(1))].start);
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorLowerBlock(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node)
{
    const MAREA_IntegerSize SYMBOL_MARK = generator->symbolCount;
    const MAREA_Uinteger32 REGISTER_MARK = generator->nextRegister;
    const MAREA_IntegerSize OWNED_MARK = generator->ownedCount;
    const MAREA_IntegerSize COUNT = MAREA_IrNodeGetChildCount(node);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index += MAREA_INTEGER_SIZE(1)) {
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLowerStatement(generator, MAREA_BytecodeGeneratorChildAt(node, index)));
    }

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorDrops(generator, node, OWNED_MARK));

    generator->ownedCount = OWNED_MARK;
    generator->symbolCount = SYMBOL_MARK;
    generator->nextRegister = REGISTER_MARK;

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorLowerStatement(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node)
{
    if (generator->functionFailed) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    const MAREA_Uinteger32 MARK = generator->nextRegister;

    switch (MAREA_IrNodeGetKind(node)) {
    case MAREA_IR_KIND_LOCAL_DECLARATION:
        return MAREA_G_LowerLocal(generator, node);
    case MAREA_IR_KIND_BLOCK:
        return MAREA_BytecodeGeneratorLowerBlock(generator, node);
    case MAREA_IR_KIND_SCOPE_BLOCK:
    case MAREA_IR_KIND_FORK_JOIN_BLOCK:
        // INFO: the join of a fork_join is its end, because every spawn runs to its end where it stands.
        return MAREA_BytecodeGeneratorLowerBlock(generator, MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(0)));
    case MAREA_IR_KIND_FOR_EACH_STATEMENT:
        return MAREA_BytecodeGeneratorLowerForEach(generator, node);
    case MAREA_IR_KIND_UNREACHABLE_STATEMENT:
        return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_TRAP, MAREA_BYTECODE_TRAP_CODE_UNREACHABLE, MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), MAREA_NULLPTR);
    default:
        break;
    }

    MAREA_BytecodeExcept except = MAREA_BYTECODE_EXCEPT_SUCCESS;

    switch (MAREA_IrNodeGetKind(node)) {
    case MAREA_IR_KIND_ASSIGNMENT:
        except = MAREA_G_LowerAssignment(generator, node);
        break;
    case MAREA_IR_KIND_EXPRESSION_STATEMENT:
        except = MAREA_G_LowerExpressionStatement(generator, node);
        break;
    case MAREA_IR_KIND_RETURN_STATEMENT:
        except = MAREA_G_LowerReturn(generator, node);
        break;
    case MAREA_IR_KIND_IF_STATEMENT:
        except = MAREA_G_LowerIf(generator, node);
        break;
    case MAREA_IR_KIND_WHILE_STATEMENT:
        except = MAREA_G_LowerWhile(generator, node);
        break;
    case MAREA_IR_KIND_PARTITION_STATEMENT:
        except = MAREA_G_LowerPartition(generator, node);
        break;
    case MAREA_IR_KIND_BREAK_STATEMENT:
        except = MAREA_G_LowerJumpOut(generator, node, MAREA_BOOLEAN_TRUE);
        break;
    case MAREA_IR_KIND_CONTINUE_STATEMENT:
        except = MAREA_G_LowerJumpOut(generator, node, MAREA_BOOLEAN_FALSE);
        break;
    default:
        except = MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A statement is not lowered yet.");
        break;
    }

    generator->nextRegister = MARK;

    return except;
}
