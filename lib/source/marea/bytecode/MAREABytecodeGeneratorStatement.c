// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/bytecode/MAREABytecodeGeneratorInternal.h"

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_LowerLocal(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node)
{
    const MAREA_Uinteger8 KIND = MAREA_BytecodeGeneratorKindOfNode(node);

    if (!((KIND < MAREA_BYTECODE_NUMBER_KIND_COUNT) || (KIND == MAREA_BYTECODE_GENERATOR_KIND_BOOLEAN))) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A local has a type that is not a scalar.");
    }

    const MAREA_IrNode *value = MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(0));

    if (value == MAREA_NULLPTR) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A local has no initial value.");
    }

    MAREA_Uinteger32 registerIndex = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), &registerIndex));

    MAREA_Uinteger8 valueKind = MAREA_BYTECODE_GENERATOR_KIND_NONE;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLowerInto(generator, value, registerIndex, KIND, &valueKind));

    // INFO: the name is bound after its value, so the value cannot read the local that it makes.
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorPushSymbol(generator, MAREA_IrNodeGetText(node), MAREA_IrNodeGetTextLength(node), registerIndex, KIND));

    generator->nextRegister = (registerIndex + MAREA_UINTEGER_32(1));

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_LowerAssignment(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node)
{
    const MAREA_IrNode *target = MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(0));
    const MAREA_IrNode *value = MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(1));

    if ((target == MAREA_NULLPTR) || (value == MAREA_NULLPTR) || (MAREA_IrNodeGetKind(target) != MAREA_IR_KIND_IDENTIFIER)) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "Only a local or a parameter can be assigned yet.");
    }

    const MAREA_BytecodeGeneratorSymbol *symbol = MAREA_BytecodeGeneratorFindSymbol(generator, MAREA_IrNodeGetText(target), MAREA_IrNodeGetTextLength(target));

    if (symbol == MAREA_NULLPTR) {
        return MAREA_BytecodeGeneratorFail(generator, target, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNRESOLVED_NAME, "An assigned name is not a local or a parameter.");
    }

    const MAREA_TokenKind OPERATOR = MAREA_IrNodeGetOperator(node);
    const MAREA_Uinteger32 TARGET = symbol->registerIndex;
    const MAREA_Uinteger8 KIND = symbol->kind;

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
        return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_RETURN_VOID, MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), MAREA_NULLPTR);
    }

    MAREA_Uinteger32 registerIndex = MAREA_UINTEGER_32(0);
    MAREA_Uinteger8 kind = MAREA_BYTECODE_GENERATOR_KIND_NONE;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLowerOperand(generator, MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(0)), generator->current->resultKind, &registerIndex, &kind));

    return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_RETURN, registerIndex, MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), MAREA_NULLPTR);
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_LowerExpressionStatement(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node)
{
    MAREA_Uinteger32 temporary = MAREA_UINTEGER_32(0);
    MAREA_Uinteger8 kind = MAREA_BYTECODE_GENERATOR_KIND_NONE;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), &temporary));

    return MAREA_BytecodeGeneratorLowerInto(generator, MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(0)), temporary, MAREA_BYTECODE_GENERATOR_KIND_NONE, &kind);
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorLowerBlock(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node)
{
    const MAREA_IntegerSize SYMBOL_MARK = generator->symbolCount;
    const MAREA_Uinteger32 REGISTER_MARK = generator->nextRegister;
    const MAREA_IntegerSize COUNT = MAREA_IrNodeGetChildCount(node);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index += MAREA_INTEGER_SIZE(1)) {
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLowerStatement(generator, MAREA_BytecodeGeneratorChildAt(node, index)));
    }

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
        return MAREA_BytecodeGeneratorLowerBlock(generator, MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(0)));
    case MAREA_IR_KIND_FOR_EACH_STATEMENT:
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNWALKABLE_COLLECTION, "No collection type can be walked by a for each yet.");
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
    default:
        except = MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A statement is not lowered yet.");
        break;
    }

    generator->nextRegister = MARK;

    return except;
}
