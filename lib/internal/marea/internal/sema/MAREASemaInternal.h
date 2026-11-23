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
    #include "marea/sema/MAREASemaTypedAst.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Body of the opaque import resolver: the resolve function and the user data it gets.
     */
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
     *
     * An import name also remembers its original: the text the same import binds without "as" (the last segment
     * of a whole path, the member path as written for a member). The original is empty (null, length zero) for a
     * name that has none, so the name stands for itself. The IR lowers an alias to this text.
     *
     * An import name also remembers its qualifier: the namespace it was imported from (null, length zero for a
     * name with none). Two names with one original but two qualifiers are two types.
     */
    typedef struct MAREA_SemaName
    {
        const MAREA_Character8 *text;
        const MAREA_AstNode *declaration;
        MAREA_IntegerSize length;
        MAREA_SemaNameKind kind;
        const MAREA_Character8 *original;
        MAREA_IntegerSize originalLength;
        const MAREA_Character8 *qualifier;
        MAREA_IntegerSize qualifierLength;
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

    /**
     * @brief Body of the opaque sema context: the caller services, the name tables and the scope state of
     * the visit in progress.
     */
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
        const MAREA_AstNode *implementation;
        MAREA_Uinteger32 selfDepth;
        MAREA_Uinteger32 instantiationDepth;
        MAREA_Uinteger32 forkDepth;
        MAREA_Uinteger32 partitionDepth;
        MAREA_Uinteger32 loopDepth;
    };

    /**
     * @brief Saved size of the scope stacks of a context, so a scope can be left.
     */
    typedef struct MAREA_SemaScopeMark
    {
        const MAREA_AstNode *function;
        const MAREA_AstNode *implementation;
        MAREA_IntegerSize genericCount;
        MAREA_IntegerSize localCount;
        MAREA_IntegerSize referenceCount;
        MAREA_Uinteger32 selfDepth;
        MAREA_Uinteger32 forkDepth;
        MAREA_Uinteger32 partitionDepth;
        MAREA_Uinteger32 loopDepth;
    } MAREA_SemaScopeMark;

    /**
     * @brief Body of the opaque typed shadow node. Opened internally, not publicly, so the
     * builder (MAREASemaTypedAst.c) and the ownership classifier (MAREASemaTypedAstOwnership.c)
     * can both read and fill it; the public header keeps the type fully opaque.
     */
    struct MAREA_SemaTypedNode
    {
        const MAREA_AstNode *astNode;
        const MAREA_AstNode *typeDeclaration;
        const MAREA_Character8 *typeName;
        MAREA_SemaTypedNode **children;
        MAREA_IntegerSize typeNameLength;
        MAREA_IntegerSize childCount;
        MAREA_SemaTypeClass typeClass;
        MAREA_SemaOwnershipMode ownershipMode;
    };

    /**
     * @brief Computes the ownership mode of a typed node already carrying its resolved type class
     * and declaration: by value, by a read-only or a write borrow, or by a move or a copy keyword.
     * @param context Context the node was built with. Must not be null.
     * @param built Typed node already filled with its type class and declaration. Must not be null.
     * @return The ownership mode. Never fails: a node this cannot classify defaults to VALUE.
     */
    MAREA_NODISCARD MAREA_SemaOwnershipMode MAREA_CALL MAREA_SemaTypedAstClassifyOwnership(MAREA_SemaContext * context, const MAREA_SemaTypedNode *built);

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
     * @brief Reads one child of a node without an out parameter.
     * @param node Node to read. Null returns null.
     * @param index Index of the child.
     * @return The child, or null when node is null or index is out of range.
     */
    MAREA_NODISCARD const MAREA_AstNode *MAREA_CALL MAREA_SemaChildAt(const MAREA_AstNode *node, MAREA_IntegerSize index);

    /**
     * @brief Compares two texts that are not NUL terminated.
     * @param left Text on the left. Must not be null when leftLength is not 0.
     * @param leftLength Length of left in bytes.
     * @param right Text on the right. Must not be null when rightLength is not 0.
     * @param rightLength Length of right in bytes.
     * @return True when both texts have the same length and the same bytes.
     */
    MAREA_NODISCARD MAREA_Boolean MAREA_CALL MAREA_SemaTextEquals(const MAREA_Character8 *left, MAREA_IntegerSize leftLength, const MAREA_Character8 *right, MAREA_IntegerSize rightLength);

    /**
     * @brief Looks a name up in the name table of the context.
     * @param context Context of the run. Must not be null.
     * @param text Text of the name, not NUL terminated.
     * @param length Length of text in bytes.
     * @return The entry, or null when the name is not in the table.
     */
    MAREA_NODISCARD const MAREA_SemaName *MAREA_CALL MAREA_SemaNameTableFind(const MAREA_SemaContext *context, const MAREA_Character8 *text, MAREA_IntegerSize length);

    /**
     * @brief Looks a name up in the locals of the scopes that are open, the innermost first. A parameter, a
     * local and the name of a for, for each, partition or with statement are locals.
     * @param context Context of the run. Must not be null.
     * @param text Text of the name, not NUL terminated.
     * @param length Length of text in bytes.
     * @return The entry, or null when no open scope declares the name. The declaration of the entry is the
     * type of the name, or null for the name of a statement.
     */
    MAREA_NODISCARD const MAREA_SemaName *MAREA_CALL MAREA_SemaLocalFind(const MAREA_SemaContext *context, const MAREA_Character8 *text, MAREA_IntegerSize length);

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
     * @brief Finds the ATTRIBUTE_ITEM of a key for a declaration. The attributes of the symbol win over the
     * file attributes of the compilation unit that holds it.
     * @param root COMPILATION_UNIT node that holds the declaration. May be null: only the symbol is read.
     * @param declaration Declaration or field node. May be null.
     * @param key Attribute key, for example "pack".
     * @return The ATTRIBUTE_ITEM node, or null when no attribute has the key.
     */
    MAREA_NODISCARD const MAREA_AstNode *MAREA_CALL MAREA_SemaAttributeFind(const MAREA_AstNode *root, const MAREA_AstNode *declaration, const MAREA_Character8 *key);

    /**
     * @brief Tells whether the closest attribute of a key has one given enum member as its value. The attributes
     * of the symbol win over the file attributes of the compilation unit that holds it.
     * @param root COMPILATION_UNIT node that holds the declaration. May be null: only the symbol is read.
     * @param declaration Declaration or field node. May be null.
     * @param key Attribute key, for example "visibility".
     * @param member Enum member, for example "public".
     * @return True when the closest attribute with that key is a single name equal to the member.
     */
    MAREA_NODISCARD MAREA_Boolean MAREA_CALL MAREA_SemaAttributeHasMember(const MAREA_AstNode *root, const MAREA_AstNode *declaration, const MAREA_Character8 *key, const MAREA_Character8 *member);

    /**
     * @brief Reads the integer value of an ATTRIBUTE_ITEM, decimal or hexadecimal.
     * @param item ATTRIBUTE_ITEM node. May be null.
     * @param outValue Receives the value. Must not be null.
     * @return True when the value is an integer literal that fits 64 bits.
     */
    MAREA_NODISCARD MAREA_Boolean MAREA_CALL MAREA_SemaAttributeReadInteger(const MAREA_AstNode *item, MAREA_Uinteger64 *outValue);

    /**
     * @brief What sema can say about the conversion of one structure to another.
     */
    typedef enum MAREA_SemaLayoutVerdict
    {
        MAREA_SEMA_LAYOUT_VERDICT_COMPATIBLE = MAREA_UINTEGER_8(0),
        MAREA_SEMA_LAYOUT_VERDICT_INCOMPATIBLE = MAREA_UINTEGER_8(1),
        MAREA_SEMA_LAYOUT_VERDICT_UNDECIDED = MAREA_UINTEGER_8(2),
    } MAREA_SemaLayoutVerdict;

    /**
     * @brief Compares the layout of two structures of the file, field by field. Two structures are compatible
     * when every field has the same primitive type at the same place, the alignment is equal, and, when both
     * have the C layout, the size is equal. A structure with a generic parameter, a field that is not a
     * primitive with a fixed width (a string, a pointer, a structure), a field with its own align or a value of
     * pack or align that sema cannot read is undecided, and so is a pair whose size depends on the backend (a
     * layout that is not C).
     * @param context Context of the run. Must not be null.
     * @param from STRUCTURE_DECLARATION node of the operand. May be null, which gives UNDECIDED.
     * @param to STRUCTURE_DECLARATION node of the target. May be null, which gives UNDECIDED.
     * @return The verdict. The same declaration on both sides is always compatible.
     */
    MAREA_NODISCARD MAREA_SemaLayoutVerdict MAREA_CALL MAREA_SemaCompareStructureLayout(const MAREA_SemaContext *context, const MAREA_AstNode *from, const MAREA_AstNode *to);

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
     * @brief Checks one function that carries the operator construct or destroy: it stands in a concrete
     * implementation of a structure or an enum, and it has the fixed shape (destroy has no parameter and
     * no result, construct has no result and every parameter defaulted). Any other node is ignored.
     * @param context Context of the run. Must not be null.
     * @param node Node about to be visited. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckLifecycle(MAREA_SemaContext * context, const MAREA_AstNode *node);

    /**
     * @brief Rejects a direct call of a function that carries the operator construct or destroy, by name or
     * by the static form Type.name(...). Any other call is ignored.
     * @param context Context of the run. Must not be null.
     * @param call CALL_EXPRESSION node. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckLifecycleCall(MAREA_SemaContext * context, const MAREA_AstNode *call);

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
     * @brief Checks a break, a continue or a return inside the body of a partition: the body of a worker ends
     * only at its own "end", so a jump that leaves it is rejected. A break or a continue that targets a loop
     * inside the body is accepted. A jump outside every partition body is not checked here.
     * @param context Context of the run. Must not be null.
     * @param node BREAK_STATEMENT, CONTINUE_STATEMENT or RETURN_STATEMENT node. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, or ERROR_NULL_POINTER on a null argument.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckJump(MAREA_SemaContext * context, const MAREA_AstNode *node);

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
     * @brief Reports TYPE_MISMATCH when the type that sema knows for an expression cannot be the primitive type
     * the context expects: a literal, a name with a declared type, a field or an element of a known type, a unary
     * or binary operator, or a call of a plain function of the file. Two primitives are the same only by name; a
     * number literal fits any number type and a fraction literal any float type. An expression with an unknown
     * type, and a type that is not a primitive, are accepted, because sema never guesses.
     * @param context Context of the run. Must not be null.
     * @param type NAMED_TYPE or FUNCTION_TYPE node that the value must have. May be null.
     * @param expression Expression node. May be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER when context is null, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckExpressionFit(MAREA_SemaContext * context, const MAREA_AstNode *type, const MAREA_AstNode *expression);

    /**
     * @brief Checks the expressions of one statement: every bare name is declared (UNDECLARED_NAME), a plain
     * assignment, a return value and the initializer of a constant fit the declared type (TYPE_MISMATCH). The
     * expressions checked are the initializer of a local, the two sides of an assignment, an expression
     * statement, the condition of an if, elseif and while, the value of a return and the value of a switch. A
     * call argument is checked by MAREA_SemaCheckCall and a local initializer by MAREA_SemaCheckOwnership.
     * @param context Context of the run. Must not be null.
     * @param node Node about to be visited. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckExpressions(MAREA_SemaContext * context, const MAREA_AstNode *node);

    /**
     * @brief Records the name of a for, for each, partition or with statement in the local scope with no type, so
     * a local of an outer scope with the same name is hidden for the length of the statement.
     * @param context Context of the run. Must not be null.
     * @param node Node about to be visited. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaDeclareBinders(MAREA_SemaContext * context, const MAREA_AstNode *node);

MAREA_END_EXTERN_C

#endif /* MAREA_INTERNAL_SEMA_MAREASEMAINTERNAL_H */
