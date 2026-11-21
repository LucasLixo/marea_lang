// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INTERNAL_IR_MAREAIRINTERNAL_H)
    #define MAREA_INTERNAL_IR_MAREAIRINTERNAL_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREAArena.h"
    #include "marea/base/MAREADiagnostic.h"
    #include "marea/base/MAREASourcePosition.h"
    #include "marea/ir/MAREAIr.h"
    #include "marea/ir/MAREAIrKind.h"
    #include "marea/ir/MAREAIrNode.h"
    #include "marea/parse/MAREAAst.h"
    #include "marea/parse/MAREAAstKind.h"
    #include "marea/sema/MAREASemaTypedAst.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Body of the opaque ir context: the arena and the sink it was created with.
     */
    struct MAREA_IrContext
    {
        MAREA_Arena *arena;
        MAREA_DiagnosticSink *sink;
    };

    /**
     * @brief Body of the opaque IR node. Position, text, operator and flags are copied from the AST node
     * at creation so that a desugared node can override them.
     */
    struct MAREA_IrNode
    {
        MAREA_SourcePosition position;
        const MAREA_Character8 *text;
        const MAREA_Character8 *typeName;
        MAREA_IrNode **children;
        MAREA_IntegerSize typeNameLength;
        MAREA_IntegerSize childCount;
        MAREA_IntegerSize childCapacity;
        MAREA_Uinteger32 textLength;
        MAREA_Uinteger32 flags;
        MAREA_TokenKind operatorKind;
        MAREA_IrKind kind;
    };

    /**
     * @brief Creates a node of a kind, copying position, text, operator and flags from an AST node.
     * @param context Context that owns the arena. Must not be null.
     * @param kind Kind of the new node.
     * @param astNode AST node to copy from. Must not be null.
     * @param outNode Receives the new node. Must not be null.
     * @return MAREA_IR_EXCEPT_SUCCESS, ERROR_NULL_POINTER when an argument is null, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_IrExcept MAREA_CALL MAREA_IrNodeCreate(MAREA_IrContext * context, MAREA_IrKind kind, const MAREA_AstNode *astNode, MAREA_IrNode **outNode);

    /**
     * @brief Appends a child to a node, growing its child array in the arena.
     * @param context Context that owns the arena. Must not be null.
     * @param parent Node that receives the child. Must not be null.
     * @param child Child to append. Must not be null.
     * @return MAREA_IR_EXCEPT_SUCCESS, ERROR_NULL_POINTER when an argument is null, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_IrExcept MAREA_CALL MAREA_IrNodeAppendChild(MAREA_IrContext * context, MAREA_IrNode * parent, MAREA_IrNode * child);

    /**
     * @brief Sets the type name of a node, read off a typed node. A null node does nothing.
     * @param node Node to change.
     * @param typedNode Typed node whose type name is copied. Null leaves the node without a type name.
     */
    void MAREA_CALL MAREA_IrNodeSetTypeNameFromTyped(MAREA_IrNode * node, const MAREA_SemaTypedNode *typedNode);

MAREA_END_EXTERN_C

#endif /* MAREA_INTERNAL_IR_MAREAIRINTERNAL_H */
