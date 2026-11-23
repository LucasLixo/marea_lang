// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_IR_MAREAIRNODE_H)
    #define MAREA_INCLUDE_IR_MAREAIRNODE_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREAArena.h"
    #include "marea/base/MAREASourcePosition.h"
    #include "marea/ir/MAREAIrKind.h"
    #include "marea/lex/MAREATokenKind.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Result of an IR node operation.
     */
    typedef enum MAREA_IrNodeExcept
    {
        MAREA_IR_NODE_EXCEPT_SUCCESS = MAREA_UINTEGER_8(0),
        MAREA_IR_NODE_EXCEPT_ERROR_NULL_POINTER = MAREA_UINTEGER_8(1),
        MAREA_IR_NODE_EXCEPT_ERROR_OUT_OF_RANGE = MAREA_UINTEGER_8(2),
    } MAREA_IrNodeExcept;

    /**
     * @brief Opaque IR node: a kind, a source position, an optional text, an optional operator, a flags
     * word, an optional type name and an ordered list of children. One node type serves every kind. A
     * node never owns its text or type name; both point into the source buffer of the lexer.
     */
    typedef struct MAREA_IrNode MAREA_IrNode;

    /**
     * @brief Returns the kind of a node.
     * @param node Node to query. Null returns MAREA_IR_KIND_COUNT.
     * @return The kind of the node.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_IrKind MAREA_CALL MAREA_IrNodeGetKind(const MAREA_IrNode *node);

    /**
     * @brief Returns the source position of a node by value, copied from the AST node it comes from.
     * @param node Node to query. Null returns a zeroed position.
     * @return The position of the node.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_SourcePosition MAREA_CALL MAREA_IrNodeGetPosition(const MAREA_IrNode *node);

    /**
     * @brief Returns the text of a node, a pointer into the source, not NUL terminated.
     * @param node Node to query. Null returns null.
     * @return The text, or null when the node has none.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_Character8 *MAREA_CALL MAREA_IrNodeGetText(const MAREA_IrNode *node);

    /**
     * @brief Returns the byte length of the text of a node.
     * @param node Node to query. Null returns 0.
     * @return The length in bytes, 0 when the node has no text.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Uinteger32 MAREA_CALL MAREA_IrNodeGetTextLength(const MAREA_IrNode *node);

    /**
     * @brief Returns the operator token kind of a node (binary, unary and assignment nodes).
     * @param node Node to query. Null returns MAREA_TOKEN_KIND_ERROR.
     * @return The operator, or MAREA_TOKEN_KIND_ERROR when the node has none.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_TokenKind MAREA_CALL MAREA_IrNodeGetOperator(const MAREA_IrNode *node);

    /**
     * @brief Returns the flags word of a node. It reuses the MAREA_AstFlag bits; only MAREA_AST_FLAG_MUTABLE
     * is set, on a write-target PARAMETER or LOCAL_DECLARATION.
     * @param node Node to query. Null returns 0.
     * @return The flags.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Uinteger32 MAREA_CALL MAREA_IrNodeGetFlags(const MAREA_IrNode *node);

    /**
     * @brief Returns how many children a node has.
     * @param node Node to query. Null returns 0.
     * @return The child count.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_IntegerSize MAREA_CALL MAREA_IrNodeGetChildCount(const MAREA_IrNode *node);

    /**
     * @brief Retrieves one child of a node.
     * @param node Node to query. Must not be null.
     * @param index Zero-based child index.
     * @param out Receives the child, valid for the arena lifetime. Must not be null.
     * @return MAREA_IR_NODE_EXCEPT_SUCCESS, MAREA_IR_NODE_EXCEPT_ERROR_NULL_POINTER when node or out is
     * null, or MAREA_IR_NODE_EXCEPT_ERROR_OUT_OF_RANGE when index is not less than the child count.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_IrNodeExcept MAREA_CALL MAREA_IrNodeGetChildAt(const MAREA_IrNode *node, MAREA_IntegerSize index, const MAREA_IrNode **out);

    /**
     * @brief Returns the type name of a node, a pointer into the source, not NUL terminated. Only a
     * LOCAL_DECLARATION, PARAMETER, FIELD_DECLARATION, struct LITERAL_VALUE and a non-generic
     * CALL_EXPRESSION result carry one.
     * @param node Node to query. Null returns null.
     * @return The type name, or null when the node has none.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_Character8 *MAREA_CALL MAREA_IrNodeGetTypeName(const MAREA_IrNode *node);

    /**
     * @brief Returns the byte length of the type name of a node.
     * @param node Node to query. Null returns 0.
     * @return The length in bytes, 0 when the node has no type name.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_IntegerSize MAREA_CALL MAREA_IrNodeGetTypeNameLength(const MAREA_IrNode *node);

    /**
     * @brief Returns the full resolved type of a node as sema spells it, with its type arguments (for example
     * array<sinteger32,4>), a copy in the arena of the ir context, not NUL terminated. Every expression node that
     * comes from the source and every PARAMETER, LOCAL_DECLARATION, FIELD_DECLARATION, VARIANT_FIELD,
     * CONSTANT_DECLARATION and FUNCTION_DECLARATION with a written type carry one. A name that only qualifies a
     * member, a node that the ir invents, and any other kind carry none. The type name of
     * MAREA_IrNodeGetTypeName is not changed by it.
     * @param node Node to query. Null returns null.
     * @return The resolved type, or null when the node has none.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_Character8 *MAREA_CALL MAREA_IrNodeGetResolvedType(const MAREA_IrNode *node);

    /**
     * @brief Returns the byte length of the resolved type of a node.
     * @param node Node to query. Null returns 0.
     * @return The length in bytes, 0 when the node has no resolved type.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_IntegerSize MAREA_CALL MAREA_IrNodeGetResolvedTypeLength(const MAREA_IrNode *node);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_IR_MAREAIRNODE_H */
