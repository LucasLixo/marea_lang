// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INTERNAL_CGEN_MAREACGENINTERNAL_H)
    #define MAREA_INTERNAL_CGEN_MAREACGENINTERNAL_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREAArena.h"
    #include "marea/base/MAREADiagnostic.h"
    #include "marea/cgen/MAREACgen.h"
    #include "marea/ir/MAREAIrNode.h"
    #include "marea/lex/MAREATokenKind.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Passes the failure of an expression that returns a MAREA_CgenExcept up to the caller.
     */
    #if !defined(MAREA_CGEN_TRY)
        #define MAREA_CGEN_TRY(expression)                              \
            do {                                                        \
                const MAREA_CgenExcept marea_try_except = (expression); \
                if (marea_try_except != MAREA_CGEN_EXCEPT_SUCCESS) {    \
                    return marea_try_except;                            \
                }                                                       \
            } while (0)
    #endif

    /**
     * @brief Kind of a scalar value cgen writes a C type for. The integer and float kinds share the order
     * MAREA_BytecodeGeneratorKind already uses, so the two stages read the same written type the same way.
     */
    typedef enum MAREA_CgenKind
    {
        MAREA_CGEN_KIND_SINTEGER_8 = MAREA_UINTEGER_8(0),
        MAREA_CGEN_KIND_SINTEGER_16 = MAREA_UINTEGER_8(1),
        MAREA_CGEN_KIND_SINTEGER_32 = MAREA_UINTEGER_8(2),
        MAREA_CGEN_KIND_SINTEGER_64 = MAREA_UINTEGER_8(3),
        MAREA_CGEN_KIND_UINTEGER_8 = MAREA_UINTEGER_8(4),
        MAREA_CGEN_KIND_UINTEGER_16 = MAREA_UINTEGER_8(5),
        MAREA_CGEN_KIND_UINTEGER_32 = MAREA_UINTEGER_8(6),
        MAREA_CGEN_KIND_UINTEGER_64 = MAREA_UINTEGER_8(7),
        MAREA_CGEN_KIND_FLOAT_32 = MAREA_UINTEGER_8(8),
        MAREA_CGEN_KIND_FLOAT_64 = MAREA_UINTEGER_8(9),
        MAREA_CGEN_KIND_NUMBER_COUNT = MAREA_UINTEGER_8(10),
        MAREA_CGEN_KIND_BOOLEAN = MAREA_UINTEGER_8(10),
        MAREA_CGEN_KIND_VOID = MAREA_UINTEGER_8(11),
        MAREA_CGEN_KIND_NONE = MAREA_UINTEGER_8(12),
    } MAREA_CgenKind;

    /**
     * @brief Growable text buffer, the arena-backed form every writer in this stage appends to. The same shape
     * the ir dump already uses, kept private to this stage.
     */
    typedef struct MAREA_CgenBuffer
    {
        MAREA_Arena *arena;
        MAREA_Character8 *data;
        MAREA_IntegerSize length;
        MAREA_IntegerSize capacity;
        MAREA_Boolean failed;
    } MAREA_CgenBuffer;

    /**
     * @brief One function of the ir module, known before any body is written, so a call expression finds its
     * mangled name, its result kind and its parameter kinds without a second pass. parameterKinds and
     * resultKind hold MAREA_CGEN_KIND_NONE for a type this backend does not cover yet.
     */
    typedef struct MAREA_CgenFunction
    {
        const MAREA_IrNode *node;
        const MAREA_Character8 *name;
        MAREA_IntegerSize nameLength;
        MAREA_Uinteger8 *parameterKinds;
        MAREA_Uinteger32 parameterCount;
        MAREA_Uinteger8 resultKind;
        MAREA_Uinteger8 padding[3];
    } MAREA_CgenFunction;

    /**
     * @brief One name in scope while a function body is written: a parameter or a local, its mangled C name and
     * its kind.
     */
    typedef struct MAREA_CgenSymbol
    {
        const MAREA_Character8 *name;
        MAREA_IntegerSize length;
        MAREA_Uinteger8 kind;
        MAREA_Uinteger8 padding[7];
    } MAREA_CgenSymbol;

    /**
     * @brief State shared by every writer of this stage while one module is generated.
     */
    typedef struct MAREA_CgenGenerator
    {
        MAREA_CgenContext *context;
        MAREA_Arena *arena;
        MAREA_DiagnosticSink *sink;
        MAREA_CgenBuffer *buffer;
        const MAREA_Character8 *namespaceText;
        MAREA_IntegerSize namespaceLength;
        MAREA_CgenFunction *functions;
        MAREA_IntegerSize functionCount;
        MAREA_CgenSymbol *symbols;
        MAREA_IntegerSize symbolCount;
        MAREA_IntegerSize symbolCapacity;
        MAREA_Uinteger8 currentResultKind;
        MAREA_Boolean functionFailed;
    } MAREA_CgenGenerator;

    /**
     * @brief Reserves at least extra bytes of room, growing the arena allocation when needed. One extra byte is
     * always kept free for the final NUL.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_CgenBufferReserve(MAREA_CgenBuffer * buffer, MAREA_IntegerSize extra);

    /**
     * @brief Appends raw bytes to a buffer. A no-op once the buffer has failed.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_CgenWrite(MAREA_CgenBuffer * buffer, const MAREA_Character8 *text, MAREA_IntegerSize length);

    /**
     * @brief Appends a NUL-terminated C string literal to a buffer.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_CgenWriteText(MAREA_CgenBuffer * buffer, const MAREA_Character8 *text);

    /**
     * @brief Appends the decimal text of an unsigned 64-bit value.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_CgenWriteNumber(MAREA_CgenBuffer * buffer, MAREA_Uinteger64 value);

    /**
     * @brief Appends a name made from raw source text (an identifier, a hidden local such as $return0, or a
     * dotted function name), sanitized into a valid C11 identifier: every byte that is not a letter, a digit
     * or an underscore becomes an underscore, and the given prefix goes in front so a mangled name never
     * collides with a C keyword or with a name this stage writes for another purpose. Not reversible, and not
     * meant to be: the only contract is that two different source names before the same prefix never collide,
     * which holds because '.' and '$' are replaced by a byte no Marea identifier ever contains in that
     * position and the prefix itself is reserved.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_CgenWriteMangled(MAREA_CgenBuffer * buffer, const MAREA_Character8 *prefix, const MAREA_Character8 *text, MAREA_IntegerSize length);

    /**
     * @brief Reads the kind a written or resolved type name stands for.
     * @return The kind, or MAREA_CGEN_KIND_NONE when the text names no scalar type this backend covers.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Uinteger8 MAREA_CALL MAREA_CgenKindFromText(const MAREA_Character8 *text, MAREA_IntegerSize length);

    /**
     * @brief Reads the kind of a node from its resolved type (falling back to its type name), the way a
     * parameter, a local or an expression names its own scalar type.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Uinteger8 MAREA_CALL MAREA_CgenKindOfNode(const MAREA_IrNode *node);

    /**
     * @brief Appends the C11 type that a kind maps to (int8_t, uint32_t, float, double, bool, void).
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_CgenWriteCType(MAREA_CgenBuffer * buffer, MAREA_Uinteger8 kind);

    /**
     * @brief Returns the suffix of the runtime helper functions the preamble of MAREA_CgenGenerateModule writes
     * for one of the eight integer kinds (marea_rt_add_s8, marea_rt_div_u32, and so on).
     * @param kind An integer kind. Any other kind returns "u64", the widest suffix, so a caller that checked the
     * kind first never reaches it.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_Character8 *MAREA_CALL MAREA_CgenKindSuffix(MAREA_Uinteger8 kind);

    /**
     * @brief Appends the default value literal of a kind (0, 0.0, false).
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_CgenWriteDefaultValue(MAREA_CgenBuffer * buffer, MAREA_Uinteger8 kind);

    /**
     * @brief Records one diagnostic and marks the function being written as failed, so the caller stops
     * writing that function's body (the module as a whole still succeeds; see MAREA_CgenGenerateModule).
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenGeneratorFail(MAREA_CgenGenerator * generator, const MAREA_IrNode *node, MAREA_CgenDiagnosticCode code, const MAREA_Character8 *message);

    /**
     * @brief Finds a parameter or a local already in scope by its source name.
     * @return The symbol, or null when no name in scope matches.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_CgenSymbol *MAREA_CALL MAREA_CgenFindSymbol(const MAREA_CgenGenerator *generator, const MAREA_Character8 *text, MAREA_IntegerSize length);

    /**
     * @brief Pushes one parameter or local into scope. Fails the function being written when the arena cannot
     * grow the symbol table.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenPushSymbol(MAREA_CgenGenerator * generator, const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_Uinteger8 kind);

    /**
     * @brief Finds a function of the module already collected by MAREA_CgenGenerateModule, the way a plain call
     * expression reads its callee: the bare name sema left it, with the namespace of this unit skipped when the
     * call wrote it in full.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_CgenFunction *MAREA_CALL MAREA_CgenFindFunction(const MAREA_CgenGenerator *generator, const MAREA_Character8 *text, MAREA_IntegerSize length);

    /**
     * @brief Writes a parenthesized C11 expression for one ir expression node, so every writer that embeds a
     * sub-expression never has to reason about operator precedence on either side.
     * @param outKind Receives the kind of the written expression. Receives MAREA_CGEN_KIND_NONE on a failure
     * already recorded through MAREA_CgenGeneratorFail.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteExpression(MAREA_CgenGenerator * generator, const MAREA_IrNode *node, MAREA_Uinteger8 *outKind);

    /**
     * @brief Writes one ir statement as one or more C11 statements.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteStatement(MAREA_CgenGenerator * generator, const MAREA_IrNode *node);

    /**
     * @brief Writes every statement of a BLOCK node inside a fresh C11 scope ("{" ... "}"), with the symbols the
     * block pushes dropped from scope again once it ends.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteBlock(MAREA_CgenGenerator * generator, const MAREA_IrNode *node);

    /**
     * @brief Writes "if (*marea_trap) { return DEFAULT; }", the statement-granularity stand-in for the vm's rule
     * that a trapped callee propagates immediately: once a division, a shift or a call sets the trap, no later
     * statement of the function runs. DEFAULT is the zero value of the function's own result kind, or nothing
     * for a function with no result.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_CgenWriteTrapGuard(MAREA_CgenGenerator * generator);

MAREA_END_EXTERN_C

#endif /* MAREA_INTERNAL_CGEN_MAREACGENINTERNAL_H */
