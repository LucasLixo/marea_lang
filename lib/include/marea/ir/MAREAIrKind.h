// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_IR_MAREAIRKIND_H)
    #define MAREA_INCLUDE_IR_MAREAIRKIND_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Kind of an IR node. A kind keeps the name and the child layout of the AST kind it comes
     * from, except MODULE (the root) and UNREACHABLE_STATEMENT (the lowering of unreachable!()). The
     * parallel kinds come last, so no earlier value moves: FORK_JOIN_BLOCK (one BLOCK child; every worker
     * started inside it ends before the node ends), PARTITION_STATEMENT (the text is the part name, the
     * flag MUTABLE is kept; children are the value, the part count and the BLOCK body) and WITH_STATEMENT
     * (the text is the bound name; children are the value and the BLOCK body). A spawn is no kind: it is a
     * UNARY_EXPRESSION with the operator KEYWORD_SPAWN. FOR_EACH_STATEMENT comes after them (the text is the
     * name of the value variable, the flag MUTABLE is kept when the loop is written with mutable; children are
     * the optional key as an IDENTIFIER, the collection and the BLOCK body, so a loop with a key has three
     * children and one without has two). TYPE_NAME and BUILTIN_NAME come last: a TYPE_NAME is a written type that
     * is an operand (the text is the name, kept whole when dotted; the children are its type arguments, each a
     * TYPE_NAME or an INTEGER_LITERAL), and a BUILTIN_NAME is the callee of a built-in call with explicit type
     * arguments (the text is the name; the children are the type arguments, each a TYPE_NAME or an
     * INTEGER_LITERAL). CHARACTER_LITERAL is appended after them so no value moves (the text is the literal as written, with its
     * single quotes). ZERO_VALUE is appended last of all: a leaf with no text, no type name and no children, the
     * explicit zero of the zero-default rule (a LOCAL_DECLARATION with no written initializer, or a struct
     * LITERAL_VALUE field the source left unnamed). The list ends with MAREA_IR_KIND_COUNT.
     */
    typedef enum MAREA_IrKind
    {
        MAREA_IR_KIND_MODULE,
        MAREA_IR_KIND_CONSTANT_DECLARATION,
        MAREA_IR_KIND_STRUCTURE_DECLARATION,
        MAREA_IR_KIND_FIELD_DECLARATION,
        MAREA_IR_KIND_ENUM_DECLARATION,
        MAREA_IR_KIND_VARIANT_DECLARATION,
        MAREA_IR_KIND_VARIANT_FIELD,
        MAREA_IR_KIND_FUNCTION_DECLARATION,
        MAREA_IR_KIND_PARAMETER_LIST,
        MAREA_IR_KIND_PARAMETER,
        MAREA_IR_KIND_BLOCK,
        MAREA_IR_KIND_LOCAL_DECLARATION,
        MAREA_IR_KIND_ASSIGNMENT,
        MAREA_IR_KIND_EXPRESSION_STATEMENT,
        MAREA_IR_KIND_IF_STATEMENT,
        MAREA_IR_KIND_ELSEIF_CLAUSE,
        MAREA_IR_KIND_ELSE_CLAUSE,
        MAREA_IR_KIND_WHILE_STATEMENT,
        MAREA_IR_KIND_RETURN_STATEMENT,
        MAREA_IR_KIND_BREAK_STATEMENT,
        MAREA_IR_KIND_CONTINUE_STATEMENT,
        MAREA_IR_KIND_SCOPE_BLOCK,
        MAREA_IR_KIND_IDENTIFIER,
        MAREA_IR_KIND_INTEGER_LITERAL,
        MAREA_IR_KIND_FLOAT_LITERAL,
        MAREA_IR_KIND_STRING_LITERAL,
        MAREA_IR_KIND_BOOLEAN_LITERAL,
        MAREA_IR_KIND_BINARY_EXPRESSION,
        MAREA_IR_KIND_UNARY_EXPRESSION,
        MAREA_IR_KIND_MEMBER_ACCESS,
        MAREA_IR_KIND_INDEX_ACCESS,
        MAREA_IR_KIND_CALL_EXPRESSION,
        MAREA_IR_KIND_ARGUMENT_LIST,
        MAREA_IR_KIND_LITERAL_VALUE,
        MAREA_IR_KIND_FIELD_ENTRY,
        MAREA_IR_KIND_INDEX_ENTRY,
        MAREA_IR_KIND_VALUE_ENTRY,
        MAREA_IR_KIND_QUALIFIED_NAME,
        MAREA_IR_KIND_UNREACHABLE_STATEMENT,
        MAREA_IR_KIND_FORK_JOIN_BLOCK,
        MAREA_IR_KIND_PARTITION_STATEMENT,
        MAREA_IR_KIND_WITH_STATEMENT,
        MAREA_IR_KIND_FOR_EACH_STATEMENT,
        MAREA_IR_KIND_TYPE_NAME,
        MAREA_IR_KIND_BUILTIN_NAME,
        MAREA_IR_KIND_CHARACTER_LITERAL,
        MAREA_IR_KIND_ZERO_VALUE,

        MAREA_IR_KIND_COUNT
    } MAREA_IrKind;

    /**
     * @brief Returns the stable upper-case name of an IR kind, for example "MODULE".
     * @param kind Kind to name.
     * @return The name, or "UNKNOWN" when kind is out of range.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_Character8 *MAREA_CALL MAREA_IrKindGetName(MAREA_IrKind kind);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_IR_MAREAIRKIND_H */
