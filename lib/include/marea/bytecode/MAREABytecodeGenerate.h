// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BYTECODE_MAREABYTECODEGENERATE_H)
    #define MAREA_INCLUDE_BYTECODE_MAREABYTECODEGENERATE_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/bytecode/MAREABytecode.h"
    #include "marea/ir/MAREAIrNode.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Lowers one IR module into a bytecode module. The IR is read through its public accessors only. One
     * function of the bytecode is made for each FUNCTION_DECLARATION, in source order, and its name is the
     * namespace of the module and the name of the function, joined by a dot. A call finds its function by that name.
     *
     * The scalar slices are lowered: the sinteger8 to uinteger64, float32, float64 and boolean values, the
     * constants of the unit (each one is lowered again where it is read), locals, assignments, return, if, while,
     * break, continue, a scope, unreachable!(), the operators, convert, a call of a function of the same module
     * and build_value! (which gives its default, because this stage has no build settings). Each other
     * construct raises a diagnostic: 5033 for a type that is not a scalar, 5034 for a for each, 5030 for any other
     * construct, 5031 for a name that is not found and 5032 for an operand that does not fit 16 bits (the
     * registers, the distance of a jump, the data blobs). One bad function raises one diagnostic, at the first
     * fault, and the other functions are still lowered.
     * @param context Context that owns the module and the sink. Must not be null.
     * @param irModule Root of the IR, a MODULE node. Must not be null.
     * @param outModule Receives the module, or null when a diagnostic was raised. Must not be null.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS (also when a diagnostic was raised),
     * MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER when an argument is null,
     * MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE when irModule is not a MODULE node, or
     * MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY when the arena cannot grow.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGenerate(MAREA_BytecodeContext * context, const MAREA_IrNode *irModule, MAREA_BytecodeModule **outModule);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_BYTECODE_MAREABYTECODEGENERATE_H */
