// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INTERNAL_PARSE_MAREAPARSEINTERNAL_H)
    #define MAREA_INTERNAL_PARSE_MAREAPARSEINTERNAL_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREAArena.h"
    #include "marea/base/MAREASourcePosition.h"
    #include "marea/lex/MAREATokenKind.h"
    #include "marea/parse/MAREAAst.h"
    #include "marea/parse/MAREAAstKind.h"

MAREA_BEGIN_EXTERN_C

    struct MAREA_AstNode
    {
        MAREA_SourcePosition position;
        const MAREA_Character8 *text;
        MAREA_AstNode **children;
        MAREA_IntegerSize childCount;
        MAREA_IntegerSize childCapacity;
        MAREA_Uinteger32 textLength;
        MAREA_Uinteger32 flags;
        MAREA_TokenKind operatorKind;
        MAREA_AstKind kind;
    };

    /**
     * @brief Creates a node in the arena. Internal to the parse stage. It is declared exported only so
     * the dump test can build a tree by hand; no other consumer may call it.
     * @param arena Arena that owns the node. Must not be null.
     * @param kind Kind of the new node.
     * @param position Position of the node.
     * @param text Text of the node, or null. Not copied.
     * @param textLength Length of text in bytes.
     * @param outNode Receives the new node. Must not be null.
     * @return MAREA_AST_EXCEPT_SUCCESS, MAREA_AST_EXCEPT_ERROR_NULL_POINTER when arena or outNode is null, or
     * MAREA_AST_EXCEPT_ERROR_OUT_OF_MEMORY when the arena cannot grow.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_AstExcept MAREA_CALL MAREA_AstNodeCreate(MAREA_Arena * arena, MAREA_AstKind kind, MAREA_SourcePosition position, const MAREA_Character8 *text, MAREA_IntegerSize textLength, MAREA_AstNode **outNode);

    /**
     * @brief Appends a child to a node. The child array doubles in the arena.
     * @param arena Arena that owns the array. Must not be null.
     * @param parent Node that receives the child. Must not be null.
     * @param child Child to append. Must not be null.
     * @return MAREA_AST_EXCEPT_SUCCESS, MAREA_AST_EXCEPT_ERROR_NULL_POINTER when an argument is null, or
     * MAREA_AST_EXCEPT_ERROR_OUT_OF_MEMORY when the arena cannot grow.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_AstExcept MAREA_CALL MAREA_AstNodeAppendChild(MAREA_Arena * arena, MAREA_AstNode * parent, MAREA_AstNode * child);

    /**
     * @brief Sets the operator of a node. A null node is ignored.
     * @param node Node to change.
     * @param operatorKind Operator token kind.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_AstNodeSetOperator(MAREA_AstNode * node, MAREA_TokenKind operatorKind);

    /**
     * @brief Sets the flags word of a node. A null node is ignored.
     * @param node Node to change.
     * @param flags New flags word.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_AstNodeSetFlags(MAREA_AstNode * node, MAREA_Uinteger32 flags);

MAREA_END_EXTERN_C

#endif /* MAREA_INTERNAL_PARSE_MAREAPARSEINTERNAL_H */
