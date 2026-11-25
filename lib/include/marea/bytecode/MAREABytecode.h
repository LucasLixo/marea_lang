// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BYTECODE_MAREABYTECODE_H)
    #define MAREA_INCLUDE_BYTECODE_MAREABYTECODE_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREAArena.h"
    #include "marea/base/MAREADiagnostic.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Value of an optional index (a string, a type or an element) that is absent.
     */
    #if !defined(MAREA_BYTECODE_NONE)
        #define MAREA_BYTECODE_NONE MAREA_UINTEGER_32(0xFFFFFFFF)
    #endif

    /**
     * @brief Result of a bytecode operation. A malformed binary or text is not an exception: it is a
     * diagnostic, and the out module is null.
     */
    typedef enum MAREA_BytecodeExcept
    {
        MAREA_BYTECODE_EXCEPT_SUCCESS = MAREA_UINTEGER_8(0),
        MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER = MAREA_UINTEGER_8(1),
        MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY = MAREA_UINTEGER_8(2),
        MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE = MAREA_UINTEGER_8(3),
    } MAREA_BytecodeExcept;

    /**
     * @brief Diagnostic code emitted by bytecode, in the 5000-5999 range reserved for this stage. Every code
     * is an error. A binary error gives the byte offset in the position, with line 0. A text error gives the
     * line and the column, both counted from 1. The codes from 5030 on come from MAREA_BytecodeGenerate: they give
     * the source line and column of the IR node, and the source offset.
     */
    typedef enum MAREA_BytecodeDiagnosticCode
    {
        MAREA_BYTECODE_DIAGNOSTIC_CODE_BAD_MAGIC = MAREA_UINTEGER_32(5001),
        MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_VERSION = MAREA_UINTEGER_32(5002),
        MAREA_BYTECODE_DIAGNOSTIC_CODE_TRUNCATED = MAREA_UINTEGER_32(5003),
        MAREA_BYTECODE_DIAGNOSTIC_CODE_BAD_SECTION_TABLE = MAREA_UINTEGER_32(5004),
        MAREA_BYTECODE_DIAGNOSTIC_CODE_BAD_REFERENCE = MAREA_UINTEGER_32(5005),
        MAREA_BYTECODE_DIAGNOSTIC_CODE_UNKNOWN_OPCODE = MAREA_UINTEGER_32(5006),
        MAREA_BYTECODE_DIAGNOSTIC_CODE_MALFORMED_SECTION = MAREA_UINTEGER_32(5007),
        MAREA_BYTECODE_DIAGNOSTIC_CODE_BAD_FLAGS = MAREA_UINTEGER_32(5008),
        MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_SYNTAX = MAREA_UINTEGER_32(5020),
        MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_BAD_SECTION = MAREA_UINTEGER_32(5021),
        MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_BAD_INDEX = MAREA_UINTEGER_32(5022),
        MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_BAD_NUMBER = MAREA_UINTEGER_32(5023),
        MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_BAD_STRING = MAREA_UINTEGER_32(5024),
        MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT = MAREA_UINTEGER_32(5030),
        MAREA_BYTECODE_DIAGNOSTIC_CODE_UNRESOLVED_NAME = MAREA_UINTEGER_32(5031),
        MAREA_BYTECODE_DIAGNOSTIC_CODE_OPERAND_OVERFLOW = MAREA_UINTEGER_32(5032),
        MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE = MAREA_UINTEGER_32(5033),
        MAREA_BYTECODE_DIAGNOSTIC_CODE_UNWALKABLE_COLLECTION = MAREA_UINTEGER_32(5034),
    } MAREA_BytecodeDiagnosticCode;

    /**
     * @brief Kind of a type of the type table.
     */
    typedef enum MAREA_BytecodeTypeKind
    {
        MAREA_BYTECODE_TYPE_KIND_PRIMITIVE = MAREA_UINTEGER_8(0),
        MAREA_BYTECODE_TYPE_KIND_STRUCTURE = MAREA_UINTEGER_8(1),
        MAREA_BYTECODE_TYPE_KIND_ENUM = MAREA_UINTEGER_8(2),
        MAREA_BYTECODE_TYPE_KIND_ARRAY = MAREA_UINTEGER_8(3),
        MAREA_BYTECODE_TYPE_KIND_VECTOR = MAREA_UINTEGER_8(4),
        MAREA_BYTECODE_TYPE_KIND_POINTER = MAREA_UINTEGER_8(5),
        MAREA_BYTECODE_TYPE_KIND_OWNED = MAREA_UINTEGER_8(6),
        MAREA_BYTECODE_TYPE_KIND_SLICE = MAREA_UINTEGER_8(7),
        MAREA_BYTECODE_TYPE_KIND_TASK = MAREA_UINTEGER_8(8),
        MAREA_BYTECODE_TYPE_KIND_FUNCTION = MAREA_UINTEGER_8(9),
    } MAREA_BytecodeTypeKind;

    /**
     * @brief Opaque context that every bytecode operation that makes a module uses: the arena and the diagnostic
     * sink. Never prints, never touches global state.
     */
    typedef struct MAREA_BytecodeContext MAREA_BytecodeContext;

    /**
     * @brief Opaque bytecode module: a string pool, a type table, a function table with the code of each function,
     * and a read-only data section. It lives in the arena of its context.
     */
    typedef struct MAREA_BytecodeModule MAREA_BytecodeModule;

    /**
     * @brief One type of the type table, by value. Open to the public because it holds only numbers, so the caller
     * fills it to add a type and reads it back with no accessor for each field. name and element are an index into
     * the string pool and into the type table, or MAREA_BYTECODE_NONE. size and alignment are in bytes. count is the
     * element count of an array. memberCount is read only: MAREA_BytecodeModuleAddType ignores it.
     */
    typedef struct MAREA_ALIGNED(8) MAREA_BytecodeTypeInfo
    {
        MAREA_Uinteger64 size;
        MAREA_Uinteger64 count;
        MAREA_Uinteger32 name;
        MAREA_Uinteger32 element;
        MAREA_Uinteger32 alignment;
        MAREA_Uinteger32 memberCount;
        MAREA_BytecodeTypeKind kind;
        MAREA_Uinteger8 padding[4];
    } MAREA_BytecodeTypeInfo;

    /**
     * @brief One member of a type, by value. Open to the public for the same reason as MAREA_BytecodeTypeInfo.
     * value is the byte offset of a field of a structure, or the value of a variant of an enum.
     */
    typedef struct MAREA_ALIGNED(8) MAREA_BytecodeMemberInfo
    {
        MAREA_Uinteger64 value;
        MAREA_Uinteger32 name;
        MAREA_Uinteger32 type;
    } MAREA_BytecodeMemberInfo;

    /**
     * @brief One function, by value. Open to the public for the same reason as MAREA_BytecodeTypeInfo. result is an
     * index into the type table, or MAREA_BYTECODE_NONE for no result. parameterCount and instructionCount are read
     * only: MAREA_BytecodeModuleAddFunction ignores them.
     */
    typedef struct MAREA_ALIGNED(8) MAREA_BytecodeFunctionInfo
    {
        MAREA_Uinteger64 frameArea;
        MAREA_Uinteger32 name;
        MAREA_Uinteger32 result;
        MAREA_Uinteger32 registerCount;
        MAREA_Uinteger32 flags;
        MAREA_Uinteger32 parameterCount;
        MAREA_Uinteger32 instructionCount;
    } MAREA_BytecodeFunctionInfo;

    /**
     * @brief One parameter of a function, by value. Open to the public for the same reason as
     * MAREA_BytecodeTypeInfo. type is an index into the type table.
     */
    typedef struct MAREA_ALIGNED(4) MAREA_BytecodeParameterInfo
    {
        MAREA_Uinteger32 type;
        MAREA_Uinteger32 flags;
    } MAREA_BytecodeParameterInfo;

    /**
     * @brief One fixed 64-bit instruction, by value. Open to the public because it is the unit of the format.
     * The meaning of a, b and c depends on the opcode.
     */
    typedef struct MAREA_ALIGNED(2) MAREA_BytecodeInstruction
    {
        MAREA_Uinteger16 opcode;
        MAREA_Uinteger16 a;
        MAREA_Uinteger16 b;
        MAREA_Uinteger16 c;
    } MAREA_BytecodeInstruction;

    /**
     * @brief Creates a bytecode context.
     * @param arena Arena that owns every module and every output buffer. Must not be null.
     * @param sink Diagnostic sink that diagnostics are appended to. Must not be null.
     * @param outContext Receives the context. Must not be null.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS, MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER when an argument is null, or
     * MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY when the arena cannot grow.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeContextCreate(MAREA_Arena * arena, MAREA_DiagnosticSink * sink, MAREA_BytecodeContext * *outContext);

    /**
     * @brief Returns the arena the context was created with.
     * @param context Context to query. Null returns null.
     * @return The arena, or null when context is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Arena *MAREA_CALL MAREA_BytecodeContextGetArena(const MAREA_BytecodeContext *context);

    /**
     * @brief Returns the diagnostic sink the context was created with.
     * @param context Context to query. Null returns null.
     * @return The sink, or null when context is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_DiagnosticSink *MAREA_CALL MAREA_BytecodeContextGetDiagnostics(const MAREA_BytecodeContext *context);

    /**
     * @brief Creates an empty module.
     * @param context Context that owns the module. Must not be null.
     * @param outModule Receives the module. Must not be null.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS, MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER when an argument is null, or
     * MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY when the arena cannot grow.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeModuleCreate(MAREA_BytecodeContext * context, MAREA_BytecodeModule * *outModule);

    /**
     * @brief Returns the context that the module was created with.
     * @param module Module to query. Null returns null.
     * @return The context, or null when module is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeContext *MAREA_CALL MAREA_BytecodeModuleGetContext(const MAREA_BytecodeModule *module);

    /**
     * @brief Adds a string to the pool. A string that is already in the pool is not added twice.
     * @param module Module to change. Must not be null.
     * @param text Bytes of the string, which may hold any byte. Must not be null when length is not 0.
     * @param length Number of bytes of the string.
     * @param outIndex Receives the index of the string. Must not be null.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS, MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER when module, outIndex, or text
     * with a length other than 0 is null, or MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeModuleAddString(MAREA_BytecodeModule * module, const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_Uinteger32 *outIndex);

    /**
     * @brief Returns how many strings the pool holds.
     * @param module Module to query. Null returns 0.
     * @return The count, or 0 when module is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_IntegerSize MAREA_CALL MAREA_BytecodeModuleGetStringCount(const MAREA_BytecodeModule *module);

    /**
     * @brief Reads one string of the pool.
     * @param module Module to query. Must not be null.
     * @param index Index of the string.
     * @param outText Receives a pointer to the bytes, valid for the arena lifetime, with a NUL after the last byte.
     * Must not be null.
     * @param outLength Receives the number of bytes, without the NUL. Must not be null.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS, MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER when an argument is null, or
     * MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE when index is not less than the count.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeModuleGetString(const MAREA_BytecodeModule *module, MAREA_Uinteger32 index, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength);

    /**
     * @brief Adds a type, with no member, at the end of the type table. The module does not check the string and
     * type references of a type: the readers check the whole module after they read it.
     * @param module Module to change. Must not be null.
     * @param info Type to add. memberCount is ignored. Must not be null.
     * @param outIndex Receives the index of the type. Must not be null.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS, MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER when an argument is null, or
     * MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeModuleAddType(MAREA_BytecodeModule * module, const MAREA_BytecodeTypeInfo *info, MAREA_Uinteger32 *outIndex);

    /**
     * @brief Returns how many types the type table holds.
     * @param module Module to query. Null returns 0.
     * @return The count, or 0 when module is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_IntegerSize MAREA_CALL MAREA_BytecodeModuleGetTypeCount(const MAREA_BytecodeModule *module);

    /**
     * @brief Reads one type of the type table.
     * @param module Module to query. Must not be null.
     * @param index Index of the type.
     * @param outInfo Receives the type, with memberCount set. Must not be null.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS, MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER when an argument is null, or
     * MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE when index is not less than the count.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeModuleGetType(const MAREA_BytecodeModule *module, MAREA_Uinteger32 index, MAREA_BytecodeTypeInfo *outInfo);

    /**
     * @brief Adds a member at the end of the member list of a type.
     * @param module Module to change. Must not be null.
     * @param typeIndex Index of the type that gets the member.
     * @param member Member to add. Must not be null.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS, MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER when module or member is null,
     * MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE when typeIndex is not less than the type count, or
     * MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeModuleAddMember(MAREA_BytecodeModule * module, MAREA_Uinteger32 typeIndex, const MAREA_BytecodeMemberInfo *member);

    /**
     * @brief Reads one member of a type.
     * @param module Module to query. Must not be null.
     * @param typeIndex Index of the type.
     * @param memberIndex Index of the member in the list of the type.
     * @param outMember Receives the member. Must not be null.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS, MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER when an argument is null, or
     * MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE when an index is out of range.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeModuleGetMember(const MAREA_BytecodeModule *module, MAREA_Uinteger32 typeIndex, MAREA_Uinteger32 memberIndex, MAREA_BytecodeMemberInfo *outMember);

    /**
     * @brief Adds a function, with no parameter and no instruction, at the end of the function table.
     * @param module Module to change. Must not be null.
     * @param info Function to add. parameterCount and instructionCount are ignored. Must not be null.
     * @param outIndex Receives the index of the function. Must not be null.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS, MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER when an argument is null, or
     * MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeModuleAddFunction(MAREA_BytecodeModule * module, const MAREA_BytecodeFunctionInfo *info, MAREA_Uinteger32 *outIndex);

    /**
     * @brief Returns how many functions the function table holds.
     * @param module Module to query. Null returns 0.
     * @return The count, or 0 when module is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_IntegerSize MAREA_CALL MAREA_BytecodeModuleGetFunctionCount(const MAREA_BytecodeModule *module);

    /**
     * @brief Reads one function.
     * @param module Module to query. Must not be null.
     * @param index Index of the function.
     * @param outInfo Receives the function, with parameterCount and instructionCount set. Must not be null.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS, MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER when an argument is null, or
     * MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE when index is not less than the count.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeModuleGetFunction(const MAREA_BytecodeModule *module, MAREA_Uinteger32 index, MAREA_BytecodeFunctionInfo *outInfo);

    /**
     * @brief Adds a parameter at the end of the parameter list of a function.
     * @param module Module to change. Must not be null.
     * @param functionIndex Index of the function that gets the parameter.
     * @param parameter Parameter to add. Must not be null.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS, MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER when module or parameter is
     * null, MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE when functionIndex is not less than the function count, or
     * MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeModuleAddParameter(MAREA_BytecodeModule * module, MAREA_Uinteger32 functionIndex, const MAREA_BytecodeParameterInfo *parameter);

    /**
     * @brief Reads one parameter of a function.
     * @param module Module to query. Must not be null.
     * @param functionIndex Index of the function.
     * @param parameterIndex Index of the parameter in the list of the function.
     * @param outParameter Receives the parameter. Must not be null.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS, MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER when an argument is null, or
     * MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE when an index is out of range.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeModuleGetParameter(const MAREA_BytecodeModule *module, MAREA_Uinteger32 functionIndex, MAREA_Uinteger32 parameterIndex, MAREA_BytecodeParameterInfo *outParameter);

    /**
     * @brief Adds an instruction at the end of the code of a function. The module does not check the opcode: the
     * readers check the whole module after they read it.
     * @param module Module to change. Must not be null.
     * @param functionIndex Index of the function that gets the instruction.
     * @param instruction Instruction to add. Must not be null.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS, MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER when module or instruction is
     * null, MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE when functionIndex is not less than the function count, or
     * MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeModuleAddInstruction(MAREA_BytecodeModule * module, MAREA_Uinteger32 functionIndex, const MAREA_BytecodeInstruction *instruction);

    /**
     * @brief Reads one instruction of a function.
     * @param module Module to query. Must not be null.
     * @param functionIndex Index of the function.
     * @param instructionIndex Index of the instruction in the code of the function.
     * @param outInstruction Receives the instruction. Must not be null.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS, MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER when an argument is null, or
     * MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE when an index is out of range.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeModuleGetInstruction(const MAREA_BytecodeModule *module, MAREA_Uinteger32 functionIndex, MAREA_Uinteger32 instructionIndex, MAREA_BytecodeInstruction *outInstruction);

    /**
     * @brief Adds a blob to the read-only data section. A blob is never merged with another one, so two equal
     * blobs keep two indexes.
     * @param module Module to change. Must not be null.
     * @param bytes Bytes of the blob. Must not be null when size is not 0.
     * @param size Number of bytes of the blob.
     * @param outIndex Receives the index of the blob. Must not be null.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS, MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER when module, outIndex, or bytes
     * with a size other than 0 is null, or MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeModuleAddData(MAREA_BytecodeModule * module, const MAREA_Byte *bytes, MAREA_IntegerSize size, MAREA_Uinteger32 *outIndex);

    /**
     * @brief Returns how many blobs the read-only data section holds.
     * @param module Module to query. Null returns 0.
     * @return The count, or 0 when module is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_IntegerSize MAREA_CALL MAREA_BytecodeModuleGetDataCount(const MAREA_BytecodeModule *module);

    /**
     * @brief Reads one blob of the read-only data section.
     * @param module Module to query. Must not be null.
     * @param index Index of the blob.
     * @param outBytes Receives a pointer to the bytes, valid for the arena lifetime. It is null for a blob of 0 bytes.
     * Must not be null.
     * @param outSize Receives the number of bytes. Must not be null.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS, MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER when an argument is null, or
     * MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE when index is not less than the count.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeModuleGetData(const MAREA_BytecodeModule *module, MAREA_Uinteger32 index, const MAREA_Byte **outBytes, MAREA_IntegerSize *outSize);

    /**
     * @brief Tells whether two modules hold the same strings, types, members, functions, parameters, instructions
     * and blobs, in the same order.
     * @param left First module. Null is accepted.
     * @param right Second module. Null is accepted.
     * @return MAREA_BOOLEAN_TRUE when both are not null and equal, otherwise MAREA_BOOLEAN_FALSE.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Boolean MAREA_CALL MAREA_BytecodeModuleEquals(const MAREA_BytecodeModule *left, const MAREA_BytecodeModule *right);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_BYTECODE_MAREABYTECODE_H */
