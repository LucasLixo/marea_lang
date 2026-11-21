// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INTERNAL_SEMA_MAREASEMAINTERNAL_H)
    #define MAREA_INTERNAL_SEMA_MAREASEMAINTERNAL_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREAArena.h"
    #include "marea/base/MAREADiagnostic.h"
    #include "marea/base/MAREASourcePosition.h"
    #include "marea/parse/MAREAAst.h"
    #include "marea/parse/MAREAAstKind.h"
    #include "marea/sema/MAREASema.h"
    #include "marea/sema/MAREASemaImportResolver.h"

MAREA_BEGIN_EXTERN_C

    struct MAREA_SemaImportResolver
    {
        MAREA_SemaImportResolveFunction resolve;
        MAREA_Null userData;
    };

    /**
     * @brief What a local name of a file stands for.
     */
    typedef enum MAREA_SemaNameKind
    {
        MAREA_SEMA_NAME_KIND_NAMESPACE = MAREA_UINTEGER_8(0),
        MAREA_SEMA_NAME_KIND_IMPORTED_SYMBOL = MAREA_UINTEGER_8(1),
        MAREA_SEMA_NAME_KIND_DECLARATION = MAREA_UINTEGER_8(2),
    } MAREA_SemaNameKind;

    /**
     * @brief One local name of a file. The text points into the source, it is not copied.
     */
    typedef struct MAREA_SemaName
    {
        const MAREA_Character8 *text;
        const MAREA_AstNode *declaration;
        MAREA_IntegerSize length;
        MAREA_SemaNameKind kind;
    } MAREA_SemaName;

    /**
     * @brief Flat table of the local names of one file: its own declarations and the local name of every
     * import. The entries live in the arena of the context.
     */
    typedef struct MAREA_SemaNameTable
    {
        MAREA_SemaName *entries;
        MAREA_IntegerSize count;
        MAREA_IntegerSize capacity;
    } MAREA_SemaNameTable;

    struct MAREA_SemaContext
    {
        MAREA_Arena *arena;
        MAREA_DiagnosticSink *sink;
        MAREA_SemaImportResolver *resolver;
        MAREA_SemaNameTable names;
        MAREA_SemaNameTable generics;
        MAREA_Uinteger32 selfDepth;
    };

    /**
     * @brief Saved size of the scope stacks of a context, so a scope can be left.
     */
    typedef struct MAREA_SemaScopeMark
    {
        MAREA_IntegerSize genericCount;
        MAREA_Uinteger32 selfDepth;
    } MAREA_SemaScopeMark;

    /**
     * @brief Adds a diagnostic with the severity ERROR to the sink of the context.
     * @param context Context of the run. Must not be null.
     * @param code Diagnostic code.
     * @param position Position of the offending node.
     * @param message Message text. Must not be null.
     */
    void MAREA_CALL MAREA_SemaReport(MAREA_SemaContext * context, MAREA_SemaDiagnosticCode code, MAREA_SourcePosition position, const MAREA_Character8 *message);

    /**
     * @brief Adds a diagnostic whose message is the prefix, then the name between single quotes.
     * @param context Context of the run. Must not be null.
     * @param code Diagnostic code.
     * @param position Position of the offending node.
     * @param prefix Text that starts the message. Must not be null.
     * @param name Text of the name, not NUL terminated.
     * @param nameLength Length of name in bytes.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, or MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY when the arena cannot grow.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaReportNamed(MAREA_SemaContext * context, MAREA_SemaDiagnosticCode code, MAREA_SourcePosition position, const MAREA_Character8 *prefix, const MAREA_Character8 *name, MAREA_IntegerSize nameLength);

    /**
     * @brief Looks a name up in the name table of the context.
     * @param context Context of the run. Must not be null.
     * @param text Text of the name, not NUL terminated.
     * @param length Length of text in bytes.
     * @return The entry, or null when the name is not in the table.
     */
    MAREA_NODISCARD const MAREA_SemaName *MAREA_CALL MAREA_SemaNameTableFind(const MAREA_SemaContext *context, const MAREA_Character8 *text, MAREA_IntegerSize length);

    /**
     * @brief Appends a name to the name table of the context. It does not test for a clash.
     * @param context Context of the run. Must not be null.
     * @param text Text of the name, not copied.
     * @param length Length of text in bytes.
     * @param kind What the name stands for.
     * @param declaration Declaration node of the name, or null for an import.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, or MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY when the arena cannot grow.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaNameTableAdd(MAREA_SemaContext * context, const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_SemaNameKind kind, const MAREA_AstNode *declaration);

    /**
     * @brief Reads the name of a top-level declaration: the first IDENTIFIER child.
     * @param node Declaration node. Null returns false.
     * @param outText Receives the text of the name. Must not be null.
     * @param outLength Receives the length of the name. Must not be null.
     * @return True when the node is a named declaration and the name was read.
     */
    MAREA_NODISCARD MAREA_Boolean MAREA_CALL MAREA_SemaDeclarationName(const MAREA_AstNode *node, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength);

    /**
     * @brief Joins the identifiers of a QUALIFIED_NAME with dots into a NUL terminated string in the arena.
     * @param context Context of the run. Must not be null.
     * @param name QUALIFIED_NAME node. Must not be null.
     * @param outText Receives the joined text. Must not be null.
     * @param outLength Receives the length of the joined text. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaQualifiedNameJoin(MAREA_SemaContext * context, const MAREA_AstNode *name, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength);

    /**
     * @brief Checks the namespace against the file path, resolves every import and fills the name table
     * with the imports and the top-level declarations, reporting a clash.
     * @param context Context of the run. Must not be null.
     * @param root COMPILATION_UNIT node. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, or MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckNamespaceAndNames(MAREA_SemaContext * context, const MAREA_AstNode *root);

    /**
     * @brief Visits a node and every node below it, in source order. Returns at once on an ERROR_NODE:
     * no check and no diagnostic runs under it. Every check of the stage is called from here.
     * @param context Context of the run. Must not be null.
     * @param node Node to visit. A null node is ignored.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, or the first exception a check returns.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaVisit(MAREA_SemaContext * context, const MAREA_AstNode *node);

    /**
     * @brief Validates every attribute of a node: the ATTRIBUTE children of a declaration, field or macro
     * statement, and the FILE_ATTRIBUTE children of a COMPILATION_UNIT. Checks the key, the value shape and
     * the keys that exclude each other.
     * @param context Context of the run. Must not be null.
     * @param node Node that may own attributes. A node without attributes is accepted.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, or MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckAttributes(MAREA_SemaContext * context, const MAREA_AstNode *node);

    /**
     * @brief Enters the scope a node opens: the generic parameters of a structure, type alias, function or
     * implementation, and the Self type of an implementation or a contract.
     * @param context Context of the run. Must not be null.
     * @param node Node about to be visited. Must not be null.
     * @param outMark Receives the mark to hand to MAREA_SemaScopeLeave. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaScopeEnter(MAREA_SemaContext * context, const MAREA_AstNode *node, MAREA_SemaScopeMark *outMark);

    /**
     * @brief Leaves the scope that MAREA_SemaScopeEnter entered.
     * @param context Context of the run. Must not be null.
     * @param mark Mark that MAREA_SemaScopeEnter returned.
     */
    void MAREA_CALL MAREA_SemaScopeLeave(MAREA_SemaContext * context, MAREA_SemaScopeMark mark);

    /**
     * @brief Resolves the name of a NAMED_TYPE node: a built-in type, Self, a generic parameter in scope,
     * a type of the file, an import, or a name the import resolver knows. Reports UNKNOWN_TYPE on the node
     * when none matches. A node whose name holds an ERROR_NODE is skipped.
     * @param context Context of the run. Must not be null.
     * @param node NAMED_TYPE node. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckNamedType(MAREA_SemaContext * context, const MAREA_AstNode *node);

    /**
     * @brief Checks the declarations of a compilation unit that need the whole file: a member name that two
     * implementation blocks of one concrete type both declare is DUPLICATE_MEMBER.
     * @param context Context of the run. Must not be null.
     * @param root COMPILATION_UNIT node. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckDeclarations(MAREA_SemaContext * context, const MAREA_AstNode *root);

MAREA_END_EXTERN_C

#endif /* MAREA_INTERNAL_SEMA_MAREASEMAINTERNAL_H */
