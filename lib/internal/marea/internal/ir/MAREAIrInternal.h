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
    #include "marea/lex/MAREATokenKind.h"
    #include "marea/parse/MAREAAst.h"
    #include "marea/parse/MAREAAstKind.h"
    #include "marea/sema/MAREASemaTypedAst.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief One alias of the unit: a local name and the original text it stands for. The texts point into the
     * source, or into the arena for a joined path. A function type alias has no original text. hasArguments is set
     * when the written right side carries type arguments, which the lowered name does not keep.
     */
    typedef struct MAREA_IrAlias
    {
        const MAREA_Character8 *local;
        const MAREA_Character8 *original;
        MAREA_IntegerSize localLength;
        MAREA_IntegerSize originalLength;
        MAREA_Boolean isImport;
        MAREA_Boolean isFunction;
        MAREA_Boolean hasArguments;
    } MAREA_IrAlias;

    /**
     * @brief Body of the opaque ir context: the arena and the sink it was created with, and the typed root
     * of the unit that is being lowered, which tells whether the unit binds a name of its own. The root is null
     * when no unit is being lowered. The alias table is built once per unit; function is the typed function that
     * is being lowered, or null at module level.
     */
    struct MAREA_IrContext
    {
        MAREA_Arena *arena;
        MAREA_DiagnosticSink *sink;
        const MAREA_SemaTypedNode *unit;
        const MAREA_SemaTypedNode *function;
        MAREA_IrAlias *aliases;
        MAREA_IntegerSize aliasCount;
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
     * @brief Sets the type name of a node, read off a typed node, with every alias lowered to its original. A
     * null node does nothing.
     * @param context Context that owns the arena and the alias table. Must not be null.
     * @param node Node to change.
     * @param typedNode Typed node whose type name is copied. Null leaves the node without a type name.
     * @return MAREA_IR_EXCEPT_SUCCESS, ERROR_NULL_POINTER when context is null, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_IrExcept MAREA_CALL MAREA_IrNodeSetTypeNameFromTyped(MAREA_IrContext * context, MAREA_IrNode * node, const MAREA_SemaTypedNode *typedNode);

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
     * @brief Reads the text span of a QUALIFIED_NAME: from its first segment to its last, a pointer into the
     * source, so a dotted name stays whole.
     * @param name AST QUALIFIED_NAME node. Null or another kind gives false.
     * @param outText Receives the name text. Must not be null.
     * @param outLength Receives the byte length of outText. Must not be null.
     * @return MAREA_BOOLEAN_TRUE when a name was found, otherwise MAREA_BOOLEAN_FALSE.
     */
    MAREA_NODISCARD MAREA_Boolean MAREA_CALL MAREA_IrAstQualifiedNameSpan(const MAREA_AstNode *name, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength);

    /**
     * @brief Sets the type name of a node from an AST type node. A NAMED_TYPE gives the text of its
     * qualified name, a pointer into the source with no type arguments. Any other node leaves the
     * node without a type name. Every alias is lowered to its original.
     * @param context Context that owns the arena and the alias table. Must not be null.
     * @param node Node to change.
     * @param type AST type node. Null leaves the node without a type name.
     * @return MAREA_IR_EXCEPT_SUCCESS, ERROR_NULL_POINTER when context is null, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_IrExcept MAREA_CALL MAREA_IrNodeSetTypeNameFromAst(MAREA_IrContext * context, MAREA_IrNode * node, const MAREA_AstNode *type);

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

    /**
     * @brief Lowers one typed expression and its subtree. A parenthesized expression lowers to its
     * inner expression. An unsupported construct, or an operand that lowers to nothing, gives no node.
     * @param context Context that owns the arena and sink. Must not be null.
     * @param typed Typed expression to lower. Must not be null.
     * @param outNode Receives the lowered node, or null when nothing is produced. Must not be null.
     * @return MAREA_IR_EXCEPT_SUCCESS, ERROR_NULL_POINTER when an argument is null, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_IrExcept MAREA_CALL MAREA_IrLowerExpression(MAREA_IrContext * context, const MAREA_SemaTypedNode *typed, MAREA_IrNode **outNode);

    /**
     * @brief Creates a node of a kind and names it after the IDENTIFIER child of a typed node, qualified
     * as qualifier.name when a qualifier is given. The type name is read off the first type child, and
     * only the MUTABLE flag survives.
     * @param context Context that owns the arena. Must not be null.
     * @param kind Kind of the new node.
     * @param typed Typed node to read the name and type from. Must not be null.
     * @param qualifier Qualifier text, or null for none.
     * @param qualifierLength Byte length of qualifier.
     * @param outNode Receives the new node. Must not be null.
     * @return MAREA_IR_EXCEPT_SUCCESS, ERROR_NULL_POINTER when an argument is null, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_IrExcept MAREA_CALL MAREA_IrCreateNamed(MAREA_IrContext * context, MAREA_IrKind kind, const MAREA_SemaTypedNode *typed, const MAREA_Character8 *qualifier, MAREA_IntegerSize qualifierLength, MAREA_IrNode **outNode);

    /**
     * @brief Lowers the expression right after the type child of a typed node, when there is one, and
     * appends it to node. When there is none and zeroWhenAbsent is true, appends a ZERO_VALUE instead,
     * the zero-default rule for a LOCAL_DECLARATION with no written initializer; a PARAMETER default, a
     * CONSTANT_DECLARATION initializer and a VARIANT_FIELD value stay outside the rule and pass false.
     * @param context Context that owns the arena. Must not be null.
     * @param typed Typed node that holds the type and the initializer. Must not be null.
     * @param node Node that receives the lowered initializer or the synthesized zero. Must not be null.
     * @param zeroWhenAbsent True to append a ZERO_VALUE when typed has no written initializer.
     * @param outComplete Receives false when the initializer lowered to nothing. Must not be null.
     * @return MAREA_IR_EXCEPT_SUCCESS, ERROR_NULL_POINTER when an argument is null, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_IrExcept MAREA_CALL MAREA_IrAppendInitializer(MAREA_IrContext * context, const MAREA_SemaTypedNode *typed, MAREA_IrNode *node, MAREA_Boolean zeroWhenAbsent, MAREA_Boolean *outComplete);

    /**
     * @brief Returns the typed expression right after a typed node's first type child (NAMED_TYPE or
     * FUNCTION_TYPE): the written default of a parameter, or the written initializer of a field or a
     * constant. The same shape also reads a function's own parameter default when filling an omitted
     * call argument.
     * @param typed Typed node to search. Null returns null.
     * @return The typed default expression, or null when the node has no type child or nothing follows it.
     */
    MAREA_NODISCARD const MAREA_SemaTypedNode *MAREA_CALL MAREA_IrTypedFindDefault(const MAREA_SemaTypedNode *typed);

    /**
     * @brief Finds the construct or destroy operator of a type: the FUNCTION_DECLARATION, inside an
     * implementation of the unit whose written type name is typeName, that carries
     * #[operator = "construct"] (wantConstruct true) or #[operator = "destroy"] (wantConstruct false).
     * Only the first ATTRIBUTE group of a candidate function is read.
     * @param context Context that holds the unit. Must not be null.
     * @param typeName Written name of the structure or enum. Null returns null.
     * @param typeNameLength Byte length of typeName.
     * @param wantConstruct True for construct, false for destroy.
     * @return The typed FUNCTION_DECLARATION that carries the operator, or null when the type declares none.
     */
    MAREA_NODISCARD const MAREA_SemaTypedNode *MAREA_CALL MAREA_IrFindLifecycleFunction(const MAREA_IrContext *context, const MAREA_Character8 *typeName, MAREA_IntegerSize typeNameLength, MAREA_Boolean wantConstruct);

    /**
     * @brief Lowers one typed BLOCK and its statements. A statement that cannot be lowered makes the
     * whole block lower to nothing, so no partial body appears.
     * @param context Context that owns the arena and sink. Must not be null.
     * @param typed Typed BLOCK node. Must not be null.
     * @param outNode Receives the BLOCK node, or null when nothing is produced. Must not be null.
     * @return MAREA_IR_EXCEPT_SUCCESS, ERROR_NULL_POINTER when an argument is null, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_IrExcept MAREA_CALL MAREA_IrLowerBlock(MAREA_IrContext * context, const MAREA_SemaTypedNode *typed, MAREA_IrNode **outNode);

    /**
     * @brief Lowers a call that carries an explicit type operand: a built-in call (size_of, align_of or
     * convert with type arguments) or a static call on a written type (array<sinteger32>.filled(8, 0)). Any
     * other call is not handled and is left to the generic path of MAREA_IrLowerExpression.
     * @param context Context that owns the arena and sink. Must not be null.
     * @param typed Typed CALL_EXPRESSION to lower. Must not be null.
     * @param outHandled Receives true when the call is one of the two forms, also when a gap inside it raised a
     * diagnostic, and false when the caller must lower the call itself. Must not be null.
     * @param outNode Receives the lowered call, or null when nothing is produced. Must not be null.
     * @return MAREA_IR_EXCEPT_SUCCESS, ERROR_NULL_POINTER when an argument is null, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_IrExcept MAREA_CALL MAREA_IrLowerTypeOperandCall(MAREA_IrContext * context, const MAREA_SemaTypedNode *typed, MAREA_Boolean *outHandled, MAREA_IrNode **outNode);

    /**
     * @brief Raises one ir error diagnostic, positioned at an AST node. The message is the text followed
     * by the name in single quotes when a name is given. A long message is cut to fit.
     * @param context Context that owns the sink. Must not be null.
     * @param code Diagnostic code, in the 4000-4999 range.
     * @param at AST node that gives the position. Null gives a zeroed position.
     * @param message Short English text that names the construct. Must not be null.
     * @param name Name text to append, or null for none.
     * @param nameLength Byte length of name.
     * @return MAREA_IR_EXCEPT_SUCCESS, ERROR_NULL_POINTER when context or message is null, or
     * ERROR_OUT_OF_MEMORY when the sink cannot grow.
     */
    MAREA_NODISCARD MAREA_IrExcept MAREA_CALL MAREA_IrRaise(MAREA_IrContext * context, MAREA_IrDiagnosticCode code, const MAREA_AstNode *at, const MAREA_Character8 *message, const MAREA_Character8 *name, MAREA_IntegerSize nameLength);

    /**
     * @brief Builds the alias table of a unit: every whole-path alias, every member alias and every plain
     * `type Name = Other;` alias that the unit declares. Generic type aliases are left out.
     * @param context Context that owns the arena. Must not be null.
     * @param unit Typed COMPILATION_UNIT. Must not be null.
     * @return MAREA_IR_EXCEPT_SUCCESS, ERROR_NULL_POINTER when an argument is null, or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_IrExcept MAREA_CALL MAREA_IrAliasBuild(MAREA_IrContext * context, const MAREA_SemaTypedNode *unit);

    /**
     * @brief Tells whether a typed function declares a name as a parameter or a local of any block, at or
     * before a source offset: the shadow check an identifier reference, or a callee matched against a
     * module-level declaration, must pass before it is treated as that outer name.
     * @param typed Typed node to search, normally a typed FUNCTION_DECLARATION. Null returns false.
     * @param text Identifier text to look for.
     * @param length Byte length of text.
     * @param offset Source offset the declaration must be at or before.
     * @return MAREA_BOOLEAN_TRUE when a parameter or a local declares the name at or before offset.
     */
    MAREA_NODISCARD MAREA_Boolean MAREA_CALL MAREA_IrFunctionDeclares(const MAREA_SemaTypedNode *typed, const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_Uinteger32 offset);

    /**
     * @brief Looks up an identifier used as a value (a function, a constant, a variant or the head of a member
     * chain). An import alias gives its original text, unless a parameter or a local of the enclosing function
     * has the same name and is declared at or before the reference.
     * @param context Context that holds the alias table. Must not be null.
     * @param text Identifier text.
     * @param length Byte length of text.
     * @param offset Source offset of the reference.
     * @param outText Receives the original text. Must not be null.
     * @param outLength Receives the byte length of outText. Must not be null.
     * @return MAREA_BOOLEAN_TRUE when the identifier is an alias that applies, otherwise MAREA_BOOLEAN_FALSE.
     */
    MAREA_NODISCARD MAREA_Boolean MAREA_CALL MAREA_IrAliasValue(const MAREA_IrContext *context, const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_Uinteger32 offset, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength);

    /**
     * @brief Lowers a written type name to its original: a chain of type aliases is followed up to 8 steps and
     * ends at an import alias, and the head of a dotted name that is a namespace alias is replaced. A name that
     * is no alias is returned unchanged. An alias of a function type lowers to a null text of length 0, as the
     * written function type has no type name.
     * @param context Context that owns the arena and holds the alias table. Must not be null.
     * @param text Type name text.
     * @param length Byte length of text.
     * @param outText Receives the lowered text. Must not be null.
     * @param outLength Receives the byte length of outText. Must not be null.
     * @return MAREA_IR_EXCEPT_SUCCESS or ERROR_OUT_OF_MEMORY.
     */
    MAREA_NODISCARD MAREA_IrExcept MAREA_CALL MAREA_IrAliasType(MAREA_IrContext * context, const MAREA_Character8 *text, MAREA_IntegerSize length, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength);

    /**
     * @brief Tells whether lowering a type name through the alias table loses something: a step of the chain
     * carries type arguments, or the chain ends at a function type.
     * @param context Context that holds the alias table. Must not be null.
     * @param text Type name text.
     * @param length Byte length of text.
     * @return MAREA_BOOLEAN_TRUE when the lowered name is not the whole type, otherwise MAREA_BOOLEAN_FALSE.
     */
    MAREA_NODISCARD MAREA_Boolean MAREA_CALL MAREA_IrAliasTypeIsLossy(MAREA_IrContext * context, const MAREA_Character8 *text, MAREA_IntegerSize length);

MAREA_END_EXTERN_C

#endif /* MAREA_INTERNAL_IR_MAREAIRINTERNAL_H */
