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
        MAREA_SemaNameTable locals;
        MAREA_SemaNameTable references;
        const MAREA_AstNode *root;
        const MAREA_AstNode *function;
        MAREA_Uinteger32 selfDepth;
        MAREA_Uinteger32 instantiationDepth;
        MAREA_Uinteger32 forkDepth;
    };

    /**
     * @brief Saved size of the scope stacks of a context, so a scope can be left.
     */
    typedef struct MAREA_SemaScopeMark
    {
        const MAREA_AstNode *function;
        MAREA_IntegerSize genericCount;
        MAREA_IntegerSize localCount;
        MAREA_IntegerSize referenceCount;
        MAREA_Uinteger32 selfDepth;
        MAREA_Uinteger32 forkDepth;
    } MAREA_SemaScopeMark;

    /**
     * @brief How the checks treat a type.
     */
    typedef enum MAREA_SemaTypeClass
    {
        MAREA_SEMA_TYPE_CLASS_PRIMITIVE = MAREA_UINTEGER_8(0),
        MAREA_SEMA_TYPE_CLASS_COPY_BUILTIN = MAREA_UINTEGER_8(1),
        MAREA_SEMA_TYPE_CLASS_MOVE_BUILTIN = MAREA_UINTEGER_8(2),
        MAREA_SEMA_TYPE_CLASS_GENERIC = MAREA_UINTEGER_8(3),
        MAREA_SEMA_TYPE_CLASS_USER = MAREA_UINTEGER_8(4),
        MAREA_SEMA_TYPE_CLASS_EXTERNAL = MAREA_UINTEGER_8(5),
    } MAREA_SemaTypeClass;

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

    /**
     * @brief Appends a name to any name table of the context, growing it in the arena.
     * @param context Context of the run. Must not be null.
     * @param table Table to append to. Must not be null.
     * @param entry Entry to append.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaTableAppend(MAREA_SemaContext * context, MAREA_SemaNameTable * table, MAREA_SemaName entry);

    /**
     * @brief Tells whether a boolean attribute is on for a declaration. The attributes of the symbol win over
     * the file attributes of the compilation unit that holds it. A missing attribute is off.
     * @param root COMPILATION_UNIT node that holds the declaration. May be null: only the symbol is read.
     * @param declaration Declaration node. Null returns false.
     * @param key Attribute key, for example "value".
     * @return True when the closest attribute with that key is on.
     */
    MAREA_NODISCARD MAREA_Boolean MAREA_CALL MAREA_SemaAttributeIsOn(const MAREA_AstNode *root, const MAREA_AstNode *declaration, const MAREA_Character8 *key);

    /**
     * @brief Checks a WHERE_CLAUSE: every constraint names a generic parameter in scope and known contracts.
     * @param context Context of the run. Must not be null.
     * @param node WHERE_CLAUSE node. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckWhereClause(MAREA_SemaContext * context, const MAREA_AstNode *node);

    /**
     * @brief Checks a CALL_EXPRESSION whose callee is a function of the file: the argument count, and the
     * where clause of the callee against the type arguments, written or inferred from the arguments.
     * @param context Context of the run. Must not be null.
     * @param node CALL_EXPRESSION node. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckCall(MAREA_SemaContext * context, const MAREA_AstNode *node);

    /**
     * @brief Records a LOCAL_DECLARATION in the local scope, so a later call can infer an argument type.
     * @param context Context of the run. Must not be null.
     * @param node LOCAL_DECLARATION node. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaDeclareLocal(MAREA_SemaContext * context, const MAREA_AstNode *node);

    /**
     * @brief Counts one more level of generic instantiation for a NAMED_TYPE that has type arguments, and
     * reports INSTANTIATION_TOO_DEEP on the first level past 64.
     * @param context Context of the run. Must not be null.
     * @param node NAMED_TYPE node. Must not be null.
     * @param outEntered Receives true when a level was counted, to hand to MAREA_SemaInstantiationLeave.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaInstantiationEnter(MAREA_SemaContext * context, const MAREA_AstNode *node, MAREA_Boolean *outEntered);

    /**
     * @brief Gives back the level that MAREA_SemaInstantiationEnter counted.
     * @param context Context of the run. Must not be null.
     * @param entered Value that MAREA_SemaInstantiationEnter returned.
     */
    void MAREA_CALL MAREA_SemaInstantiationLeave(MAREA_SemaContext * context, MAREA_Boolean entered);

    /**
     * @brief Tells whether a name is a generic parameter of a declaration that is being visited.
     * @param context Context of the run. Must not be null.
     * @param text Text of the name, not NUL terminated.
     * @param length Length of text in bytes.
     * @return True when the name is a generic parameter in scope.
     */
    MAREA_NODISCARD MAREA_Boolean MAREA_CALL MAREA_SemaIsGenericInScope(const MAREA_SemaContext *context, const MAREA_Character8 *text, MAREA_IntegerSize length);

    /**
     * @brief Sorts a type node: a primitive, a built-in that is copied or moved, a generic parameter in scope,
     * a user type of the file (aliases are followed), or a type that sema cannot see (a library type).
     * @param context Context of the run. Must not be null.
     * @param type NAMED_TYPE or FUNCTION_TYPE node. May be null, which gives EXTERNAL.
     * @param outDeclaration Receives the declaration of a USER type, or null. Must not be null.
     * @param outName Receives the name of the type when it is a single name. Must not be null.
     * @param outLength Receives the length of the name. Must not be null.
     * @return The class of the type.
     */
    MAREA_NODISCARD MAREA_SemaTypeClass MAREA_CALL MAREA_SemaClassifyType(const MAREA_SemaContext *context, const MAREA_AstNode *type, const MAREA_AstNode **outDeclaration, const MAREA_Character8 **outName, MAREA_IntegerSize *outLength);

    /**
     * @brief Finds the declared type of an expression when it is cheap to know: a local or parameter, a
     * struct literal, a field of a known structure, or an element of an array, slice, vector or pointer.
     * @param context Context of the run. Must not be null.
     * @param expression Expression node. May be null.
     * @return A type node, or null when the type is unknown.
     */
    MAREA_NODISCARD const MAREA_AstNode *MAREA_CALL MAREA_SemaInferType(const MAREA_SemaContext *context, const MAREA_AstNode *expression);

    /**
     * @brief Records the references a node binds: the borrow parameters of a function, the variables of a
     * for each statement, the part of a partition and the name of a with statement.
     * @param context Context of the run. Must not be null.
     * @param node Node about to be visited. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaBindReferences(MAREA_SemaContext * context, const MAREA_AstNode *node);

    /**
     * @brief Tells whether a name is a reference that cannot leave its scope.
     * @param context Context of the run. Must not be null.
     * @param text Text of the name, not NUL terminated.
     * @param length Length of text in bytes.
     * @return True when the closest binding of the name is a reference.
     */
    MAREA_NODISCARD MAREA_Boolean MAREA_CALL MAREA_SemaIsReference(const MAREA_SemaContext *context, const MAREA_Character8 *text, MAREA_IntegerSize length);

    /**
     * @brief Checks the ownership rules of one node: the reference word on a parameter, the copy and move
     * rule of an assignment or local initializer, and a reference that is returned or stored in a field.
     * @param context Context of the run. Must not be null.
     * @param node Node about to be visited. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckOwnership(MAREA_SemaContext * context, const MAREA_AstNode *node);

    /**
     * @brief Checks that a call does not pass one name twice when one of the parameters is a write borrow.
     * @param context Context of the run. Must not be null.
     * @param call CALL_EXPRESSION node. Must not be null.
     * @param parameters PARAMETER_LIST of the callee. Must not be null.
     * @param arguments ARGUMENT_LIST of the call. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckBorrowArguments(MAREA_SemaContext * context, const MAREA_AstNode *call, const MAREA_AstNode *parameters, const MAREA_AstNode *arguments);

    /**
     * @brief Checks the arguments of a spawn inside a fork_join block: each must be a part of a partition, a
     * literal, a constant, or a name that is neither a write borrow nor the source of an active mutable
     * partition. A spawn outside a fork_join block is not checked here.
     * @param context Context of the run. Must not be null.
     * @param node UNARY_EXPRESSION node whose operator is spawn. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckSpawn(MAREA_SemaContext * context, const MAREA_AstNode *node);

    /**
     * @brief Finds the return type of a function or contract function: the first type after the parameter list.
     * @param function FUNCTION_DECLARATION, CONTRACT_FUNCTION or FUNCTION_TYPE node. May be null.
     * @return The type node, or null when there is none.
     */
    MAREA_NODISCARD const MAREA_AstNode *MAREA_CALL MAREA_SemaReturnType(const MAREA_AstNode *function);

    /**
     * @brief Checks the control flow of one node: a function with a return type ends every path with a
     * return, and a switch over an enum names every variant or has a default arm.
     * @param context Context of the run. Must not be null.
     * @param node Node about to be visited. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckControlFlow(MAREA_SemaContext * context, const MAREA_AstNode *node);

    /**
     * @brief Checks the entry point: a function named main in the namespace main has the signature
     * (arguments: array<string>): integer32. Sema never requires a main to exist.
     * @param context Context of the run. Must not be null.
     * @param root COMPILATION_UNIT node. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, or MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER on a null argument.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckEntryPoint(MAREA_SemaContext * context, const MAREA_AstNode *root);

    /**
     * @brief Reports TYPE_MISMATCH when a literal cannot have a primitive type: a string for a number or a
     * boolean, a boolean for anything but a boolean, a fraction for an integer. Any other pair is accepted,
     * because sema does not type the other expressions.
     * @param context Context of the run. Must not be null.
     * @param type NAMED_TYPE or FUNCTION_TYPE node that the value must have. May be null.
     * @param expression Expression node. May be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER when context is null, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckLiteralFit(MAREA_SemaContext * context, const MAREA_AstNode *type, const MAREA_AstNode *expression);

MAREA_END_EXTERN_C

#endif /* MAREA_INTERNAL_SEMA_MAREASEMAINTERNAL_H */
