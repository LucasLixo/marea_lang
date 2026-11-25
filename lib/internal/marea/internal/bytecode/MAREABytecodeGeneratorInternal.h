// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INTERNAL_BYTECODE_MAREABYTECODEGENERATORINTERNAL_H)
    #define MAREA_INTERNAL_BYTECODE_MAREABYTECODEGENERATORINTERNAL_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREAArena.h"
    #include "marea/bytecode/MAREABytecode.h"
    #include "marea/bytecode/MAREABytecodeOpcode.h"
    #include "marea/ir/MAREAIrNode.h"
    #include "marea/lex/MAREATokenKind.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Passes the failure of an expression that returns a MAREA_BytecodeExcept up to the caller.
     */
    #if !defined(MAREA_BYTECODE_TRY)
        #define MAREA_BYTECODE_TRY(expression)                              \
            do {                                                            \
                const MAREA_BytecodeExcept marea_try_except = (expression); \
                if (marea_try_except != MAREA_BYTECODE_EXCEPT_SUCCESS) {    \
                    return marea_try_except;                                \
                }                                                           \
            } while (0)
    #endif

    /**
     * @brief Kinds of a value that the lowering tracks. The values below MAREA_BYTECODE_NUMBER_KIND_COUNT are the
     * number kinds of the opcodes, so a kind is also the operand of a typed opcode.
     */
    typedef enum MAREA_BytecodeGeneratorKind
    {
        MAREA_BYTECODE_GENERATOR_KIND_BOOLEAN = MAREA_UINTEGER_8(10),
        MAREA_BYTECODE_GENERATOR_KIND_VOID = MAREA_UINTEGER_8(11),
        MAREA_BYTECODE_GENERATOR_KIND_NONE = MAREA_UINTEGER_8(12),
    } MAREA_BytecodeGeneratorKind;

    /**
     * @brief Largest register index that the lowering gives out. The operand of an instruction has 16 bits.
     */
    #if !defined(MAREA_BYTECODE_GENERATOR_REGISTER_LIMIT)
        #define MAREA_BYTECODE_GENERATOR_REGISTER_LIMIT MAREA_UINTEGER_32(65535)
    #endif

    /**
     * @brief Number of levels of constants that read constants. A cycle stops at this depth.
     */
    #if !defined(MAREA_BYTECODE_GENERATOR_CONSTANT_DEPTH)
        #define MAREA_BYTECODE_GENERATOR_CONSTANT_DEPTH MAREA_UINTEGER_32(32)
    #endif

    /**
     * @brief One name in scope: a parameter or a local, and the register that holds it.
     */
    typedef struct MAREA_BytecodeGeneratorSymbol
    {
        const MAREA_Character8 *name;
        MAREA_IntegerSize length;
        MAREA_Uinteger32 registerIndex;
        MAREA_Uinteger8 kind;
        MAREA_Uinteger8 padding[3];
    } MAREA_BytecodeGeneratorSymbol;

    /**
     * @brief One function of the IR module, known before any body is lowered, so a call finds it by name.
     * parameterKinds and resultKind hold MAREA_BYTECODE_GENERATOR_KIND_NONE for a type that is not a scalar.
     */
    typedef struct MAREA_BytecodeGeneratorFunction
    {
        const MAREA_IrNode *node;
        const MAREA_Character8 *name;
        MAREA_Uinteger8 *parameterKinds;
        MAREA_IntegerSize nameLength;
        MAREA_Uinteger32 parameterCount;
        MAREA_Uinteger8 resultKind;
        MAREA_Uinteger8 padding[3];
    } MAREA_BytecodeGeneratorFunction;

    /**
     * @brief One loop that is open: where a continue goes, and where its break jumps start in the patch list.
     */
    typedef struct MAREA_BytecodeGeneratorLoop
    {
        MAREA_IntegerSize patchMark;
        MAREA_Uinteger32 start;
        MAREA_Uinteger8 padding[4];
    } MAREA_BytecodeGeneratorLoop;

    /**
     * @brief The state of one MAREA_BytecodeGenerate call. Everything lives in the arena of the context.
     */
    typedef struct MAREA_BytecodeGenerator
    {
        MAREA_BytecodeContext *context;
        MAREA_BytecodeModule *module;
        MAREA_Arena *arena;
        const MAREA_IrNode *root;
        const MAREA_Character8 *namespaceText;
        MAREA_BytecodeGeneratorFunction *functions;
        const MAREA_IrNode **constants;
        MAREA_Uinteger64 *dataValues;
        MAREA_BytecodeInstruction *code;
        MAREA_BytecodeGeneratorSymbol *symbols;
        MAREA_BytecodeGeneratorLoop *loops;
        MAREA_Uinteger32 *patches;
        const MAREA_BytecodeGeneratorFunction *current;
        MAREA_IntegerSize namespaceLength;
        MAREA_IntegerSize functionCount;
        MAREA_IntegerSize functionCapacity;
        MAREA_IntegerSize constantCount;
        MAREA_IntegerSize constantCapacity;
        MAREA_IntegerSize dataCount;
        MAREA_IntegerSize dataCapacity;
        MAREA_IntegerSize codeCount;
        MAREA_IntegerSize codeCapacity;
        MAREA_IntegerSize symbolCount;
        MAREA_IntegerSize symbolCapacity;
        MAREA_IntegerSize loopCount;
        MAREA_IntegerSize loopCapacity;
        MAREA_IntegerSize patchCount;
        MAREA_IntegerSize patchCapacity;
        MAREA_Uinteger32 typeIndex[MAREA_BYTECODE_NUMBER_KIND_COUNT + 1];
        MAREA_Uinteger32 nextRegister;
        MAREA_Uinteger32 maxRegister;
        MAREA_Uinteger32 constantDepth;
        MAREA_Boolean functionFailed;
        MAREA_Boolean anyFailed;
    } MAREA_BytecodeGenerator;

    /**
     * @brief Makes room for one more item in an array of the generator, as MAREA_BytecodeGrow does.
     * @param generator State of the call. Must not be null.
     * @param current The array now, or null when it is empty.
     * @param capacity In and out: the capacity in items. Must not be null.
     * @param count Items in use.
     * @param itemSize Size of one item.
     * @param alignment Alignment of one item.
     * @param outItems Receives the array, which is the same block when nothing grew. Must not be null.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS or MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorGrow(MAREA_BytecodeGenerator * generator, MAREA_Null current, MAREA_IntegerSize * capacity, MAREA_IntegerSize count, MAREA_IntegerSize itemSize, MAREA_IntegerSize alignment, MAREA_Null * outItems);

    /**
     * @brief Raises one diagnostic at the position of an IR node, and marks the function as failed. Only the first
     * fault of a function is raised: when the function has already failed, nothing is done.
     * @param generator State of the call. Must not be null.
     * @param node Node that holds the fault. Must not be null.
     * @param code Diagnostic code.
     * @param message NUL-terminated text of this stage. Must not be null.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS or MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorFail(MAREA_BytecodeGenerator * generator, const MAREA_IrNode *node, MAREA_BytecodeDiagnosticCode code, const MAREA_Character8 *message);

    /**
     * @brief Appends one instruction to the code of the function that is lowered. Nothing is added after a fault.
     * @param generator State of the call. Must not be null.
     * @param opcode Opcode of the instruction.
     * @param a First operand.
     * @param b Second operand.
     * @param c Third operand.
     * @param outIndex Receives the index of the instruction, or null when it is not needed.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS or MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorEmit(MAREA_BytecodeGenerator * generator, MAREA_Uinteger16 opcode, MAREA_Uinteger32 a, MAREA_Uinteger32 b, MAREA_Uinteger32 c, MAREA_Uinteger32 * outIndex);

    /**
     * @brief Gives the target of a jump that was emitted before the target was known.
     * @param generator State of the call. Must not be null.
     * @param node Node for the diagnostic when the distance does not fit 16 bits. Must not be null.
     * @param index Index of the jump instruction.
     * @param target Index of the instruction that the jump must reach.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS or MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorPatchJump(MAREA_BytecodeGenerator * generator, const MAREA_IrNode *node, MAREA_Uinteger32 index, MAREA_Uinteger32 target);

    /**
     * @brief Reserves consecutive registers at the top of the frame.
     * @param generator State of the call. Must not be null.
     * @param node Node for the diagnostic when the frame is full. Must not be null.
     * @param count Number of registers.
     * @param outFirst Receives the first register. It is 0 after a fault.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS or MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorAllocate(MAREA_BytecodeGenerator * generator, const MAREA_IrNode *node, MAREA_Uinteger32 count, MAREA_Uinteger32 *outFirst);

    /**
     * @brief Loads a 64-bit register constant: one immediate when the bits are a signed 32-bit value, else a blob of
     * the data section that is shared with an equal one.
     * @param generator State of the call. Must not be null.
     * @param node Node for the diagnostic. Must not be null.
     * @param destination Register that receives the value.
     * @param bits The 64 bits of the register.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS or MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorEmitConstant(MAREA_BytecodeGenerator * generator, const MAREA_IrNode *node, MAREA_Uinteger32 destination, MAREA_Uinteger64 bits);

    /**
     * @brief Adds a name to the scope, bound to a register.
     * @param generator State of the call. Must not be null.
     * @param name Bytes of the name. Must not be null.
     * @param length Number of bytes.
     * @param registerIndex Register of the value.
     * @param kind Kind of the value.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS or MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorPushSymbol(MAREA_BytecodeGenerator * generator, const MAREA_Character8 *name, MAREA_IntegerSize length, MAREA_Uinteger32 registerIndex, MAREA_Uinteger8 kind);

    /**
     * @brief Finds the innermost name in scope.
     * @param generator State of the call. Must not be null.
     * @param name Bytes of the name. Must not be null.
     * @param length Number of bytes.
     * @return The symbol, or null when the name is not in scope.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_BytecodeGeneratorSymbol *MAREA_CALL MAREA_BytecodeGeneratorFindSymbol(const MAREA_BytecodeGenerator *generator, const MAREA_Character8 *name, MAREA_IntegerSize length);

    /**
     * @brief Reads one child of an IR node.
     * @param node Node to read. May be null.
     * @param index Index of the child.
     * @return The child, or null when the node is null or has no such child.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_IrNode *MAREA_CALL MAREA_BytecodeGeneratorChildAt(const MAREA_IrNode *node, MAREA_IntegerSize index);

    /**
     * @brief Reads a resolved type as a kind.
     * @param text Bytes of the type. May be null.
     * @param length Number of bytes.
     * @return A number kind, MAREA_BYTECODE_GENERATOR_KIND_BOOLEAN, or MAREA_BYTECODE_GENERATOR_KIND_NONE for any
     * other type.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Uinteger8 MAREA_CALL MAREA_BytecodeGeneratorKindFromText(const MAREA_Character8 *text, MAREA_IntegerSize length);

    /**
     * @brief Reads the resolved type of a node as a kind.
     * @param node Node to read. May be null.
     * @return As MAREA_BytecodeGeneratorKindFromText, and MAREA_BYTECODE_GENERATOR_KIND_NONE for a node with no type.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Uinteger8 MAREA_CALL MAREA_BytecodeGeneratorKindOfNode(const MAREA_IrNode *node);

    /**
     * @brief Returns the index of the primitive type of a kind in the type table, and adds it on the first use.
     * @param generator State of the call. Must not be null.
     * @param kind A number kind or MAREA_BYTECODE_GENERATOR_KIND_BOOLEAN.
     * @param outIndex Receives the index. Must not be null.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS or MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorTypeIndex(MAREA_BytecodeGenerator * generator, MAREA_Uinteger8 kind, MAREA_Uinteger32 * outIndex);

    /**
     * @brief Reads the text of an integer literal: decimal, 0x, 0b, with underscores between digits.
     * @param text Bytes of the literal. Must not be null.
     * @param length Number of bytes.
     * @param outValue Receives the value. Must not be null.
     * @return MAREA_BOOLEAN_TRUE when the text is a literal that fits 64 bits, otherwise MAREA_BOOLEAN_FALSE.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Boolean MAREA_CALL MAREA_BytecodeGeneratorParseInteger(const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_Uinteger64 *outValue);

    /**
     * @brief Reads the text of a float literal, with underscores between digits.
     * @param text Bytes of the literal. Must not be null.
     * @param length Number of bytes.
     * @param outValue Receives the value. Must not be null.
     * @return MAREA_BOOLEAN_TRUE when the whole text is a number, otherwise MAREA_BOOLEAN_FALSE.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Boolean MAREA_CALL MAREA_BytecodeGeneratorParseFloat(const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_Float64 *outValue);

    /**
     * @brief Lowers an expression into a given register.
     * @param generator State of the call. Must not be null.
     * @param node Expression to lower. Must not be null.
     * @param destination Register that receives the value. The expression never reads it before it writes it.
     * @param hint Kind that a literal takes when its own type is unknown, or MAREA_BYTECODE_GENERATOR_KIND_NONE.
     * @param outKind Receives the kind of the value, MAREA_BYTECODE_GENERATOR_KIND_VOID for a call with no result.
     * Must not be null.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS or MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorLowerInto(MAREA_BytecodeGenerator * generator, const MAREA_IrNode *node, MAREA_Uinteger32 destination, MAREA_Uinteger8 hint, MAREA_Uinteger8 *outKind);

    /**
     * @brief Lowers an expression to some register that the caller only reads: the register of a name when the
     * expression is a name, else a new temporary.
     * @param generator State of the call. Must not be null.
     * @param node Expression to lower. Must not be null.
     * @param hint As for MAREA_BytecodeGeneratorLowerInto.
     * @param outRegister Receives the register. Must not be null.
     * @param outKind Receives the kind. Must not be null.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS or MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorLowerOperand(MAREA_BytecodeGenerator * generator, const MAREA_IrNode *node, MAREA_Uinteger8 hint, MAREA_Uinteger32 *outRegister, MAREA_Uinteger8 *outKind);

    /**
     * @brief Finds the opcode of a binary operator or of a compound assignment operator, and how it is emitted.
     * @param token Operator token.
     * @param outOpcode Receives the opcode of an arithmetic, bit or shift operator. Must not be null.
     * @param outThreeRegisters Receives MAREA_BOOLEAN_TRUE for a bit operator, whose opcode takes a destination and
     * two sources, and MAREA_BOOLEAN_FALSE for a typed in-place opcode. Must not be null.
     * @return MAREA_BOOLEAN_TRUE when the operator is an arithmetic, bit or shift operator or its compound form.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Boolean MAREA_CALL MAREA_BytecodeGeneratorOperatorOpcode(MAREA_TokenKind token, MAREA_Uinteger16 * outOpcode, MAREA_Boolean * outThreeRegisters);

    /**
     * @brief Lowers one statement.
     * @param generator State of the call. Must not be null.
     * @param node Statement to lower. Must not be null.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS or MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorLowerStatement(MAREA_BytecodeGenerator * generator, const MAREA_IrNode *node);

    /**
     * @brief Lowers a BLOCK in its own scope: the names and the registers it declares end with it.
     * @param generator State of the call. Must not be null.
     * @param node Block to lower. Must not be null.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS or MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorLowerBlock(MAREA_BytecodeGenerator * generator, const MAREA_IrNode *node);

MAREA_END_EXTERN_C

#endif /* MAREA_INTERNAL_BYTECODE_MAREABYTECODEGENERATORINTERNAL_H */
