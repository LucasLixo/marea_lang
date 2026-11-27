// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/bytecode/MAREABytecodeInternal.h"
#include "marea/bytecode/MAREABytecodeOpcode.h"

static const MAREA_Character8 *const MAREA_G_TYPE_KIND_NAMES[] = {
    "primitive",
    "structure",
    "enum",
    "array",
    "vector",
    "pointer",
    "owned",
    "slice",
    "task",
    "function",
};

const MAREA_Character8 *MAREA_CALL MAREA_BytecodeTypeKindGetName(MAREA_Uinteger32 kind)
{
    if (kind >= MAREA_ARRAY_SIZE(MAREA_G_TYPE_KIND_NAMES)) {
        return MAREA_NULLPTR;
    }

    return MAREA_G_TYPE_KIND_NAMES[kind];
}

static MAREA_Boolean MAREA_CALL MAREA_G_OptionalIndexIsValid(MAREA_Uinteger32 index, MAREA_IntegerSize count)
{
    return ((index == MAREA_BYTECODE_NONE) || (index < count));
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_RaiseBadReference(const MAREA_BytecodeModule *module, MAREA_Boolean *outValid, const MAREA_Character8 *message)
{
    *outValid = MAREA_BOOLEAN_FALSE;

    return MAREA_BytecodeRaise(module->context, MAREA_BYTECODE_DIAGNOSTIC_CODE_BAD_REFERENCE, MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), message);
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_ValidateTypes(const MAREA_BytecodeModule *module, MAREA_Boolean *outValid)
{
    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < module->typeCount; index += MAREA_INTEGER_SIZE(1)) {
        const MAREA_BytecodeType *type = &module->types[index];

        if (MAREA_BytecodeTypeKindGetName((MAREA_Uinteger32)type->info.kind) == MAREA_NULLPTR) {
            *outValid = MAREA_BOOLEAN_FALSE;

            return MAREA_BytecodeRaise(module->context, MAREA_BYTECODE_DIAGNOSTIC_CODE_MALFORMED_SECTION, MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), "A type has a kind that is not in the table.");
        }

        if (!MAREA_G_OptionalIndexIsValid(type->info.name, module->stringCount)) {
            return MAREA_G_RaiseBadReference(module, outValid, "A type name is not in the string pool.");
        }

        if (!MAREA_G_OptionalIndexIsValid(type->info.element, module->typeCount)) {
            return MAREA_G_RaiseBadReference(module, outValid, "A type element is not in the type table.");
        }

        for (MAREA_Uinteger32 member = MAREA_UINTEGER_32(0); member < type->info.memberCount; member += MAREA_UINTEGER_32(1)) {
            if (!MAREA_G_OptionalIndexIsValid(type->members[member].name, module->stringCount)) {
                return MAREA_G_RaiseBadReference(module, outValid, "A member name is not in the string pool.");
            }

            if (type->members[member].type >= module->typeCount) {
                return MAREA_G_RaiseBadReference(module, outValid, "A member type is not in the type table.");
            }
        }
    }

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_ValidateFunctions(const MAREA_BytecodeModule *module, MAREA_Boolean *outValid)
{
    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < module->functionCount; index += MAREA_INTEGER_SIZE(1)) {
        const MAREA_BytecodeFunction *function = &module->functions[index];

        if (!MAREA_G_OptionalIndexIsValid(function->info.name, module->stringCount)) {
            return MAREA_G_RaiseBadReference(module, outValid, "A function name is not in the string pool.");
        }

        if (!MAREA_G_OptionalIndexIsValid(function->info.result, module->typeCount)) {
            return MAREA_G_RaiseBadReference(module, outValid, "A function result is not in the type table.");
        }

        for (MAREA_Uinteger32 parameter = MAREA_UINTEGER_32(0); parameter < function->info.parameterCount; parameter += MAREA_UINTEGER_32(1)) {
            if (function->parameters[parameter].type >= module->typeCount) {
                return MAREA_G_RaiseBadReference(module, outValid, "A parameter type is not in the type table.");
            }
        }

        for (MAREA_Uinteger32 instruction = MAREA_UINTEGER_32(0); instruction < function->info.instructionCount; instruction += MAREA_UINTEGER_32(1)) {
            if (MAREA_BytecodeOpcodeGetName(function->instructions[instruction].opcode) == MAREA_NULLPTR) {
                *outValid = MAREA_BOOLEAN_FALSE;

                return MAREA_BytecodeRaise(module->context, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNKNOWN_OPCODE, MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), "An instruction has an opcode that is not in the table.");
            }
        }
    }

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeModuleValidate(const MAREA_BytecodeModule *module, MAREA_Boolean *outValid)
{
    if (module == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outValid == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    *outValid = MAREA_BOOLEAN_TRUE;

    MAREA_BytecodeExcept except = MAREA_G_ValidateTypes(module, outValid);

    if ((except != MAREA_BYTECODE_EXCEPT_SUCCESS) || !(*outValid)) {
        return except;
    }

    return MAREA_G_ValidateFunctions(module, outValid);
}
