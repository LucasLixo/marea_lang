// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_PARSE_MAREAAST_H)
    #define MAREA_INCLUDE_PARSE_MAREAAST_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREAArena.h"
    #include "marea/base/MAREASourcePosition.h"
    #include "marea/lex/MAREATokenKind.h"
    #include "marea/parse/MAREAAstKind.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Result of an AST operation.
     */
    typedef enum MAREA_AstExcept
    {
        MAREA_AST_EXCEPT_SUCCESS = MAREA_UINTEGER_8(0),
        MAREA_AST_EXCEPT_ERROR_NULL_POINTER = MAREA_UINTEGER_8(1),
        MAREA_AST_EXCEPT_ERROR_OUT_OF_MEMORY = MAREA_UINTEGER_8(2),
        MAREA_AST_EXCEPT_ERROR_OUT_OF_RANGE = MAREA_UINTEGER_8(3),
    } MAREA_AstExcept;

    /**
     * @brief Bits of the flags word of a node. A flag is set only on the kinds that the AST contract
     * names for it.
     */
    typedef enum MAREA_AstFlag
    {
        MAREA_AST_FLAG_MUTABLE = MAREA_UINTEGER_32(1),
        // INFO: bit 2 held the retired REFERENCE flag (the "reference" parameter keyword); a parameter's
        // passing mode is the "semantics" attribute now, so the bit is not reused.
        MAREA_AST_FLAG_CONSTANT = MAREA_UINTEGER_32(4),
        MAREA_AST_FLAG_HAS_ALIAS = MAREA_UINTEGER_32(8),
        MAREA_AST_FLAG_FILE = MAREA_UINTEGER_32(16),
        MAREA_AST_FLAG_HAS_MEMBERS = MAREA_UINTEGER_32(32),
    } MAREA_AstFlag;

    /**
     * @brief Opaque AST node: a kind, a source position, an optional text, an optional operator, a
     * flags word and an ordered list of children. A node never owns its text; it points into the
     * source buffer of the lexer.
     */
    typedef struct MAREA_AstNode MAREA_AstNode;

    /**
     * @brief Returns the kind of a node.
     * @param node Node to query. Null returns MAREA_AST_KIND_ERROR_NODE.
     * @return The kind of the node.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_AstKind MAREA_CALL MAREA_AstNodeGetKind(const MAREA_AstNode *node);

    /**
     * @brief Returns the position of a node by value: the first token, with a length that covers the span.
     * @param node Node to query. Null returns a zeroed position.
     * @return The position of the node.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_SourcePosition MAREA_CALL MAREA_AstNodeGetPosition(const MAREA_AstNode *node);

    /**
     * @brief Returns the text of a node, a pointer into the source, not NUL terminated.
     * @param node Node to query. Null returns null.
     * @return The text, or null when the node has none.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_Character8 *MAREA_CALL MAREA_AstNodeGetText(const MAREA_AstNode *node);

    /**
     * @brief Returns the byte length of the text of a node.
     * @param node Node to query. Null returns 0.
     * @return The length in bytes, 0 when the node has no text.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Uinteger32 MAREA_CALL MAREA_AstNodeGetTextLength(const MAREA_AstNode *node);

    /**
     * @brief Returns the operator token kind of a node (binary, unary and assignment nodes).
     * @param node Node to query. Null returns MAREA_TOKEN_KIND_ERROR.
     * @return The operator, or MAREA_TOKEN_KIND_ERROR when the node has none.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_TokenKind MAREA_CALL MAREA_AstNodeGetOperator(const MAREA_AstNode *node);

    /**
     * @brief Returns the flags word of a node, a combination of MAREA_AstFlag bits.
     * @param node Node to query. Null returns 0.
     * @return The flags.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Uinteger32 MAREA_CALL MAREA_AstNodeGetFlags(const MAREA_AstNode *node);

    /**
     * @brief Returns how many children a node has.
     * @param node Node to query. Null returns 0.
     * @return The child count.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_IntegerSize MAREA_CALL MAREA_AstNodeGetChildCount(const MAREA_AstNode *node);

    /**
     * @brief Retrieves one child of a node.
     * @param node Node to query. Must not be null.
     * @param index Zero-based child index.
     * @param out Receives the child, valid for the arena lifetime. Must not be null.
     * @return MAREA_AST_EXCEPT_SUCCESS, MAREA_AST_EXCEPT_ERROR_NULL_POINTER when node or out is null,
     * or MAREA_AST_EXCEPT_ERROR_OUT_OF_RANGE when index is not less than the child count.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_AstExcept MAREA_CALL MAREA_AstNodeGetChildAt(const MAREA_AstNode *node, MAREA_IntegerSize index, const MAREA_AstNode **out);

    /**
     * @brief Writes the tree under root as stable text, one node per line, into the arena.
     * @param arena Arena that owns the text. Must not be null.
     * @param root Root of the tree to write. Must not be null.
     * @param outText Receives a NUL-terminated copy of the text. Must not be null.
     * @param outLength Receives the length of the text without the NUL. Must not be null.
     * @return MAREA_AST_EXCEPT_SUCCESS, MAREA_AST_EXCEPT_ERROR_NULL_POINTER when an argument is null,
     * or MAREA_AST_EXCEPT_ERROR_OUT_OF_MEMORY when the arena cannot grow.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_AstExcept MAREA_CALL MAREA_AstDump(MAREA_Arena * arena, const MAREA_AstNode *root, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_PARSE_MAREAAST_H */
