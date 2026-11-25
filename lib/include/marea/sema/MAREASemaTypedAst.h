// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_SEMA_MAREASEMATYPEDAST_H)
    #define MAREA_INCLUDE_SEMA_MAREASEMATYPEDAST_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/parse/MAREAAst.h"
    #include "marea/sema/MAREASema.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief How the checks treat a type: a built-in primitive, a built-in that copies or moves, an
     * unbound generic parameter, a user structure or enum, or a type from outside the language core.
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
     * @brief How a typed node carries its value: by value, by a read-only or a write borrow, or by a
     * move or a copy at an assignment.
     */
    typedef enum MAREA_SemaOwnershipMode
    {
        MAREA_SEMA_OWNERSHIP_MODE_VALUE = MAREA_UINTEGER_8(0),
        MAREA_SEMA_OWNERSHIP_MODE_BORROW = MAREA_UINTEGER_8(1),
        MAREA_SEMA_OWNERSHIP_MODE_MUTABLE_BORROW = MAREA_UINTEGER_8(2),
        MAREA_SEMA_OWNERSHIP_MODE_MOVE = MAREA_UINTEGER_8(3),
        MAREA_SEMA_OWNERSHIP_MODE_COPY = MAREA_UINTEGER_8(4),
    } MAREA_SemaOwnershipMode;

    /**
     * @brief Result of a typed AST operation.
     */
    typedef enum MAREA_SemaTypedAstExcept
    {
        MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS = MAREA_UINTEGER_8(0),
        MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER = MAREA_UINTEGER_8(1),
        MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_OUT_OF_MEMORY = MAREA_UINTEGER_8(2),
        MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NOT_CHECKED = MAREA_UINTEGER_8(3),
        MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NO_TYPE = MAREA_UINTEGER_8(4),
        MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NO_LAYOUT = MAREA_UINTEGER_8(5),
        MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_UNDECIDED = MAREA_UINTEGER_8(6),
        MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NO_BACKEND = MAREA_UINTEGER_8(7),
    } MAREA_SemaTypedAstExcept;

    /**
     * @brief Opaque typed shadow node: the original MAREA_AstNode, its resolved type class and
     * ownership mode, and its children in the shape of the original tree. This is the hand-off
     * contract the ir stage consumes.
     */
    typedef struct MAREA_SemaTypedNode MAREA_SemaTypedNode;

    /**
     * @brief Builds a typed shadow tree for a compilation unit sema already checked.
     * @param context Context root was checked with. Must not be null.
     * @param root COMPILATION_UNIT node already passed to MAREA_SemaCheckCompilationUnit on context,
     * with zero errors. Must not be null.
     * @param outRoot Receives the typed root node, valid for the arena lifetime. Must not be null.
     * @return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS, ERROR_NULL_POINTER when an argument is null,
     * ERROR_OUT_OF_MEMORY when the arena cannot grow, or ERROR_NOT_CHECKED when root was not the last
     * tree checked on context or MAREA_DiagnosticSinkGetErrorCount on MAREA_SemaContextGetDiagnostics
     * reports an error.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaBuildTypedAst(MAREA_SemaContext * context, const MAREA_AstNode *root, const MAREA_SemaTypedNode **outRoot);

    /**
     * @brief Returns the original untyped node a typed node shadows.
     * @param node Typed node to query. Null returns null.
     * @return The original node, or null when node is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_AstNode *MAREA_CALL MAREA_SemaTypedNodeGetAstNode(const MAREA_SemaTypedNode *node);

    /**
     * @brief Returns how many typed children a typed node has.
     * @param node Typed node to query. Null returns 0.
     * @return The child count, equal to MAREA_AstNodeGetChildCount of the original node.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_IntegerSize MAREA_CALL MAREA_SemaTypedNodeGetChildCount(const MAREA_SemaTypedNode *node);

    /**
     * @brief Retrieves one typed child of a typed node.
     * @param node Typed node to query. Must not be null.
     * @param index Zero-based child index.
     * @param out Receives the child, valid for the arena lifetime. Must not be null.
     * @return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS, or ERROR_NULL_POINTER when node or out is null, or
     * when index is not less than the child count.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypedNodeGetChildAt(const MAREA_SemaTypedNode *node, MAREA_IntegerSize index, const MAREA_SemaTypedNode **out);

    /**
     * @brief Returns the resolved type class of a typed node.
     * @param node Typed node to query. Null returns MAREA_SEMA_TYPE_CLASS_PRIMITIVE.
     * @return The type class, or MAREA_SEMA_TYPE_CLASS_PRIMITIVE when node is null or carries no type.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_SemaTypeClass MAREA_CALL MAREA_SemaTypedNodeGetTypeClass(const MAREA_SemaTypedNode *node);

    /**
     * @brief Returns the declaration node of a USER type class.
     * @param node Typed node to query. Must not be null.
     * @param out Receives the declaration, or null when the type class is not USER. Must not be null.
     * @return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS, or ERROR_NULL_POINTER when node or out is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypedNodeGetTypeDeclaration(const MAREA_SemaTypedNode *node, const MAREA_AstNode **out);

    /**
     * @brief Returns the type name of a typed node, a pointer into the source, not NUL terminated.
     * @param node Typed node to query. Must not be null.
     * @param outText Receives the name text, or null when the type carries none. Must not be null.
     * @param outLength Receives the byte length of outText, 0 when outText is null. Must not be null.
     * @return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS, or ERROR_NULL_POINTER when node, outText or
     * outLength is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypedNodeGetTypeName(const MAREA_SemaTypedNode *node, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength);

    /**
     * @brief Returns the canonical full text of the type a typed node has, with its type arguments, in the
     * arena. A node whose type is written in the source has one: PARAMETER, LOCAL_DECLARATION,
     * FIELD_DECLARATION, VARIANT_FIELD, CONSTANT_DECLARATION and FUNCTION_DECLARATION (its result type). A literal
     * has one too, and so has the declared name of a for range and of a for each (the type of the bounds, the element
     * type of the collection and, for a key, uinteger64): a string, a character and a boolean literal give string, character and boolean, and a number
     * literal gives the primitive type its context fixes, else sinteger32 for an integer and float64 for a float. So
     * has an expression whose type sema can know: an operation, a member, an index, a call, a literal value and an
     * identifier read as a value (the rules are in the sema doc, "Resolved type of a literal" and "Resolved type of
     * an expression"). The text of a literal points into the source or is static; any other text is in the arena.
     *
     * The spelling is fixed: an import alias is replaced by what it stands for, a local type alias by the type it
     * names, the type arguments follow in angle brackets joined by a comma with no space (array<sinteger32,4>), a
     * function type is function(sinteger32,sinteger32):sinteger32 with no parameter name, and a dotted name is kept
     * whole.
     * @param node Typed node to query. Must not be null.
     * @param outText Receives the text, valid for the arena lifetime, not NUL terminated, or null when the node has no
     * type. Must not be null.
     * @param outLength Receives the byte length of outText, 0 when outText is null. Must not be null.
     * @return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS, ERROR_NULL_POINTER when node, outText or outLength is null, or
     * ERROR_NO_TYPE when the node has no type, the written type is not a name or a function type sema can spell, or
     * an expression needs a type argument that sema does not know.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypedNodeGetResolvedType(const MAREA_SemaTypedNode *node, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength);

    /**
     * @brief Returns the backend of a function: the member of the key backend that decides for it, already
     * resolved by sema against the target description of the context (see the sema doc, "Backend"): the
     * function attribute over the file attribute, a when item that holds over a plain item, and auto replaced by
     * the best backend the target gives. Only a function of a file in marea.math has one.
     * @param node Typed node to query. Must not be null.
     * @param outText Receives the member name (auto never, only scalar, vector_extension or intrinsics), NUL
     * terminated, valid for the lifetime of the arena, or null when there is none. Must not be null.
     * @param outLength Receives the byte length of outText, 0 when outText is null. Must not be null.
     * @return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS, ERROR_NULL_POINTER when node, outText or outLength is null, or
     * ERROR_NO_BACKEND when the node is not a function of a file in marea.math.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypedNodeGetBackend(const MAREA_SemaTypedNode *node, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength);

    /**
     * @brief Returns the resolved ownership mode of a typed node.
     * @param node Typed node to query. Null returns MAREA_SEMA_OWNERSHIP_MODE_VALUE.
     * @return The ownership mode, or MAREA_SEMA_OWNERSHIP_MODE_VALUE when node is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_SemaOwnershipMode MAREA_CALL MAREA_SemaTypedNodeGetOwnershipMode(const MAREA_SemaTypedNode *node);

    /**
     * @brief Tells whether a typed node is a name that only qualifies a member and is not a value: the name of a
     * structure or an enum, a namespace or an import alias, or a name with no declaration that sema takes for a
     * namespace, written before the member of a member access (Planet in Planet.Earth, stdio in stdio.puts). Such a
     * node has no resolved type and is not an expression that is missing a type.
     * @param node Typed node to query. Null returns false.
     * @return True for such an IDENTIFIER, false for any other node and for a null node.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Boolean MAREA_CALL MAREA_SemaTypedNodeIsQualifier(const MAREA_SemaTypedNode *node);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_SEMA_MAREASEMATYPEDAST_H */
