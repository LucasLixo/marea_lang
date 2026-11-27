// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INTERNAL_BYTECODE_MAREABYTECODEINTERNAL_H)
    #define MAREA_INTERNAL_BYTECODE_MAREABYTECODEINTERNAL_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREAArena.h"
    #include "marea/base/MAREADiagnostic.h"
    #include "marea/bytecode/MAREABytecode.h"
    #include "marea/base/MAREATypeRegistry.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Body of the opaque context: the arena and the sink it was created with.
     */
    struct MAREA_BytecodeContext
    {
        MAREA_Arena *arena;
        MAREA_DiagnosticSink *sink;
        const MAREA_TypeRegistry *registry;
    };

    /**
     * @brief One string of the pool. The bytes are followed by a NUL that the length does not count.
     */
    typedef struct MAREA_BytecodeString
    {
        const MAREA_Character8 *text;
        MAREA_IntegerSize length;
    } MAREA_BytecodeString;

    /**
     * @brief One type with its member list. info.memberCount is the number of members in use.
     */
    typedef struct MAREA_BytecodeType
    {
        MAREA_BytecodeTypeInfo info;
        MAREA_BytecodeMemberInfo *members;
        MAREA_IntegerSize memberCapacity;
    } MAREA_BytecodeType;

    /**
     * @brief One function with its parameters and its code. info.parameterCount and info.instructionCount are the
     * numbers in use.
     */
    typedef struct MAREA_BytecodeFunction
    {
        MAREA_BytecodeFunctionInfo info;
        MAREA_BytecodeParameterInfo *parameters;
        MAREA_BytecodeInstruction *instructions;
        MAREA_IntegerSize parameterCapacity;
        MAREA_IntegerSize instructionCapacity;
    } MAREA_BytecodeFunction;

    /**
     * @brief One blob of the read-only data section. bytes is null when size is 0.
     */
    typedef struct MAREA_BytecodeBlob
    {
        const MAREA_Byte *bytes;
        MAREA_IntegerSize size;
    } MAREA_BytecodeBlob;

    /**
     * @brief Body of the opaque module. Every array lives in the arena of the context and grows by doubling, so an
     * old block stays in the arena until the arena is destroyed.
     */
    struct MAREA_BytecodeModule
    {
        MAREA_BytecodeContext *context;
        MAREA_BytecodeString *strings;
        MAREA_BytecodeType *types;
        MAREA_BytecodeFunction *functions;
        MAREA_BytecodeBlob *blobs;
        MAREA_IntegerSize stringCount;
        MAREA_IntegerSize stringCapacity;
        MAREA_IntegerSize typeCount;
        MAREA_IntegerSize typeCapacity;
        MAREA_IntegerSize functionCount;
        MAREA_IntegerSize functionCapacity;
        MAREA_IntegerSize blobCount;
        MAREA_IntegerSize blobCapacity;
    };

    /**
     * @brief Highest value of MAREA_BytecodeTypeKind.
     */
    #if !defined(MAREA_BYTECODE_TYPE_KIND_MAXIMUM)
        #define MAREA_BYTECODE_TYPE_KIND_MAXIMUM MAREA_BYTECODE_TYPE_KIND_FUNCTION
    #endif

    /**
     * @brief Adds a string at the end of the pool without looking for an equal one. The readers use it, so a pool that
     * holds an equal string twice is read as it is written.
     * @param module Module to change. Must not be null.
     * @param text Bytes of the string. Must not be null when length is not 0.
     * @param length Number of bytes.
     * @param outIndex Receives the index. Must not be null.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS, MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER or
     * MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeModuleAppendString(MAREA_BytecodeModule * module, const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_Uinteger32 *outIndex);

    /**
     * @brief Grows an arena array when it is full: the new capacity is twice the old one, or 4 at the start.
     * @param arena Arena that owns the array. Must not be null.
     * @param items In and out: the array. Must not be null.
     * @param capacity In and out: the capacity, in items. Must not be null.
     * @param count Items in use. When it is less than capacity, nothing is done.
     * @param itemSize Size of one item in bytes.
     * @param alignment Alignment of one item.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS, MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER or
     * MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGrow(MAREA_Arena * arena, MAREA_Null * items, MAREA_IntegerSize * capacity, MAREA_IntegerSize count, MAREA_IntegerSize itemSize, MAREA_IntegerSize alignment);

    /**
     * @brief Appends one error diagnostic to the sink of the context. The message is a literal of this stage.
     * @param context Context that owns the sink. Must not be null.
     * @param code Diagnostic code.
     * @param offset Byte offset for a binary error, or character offset for a text error.
     * @param line Line for a text error, counted from 1, or 0 for a binary error and for an error with no line.
     * @param column Column for a text error, counted from 1, or 0.
     * @param message NUL-terminated text. Must not be null.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS, MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER or
     * MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeRaise(MAREA_BytecodeContext * context, MAREA_BytecodeDiagnosticCode code, MAREA_Uinteger32 offset, MAREA_Uinteger32 line, MAREA_Uinteger32 column, const MAREA_Character8 *message);

    /**
     * @brief Checks every string, type and opcode reference of a module against its final tables, and every type kind.
     * The first fault is raised as a diagnostic with line 0.
     * @param module Module to check. Must not be null.
     * @param outValid Receives MAREA_BOOLEAN_TRUE when the module has no fault. Must not be null.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS, MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER or
     * MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeModuleValidate(const MAREA_BytecodeModule *module, MAREA_Boolean *outValid);

    /**
     * @brief Returns the text name of a type kind, or null when the kind is out of range.
     * @param kind Kind to name.
     * @return A NUL-terminated name with static storage, or null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_Character8 *MAREA_CALL MAREA_BytecodeTypeKindGetName(MAREA_Uinteger32 kind);

MAREA_END_EXTERN_C

#endif /* MAREA_INTERNAL_BYTECODE_MAREABYTECODEINTERNAL_H */
