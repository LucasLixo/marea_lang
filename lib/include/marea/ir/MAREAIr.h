// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_IR_MAREAIR_H)
    #define MAREA_INCLUDE_IR_MAREAIR_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREAArena.h"
    #include "marea/base/MAREADiagnostic.h"
    #include "marea/ir/MAREAIrKind.h"
    #include "marea/ir/MAREAIrNode.h"
    #include "marea/sema/MAREASemaTypedAst.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Result of an ir operation. A construct that cannot be lowered is not an exception: it is a
     * diagnostic.
     */
    typedef enum MAREA_IrExcept
    {
        MAREA_IR_EXCEPT_SUCCESS = MAREA_UINTEGER_8(0),
        MAREA_IR_EXCEPT_ERROR_NULL_POINTER = MAREA_UINTEGER_8(1),
        MAREA_IR_EXCEPT_ERROR_OUT_OF_MEMORY = MAREA_UINTEGER_8(2),
    } MAREA_IrExcept;

    /**
     * @brief Diagnostic code emitted by ir, in the 4000-4999 range reserved for this stage. Every code is
     * an error. The codes 4001 to 4003 were retired when fork_join, switch and for each became lowered
     * constructs. They have no name and are never reused.
     */
    typedef enum MAREA_IrDiagnosticCode
    {
        MAREA_IR_DIAGNOSTIC_CODE_UNINSTANTIATED_GENERIC = MAREA_UINTEGER_32(4004),
        MAREA_IR_DIAGNOSTIC_CODE_UNSUPPORTED_MACRO = MAREA_UINTEGER_32(4005),
        MAREA_IR_DIAGNOSTIC_CODE_UNTYPED_EXPRESSION = MAREA_UINTEGER_32(4006),
    } MAREA_IrDiagnosticCode;

    /**
     * @brief Opaque context that drives MAREA_IrLowerCompilationUnit: the arena and diagnostic sink it
     * uses. Never prints, never touches global state.
     */
    typedef struct MAREA_IrContext MAREA_IrContext;

    /**
     * @brief Creates an ir context.
     * @param arena Arena that owns the IR tree and the working data. Must not be null.
     * @param sink Diagnostic sink that diagnostics are appended to. Must not be null.
     * @param outContext Receives the context. Must not be null.
     * @return MAREA_IR_EXCEPT_SUCCESS, MAREA_IR_EXCEPT_ERROR_NULL_POINTER when an argument is null, or
     * MAREA_IR_EXCEPT_ERROR_OUT_OF_MEMORY when the arena cannot grow.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_IrExcept MAREA_CALL MAREA_IrContextCreate(MAREA_Arena * arena, MAREA_DiagnosticSink * sink, MAREA_IrContext * *outContext);

    /**
     * @brief Returns the arena the context was created with.
     * @param context Context to query. Null returns null.
     * @return The arena, or null when context is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Arena *MAREA_CALL MAREA_IrContextGetArena(const MAREA_IrContext *context);

    /**
     * @brief Returns the diagnostic sink the context was created with.
     * @param context Context to query. Null returns null.
     * @return The sink, or null when context is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_DiagnosticSink *MAREA_CALL MAREA_IrContextGetDiagnostics(const MAREA_IrContext *context);

    /**
     * @brief Lowers one checked compilation unit into an IR tree rooted at MAREA_IR_KIND_MODULE.
     * @param context Context that owns the arena and sink. Must not be null.
     * @param typedRoot COMPILATION_UNIT typed node from MAREA_SemaBuildTypedAst. Must not be null, and
     * must shadow a COMPILATION_UNIT node.
     * @param outModule Receives the module root, valid for the arena lifetime. Must not be null.
     * @return MAREA_IR_EXCEPT_SUCCESS, MAREA_IR_EXCEPT_ERROR_NULL_POINTER when an argument is null or
     * typedRoot is not a COMPILATION_UNIT, or MAREA_IR_EXCEPT_ERROR_OUT_OF_MEMORY when the arena cannot
     * grow.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_IrExcept MAREA_CALL MAREA_IrLowerCompilationUnit(MAREA_IrContext * context, const MAREA_SemaTypedNode *typedRoot, const MAREA_IrNode **outModule);

    /**
     * @brief Writes the tree under module as stable text, one node per line, into the arena. Each line is
     * the kind name indented two spaces per depth, then the text in double quotes when present, then
     * type=NAME when the node carries a type name, then resolved=TEXT when it carries a resolved type, then op=OPERATOR, then flags=NUMBER when not zero.
     * The tree is followed by the layout table of the module (see MAREAIrLayout.h) as a section that starts with the line LAYOUT_TABLE, when the module has an entry.
     * @param arena Arena that owns the text. Must not be null.
     * @param module Root of the tree to write. Must not be null.
     * @param outText Receives a NUL-terminated copy of the text. Must not be null.
     * @param outLength Receives the length of the text without the NUL. Must not be null.
     * @return MAREA_IR_EXCEPT_SUCCESS, MAREA_IR_EXCEPT_ERROR_NULL_POINTER when an argument is null, or
     * MAREA_IR_EXCEPT_ERROR_OUT_OF_MEMORY when the arena cannot grow.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_IrExcept MAREA_CALL MAREA_IrModuleDump(MAREA_Arena * arena, const MAREA_IrNode *module, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_IR_MAREAIR_H */
