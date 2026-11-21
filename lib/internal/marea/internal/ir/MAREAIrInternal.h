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

    /**
     * @brief Reads the type name of an AST type node: the text span of the qualified name of a
     * NAMED_TYPE, a pointer into the source with no type arguments.
     * @param type AST type node. Null or a FUNCTION_TYPE gives false.
     * @param outText Receives the name text. Must not be null.
     * @param outLength Receives the byte length of outText. Must not be null.
     * @return MAREA_BOOLEAN_TRUE when a name was found, otherwise MAREA_BOOLEAN_FALSE.
     */
    MAREA_NODISCARD MAREA_Boolean MAREA_CALL MAREA_IrAstTypeName(const MAREA_AstNode *type, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength);

    /**
     * @brief Sets the type name of a node from an AST type node. A NAMED_TYPE gives the text of its
     * qualified name, a pointer into the source with no type arguments. Any other node leaves the
     * node without a type name.
     * @param node Node to change.
     * @param type AST type node. Null leaves the node without a type name.
     */
    void MAREA_CALL MAREA_IrNodeSetTypeNameFromAst(MAREA_IrNode * node, const MAREA_AstNode *type);

    /**
     * @brief Sets the text of a node to a name, qualified as qualifier.name when a qualifier is given.
     * The qualified text is a copy in the arena; an unqualified text points into the source.
     * @param context Context that owns the arena. Must not be null.
     * @param node Node to change. Must not be null.
     * @param qualifier Qualifier text, or null for none.
     * @param qualifierLength Byte length of qualifier.
     * @param name Name text. Must not be null.
     * @param nameLength Byte length of name.
     * @return MAREA_IR_EXCEPT_SUCCESS, ERROR_NULL_POINTER when an argument is null, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_IrExcept MAREA_CALL MAREA_IrNodeSetQualifiedText(MAREA_IrContext * context, MAREA_IrNode * node, const MAREA_Character8 *qualifier, MAREA_IntegerSize qualifierLength, const MAREA_Character8 *name, MAREA_IntegerSize nameLength);

    /**
     * @brief Returns one typed child of a typed node, or null when the index is out of range.
     * @param typed Typed node to query. Null returns null.
     * @param index Zero-based child index.
     * @return The typed child, or null.
     */
    MAREA_NODISCARD const MAREA_SemaTypedNode *MAREA_CALL MAREA_IrTypedChildAt(const MAREA_SemaTypedNode *typed, MAREA_IntegerSize index);

    /**
     * @brief Returns the first typed child whose AST kind is kind, or null when there is none.
     * @param typed Typed node to query. Null returns null.
     * @param kind AST kind to look for.
     * @return The typed child, or null.
     */
    MAREA_NODISCARD const MAREA_SemaTypedNode *MAREA_CALL MAREA_IrTypedFindChild(const MAREA_SemaTypedNode *typed, MAREA_AstKind kind);

    /**
     * @brief Returns the AST kind a typed node shadows.
     * @param typed Typed node to query. Null returns MAREA_AST_KIND_ERROR_NODE.
     * @return The AST kind.
     */
    MAREA_NODISCARD MAREA_AstKind MAREA_CALL MAREA_IrTypedKind(const MAREA_SemaTypedNode *typed);

    /**
     * @brief Lowers one top-level item or implementation member and appends the result to a module or a
     * parent. A construct that produces no node (contract, import, namespace) appends nothing.
     * @param context Context that owns the arena and sink. Must not be null.
     * @param typed Typed item to lower. Must not be null.
     * @param qualifier Qualifier text for the name, or null for none.
     * @param qualifierLength Byte length of qualifier.
     * @param parent Node that receives the lowered nodes. Must not be null.
     * @return MAREA_IR_EXCEPT_SUCCESS, ERROR_NULL_POINTER when an argument is null, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_IrExcept MAREA_CALL MAREA_IrLowerDeclaration(MAREA_IrContext * context, const MAREA_SemaTypedNode *typed, const MAREA_Character8 *qualifier, MAREA_IntegerSize qualifierLength, MAREA_IrNode *parent);

MAREA_END_EXTERN_C

#endif /* MAREA_INTERNAL_IR_MAREAIRINTERNAL_H */
