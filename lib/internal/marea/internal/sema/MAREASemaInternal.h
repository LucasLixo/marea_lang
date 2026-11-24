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
        MAREA_SemaImportSignatureFunction signature;
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
     *
     * The name of a for, for each, partition or with statement also remembers the type its statement gives it,
     * once the typed tree builder has read the header of the statement (null before that, and for every other name).
     * The entry lives as long as the scope of the name, so the type ends with it.
     *
     * An entry of the locals table is also the move state of its local: moved is true from a move of the whole
     * local until a plain assignment to it, and movedAt is the move expression that set it. The state pushes and
     * pops with the entry, so no second table is kept in step with the scopes. partAt is the field or element
     * chain (a MEMBER_ACCESS or INDEX_ACCESS node) of the first part moved out of the local, or null: only one
     * moved part is kept per local, and a plain assignment to the part or to the whole local clears it.
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
        const struct MAREA_SemaExprType *statementType;
        const MAREA_AstNode *movedAt;
        const MAREA_AstNode *partAt;
        MAREA_Boolean moved;
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
     * @brief One value that a worker of a fork_join block, or a partition of it, has taken: the place it was
     * taken from (a name or a chain of field accesses), whether it is a write, and identity, the declaration
     * that tells two values of the same name apart (a null identity stands for a local with no declaration node).
     */
    typedef struct MAREA_SemaCapture
    {
        const MAREA_AstNode *identity;
        const MAREA_AstNode *place;
        MAREA_Boolean write;
    } MAREA_SemaCapture;

    /**
     * @brief The values taken so far by the workers of the fork_join blocks in progress. It grows with each
     * spawn or partition and shrinks back when its fork_join block ends. The entries live in the arena of the context.
     */
    typedef struct MAREA_SemaCaptureTable
    {
        MAREA_SemaCapture *entries;
        MAREA_IntegerSize count;
        MAREA_IntegerSize capacity;
    } MAREA_SemaCaptureTable;

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
        MAREA_SemaNameTable readOnly;
        MAREA_SemaCaptureTable captures;
        const MAREA_AstNode *forkBody;
        MAREA_IntegerSize captureLocalBase;
        MAREA_IntegerSize localStart;
        MAREA_IntegerSize functionLocalStart;
        const MAREA_AstNode *root;
        const MAREA_AstNode *function;
        const MAREA_AstNode *implementation;
        const MAREA_AstNode *parent;
        MAREA_IntegerSize parentIndex;
        MAREA_Uinteger32 selfDepth;
        MAREA_Uinteger32 instantiationDepth;
        MAREA_Uinteger32 forkDepth;
        MAREA_Uinteger32 partitionDepth;
        MAREA_Uinteger32 loopDepth;
        MAREA_Uinteger32 unsafeDepth;
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
        MAREA_IntegerSize readOnlyCount;
        MAREA_IntegerSize captureCount;
        MAREA_IntegerSize captureLocalBase;
        const MAREA_AstNode *forkBody;
        MAREA_Boolean captureScope;
        MAREA_IntegerSize localStart;
        MAREA_IntegerSize functionLocalStart;
        MAREA_Uinteger32 selfDepth;
        MAREA_Uinteger32 forkDepth;
        MAREA_Uinteger32 partitionDepth;
        MAREA_Uinteger32 loopDepth;
        MAREA_Uinteger32 unsafeDepth;
    } MAREA_SemaScopeMark;

    /**
     * @brief The scope a type is read in when it was written for another declaration: the generic parameters of
     * a structure, a function or an implementation block, the type arguments that fill them, and the type that
     * Self stands for. A name that is one of the parameters is replaced by the argument at the same place.
     * Frames are chained, innermost first, and live in the arena of the context.
     *
     * A frame can also mark a type written in another file (a function of a FOUND namespace): foreignRoot is the tree of
     * that file and foreignName is the last segment of its namespace, a pointer that lives as long as the arena. A name
     * written under such a frame is read in that tree, never in the file being built.
     */
    typedef struct MAREA_SemaTypeFrame MAREA_SemaTypeFrame;

    struct MAREA_SemaTypeFrame
    {
        const MAREA_AstNode *parameters;
        const MAREA_AstNode *arguments;
        const MAREA_AstNode *selfDeclaration;
        const MAREA_AstNode *selfArguments;
        const MAREA_SemaTypeFrame *argumentFrame;
        const MAREA_SemaTypeFrame *next;
        const MAREA_AstNode *foreignRoot;
        const MAREA_Character8 *foreignName;
        MAREA_IntegerSize foreignNameLength;
    };

    /**
     * @brief What sema knows of the type of one expression, kept until the text is written. It is one of:
     * a type node read in a frame (a NAMED_TYPE, a FUNCTION_TYPE, or a FUNCTION_DECLARATION that stands for its
     * function type), a structure or enum declaration with the type arguments that fill it, or a fixed text.
     * An empty value means the type is not known.
     */
    typedef struct MAREA_SemaExprType
    {
        const MAREA_AstNode *node;
        const MAREA_AstNode *declaration;
        const MAREA_AstNode *arguments;
        const MAREA_SemaTypeFrame *frame;
        const MAREA_Character8 *fixed;
        MAREA_IntegerSize fixedLength;
    } MAREA_SemaExprType;

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
        const MAREA_Character8 *resolvedType;
        MAREA_SemaExprType exprType;
        MAREA_SemaTypedNode **children;
        MAREA_IntegerSize typeNameLength;
        MAREA_IntegerSize resolvedTypeLength;
        MAREA_IntegerSize childCount;
        MAREA_SemaTypeClass typeClass;
        MAREA_SemaOwnershipMode ownershipMode;
        MAREA_Boolean isQualifier;
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
     * @brief Gives the type that a for, for each, partition or with statement gives to its name, once the typed
     * children that decide it are built: the counter of a range has the type of the bounds, the value of a for each
     * the element type of an array, a vector or a slice and its key uinteger64, the part of a partition a slice of the
     * element type of the value, and the name of a with the type of its expression. The type is kept on the entry of
     * the name in the locals of the context, so it ends with the scope. Any other node, a child that is not the
     * last one that decides, and a type sema does not know are left as they are and are not an error.
     * @param context Context the node is being built with. Must not be null.
     * @param built Typed node of the statement, with the children up to index built. Must not be null.
     * @param index Position of the child that was built last.
     * @return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypedAstTypeScopeNames(MAREA_SemaContext * context, const MAREA_SemaTypedNode *built, MAREA_IntegerSize index);

    /**
     * @brief Reads what sema knows of the type of a typed child: the type the child stored, or the text of a literal.
     * @param built Typed node. Must not be null.
     * @param index Position of the child. A missing child gives an empty type.
     * @return The type, empty when it is not known.
     */
    MAREA_NODISCARD MAREA_SemaExprType MAREA_CALL MAREA_SemaTypedAstTypeOfChild(const MAREA_SemaTypedNode *built, MAREA_IntegerSize index);

    /**
     * @brief The type of an operation on its first two children: the first child that is not a number literal and has a
     * type gives it, and two literals give the type of the first.
     * @param built Typed node with two typed children. Must not be null.
     * @return The type, empty when it is not known.
     */
    MAREA_NODISCARD MAREA_SemaExprType MAREA_CALL MAREA_SemaTypedAstTypeOfOperands(const MAREA_SemaTypedNode *built);

    /**
     * @brief Reads the element type of a collection: the first type argument of array, slice and vector, and of
     * pointer when it is asked for, bare or under the reserved namespace c, also through owned<T>.
     * @param context Context the node is being built with. Must not be null.
     * @param collection Type of the collection. Must not be null.
     * @param withPointer True when pointer<T> is a collection too, as it is for an index access.
     * @param outElement Receives the element type. Must not be null.
     * @return True when the collection is one of them and the element was read, else false and outElement is empty.
     */
    MAREA_NODISCARD MAREA_Boolean MAREA_CALL MAREA_SemaTypedAstElementOf(const MAREA_SemaContext *context, const MAREA_SemaExprType *collection, MAREA_Boolean withPointer, MAREA_SemaExprType *outElement);

    /**
     * @brief Fills the resolved type text of a typed node that declares a type: a PARAMETER, LOCAL_DECLARATION,
     * FIELD_DECLARATION, VARIANT_FIELD, CONSTANT_DECLARATION or FUNCTION_DECLARATION (its result). The text is built
     * in the arena of the context in the scope the node was entered with. Any other node, a function with no result
     * and a written type sema cannot spell keep no text and are not an error.
     * @param context Context the node is being built with. Must not be null.
     * @param built Typed node whose astNode is set. Must not be null.
     * @return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypedAstResolveDeclaredType(MAREA_SemaContext * context, MAREA_SemaTypedNode * built);

    /**
     * @brief Finds the position of a name among the generic parameters of a declaration.
     * @param parameters GENERIC_PARAMETERS node. May be null, which means no parameter.
     * @param name Text of the name, not NUL terminated.
     * @param nameLength Length of name in bytes.
     * @return The position of the parameter, or the count of parameters when the name is none of them.
     */
    MAREA_NODISCARD MAREA_IntegerSize MAREA_CALL MAREA_SemaGenericParameterIndex(const MAREA_AstNode *parameters, const MAREA_Character8 *name, MAREA_IntegerSize nameLength);

    /**
     * @brief Writes the canonical text of the type of an expression, in the arena of the context, in the scope
     * that is open now. The spelling is the one of MAREA_SemaTypedAstResolveDeclaredType; a name that a frame fills
     * is replaced by its argument, and Self by the type the frame gives.
     * @param context Context the node is being built with. Must not be null.
     * @param type What sema knows of the type. Must not be null.
     * @param outText Receives the text, or null when the type has no spelling (a parameter that no argument fills, a
     * type sema cannot spell). Must not be null.
     * @param outLength Receives the length of the text, 0 when there is none. Must not be null.
     * @return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypedAstRenderExprType(MAREA_SemaContext * context, const MAREA_SemaExprType *type, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength);

    /**
     * @brief Infers the result of a call of a generic function with no written type arguments: for each parameter
     * whose written type is a bare generic parameter or a compound of one (array<Item,N>, vector<Item>), matches it
     * against the resolved type text of the typed argument at the same place. The first binding of a name wins; a
     * second one that disagrees makes the whole call keep no inferred type, the way a parameter sema cannot read
     * already does. When the result type is itself a bound bare generic parameter, the binding's text is returned as
     * a fixed type; any other shape (an unbound parameter, a compound result, an already-explicit call) gives an
     * empty type, unchanged from before this inference existed.
     * @param context Context the node is being built with. Must not be null.
     * @param function FUNCTION_DECLARATION of the callee, read with its own generic parameters. Must not be null.
     * @param resultType Written result type of function, as MAREA_SemaReturnType gives it. Must not be null.
     * @param call Typed node of the call expression, with its children (the argument list included) already built.
     * Must not be null.
     * @return The inferred type as a fixed text, or an empty type when nothing could be inferred.
     */
    MAREA_NODISCARD MAREA_SemaExprType MAREA_CALL MAREA_SemaTypedAstInferCallResult(MAREA_SemaContext * context, const MAREA_AstNode *function, const MAREA_AstNode *resultType, const MAREA_SemaTypedNode *call);

    /**
     * @brief Finds the contract function named member that a where clause of scope lists for the generic parameter
     * parameterName, searched through every contract the clause names for it, the first one wins. Used to type a
     * method call on a receiver whose type is a generic parameter, since such a value has no structure or enum to
     * read a member on: only the contracts its own where clause promises.
     * @param context Context the node is being built with. Must not be null.
     * @param scope A FUNCTION_DECLARATION or an IMPLEMENTATION_DECLARATION that may carry a WHERE_CLAUSE. May be
     * null, which gives null.
     * @param parameterName Text of the generic parameter name. Must not be null when parameterNameLength is not 0.
     * @param parameterNameLength Length of parameterName in bytes.
     * @param memberName Text of the member name being called.
     * @param memberNameLength Length of memberName in bytes.
     * @return The CONTRACT_FUNCTION declaration, or null when no contract of scope's where clause declares it.
     */
    MAREA_NODISCARD const MAREA_AstNode *MAREA_CALL MAREA_SemaFindContractMember(const MAREA_SemaContext *context, const MAREA_AstNode *scope, const MAREA_Character8 *parameterName, MAREA_IntegerSize parameterNameLength, const MAREA_Character8 *memberName, MAREA_IntegerSize memberNameLength);

    /**
     * @brief Fills the resolved type of a typed node that is an expression: an operation, a literal value, or an
     * identifier read as a value. It reads the types of the typed children, so every child must be built first.
     * Any other node, and an expression sema cannot type, keep no text and are not an error.
     * @param context Context the node is being built with. Must not be null.
     * @param built Typed node whose astNode and children are set. Must not be null.
     * @param parent AST node that holds the node, or null for the root. Tells an identifier that is a value from
     * a declared name.
     * @param index Position of the node among the children of parent.
     * @return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypedAstResolveExpressionType(MAREA_SemaContext * context, MAREA_SemaTypedNode * built, const MAREA_AstNode *parent, MAREA_IntegerSize index);

    /**
     * @brief Finds the frame that marks a type written in another file, the innermost one of the chain.
     * @param frame First frame of a chain. May be null.
     * @return The frame whose foreignRoot is set, or null when the type is written in the file being built.
     */
    MAREA_NODISCARD const MAREA_SemaTypeFrame *MAREA_CALL MAREA_SemaTypeFrameFindForeign(const MAREA_SemaTypeFrame *frame);

    /**
     * @brief Tells whether an identifier only qualifies a member and is not a value. It is called for an identifier that
     * has no type, and it is true when the identifier is the base of a member access and names, with no local or generic
     * parameter hiding it, a structure or an enum, a namespace or an import alias, an imported type of a FOUND namespace,
     * or nothing at all (sema takes a name with no declaration for a namespace).
     * @param context Context the node is being built with. Must not be null.
     * @param node AST node to test. May be null, which gives false.
     * @param parent AST node that holds node. May be null, which gives false.
     * @param index Position of node among the children of parent.
     * @return True when node is such a qualifier, false otherwise.
     */
    MAREA_NODISCARD MAREA_Boolean MAREA_CALL MAREA_SemaTypedAstIsQualifier(const MAREA_SemaContext *context, const MAREA_AstNode *node, const MAREA_AstNode *parent, MAREA_IntegerSize index);

    /**
     * @brief Types the member of a member access whose base names a namespace that the resolver answers FOUND: a public
     * function (its function type, read in the tree of that namespace) or a public constant (its declared type). A symbol
     * that is not public, a type, and a base that a local hides or that is not a namespace leave outType empty.
     * @param context Context the node is being built with. Must not be null.
     * @param base AST node on the left of the member access. May be null, which leaves outType empty.
     * @param member IDENTIFIER on the right of the member access. May be null, which leaves outType empty.
     * @param callTypeArguments TYPE_ARGUMENTS written on the call of the member, or null.
     * @param outType Receives what sema knows of the type, empty when it knows none. Must not be null.
     * @return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null context or outType, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypedAstFoundMemberType(MAREA_SemaContext * context, const MAREA_AstNode *base, const MAREA_AstNode *member, const MAREA_AstNode *callTypeArguments, MAREA_SemaExprType *outType);

    /**
     * @brief Types a bare name that an import bound to one public symbol of a FOUND namespace (import a.b.c as d;
     * import a.b { c };), the same way MAREA_SemaTypedAstFoundMemberType types a member.
     * @param context Context the node is being built with. Must not be null.
     * @param identifier IDENTIFIER read as a value. May be null, which leaves outType empty.
     * @param callTypeArguments TYPE_ARGUMENTS written on the call of the name, or null.
     * @param outType Receives what sema knows of the type, empty when it knows none. Must not be null.
     * @return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null context or outType, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypedAstFoundNameType(MAREA_SemaContext * context, const MAREA_AstNode *identifier, const MAREA_AstNode *callTypeArguments, MAREA_SemaExprType *outType);

    /**
     * @brief Gives the signature of a call whose callee is a member access on a namespace the resolver
     * answers EXTERNAL for, when the resolver also carries a signature function and that function knows
     * the member. Any other base, a namespace the resolver does not answer EXTERNAL for, or a member the
     * signature function does not know, leaves outSignature null: the call stays as opaque as with no
     * signature function at all.
     * @param context Context the node is being built with. Must not be null.
     * @param base AST node on the left of the member access. May be null, which leaves outSignature null.
     * @param member IDENTIFIER on the right of the member access. May be null, which leaves outSignature null.
     * @param outSignature Receives the signature, or null when none applies. Must not be null.
     * @return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null context or outSignature.
     */
    MAREA_NODISCARD MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypedAstExternalMemberSignature(MAREA_SemaContext * context, const MAREA_AstNode *base, const MAREA_AstNode *member, const MAREA_SemaSignature **outSignature);

    /**
     * @brief Gives the signature of a call whose callee is a bare name an import bound to one member of a
     * namespace the resolver answers EXTERNAL for (import c.math.pow as power;), the same way
     * MAREA_SemaTypedAstExternalMemberSignature gives it for a member access.
     * @param context Context the node is being built with. Must not be null.
     * @param identifier IDENTIFIER read as the callee. May be null, which leaves outSignature null.
     * @param outSignature Receives the signature, or null when none applies. Must not be null.
     * @return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null context or outSignature.
     */
    MAREA_NODISCARD MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypedAstExternalNameSignature(MAREA_SemaContext * context, const MAREA_AstNode *identifier, const MAREA_SemaSignature **outSignature);

    /**
     * @brief Peels an expression type down to a plain NAMED_TYPE, the same way MAREA_SemaTypedAstElementOf reads a
     * collection before it looks at the type arguments: a generic parameter a frame fills, Self, a local alias with
     * no generic parameters and owned<T> are all followed. A structure, an enum, a type sema cannot peel or a step
     * limit reached gives false, and outPeeled is left as a copy of type.
     * @param context Context the node is being built with. Must not be null.
     * @param type Type to peel. Must not be null.
     * @param outPeeled Receives the peeled type. Must not be null.
     * @return MAREA_BOOLEAN_TRUE when the result is a plain NAMED_TYPE, else MAREA_BOOLEAN_FALSE.
     */
    MAREA_NODISCARD MAREA_Boolean MAREA_CALL MAREA_SemaTypedAstPeelToNamedType(const MAREA_SemaContext *context, const MAREA_SemaExprType *type, MAREA_SemaExprType *outPeeled);

    /**
     * @brief Types a call whose callee is a member of a library type the host gave a signature for: a static call
     * on the type itself (array<T,N>.filled(0.0), c.array<sinteger32>.filled(8, 0)), or an instance call on a value
     * whose type is array, vector, slice, pointer, owned, task or table, bare or under the reserved namespace c
     * (tasks.append(x), item.result()). The signature is asked of the resolver the same way the signature of an
     * EXTERNAL member is, keyed by the canonical "c.<name>" namespace and the member name, never by the namespace an
     * import gave the name. A result spelled "Self" gives the receiver's own type; a result that names one of the
     * library's own generic parameters (array: Item, Count; table: Key, Value; vector, slice, pointer, owned, task:
     * Item) gives the receiver's type argument at that place, read in the receiver's own frame; any other spelling is
     * the literal result, exactly as an EXTERNAL call's signature is read. A resolver with no signature function, a
     * receiver that is not one of these library types, or a member the signature function does not know, leaves this
     * empty: the call stays exactly as opaque as before this rule existed.
     * @param context Context the node is being built with. Must not be null.
     * @param built Typed node of the call, with its callee child already built. Must not be null.
     * @return The type, or an empty value when no library signature applies.
     */
    MAREA_NODISCARD MAREA_SemaExprType MAREA_CALL MAREA_SemaTypedAstLibraryCallType(MAREA_SemaContext * context, const MAREA_SemaTypedNode *built);

    /**
     * @brief Reads the value type of a table on an index access: the second type argument of table<K, V>, bare or
     * under the reserved namespace c. Purely structural, the way MAREA_SemaTypedAstElementOf reads an array's
     * element, so it asks no signature of the resolver. Any other collection, or a table with fewer than two type
     * arguments, gives false.
     * @param context Context the node is being built with. Must not be null.
     * @param collection Type of the collection. Must not be null.
     * @param outValue Receives the value type. Must not be null.
     * @return MAREA_BOOLEAN_TRUE when the collection is a table and the value was read, else MAREA_BOOLEAN_FALSE.
     */
    MAREA_NODISCARD MAREA_Boolean MAREA_CALL MAREA_SemaTypedAstTableValueOf(const MAREA_SemaContext *context, const MAREA_SemaExprType *collection, MAREA_SemaExprType *outValue);

    /**
     * @brief The type of a call that returns nothing: the fixed text "void". It is only the spelling of a resolved
     * type, never a type that a declaration can write.
     * @return The type, with the fixed text set.
     */
    MAREA_NODISCARD MAREA_SemaExprType MAREA_CALL MAREA_SemaTypedAstVoidType(void);

    /**
     * @brief Tells whether a type is the one MAREA_SemaTypedAstVoidType gives.
     * @param type Type to read. May be null, which is not void.
     * @return MAREA_BOOLEAN_TRUE for void, else MAREA_BOOLEAN_FALSE.
     */
    MAREA_NODISCARD MAREA_Boolean MAREA_CALL MAREA_SemaTypedAstTypeIsVoid(const MAREA_SemaExprType *type);

    /**
     * @brief Gives the type of a call of the built-in size_of<T>() or align_of<T>(): uinteger64. The call must have
     * one type argument, no argument, and no name of the same text bound in the unit.
     * @param context Context the node is being built with. Must not be null.
     * @param call CALL_EXPRESSION node. Must not be null.
     * @return The type, or an empty value (no known type) when the call is not one of the two built-ins.
     */
    MAREA_NODISCARD MAREA_SemaExprType MAREA_CALL MAREA_SemaTypedAstBuiltinCallType(const MAREA_SemaContext *context, const MAREA_AstNode *call);

    /**
     * @brief Re-types the operations below a typed node after the number literals below it changed type. A binary,
     * a unary and a parenthesized expression take their type from their children, so they are made again, deepest
     * first, from the types the children have now. Any other node is left as it is.
     * @param context Context the node is being built with. Must not be null.
     * @param built Typed node with its children built. Must not be null.
     * @return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypedAstRetypeOperations(MAREA_SemaContext * context, MAREA_SemaTypedNode * built);

    /**
     * @brief Gives a literal node the type it has when no context fixes it: sinteger32 for an integer literal and
     * float64 for a float literal (an assumption of the plan, the repository names no default), string for a string,
     * character for a character and boolean for a boolean. Any other node is left as it is. The text is static.
     * @param built Typed node whose astNode is set. May be null.
     */
    void MAREA_CALL MAREA_SemaTypedAstResolveLiteralDefault(MAREA_SemaTypedNode * built);

    /**
     * @brief Replaces the default type of the number literals below a typed node by the type that the node gives them
     * as their context. The contexts are the declared type of a local, a constant or a variant field, the type of a
     * parameter for an argument or a default value, the result type of a return, the target of an assignment, the
     * other operand of an arithmetic, bitwise or compare expression, of a range and of a switch case, the field or
     * element type of a literal value, and the target of a convert. A context fixes a literal only when it is a
     * primitive that the literal fits, and a literal that a nearer context already fixed keeps its type. It is called
     * once the children of the node are built, in the scope the node was entered with, and it adds no diagnostic.
     * @param context Context the node is being built with. May be null.
     * @param built Typed node with its children built. May be null.
     */
    void MAREA_CALL MAREA_SemaTypedAstResolveLiteralContext(const MAREA_SemaContext *context, const MAREA_SemaTypedNode *built);

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
     * @brief Tells whether an enum declares a variant with a name.
     * @param enumeration ENUM_DECLARATION node. Must not be null.
     * @param name Text of the name, not NUL terminated.
     * @param nameLength Length of name in bytes.
     * @return True when a VARIANT_DECLARATION of the enum has the name.
     */
    MAREA_NODISCARD MAREA_Boolean MAREA_CALL MAREA_SemaEnumHasVariant(const MAREA_AstNode *enumeration, const MAREA_Character8 *name, MAREA_IntegerSize nameLength);

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
     * @brief Reports OPAQUE_FIELD_BY_VALUE on a FIELD_DECLARATION whose written type is the bare name of an
     * opaque structure of the same file. An owner, a pointer or any other generic of it is not reported.
     * @param context Context of the run. Must not be null.
     * @param node FIELD_DECLARATION node. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS or ERROR_NULL_POINTER on a null argument.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckOpaqueField(MAREA_SemaContext * context, const MAREA_AstNode *node);

    /**
     * @brief Gives the namespace that a name entry stands for, as the resolver knows it: a namespace import is its
     * qualifier then its own name (an alias gives the original), a symbol import is the namespace of its qualifier.
     * Two checks read a namespace this way, the typed tree builder and MAREA_SemaCheckVisibility.
     * @param context Context of the run. Must not be null.
     * @param entry Name entry. Must not be null.
     * @param outText Receives the text of the namespace, in the source or in the arena. Must not be null.
     * @param outLength Receives the length of outText. Must not be null.
     * @return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaNamespaceOf(MAREA_SemaContext * context, const MAREA_SemaName *entry, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength);

    /**
     * @brief Gives the tree of a namespace, only when the resolver answers FOUND for it.
     * @param context Context of the run. May be null, which gives null.
     * @param text Text of the namespace, not NUL terminated. May be null, which gives null.
     * @param length Length of text in bytes.
     * @return The tree, or null when the resolver answers EXTERNAL or NOT_FOUND.
     */
    MAREA_NODISCARD const MAREA_AstNode *MAREA_CALL MAREA_SemaFoundRoot(const MAREA_SemaContext *context, const MAREA_Character8 *text, MAREA_IntegerSize length);

    /**
     * @brief Checks the use of a symbol, a type or a field of another namespace that a whole-namespace import
     * brings: a member of a FOUND namespace that is explicitly private is PRIVATE_SYMBOL_ACCESS (a MEMBER_ACCESS
     * whose base is the namespace name), and a written type ns.T that is explicitly private is
     * PRIVATE_TYPE_ACCESS (a NAMED_TYPE). An explicitly private field of a non-private structure of a FOUND
     * namespace is PRIVATE_FIELD_ACCESS, read or written through a name written as ns.T (a MEMBER_ACCESS) or set
     * in a struct literal (a LITERAL_VALUE). A namespace that the resolver answers EXTERNAL or NOT_FOUND, a
     * member that the tree does not declare and a value whose type is not a plain ns.T are never checked. Any
     * other node kind raises nothing.
     * @param context Context of the run. Must not be null.
     * @param node Node to check. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckVisibility(MAREA_SemaContext * context, const MAREA_AstNode *node);

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
     * @brief Tells whether a declaration is explicitly private. The default visibility is public, so a
     * declaration with no visibility attribute is not private.
     * @param root COMPILATION_UNIT node that holds the declaration. May be null: only the declaration is read.
     * @param declaration Declaration or field node. May be null, which gives false.
     * @return True only when the closest visibility attribute is private.
     */
    MAREA_NODISCARD MAREA_Boolean MAREA_CALL MAREA_SemaIsPrivate(const MAREA_AstNode *root, const MAREA_AstNode *declaration);

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
     * @brief Tells whether the type arguments of a value differ from the declared generic type. Only two generic
     * types of one head and one argument count are compared: two primitives by name, two value arguments by text and
     * two nested generic types by the same rule. A generic parameter, an alias, an import and an unknown type agree.
     * @param context Context of the run. Must not be null.
     * @param type Declared type node. May be null.
     * @param expression Value expression. A copy or move is looked through. May be null.
     * @return MAREA_BOOLEAN_TRUE when a pair of type arguments differs, else MAREA_BOOLEAN_FALSE.
     */
    MAREA_NODISCARD MAREA_Boolean MAREA_CALL MAREA_SemaTypeArgumentsDiffer(const MAREA_SemaContext *context, const MAREA_AstNode *type, const MAREA_AstNode *expression);

    /**
     * @brief Finds the element type of a collection expression: one owned wrapper is peeled, then the first type
     * argument of vector, array or slice.
     * @param context Context of the run. Must not be null.
     * @param collection Collection expression. May be null.
     * @return The element type node, or null when it is unknown.
     */
    MAREA_NODISCARD const MAREA_AstNode *MAREA_CALL MAREA_SemaElementType(const MAREA_SemaContext *context, const MAREA_AstNode *collection);

    /**
     * @brief Records the references a node binds: the borrow parameters of a function, the variables of a
     * for each statement, the part of a partition and the name of a with statement.
     * @param context Context of the run. Must not be null.
     * @param node Node about to be visited. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaBindReferences(MAREA_SemaContext * context, const MAREA_AstNode *node);

    /**
     * @brief Records the writability of the names a node binds: a parameter of a function, the variables
     * of a for each statement, the part of a partition and the name of a with statement. An entry with a
     * declaration is a read-only borrow, an entry without one is writable. Reports nothing.
     * @param context Context of the run. Must not be null.
     * @param node Node about to be visited. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaBindReadOnly(MAREA_SemaContext * context, const MAREA_AstNode *node);

    /**
     * @brief Tells whether an expression is rooted in a read-only borrow. The root is the identifier at the
     * bottom of its member-access and index-access chain; a root that is not recorded is writable.
     * @param context Context of the run. Must not be null.
     * @param expression Expression node. May be null.
     * @return True when the closest binding of the root name is a read-only borrow.
     */
    MAREA_NODISCARD MAREA_Boolean MAREA_CALL MAREA_SemaIsReadOnly(const MAREA_SemaContext *context, const MAREA_AstNode *expression);

    /**
     * @brief Tells whether a name is a reference that cannot leave its scope.
     * @param context Context of the run. Must not be null.
     * @param text Text of the name, not NUL terminated.
     * @param length Length of text in bytes.
     * @return True when the closest binding of the name is a reference.
     */
    MAREA_NODISCARD MAREA_Boolean MAREA_CALL MAREA_SemaIsReference(const MAREA_SemaContext *context, const MAREA_Character8 *text, MAREA_IntegerSize length);

    /**
     * @brief Checks the ownership rules of one node: the "semantics" attribute on a parameter, the copy
     * and move rule of an assignment or local initializer, and a reference that is returned or stored in
     * a field.
     * @param context Context of the run. Must not be null.
     * @param node Node about to be visited. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckOwnership(MAREA_SemaContext * context, const MAREA_AstNode *node);

    /**
     * @brief Tells whether a value of a type is by value: a primitive, an enum, a copy-contract library type, or
     * (when unknownIsValue is true) a type that sema cannot see. Such a value is copied, never moved out.
     * @param context Context of the run. Must not be null.
     * @param type Type node. May be null, which reads as unknown.
     * @param unknownIsValue True when a type that sema cannot see counts as by value.
     * @return True when the type is by value.
     */
    MAREA_NODISCARD MAREA_Boolean MAREA_CALL MAREA_SemaTypeIsByValue(const MAREA_SemaContext *context, const MAREA_AstNode *type, MAREA_Boolean unknownIsValue);

    /**
     * @brief Tells whether every path of a block ends the function, by the same rule as the missing-return check.
     * @param context Context of the run. Must not be null.
     * @param block Block node. A null block counts as ending.
     * @return True when the block ends the function on every path.
     */
    MAREA_NODISCARD MAREA_Boolean MAREA_CALL MAREA_SemaBlockTerminates(const MAREA_SemaContext *context, const MAREA_AstNode *block);

    /**
     * @brief State saved around one branch statement (if, switch) or loop while its children are visited: the
     * moved bit and the moved part of every local that exists before the statement, as the arms start, and as
     * the arms merge. Zero filled and inert for any other node.
     */
    typedef struct MAREA_SemaMoveFrame
    {
        MAREA_Boolean *base;
        MAREA_Boolean *merged;
        const MAREA_AstNode **partBase;
        const MAREA_AstNode **partMerged;
        MAREA_IntegerSize count;
        MAREA_Boolean armSeen;
        MAREA_Boolean hasFallback;
    } MAREA_SemaMoveFrame;

    /**
     * @brief Reports a read of a local that was moved: VALUE_MOVED_AGAIN when the read is the operand of a
     * move, USE_OF_MOVED_VALUE for any other read. A plain assignment target is not a read. A use of a local
     * through a path that holds, or lies inside, a part that was moved out is USE_OF_PARTIALLY_MOVED_VALUE, and
     * so is the move of a for each element that is not by value.
     * @param context Context of the run. Must not be null.
     * @param node Node about to be visited. Must not be null. Only an identifier is checked.
     * @param parent Parent of the node, or null at the root.
     * @param parentIndex Index of the node among the children of the parent.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaMoveCheckUse(MAREA_SemaContext * context, const MAREA_AstNode *node, const MAREA_AstNode *parent, MAREA_IntegerSize parentIndex);

    /**
     * @brief Records the effect of a node on the move state once its children were visited: a move of a whole
     * local marks it moved, a move of a field or an element of a local marks that part, a plain assignment to a
     * whole local or to the moved part makes it live again.
     * @param context Context of the run. Must not be null.
     * @param node Node whose children were visited. Must not be null.
     */
    void MAREA_CALL MAREA_SemaMoveRecord(MAREA_SemaContext * context, const MAREA_AstNode *node);

    /**
     * @brief Opens a frame for a branch or loop node, before its scope is entered, so the names that the scope
     * binds are not part of the frame.
     * @param context Context of the run. Must not be null.
     * @param node Node about to be visited. Must not be null.
     * @param outFrame Receives the frame. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaMoveEnter(MAREA_SemaContext * context, const MAREA_AstNode *node, MAREA_SemaMoveFrame *outFrame);

    /**
     * @brief Starts one child of a branch or loop: an arm starts from the state the branch started with.
     * @param context Context of the run. Must not be null.
     * @param node Branch or loop node. Must not be null.
     * @param child Child about to be visited. May be null.
     * @param frame Frame of the node. Must not be null.
     */
    void MAREA_CALL MAREA_SemaMoveChildEnter(MAREA_SemaContext * context, const MAREA_AstNode *node, const MAREA_AstNode *child, MAREA_SemaMoveFrame *frame);

    /**
     * @brief Ends one child of a branch or loop: a condition joins the base state, an arm that does not end the
     * function joins the merge, and a loop body that moves an outer local without leaving the loop is reported.
     * @param context Context of the run. Must not be null.
     * @param node Branch or loop node. Must not be null.
     * @param child Child just visited. May be null.
     * @param frame Frame of the node. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaMoveChildLeave(MAREA_SemaContext * context, const MAREA_AstNode *node, const MAREA_AstNode *child, MAREA_SemaMoveFrame *frame);

    /**
     * @brief Closes a frame: a local is moved after a branch when any arm that reaches the end (no return, no break among its statements) moved it, or
     * when it was moved before a branch that has no else or default; after a loop no move of the body counts.
     * The moved part still counts only when every such arm moved the same part.
     * @param context Context of the run. Must not be null.
     * @param node Branch or loop node. Must not be null.
     * @param frame Frame of the node. Must not be null.
     */
    void MAREA_CALL MAREA_SemaMoveLeave(MAREA_SemaContext * context, const MAREA_AstNode *node, MAREA_SemaMoveFrame *frame);

    /**
     * @brief A parameter's effective passing mode: its own "semantics" attribute, or, when it has none,
     * the default by type class.
     */
    typedef enum MAREA_SemaParameterSemantics
    {
        MAREA_SEMA_PARAMETER_SEMANTICS_VALUE_READ_ONLY = MAREA_UINTEGER_8(0),
        MAREA_SEMA_PARAMETER_SEMANTICS_REFERENCE_READ_ONLY = MAREA_UINTEGER_8(1),
        MAREA_SEMA_PARAMETER_SEMANTICS_REFERENCE_MUTABLE = MAREA_UINTEGER_8(2),
    } MAREA_SemaParameterSemantics;

    /**
     * @brief Reads the effective passing mode of a PARAMETER: its own "semantics" attribute
     * (value_read_only, reference_read_only or reference_mutable), or, when it has none, the default by
     * type class - a primitive or the one documented by-value exception (an enum) is VALUE_READ_ONLY,
     * a move-in type (owned, task, vector) is REFERENCE_MUTABLE (it is never a borrow), anything else is
     * REFERENCE_READ_ONLY. The one place every ownership check reads a parameter's passing mode.
     * @param context Context of the run. Must not be null.
     * @param parameter PARAMETER node. Must not be null.
     * @return The effective semantics.
     */
    MAREA_NODISCARD MAREA_SemaParameterSemantics MAREA_CALL MAREA_SemaGetParameterSemantics(const MAREA_SemaContext *context, const MAREA_AstNode *parameter);

    /**
     * @brief Tells whether a PARAMETER carries a default value expression, as opposed to only its name,
     * type and, optionally, a trailing "semantics" attribute.
     * @param parameter PARAMETER node. May be null.
     * @return True when the parameter has a default value.
     */
    MAREA_NODISCARD MAREA_Boolean MAREA_CALL MAREA_SemaParameterHasDefault(const MAREA_AstNode *parameter);

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
     * @brief Finds the construct operator of a concrete structure or enum: the member of its
     * implementation whose operator attribute is "construct". Used to resolve a call whose callee
     * names the type, Type(args), against construct's own parameters.
     * @param context Context of the run. Must not be null.
     * @param typeDeclaration STRUCTURE_DECLARATION or ENUM_DECLARATION node. Must not be null.
     * @return The FUNCTION_DECLARATION that carries construct, or null when the type declares none.
     */
    MAREA_NODISCARD const MAREA_AstNode *MAREA_CALL MAREA_SemaFindConstructOperator(const MAREA_SemaContext *context, const MAREA_AstNode *typeDeclaration);

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
     * @brief Checks that a write-borrow parameter is given a writable place: a read-only borrow is rejected,
     * and so is a call result or a literal, which has no storage to write back.
     * @param context Context of the run. Must not be null.
     * @param parameters PARAMETER_LIST of the callee. Must not be null.
     * @param arguments ARGUMENT_LIST of the call. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckWriteArguments(MAREA_SemaContext * context, const MAREA_AstNode *parameters, const MAREA_AstNode *arguments);

    /**
     * @brief Checks that an existing value (a name, a field or an element) given to an owned parameter carries
     * copy or move: reports ASSIGNMENT_NEEDS_COPY_OR_MOVE at the argument otherwise. A call result, a literal,
     * an argument of an unknown type and a vector or task parameter raise nothing.
     * @param context Context of the run. Must not be null.
     * @param parameters PARAMETER_LIST of the callee. Must not be null.
     * @param arguments ARGUMENT_LIST of the call. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckOwnedArguments(MAREA_SemaContext * context, const MAREA_AstNode *parameters, const MAREA_AstNode *arguments);

    /**
     * @brief Checks the arguments of a spawn inside a fork_join block: each must be a part of a partition, a
     * literal, a constant, or a name that is neither a write borrow nor the source of an active mutable
     * partition. A spawn outside a fork_join block raises PARALLEL_CONSTRUCT_OUTSIDE_FORK_JOIN and nothing else.
     * @param context Context of the run. Must not be null.
     * @param node UNARY_EXPRESSION node whose operator is spawn. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckSpawn(MAREA_SemaContext * context, const MAREA_AstNode *node);

    /**
     * @brief Compares one valid argument of a spawn with what the earlier workers of the same fork_join block
     * took, then records it. Two uses of one value, or of one value and a field of it, conflict when one is a
     * write, and CONFLICTING_PARALLEL_ACCESS is reported. A write in a spawn that repeats (inside a loop or
     * a partition body) of a value declared before the block conflicts with itself. An argument that is not a
     * place, or a callee that sema cannot see, is not recorded.
     * @param context Context of the run. Must not be null.
     * @param call CALL_EXPRESSION of the spawn. Must not be null.
     * @param index Index of the argument in the call.
     * @param argument The argument node. Must not be null.
     * @param ownStart Size of the capture table before the spawn, so two arguments of one call are not compared.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCaptureArgument(MAREA_SemaContext * context, const MAREA_AstNode *call, MAREA_IntegerSize index, const MAREA_AstNode *argument, MAREA_IntegerSize ownStart);

    /**
     * @brief Compares the source of a partition with what the workers of the same fork_join block took, then
     * records it as a write for a partition mutable and as a read for any other.
     * @param context Context of the run. Must not be null.
     * @param node PARTITION_STATEMENT node. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCapturePartition(MAREA_SemaContext * context, const MAREA_AstNode *node);

    /**
     * @brief Checks that the arguments of one call do not name overlapping places (a field and its owner, one
     * field twice, one element twice, a loop element and its own collection, a value and a move of it) when one
     * of them is a write borrow or is moved. A name passed twice is left to MAREA_SemaCheckBorrowArguments.
     * Reports EXCLUSIVE_BORROW_CONFLICT.
     * @param context Context of the run. Must not be null.
     * @param call CALL_EXPRESSION node. Must not be null.
     * @param parameters PARAMETER_LIST of the callee. Must not be null.
     * @param arguments ARGUMENT_LIST of the call. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckAliasedArguments(MAREA_SemaContext * context, const MAREA_AstNode *call, const MAREA_AstNode *parameters, const MAREA_AstNode *arguments);

    /**
     * @brief Checks the body of a for each or a with statement against its source: while the body runs, the
     * source is borrowed, so a write of it, a move of it and a call that changes it (append and the like) report VALUE_CHANGED_WHILE_BORROWED. Any other node is ignored.
     * @param context Context of the run. Must not be null.
     * @param node Node in the visit. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckBorrowedSource(MAREA_SemaContext * context, const MAREA_AstNode *node);

    /**
     * @brief Checks a statement that stands directly in a fork_join block (the code of the block, not of a
     * worker) against what the workers started before it took: a use of a value that a worker writes, or a
     * write of a value that a worker uses, reports CONFLICTING_PARALLEL_ACCESS. Any other node is ignored.
     * @param context Context of the run. Must not be null.
     * @param node Node in the visit. Must not be null.
     * @param parent Parent of the node, null for the root.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCaptureBody(MAREA_SemaContext * context, const MAREA_AstNode *node, const MAREA_AstNode *parent);

    /**
     * @brief Checks a partition statement: it needs an enclosing fork_join block in the current function, or
     * PARALLEL_CONSTRUCT_OUTSIDE_FORK_JOIN is reported.
     * @param context Context of the run. Must not be null.
     * @param node PARTITION_STATEMENT node. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, or ERROR_NULL_POINTER on a null argument.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckPartition(MAREA_SemaContext * context, const MAREA_AstNode *node);

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
     * @brief Gives the FUNCTION_TYPE node that a written type stands for: the node itself, or the target of an
     * alias chain of the file (type Callback = function(text: string);).
     * @param context Context of the run. Must not be null.
     * @param type NAMED_TYPE or FUNCTION_TYPE node. May be null.
     * @return The FUNCTION_TYPE node, or null when the type is not a function type that sema can see.
     */
    MAREA_NODISCARD const MAREA_AstNode *MAREA_CALL MAREA_SemaResolveFunctionType(const MAREA_SemaContext *context, const MAREA_AstNode *type);

    /**
     * @brief Tells whether a function declaration has the signature of a function type: the same parameter count,
     * the same passing mode and type of each parameter, and the same result type, compared as written (an alias
     * renders as its original). A generic function, and a function type that mentions a generic parameter in
     * scope, are never compared and give true, because sema never guesses.
     * @param context Context of the run. Must not be null.
     * @param function FUNCTION_DECLARATION node. Must not be null.
     * @param functionType FUNCTION_TYPE node. Must not be null.
     * @return MAREA_BOOLEAN_TRUE when the function fits or when sema cannot compare, MAREA_BOOLEAN_FALSE otherwise.
     */
    MAREA_NODISCARD MAREA_Boolean MAREA_CALL MAREA_SemaFunctionFitsFunctionType(const MAREA_SemaContext *context, const MAREA_AstNode *function, const MAREA_AstNode *functionType);

    /**
     * @brief Reports TYPE_MISMATCH the same way MAREA_SemaCheckExpressionFit does, against a primitive type
     * named by fixed text instead of a written NAMED_TYPE node: the shape of the signature of a member of
     * an EXTERNAL namespace, which the host gives as canonical type spellings, not as AST nodes.
     * @param context Context of the run. Must not be null.
     * @param primitiveName Primitive type name, not NUL terminated. May be null, which accepts anything.
     * @param primitiveLength Length of primitiveName in bytes.
     * @param expression Expression node. May be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER when context is null, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckExpressionFitsPrimitive(MAREA_SemaContext * context, const MAREA_Character8 *primitiveName, MAREA_IntegerSize primitiveLength, const MAREA_AstNode *expression);

    /**
     * @brief Reports LITERAL_OUT_OF_RANGE when an expression is an integer literal (decimal, 0x or 0b, with "_"
     * between digits, one leading minus and any parentheses allowed) whose value does not fit the integer primitive
     * that type names. A negated literal never fits an unsigned type, except zero. A float, string, boolean or
     * character target is never checked here, so TYPE_MISMATCH is the only code for it. A type that is null or is
     * not a primitive is checked against the widest range: the literal must fit uinteger64, or sinteger64 when it
     * is negative. An expression that is not an integer literal is accepted.
     * @param context Context of the run. Must not be null.
     * @param type NAMED_TYPE or FUNCTION_TYPE node that the value must have. May be null.
     * @param expression Expression node. May be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER when context is null, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckLiteralRange(MAREA_SemaContext * context, const MAREA_AstNode *type, const MAREA_AstNode *expression);

    /**
     * @brief Reports LITERAL_OUT_OF_RANGE the same way MAREA_SemaCheckLiteralRange does, against a primitive type
     * named by fixed text, the shape of the signature of a member of an EXTERNAL namespace.
     * @param context Context of the run. Must not be null.
     * @param primitiveName Primitive type name, not NUL terminated. May be null, which checks the widest range.
     * @param primitiveLength Length of primitiveName in bytes.
     * @param expression Expression node. May be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER when context is null, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckLiteralRangePrimitive(MAREA_SemaContext * context, const MAREA_Character8 *primitiveName, MAREA_IntegerSize primitiveLength, const MAREA_AstNode *expression);

    /**
     * @brief Runs MAREA_SemaCheckLiteralRange on every field entry of a struct literal, against the type that the
     * structure of the file declares for that field. A literal of a type that is not a structure of the file is
     * accepted, and a field the structure does not declare is checked against the widest range.
     * @param context Context of the run. Must not be null.
     * @param literal LITERAL_VALUE node. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER when an argument is null, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckLiteralFields(MAREA_SemaContext * context, const MAREA_AstNode *literal);

    /**
     * @brief Gives the primitive type that sema knows for a written type, the same type MAREA_SemaCheckExpressionFit
     * uses as its target. A local type alias is followed. Any type that is not a primitive gives false.
     * @param context Context of the run. Must not be null.
     * @param type NAMED_TYPE or FUNCTION_TYPE node. May be null.
     * @param outName Receives the primitive name, a pointer into the source or a static text, not NUL terminated.
     * Must not be null.
     * @param outLength Receives the byte length of outName. Must not be null.
     * @return MAREA_BOOLEAN_TRUE when type is a primitive, else MAREA_BOOLEAN_FALSE with both outputs cleared.
     */
    MAREA_NODISCARD MAREA_Boolean MAREA_CALL MAREA_SemaPrimitiveOfType(const MAREA_SemaContext *context, const MAREA_AstNode *type, const MAREA_Character8 **outName, MAREA_IntegerSize *outLength);

    /**
     * @brief Gives the primitive type that sema knows for an expression, the same shape the TYPE_MISMATCH checks
     * compare. A number literal has no primitive of its own, so it gives false.
     * @param context Context of the run. Must not be null.
     * @param expression Expression node. May be null.
     * @param outName Receives the primitive name, a pointer into the source or a static text, not NUL terminated.
     * Must not be null.
     * @param outLength Receives the byte length of outName. Must not be null.
     * @return MAREA_BOOLEAN_TRUE when the expression has a known primitive type, else MAREA_BOOLEAN_FALSE with both
     * outputs cleared.
     */
    MAREA_NODISCARD MAREA_Boolean MAREA_CALL MAREA_SemaPrimitiveOfExpression(const MAREA_SemaContext *context, const MAREA_AstNode *expression, const MAREA_Character8 **outName, MAREA_IntegerSize *outLength);

    /**
     * @brief Tells whether sema knows an expression is not an array. A number literal, a primitive (a string
     * included), a structure and an enum are known not to be an array; every other shape, an unclassified one
     * included, answers false, so a slice, a pointer, a library type and a call result are accepted.
     * @param context Context of the run. Must not be null.
     * @param expression Expression node. May be null.
     * @return MAREA_BOOLEAN_TRUE when the expression is known not to be an array, else MAREA_BOOLEAN_FALSE.
     */
    MAREA_NODISCARD MAREA_Boolean MAREA_CALL MAREA_SemaExpressionIsKnownNotArray(const MAREA_SemaContext *context, const MAREA_AstNode *expression);

    /**
     * @brief Reports NEGATE_UNSIGNED when a unary minus has an operand of a known unsigned primitive type. A negated
     * literal is not this rule.
     * @param context Context of the run. Must not be null.
     * @param unary UNARY_EXPRESSION node. May be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER when context is null, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckNegation(MAREA_SemaContext * context, const MAREA_AstNode *unary);

    /**
     * @brief Reports SIGNED_DIVISION_OVERFLOW when an expression is a division or modulo whose left side folds to the
     * lowest value of a signed type and whose right side folds to -1. The signed type is the primitive of an operand,
     * else the primitive of targetType. With no known signed type nothing is reported.
     * @param context Context of the run. Must not be null.
     * @param targetType Declared type that holds the expression. May be null.
     * @param expression Expression node. May be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER when context is null, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckSignedDivision(MAREA_SemaContext * context, const MAREA_AstNode *targetType, const MAREA_AstNode *expression);

    /**
     * @brief Reports DIVISION_BY_ZERO for /, %, /= and %= whose divisor is a constant zero and the left side is an
     * integer, and SHIFT_COUNT_TOO_LARGE for a shift or compound shift whose constant count is negative or not below
     * the width of the left primitive integer type. A constant is an integer literal (decimal, 0x, 0b, "_"), one
     * leading minus on a constant, or a parenthesized constant; a named constant is not folded. Any other operator,
     * a divisor or count that is not constant, and a left side with no known width, raise nothing.
     * @param context Context of the run. Must not be null.
     * @param operatorKind Operator token of the binary expression or the compound assignment.
     * @param left Left operand node (the assignment target for a compound assignment). May be null.
     * @param right Right operand node. May be null.
     * @param position Position of the expression, used for the diagnostic.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, or ERROR_NULL_POINTER when context is null.
     */
    MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckConstantOperands(MAREA_SemaContext * context, MAREA_TokenKind operatorKind, const MAREA_AstNode *left, const MAREA_AstNode *right, MAREA_SourcePosition position);

    /**
     * @brief Reports ASSIGNMENT_TO_CONSTANT for an assignment (= or a compound form) whose target is a file
     * constant, or a MEMBER_ACCESS or INDEX_ACCESS path whose root identifier is one. A local, parameter or for
     * variable of the same name hides the constant, so it raises nothing.
     * @param context Context of the run. Must not be null.
     * @param assignment ASSIGNMENT node: child 0 is the target. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, or ERROR_NULL_POINTER when an argument is null.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckConstantAssignment(MAREA_SemaContext * context, const MAREA_AstNode *assignment);

    /**
     * @brief Reports CYCLIC_CONSTANT once for each cycle of file constants, where the initial value of a constant
     * names a constant whose value leads back to the first one. A name counts only where it stands as a value: a
     * member name, a field key, a type and a call target are not followed. The cycle is reported at its member that
     * comes first in the file. A constant that only uses a cycle is not reported, and the search keeps its state in
     * the arena, with no recursion over the constants.
     * @param context Context of the run, after the name table is filled. Must not be null.
     * @param root Compilation unit node. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER when an argument is null, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckConstantCycles(MAREA_SemaContext * context, const MAREA_AstNode *root);

    /**
     * @brief Reports COMPILE_TIME_ASSERTION_FAILED for a compile_error macro call, whatever its arguments are, and for
     * a static_assert macro call with two arguments whose condition is a constant that is false. The condition is
     * read by a small evaluator: boolean and integer literals, a file constant (depth limit 32, so a cycle ends as
     * unknown), parentheses, not, unary minus, and, or, the comparisons and + - * / % over 64-bit integers. Overflow,
     * a division by zero, a call, size_of, align_of, a name that is not a file constant (a local or a parameter of
     * the same name hides the constant) and every other node are unknown, and an unknown condition is accepted. Any
     * other macro, and a static_assert with another argument count, is skipped. The message holds the text of the
     * string literal argument when there is one.
     * @param context Context of the run. Must not be null.
     * @param call Macro call node. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER when an argument is null, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckMacroEvaluation(MAREA_SemaContext * context, const MAREA_AstNode *call);

    /**
     * @brief Tells whether a number literal fits a primitive type under the rule of MAREA_SemaCheckExpressionFit:
     * an integer literal fits any number type, a float literal fits float32 and float64 only.
     * @param kind MAREA_AST_KIND_INTEGER_LITERAL or MAREA_AST_KIND_FLOAT_LITERAL. Any other kind gives false.
     * @param name Primitive type name, not NUL terminated. Must not be null.
     * @param length Byte length of name.
     * @return MAREA_BOOLEAN_TRUE when the literal fits, else MAREA_BOOLEAN_FALSE.
     */
    MAREA_NODISCARD MAREA_Boolean MAREA_CALL MAREA_SemaNumberLiteralFitsPrimitive(MAREA_AstKind kind, const MAREA_Character8 *name, MAREA_IntegerSize length);

    /**
     * @brief Returns the target type of a built-in conversion: the one type argument of convert<T>(x) when the call
     * has one argument and the unit binds no name convert of its own.
     * @param context Context of the run. Must not be null.
     * @param call CALL_EXPRESSION node. Must not be null.
     * @return The type argument node, or null when the call is not a built-in conversion.
     */
    MAREA_NODISCARD const MAREA_AstNode *MAREA_CALL MAREA_SemaConvertTarget(const MAREA_SemaContext *context, const MAREA_AstNode *call);

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
     * @brief Reports NAME_ALREADY_DECLARED for a name that is declared twice in one scope: a generic parameter
     * that repeats an earlier one or names a top-level declaration, a repeated parameter, and a local that repeats
     * a parameter or a local of the same block. A name that hides one of an outer block raises nothing.
     * @param context Context of the run. Must not be null.
     * @param node Node about to be visited. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckDuplicateNames(MAREA_SemaContext * context, const MAREA_AstNode *node);

    /**
     * @brief Records the name of a for, for each, partition or with statement in the local scope with no type, so
     * a local of an outer scope with the same name is hidden for the length of the statement.
     * @param context Context of the run. Must not be null.
     * @param node Node about to be visited. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaDeclareBinders(MAREA_SemaContext * context, const MAREA_AstNode *node);

    /**
     * @brief Checks what the operands of an operator may be. A bitwise operator (&, |, ^, ~ and the compound forms)
     * rejects a float, a boolean or a string operand with BITWISE_OPERAND_NOT_INTEGER; and, or and not reject an
     * operand that is not a boolean with LOGICAL_OPERAND_NOT_BOOLEAN. An operand of an unknown type is accepted, and
     * any other operator is ignored. The caller calls it only when the operands already agree in type, so one node
     * never carries this code together with TYPE_MISMATCH.
     * @param context Context of the run. Must not be null.
     * @param operatorKind Token kind of the operator.
     * @param left Left operand, or the only operand of a unary operator. May be null.
     * @param right Right operand. Null for a unary operator.
     * @param position Position the diagnostic is reported at.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null context, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckOperatorOperands(MAREA_SemaContext * context, MAREA_TokenKind operatorKind, const MAREA_AstNode *left, const MAREA_AstNode *right, MAREA_SourcePosition position);

    /**
     * @brief Raises RAW_POINTER_OUTSIDE_UNSAFE when a local is declared with the type pointer and no unsafe block
     * encloses it. A parameter, a return type and a structure field are not checked.
     * @param context Context of the run. Must not be null.
     * @param local The LOCAL_DECLARATION node. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckRawPointerLocal(MAREA_SemaContext * context, const MAREA_AstNode *local);

    /**
     * @brief Raises LOCAL_WITHOUT_INITIAL_VALUE when a local has no written value, that is, fewer than three
     * children. The check is syntactic: a branch that assigns the local later does not change it. A local with an
     * error node as its name or type is not checked.
     * @param context Context of the run. Must not be null.
     * @param local The LOCAL_DECLARATION node. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckLocalInitializer(MAREA_SemaContext * context, const MAREA_AstNode *local);

    /**
     * @brief Raises NON_BOOLEAN_CONDITION when the condition of an if, an elseif or a while is a number, string or
     * character literal, or has a known primitive type that is not boolean. An unknown type raises nothing.
     * @param context Context of the run. Must not be null.
     * @param condition The condition expression, any parentheses included. May be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null context, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckCondition(MAREA_SemaContext * context, const MAREA_AstNode *condition);

    /**
     * @brief Tells whether a name is one of the built-in macros: assert, debug_assert, unreachable, compile_error,
     * static_assert, include_bytes, include_text, format, print, log, line, file and namespace.
     * @param text Text of the name, not NUL terminated. May be null when length is 0.
     * @param length Length of text in bytes.
     * @return MAREA_BOOLEAN_TRUE for a built-in macro name, else MAREA_BOOLEAN_FALSE.
     */
    MAREA_NODISCARD MAREA_Boolean MAREA_CALL MAREA_SemaIsBuiltInMacroName(const MAREA_Character8 *text, MAREA_IntegerSize length);

    /**
     * @brief Tells whether a name is one of the 13 primitive type names: boolean, sinteger8 to sinteger64, uinteger8 to
     * uinteger64, float32, float64, character and string.
     * @param text Text of the name, not NUL terminated.
     * @param length Length of text in bytes.
     * @return MAREA_BOOLEAN_TRUE for a primitive type name, else MAREA_BOOLEAN_FALSE.
     */
    MAREA_NODISCARD MAREA_Boolean MAREA_CALL MAREA_SemaIsPrimitiveTypeName(const MAREA_Character8 *text, MAREA_IntegerSize length);

    /**
     * @brief What a built-in macro gives as a value, as far as sema types it.
     */
    typedef enum MAREA_SemaMacroValue
    {
        MAREA_SEMA_MACRO_VALUE_UNKNOWN = MAREA_UINTEGER_8(0),
        MAREA_SEMA_MACRO_VALUE_NONE = MAREA_UINTEGER_8(1),
        MAREA_SEMA_MACRO_VALUE_STRING = MAREA_UINTEGER_8(2),
    } MAREA_SemaMacroValue;

    /**
     * @brief Gives the value of a built-in macro: NONE for assert, debug_assert, unreachable, compile_error,
     * static_assert, print and log, STRING for format, include_text, file and namespace, and UNKNOWN for include_bytes
     * and line (sema does not type them) and for a name that is no built-in macro.
     * @param text Text of the name, not NUL terminated. May be null when length is 0.
     * @param length Length of text in bytes.
     * @return The value.
     */
    MAREA_NODISCARD MAREA_SemaMacroValue MAREA_CALL MAREA_SemaMacroValueOf(const MAREA_Character8 *text, MAREA_IntegerSize length);

    /**
     * @brief Checks one call of a built-in macro (a MACRO_CALL node). A name that no built-in macro has is
     * UNKNOWN_MACRO. A call with the wrong number of arguments, a literal of the wrong kind for a condition or a
     * text, a log level that is not one of trace, debug, info, warning and error, or a format text whose count of
     * {} differs from the count of values is INVALID_MACRO_ARGUMENT. At most one diagnostic is raised for a call. A
     * call with an ERROR_NODE below it raises nothing. The type of a value is never checked.
     * @param context Context of the run. Must not be null.
     * @param call MACRO_CALL node. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, ERROR_NULL_POINTER on a null argument, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckMacro(MAREA_SemaContext * context, const MAREA_AstNode *call);

MAREA_END_EXTERN_C

#endif /* MAREA_INTERNAL_SEMA_MAREASEMAINTERNAL_H */
