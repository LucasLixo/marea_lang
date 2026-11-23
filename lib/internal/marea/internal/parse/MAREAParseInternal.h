// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INTERNAL_PARSE_MAREAPARSEINTERNAL_H)
    #define MAREA_INTERNAL_PARSE_MAREAPARSEINTERNAL_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREAArena.h"
    #include "marea/base/MAREADiagnostic.h"
    #include "marea/base/MAREASourcePosition.h"
    #include "marea/lex/MAREAToken.h"
    #include "marea/lex/MAREATokenKind.h"
    #include "marea/parse/MAREAAst.h"
    #include "marea/parse/MAREAAstKind.h"
    #include "marea/parse/MAREAParser.h"

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
     * @brief Creates a node in the arena. Internal to the parse stage.
     * @param arena Arena that owns the node. Must not be null.
     * @param kind Kind of the new node.
     * @param position Position of the node.
     * @param text Text of the node, or null. Not copied.
     * @param textLength Length of text in bytes.
     * @param outNode Receives the new node. Must not be null.
     * @return MAREA_AST_EXCEPT_SUCCESS, MAREA_AST_EXCEPT_ERROR_NULL_POINTER when arena or outNode is null, or
     * MAREA_AST_EXCEPT_ERROR_OUT_OF_MEMORY when the arena cannot grow.
     */
    MAREA_NODISCARD MAREA_AstExcept MAREA_CALL MAREA_AstNodeCreate(MAREA_Arena * arena, MAREA_AstKind kind, MAREA_SourcePosition position, const MAREA_Character8 *text, MAREA_IntegerSize textLength, MAREA_AstNode **outNode);

    /**
     * @brief Appends a child to a node. The child array doubles in the arena.
     * @param arena Arena that owns the array. Must not be null.
     * @param parent Node that receives the child. Must not be null.
     * @param child Child to append. Must not be null.
     * @return MAREA_AST_EXCEPT_SUCCESS, MAREA_AST_EXCEPT_ERROR_NULL_POINTER when an argument is null, or
     * MAREA_AST_EXCEPT_ERROR_OUT_OF_MEMORY when the arena cannot grow.
     */
    MAREA_NODISCARD MAREA_AstExcept MAREA_CALL MAREA_AstNodeAppendChild(MAREA_Arena * arena, MAREA_AstNode * parent, MAREA_AstNode * child);

    /**
     * @brief Sets the operator of a node. A null node is ignored.
     * @param node Node to change.
     * @param operatorKind Operator token kind.
     */
    void MAREA_CALL MAREA_AstNodeSetOperator(MAREA_AstNode * node, MAREA_TokenKind operatorKind);

    /**
     * @brief Sets the flags word of a node. A null node is ignored.
     * @param node Node to change.
     * @param flags New flags word.
     */
    void MAREA_CALL MAREA_AstNodeSetFlags(MAREA_AstNode * node, MAREA_Uinteger32 flags);

    /**
     * @brief Sets the length of the position of a node so the node ends at endOffset. Ignored when node
     * is null or endOffset is not past the start of the node.
     * @param node Node to change.
     * @param endOffset Byte offset one past the last byte of the node.
     */
    void MAREA_CALL MAREA_AstNodeSetEnd(MAREA_AstNode * node, MAREA_Uinteger32 endOffset);

    #if !defined(MAREA_PARSE_DEPTH_LIMIT)
        #define MAREA_PARSE_DEPTH_LIMIT MAREA_UINTEGER_32(256)
    #endif

    struct MAREA_ParseContext
    {
        MAREA_Arena *arena;
        MAREA_DiagnosticSink *sink;
    };

    /**
     * @brief Cursor and bookkeeping of one parse run. The token list is read only through the public lex
     * accessors. While halfGreater is true, the current token is a SHIFT_RIGHT whose first angle bracket
     * was already consumed, so it reads as a GREATER one byte later.
     */
    typedef struct MAREA_ParseState
    {
        MAREA_ParseContext *context;
        const MAREA_TokenList *list;
        MAREA_IntegerSize count;
        MAREA_IntegerSize index;
        MAREA_Uinteger32 lastEnd;
        MAREA_Uinteger32 lastErrorOffset;
        MAREA_Uinteger32 depth;
        MAREA_Boolean halfGreater;
        MAREA_Boolean hasError;
        MAREA_Boolean outOfMemory;
    } MAREA_ParseState;

    /**
     * @brief Returns the token that is ahead tokens past the current one, or the END_OF_FILE token when
     * that is past the end. Never null.
     */
    const MAREA_Token *MAREA_CALL MAREA_ParsePeekToken(const MAREA_ParseState *state, MAREA_IntegerSize ahead);

    /**
     * @brief Returns the kind of the token ahead tokens past the current one. At ahead 0 a half consumed
     * SHIFT_RIGHT reads as GREATER.
     */
    MAREA_TokenKind MAREA_CALL MAREA_ParsePeekKind(const MAREA_ParseState *state, MAREA_IntegerSize ahead);

    /**
     * @brief Tells whether the current token has the given kind.
     */
    MAREA_Boolean MAREA_CALL MAREA_ParseAt(const MAREA_ParseState *state, MAREA_TokenKind kind);

    /**
     * @brief Consumes the current token and returns it. At END_OF_FILE nothing is consumed.
     */
    const MAREA_Token *MAREA_CALL MAREA_ParseAdvance(MAREA_ParseState * state);

    /**
     * @brief Consumes the current token when it has the given kind. Returns whether it did.
     */
    MAREA_Boolean MAREA_CALL MAREA_ParseAccept(MAREA_ParseState * state, MAREA_TokenKind kind);

    /**
     * @brief Consumes a token of the given kind, or reports EXPECTED_TOKEN with message at the current
     * token and consumes nothing. Returns whether the token was there.
     */
    MAREA_Boolean MAREA_CALL MAREA_ParseExpect(MAREA_ParseState * state, MAREA_TokenKind kind, const MAREA_Character8 *message);

    /**
     * @brief Consumes one closing angle bracket. A SHIFT_RIGHT token is split: the first call consumes its
     * first bracket, the second call the token. Reports EXPECTED_TOKEN with message when there is none.
     */
    MAREA_Boolean MAREA_CALL MAREA_ParseExpectGreater(MAREA_ParseState * state, const MAREA_Character8 *message);

    /**
     * @brief Adds one parse diagnostic at the position of token. Two errors at the same offset in a row
     * give one diagnostic, so a recovery cannot flood the sink.
     */
    void MAREA_CALL MAREA_ParseReport(MAREA_ParseState * state, MAREA_ParseDiagnosticCode code, const MAREA_Token *token, const MAREA_Character8 *message);

    /**
     * @brief Enters one nesting level. At the depth limit it reports RECURSION_TOO_DEEP and returns false;
     * the caller must then not call MAREA_ParseLeave.
     */
    MAREA_Boolean MAREA_CALL MAREA_ParseEnter(MAREA_ParseState * state);

    /**
     * @brief Leaves a nesting level entered with MAREA_ParseEnter.
     */
    void MAREA_CALL MAREA_ParseLeave(MAREA_ParseState * state);

    /**
     * @brief Tells whether kind starts a top level item or a statement, so recovery stops before it.
     */
    MAREA_Boolean MAREA_CALL MAREA_ParseIsSynchronizationKind(MAREA_TokenKind kind);

    /**
     * @brief Skips tokens up to a synchronisation point: a semicolon (consumed), or end, case, default,
     * elseif, else, a keyword that starts a statement or an item, or the end of file (not consumed).
     */
    void MAREA_CALL MAREA_ParseSynchronize(MAREA_ParseState * state);

    /**
     * @brief Creates a node positioned at the current token, without text. Null after an out of memory.
     */
    MAREA_AstNode *MAREA_CALL MAREA_ParseNodeBegin(MAREA_ParseState * state, MAREA_AstKind kind);

    /**
     * @brief Creates a node that starts at position, without text. Used for a node that begins at its
     * first operand. Null after an out of memory.
     */
    MAREA_AstNode *MAREA_CALL MAREA_ParseNodeBeginAt(MAREA_ParseState * state, MAREA_AstKind kind, MAREA_SourcePosition position);

    /**
     * @brief Creates a node positioned at token, with the text of token. Null after an out of memory.
     */
    MAREA_AstNode *MAREA_CALL MAREA_ParseNodeFromToken(MAREA_ParseState * state, MAREA_AstKind kind, const MAREA_Token *token);

    /**
     * @brief Sets the length of the position of node so it ends at the last consumed token. Null safe.
     */
    void MAREA_CALL MAREA_ParseNodeEnd(const MAREA_ParseState *state, MAREA_AstNode *node);

    /**
     * @brief Appends child to parent. Null safe on both: a null child (after an out of memory) is dropped.
     */
    void MAREA_CALL MAREA_ParseNodeAdd(MAREA_ParseState * state, MAREA_AstNode * parent, MAREA_AstNode * child);

    /**
     * @brief Creates an ERROR_NODE positioned at the current token, without consuming it.
     */
    MAREA_AstNode *MAREA_CALL MAREA_ParseErrorNode(MAREA_ParseState * state);

    /**
     * @brief Parses an identifier into an IDENTIFIER node, or reports EXPECTED_TOKEN with message and
     * returns an ERROR_NODE without consuming.
     */
    MAREA_AstNode *MAREA_CALL MAREA_ParseIdentifier(MAREA_ParseState * state, const MAREA_Character8 *message);

    /**
     * @brief Parses qualified_name into a QUALIFIED_NAME node whose children are IDENTIFIER nodes.
     */
    MAREA_AstNode *MAREA_CALL MAREA_ParseQualifiedName(MAREA_ParseState * state);

    /**
     * @brief Parses zero or more attribute lists of the given opener (HASH_BRACKET or HASH_BANG_BRACKET) and
     * appends each as an ATTRIBUTE or FILE_ATTRIBUTE child of holder.
     */
    void MAREA_CALL MAREA_ParseAttributes(MAREA_ParseState * state, MAREA_AstNode * holder, MAREA_TokenKind opener);

    /**
     * @brief Parses a type: a named type or a function type. Reports EXPECTED_TYPE and returns an
     * ERROR_NODE without consuming when none starts here.
     */
    MAREA_AstNode *MAREA_CALL MAREA_ParseType(MAREA_ParseState * state);

    /**
     * @brief Parses named_type into a NAMED_TYPE node: a qualified name and optional type arguments.
     */
    MAREA_AstNode *MAREA_CALL MAREA_ParseNamedType(MAREA_ParseState * state);

    /**
     * @brief Parses type_arguments into a TYPE_ARGUMENTS node. The current token must be a less than.
     */
    MAREA_AstNode *MAREA_CALL MAREA_ParseTypeArguments(MAREA_ParseState * state);

    /**
     * @brief Parses a parenthesized parameter list into a PARAMETER_LIST node. Always returns a node.
     * @param allowDefault MAREA_BOOLEAN_TRUE lets a parameter have "= expression"; MAREA_BOOLEAN_FALSE (a function type) reports UNEXPECTED_TOKEN and still parses the expression.
     */
    MAREA_AstNode *MAREA_CALL MAREA_ParseParameterList(MAREA_ParseState * state, MAREA_Boolean allowDefault);

    /**
     * @brief Parses generic_parameters into a GENERIC_PARAMETERS node. The current token must be a less than.
     */
    MAREA_AstNode *MAREA_CALL MAREA_ParseGenericParameters(MAREA_ParseState * state);

    /**
     * @brief Parses where_clause into a WHERE_CLAUSE node. The current token must be where.
     */
    MAREA_AstNode *MAREA_CALL MAREA_ParseWhereClause(MAREA_ParseState * state);

    /**
     * @brief Parses an expression. Reports EXPECTED_EXPRESSION and returns an ERROR_NODE without
     * consuming when none starts here.
     */
    MAREA_AstNode *MAREA_CALL MAREA_ParseExpression(MAREA_ParseState * state);

    /**
     * @brief Parses a block: statements up to end, the end of file, or the start of a top level
     * declaration. The words elseif, else, case and default are reported as UNEXPECTED_TOKEN and
     * skipped, because every branch has its own end. The terminator is not consumed.
     */
    MAREA_AstNode *MAREA_CALL MAREA_ParseBlock(MAREA_ParseState * state);

    /**
     * @brief Parses one statement. Returns an ERROR_NODE without consuming when none starts here.
     */
    MAREA_AstNode *MAREA_CALL MAREA_ParseStatement(MAREA_ParseState * state);

    /**
     * @brief Moves the children of holder, the attributes parsed before an item, to the front of node.
     */
    void MAREA_CALL MAREA_ParseAdoptAttributes(MAREA_ParseState * state, MAREA_AstNode * node, const MAREA_AstNode *holder);

    /**
     * @brief Marks a bad member of a block with an ERROR_NODE under parent, skips to a safe token and
     * moves one token on when nothing was consumed since before. Keeps member loops from stalling.
     */
    void MAREA_CALL MAREA_ParseSkipBadMember(MAREA_ParseState * state, MAREA_IntegerSize before, MAREA_AstNode * parent);

    /**
     * @brief Tells whether kind starts a top level declaration that MAREA_ParseDeclaration handles.
     */
    MAREA_Boolean MAREA_CALL MAREA_ParseIsDeclarationStart(MAREA_TokenKind kind);

    /**
     * @brief Parses one declaration whose keyword is the current token. The attributes already parsed wait
     * in holder and become the leading children of the node.
     */
    MAREA_AstNode *MAREA_CALL MAREA_ParseDeclaration(MAREA_ParseState * state, MAREA_AstNode * holder);

    /**
     * @brief Consumes the end that closes a construct, or reports UNTERMINATED_BLOCK at the end of file or
     * EXPECTED_TOKEN with message elsewhere.
     */
    void MAREA_CALL MAREA_ParseExpectEnd(MAREA_ParseState * state, const MAREA_Character8 *message);

MAREA_END_EXTERN_C

#endif /* MAREA_INTERNAL_PARSE_MAREAPARSEINTERNAL_H */
