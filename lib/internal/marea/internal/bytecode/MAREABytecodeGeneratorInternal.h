// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INTERNAL_BYTECODE_MAREABYTECODEGENERATORINTERNAL_H)
    #define MAREA_INTERNAL_BYTECODE_MAREABYTECODEGENERATORINTERNAL_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREAArena.h"
    #include "marea/base/MAREATypeRegistry.h"
    #include "marea/bytecode/MAREABytecode.h"
    #include "marea/bytecode/MAREABytecodeOpcode.h"
    #include "marea/ir/MAREAIrLayout.h"
    #include "marea/ir/MAREAIrNode.h"
    #include "marea/lex/MAREATokenKind.h"
    #include "marea/parse/MAREAAst.h"

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
        // INFO: a value of this kind is a structure or a plain-data enum: its register holds the address of its
        // bytes (in this frame's own memory area, or in an ancestor's, when it came in as a parameter), never the
        // bytes themselves. MAREA_BytecodeGeneratorAggregateKind is the only place that gives this kind out, and it
        // also gives the bytecode type index, the size and the alignment that go with it.
        MAREA_BYTECODE_GENERATOR_KIND_AGGREGATE = MAREA_UINTEGER_8(13),
    } MAREA_BytecodeGeneratorKind;

    /**
     * @brief Largest register index that the lowering gives out. The operand of an instruction has 16 bits.
     */
    #if !defined(MAREA_BYTECODE_GENERATOR_REGISTER_LIMIT)
        #define MAREA_BYTECODE_GENERATOR_REGISTER_LIMIT MAREA_UINTEGER_32(65535)
    #endif

    /**
     * @brief Largest byte offset the frame-area bump allocator gives out. The operand of addr_of and field is 16
     * bits, and MAREA_VM_FRAME_MEMORY_LIMIT of the vm internal header must stay at or above this.
     */
    #if !defined(MAREA_BYTECODE_GENERATOR_FRAME_LIMIT)
        #define MAREA_BYTECODE_GENERATOR_FRAME_LIMIT MAREA_UINTEGER_32(4096)
    #endif

    /**
     * @brief Number of levels of constants that read constants. A cycle stops at this depth.
     */
    #if !defined(MAREA_BYTECODE_GENERATOR_CONSTANT_DEPTH)
        #define MAREA_BYTECODE_GENERATOR_CONSTANT_DEPTH MAREA_UINTEGER_32(32)
    #endif

    /**
     * @brief One name in scope: a parameter or a local, and the register that holds it. aggregateType is the
     * bytecode type index of the structure or enum the register's address names, or MAREA_BYTECODE_NONE when kind
     * is not MAREA_BYTECODE_GENERATOR_KIND_AGGREGATE.
     */
    typedef struct MAREA_BytecodeGeneratorSymbol
    {
        const MAREA_Character8 *name;
        MAREA_IntegerSize length;
        MAREA_Uinteger32 registerIndex;
        MAREA_Uinteger32 aggregateType;
        MAREA_Uinteger8 kind;
        // INFO: the register holds the address of a scalar, so the name reads and writes that place.
        MAREA_Boolean isPlace;
        MAREA_Uinteger8 padding[2];
    } MAREA_BytecodeGeneratorSymbol;

    /**
     * @brief One structure that has been given a row of the bytecode type table: the IR declaration (for its field
     * list, read again whenever a field is read or written) and the row it was given (for size and alignment).
     * Cached by name the first time a local, a parameter or a field needs it, the same way MAREA_BytecodeGenerator's
     * own typeIndex caches a primitive.
     */
    typedef struct MAREA_BytecodeGeneratorAggregate
    {
        const MAREA_IrNode *declaration;
        const MAREA_IrLayout *layout;
        const MAREA_Character8 *name;
        MAREA_IntegerSize nameLength;
        MAREA_Uinteger32 typeIndex;
        MAREA_Uinteger64 size;
        MAREA_Uinteger64 alignment;
        // INFO: an inline run holds count elements of elementKind, one every stride bytes. It has no declaration
        // and no layout.
        MAREA_Uinteger64 count;
        MAREA_Uinteger64 stride;
        MAREA_Uinteger32 elementAggregate;
        MAREA_Uinteger8 elementKind;
        MAREA_Boolean isRun;
    } MAREA_BytecodeGeneratorAggregate;

    /**
     * @brief One function of the IR module, known before any body is lowered, so a call finds it by name.
     * parameterKinds and resultKind hold MAREA_BYTECODE_GENERATOR_KIND_NONE for a type that is not a scalar.
     */
    typedef struct MAREA_BytecodeGeneratorFunction
    {
        const MAREA_IrNode *node;
        const MAREA_Character8 *name;
        MAREA_Uinteger8 *parameterKinds;
        // INFO: the aggregate of each parameter that is one, else MAREA_BYTECODE_NONE.
        MAREA_Uinteger32 *parameterAggregates;
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
     * @brief One import: a call of a registered C function, known by its full name and its shape. parameters holds
     * the type and the flags of every argument, result is a type index or MAREA_BYTECODE_NONE. Two calls share an
     * import when the name, the parameters and the result are the same.
     */
    typedef struct MAREA_BytecodeGeneratorImport
    {
        const MAREA_Character8 *name;
        const MAREA_BytecodeParameterInfo *parameters;
        MAREA_IntegerSize nameLength;
        MAREA_Uinteger32 parameterCount;
        MAREA_Uinteger32 result;
    } MAREA_BytecodeGeneratorImport;

    /**
     * @brief The state of one MAREA_BytecodeGenerate call. patches holds the jumps of the break statements of the open
     * loops, and endPatches the jumps that end the arms of the open if statements: two lists, because a break stands
     * inside an if. Everything lives in the arena of the context.
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
        // INFO: the number of blobs that hold a block of bytes, which stand between the constants of dataValues.
        MAREA_Uinteger32 *dataBlobs;
        MAREA_BytecodeInstruction *code;
        MAREA_BytecodeGeneratorSymbol *symbols;
        MAREA_BytecodeGeneratorLoop *loops;
        MAREA_Uinteger32 *patches;
        MAREA_Uinteger32 *endPatches;
        MAREA_BytecodeGeneratorAggregate *aggregates;
        MAREA_BytecodeGeneratorImport *imports;
        const MAREA_BytecodeGeneratorFunction *current;
        MAREA_IntegerSize namespaceLength;
        MAREA_IntegerSize functionCount;
        MAREA_IntegerSize functionCapacity;
        MAREA_IntegerSize constantCount;
        MAREA_IntegerSize constantCapacity;
        MAREA_IntegerSize dataCount;
        MAREA_IntegerSize dataCapacity;
        MAREA_IntegerSize dataBlobCapacity;
        MAREA_IntegerSize codeCount;
        MAREA_IntegerSize codeCapacity;
        MAREA_IntegerSize symbolCount;
        MAREA_IntegerSize symbolCapacity;
        MAREA_IntegerSize loopCount;
        MAREA_IntegerSize loopCapacity;
        MAREA_IntegerSize patchCount;
        MAREA_IntegerSize patchCapacity;
        MAREA_IntegerSize endPatchCount;
        MAREA_IntegerSize endPatchCapacity;
        MAREA_IntegerSize aggregateCount;
        MAREA_IntegerSize aggregateCapacity;
        MAREA_IntegerSize importCount;
        MAREA_IntegerSize importCapacity;
        MAREA_Uinteger32 typeIndex[MAREA_BYTECODE_NUMBER_KIND_COUNT + 1];
        MAREA_Uinteger32 nextRegister;
        MAREA_Uinteger32 maxRegister;
        MAREA_Uinteger32 nextFrameOffset;
        MAREA_Uinteger32 maxFrameOffset;
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
     * @param aggregateIndex The structure's index into generator->aggregates when kind is
     * MAREA_BYTECODE_GENERATOR_KIND_AGGREGATE, else MAREA_BYTECODE_NONE.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS or MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorPushSymbol(MAREA_BytecodeGenerator * generator, const MAREA_Character8 *name, MAREA_IntegerSize length, MAREA_Uinteger32 registerIndex, MAREA_Uinteger8 kind, MAREA_Uinteger32 aggregateIndex);

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
     * @brief Reads the kind of a node's resolved type, the same as MAREA_BytecodeGeneratorKindOfNode, except that a
     * type that names a structure of this unit (declared with STRUCTURE_DECLARATION, measured by sema, no field
     * left undecided) gives MAREA_BYTECODE_GENERATOR_KIND_AGGREGATE instead of MAREA_BYTECODE_GENERATOR_KIND_NONE.
     * The structure's own row of the bytecode type table is added on its first use (its fields, scalar or a
     * nested structure, each a member with its offset read from the IR layout table) and cached by name after
     * that, the same way a primitive's row is cached by typeIndex.
     * @param generator State of the call. Must not be null.
     * @param node Node to read. May be null.
     * @param outKind Receives the kind. Must not be null.
     * @param outAggregateIndex Receives the index into generator->aggregates, or MAREA_BYTECODE_NONE when outKind
     * is not MAREA_BYTECODE_GENERATOR_KIND_AGGREGATE. Must not be null.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS or MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorAggregateKind(MAREA_BytecodeGenerator * generator, const MAREA_IrNode *node, MAREA_Uinteger8 *outKind, MAREA_Uinteger32 *outAggregateIndex);

    /**
     * @brief Finds one field of a structure already in generator->aggregates by its written name.
     * @param generator State of the call. Must not be null.
     * @param aggregateIndex Index into generator->aggregates, as MAREA_BytecodeGeneratorAggregateKind gave it.
     * @param name Bytes of the field name. Must not be null.
     * @param length Number of bytes.
     * @param outOffset Receives the byte offset of the field from the start of the structure. Must not be null.
     * @param outField Receives the FIELD_DECLARATION node of the field, for its own resolved type. Must not be
     * null.
     * @return MAREA_BOOLEAN_TRUE when the structure declares a field of that name, else MAREA_BOOLEAN_FALSE.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Boolean MAREA_CALL MAREA_BytecodeGeneratorAggregateField(const MAREA_BytecodeGenerator *generator, MAREA_Uinteger32 aggregateIndex, const MAREA_Character8 *name, MAREA_IntegerSize length, MAREA_Uinteger64 *outOffset, const MAREA_IrNode **outField);

    /**
     * @brief Reserves a byte range of the frame's own memory area, at or above the given alignment.
     * @param generator State of the call. Must not be null.
     * @param node Node for the diagnostic when the frame area is full. Must not be null.
     * @param size Number of bytes to reserve.
     * @param alignment Alignment the offset must land on; 1 when the caller has none to ask for.
     * @param outOffset Receives the offset. It is 0 after a fault. Must not be null.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS or MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorAllocateFrame(MAREA_BytecodeGenerator * generator, const MAREA_IrNode *node, MAREA_Uinteger64 size, MAREA_Uinteger64 alignment, MAREA_Uinteger32 *outOffset);

    /**
     * @brief Lowers an expression that names the address of a structure value, without copying it: the register of
     * a local or a parameter when the expression is a plain name, or a field's address when it is a MEMBER_ACCESS
     * on another structure. Used to read a structure (MAREA_G_LowerMemberAccess) and as the source of a copy or a
     * move (MAREA_BytecodeGeneratorConstructAggregate).
     * @param generator State of the call. Must not be null.
     * @param node Expression to lower. Must not be null.
     * @param outRegister Receives the register that holds the address. Must not be null.
     * @param outAggregateIndex Receives the structure's index into generator->aggregates. Must not be null.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS or MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorAggregateAddress(MAREA_BytecodeGenerator * generator, const MAREA_IrNode *node, MAREA_Uinteger32 *outRegister, MAREA_Uinteger32 *outAggregateIndex);

    /**
     * @brief Builds a structure value at a given address: the zero rule followed by a field store for a
     * LITERAL_VALUE, a byte copy for a plain name or a field read of another structure, or a byte copy that then
     * zeroes the source for `move name`. Recurses into MAREA_BytecodeGeneratorConstructAggregate for a field whose
     * own value is itself a nested LITERAL_VALUE, so a structure inside a structure is built in one pass.
     * @param generator State of the call. Must not be null.
     * @param node The LITERAL_VALUE, the name, the field read or the `move` to build from. Must not be null.
     * @param destinationAddress Register that already holds the address the value is built at (MAREA_BYTECODE_OPCODE_ADDRESS_OF
     * or MAREA_BYTECODE_OPCODE_FIELD, emitted by the caller). Must not be null.
     * @param aggregateIndex The structure's index into generator->aggregates, the type destinationAddress is built
     * as.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS or MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorConstructAggregate(MAREA_BytecodeGenerator * generator, const MAREA_IrNode *node, MAREA_Uinteger32 destinationAddress, MAREA_Uinteger32 aggregateIndex);

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
     * @brief Lowers a CALL_EXPRESSION: a convert, a call of a function of this unit, or a call of a registered C
     * function.
     * @param generator State of the call. Must not be null.
     * @param node Call to lower. Must not be null.
     * @param destination Register that receives the result, the address of the result for a structure.
     * @param valueUsed MAREA_BOOLEAN_FALSE for a call statement, which may have no result and no type.
     * @param outKind Receives the kind of the result, MAREA_BYTECODE_GENERATOR_KIND_VOID for no result. Must not be
     * null.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS or MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorLowerCall(MAREA_BytecodeGenerator * generator, const MAREA_IrNode *node, MAREA_Uinteger32 destination, MAREA_Boolean valueUsed, MAREA_Uinteger8 *outKind);

    /**
     * @brief Reads the full name that a member access callee gives when its base is a namespace.
     * @param generator State of the call. Must not be null.
     * @param callee A MEMBER_ACCESS callee. Must not be null.
     * @param outName Receives the joined name, or null when the base is a value or a type of this unit. Must not be
     * null.
     * @param outLength Receives the length of the name. Must not be null.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS or MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorHostName(MAREA_BytecodeGenerator * generator, const MAREA_IrNode *callee, const MAREA_Character8 **outName, MAREA_IntegerSize *outLength);

    /**
     * @brief Lowers a call of a registered C function: the call window, the storage of a structure result, the
     * arguments, and the import that the call names.
     * @param generator State of the call. Must not be null.
     * @param node The CALL_EXPRESSION. Must not be null.
     * @param name Full name of the registration. Must not be null and must live in the arena or the IR.
     * @param length Number of bytes of the name.
     * @param destination As for MAREA_BytecodeGeneratorLowerCall.
     * @param valueUsed As for MAREA_BytecodeGeneratorLowerCall.
     * @param outKind As for MAREA_BytecodeGeneratorLowerCall. Must not be null.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS or MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorLowerHostCall(MAREA_BytecodeGenerator * generator, const MAREA_IrNode *node, const MAREA_Character8 *name, MAREA_IntegerSize length, MAREA_Uinteger32 destination, MAREA_Boolean valueUsed, MAREA_Uinteger8 *outKind);

    /**
     * @brief Adds every import to the function table of the module, after the functions of the unit.
     * @param generator State of the call. Must not be null.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS or MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorPublishImports(MAREA_BytecodeGenerator * generator);

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

    /**
     * @brief Reads a type text as an aggregate: a structure of this unit or an inline run that the registry
     * describes. outKind is MAREA_BYTECODE_GENERATOR_KIND_NONE for any other text.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorTextKind(MAREA_BytecodeGenerator * generator, const MAREA_IrNode *node, const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_Uinteger8 *outKind, MAREA_Uinteger32 *outAggregateIndex);

    /**
     * @brief Gives the aggregate of an inline run type text, or MAREA_BYTECODE_NONE when the text is none.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorRunKind(MAREA_BytecodeGenerator * generator, const MAREA_IrNode *node, const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_Uinteger32 *outAggregateIndex);

    /**
     * @brief Builds a value of an aggregate in bytes of its own and gives the register of its address.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorMaterialize(MAREA_BytecodeGenerator * generator, const MAREA_IrNode *node, MAREA_Uinteger32 aggregateIndex, MAREA_Uinteger32 *outRegister);

    /**
     * @brief Lowers the place that a name, a field or an element gives: the register of its address, the kind of
     * the value there, and its aggregate when it is one.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorPlace(MAREA_BytecodeGenerator * generator, const MAREA_IrNode *node, MAREA_Uinteger32 *outRegister, MAREA_Uinteger8 *outKind, MAREA_Uinteger32 *outAggregateIndex);

    /**
     * @brief Lowers the checked place of an element of an INDEX_ACCESS.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorElementPlace(MAREA_BytecodeGenerator * generator, const MAREA_IrNode *node, MAREA_Uinteger32 *outRegister, MAREA_Uinteger8 *outKind, MAREA_Uinteger32 *outAggregateIndex);

    /**
     * @brief Builds an array from a LITERAL_VALUE at a given address.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorConstructRun(MAREA_BytecodeGenerator * generator, const MAREA_IrNode *node, MAREA_Uinteger32 destinationAddress, MAREA_Uinteger32 aggregateIndex);

    /**
     * @brief Lowers a CALL_EXPRESSION that calls a member of an array. outHandled is false for any other call.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorRunCall(MAREA_BytecodeGenerator * generator, const MAREA_IrNode *node, MAREA_Uinteger32 destination, MAREA_Boolean *outHandled, MAREA_Uinteger8 *outKind);

    /**
     * @brief Lowers a FOR_EACH_STATEMENT over an array.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorLowerForEach(MAREA_BytecodeGenerator * generator, const MAREA_IrNode *node);

    /**
     * @brief Lowers the body of a loop that starts at start, with the jump that leaves it, and closes the scope.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorLowerLoopBody(MAREA_BytecodeGenerator * generator, const MAREA_IrNode *node, const MAREA_IrNode *body, MAREA_Uinteger32 start, MAREA_Uinteger32 exitJump, MAREA_IntegerSize symbolMark, MAREA_Uinteger32 registerMark);

    /**
     * @brief Builds the array of an INCLUDED_BYTES node at a given address.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorConstructIncluded(MAREA_BytecodeGenerator * generator, const MAREA_IrNode *node, MAREA_Uinteger32 destinationAddress, MAREA_Uinteger32 aggregateIndex);

MAREA_END_EXTERN_C

#endif /* MAREA_INTERNAL_BYTECODE_MAREABYTECODEGENERATORINTERNAL_H */
