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
     * single quotes). The value after CHARACTER_LITERAL is retired: it was ZERO_VALUE, a kind that no accepted program
     * could reach once sema rejected a local with no initializer and a struct literal that omits a field. It stays
     * unused, so that no kind that follows it changes its value, and MAREA_IrKindGetName names it "UNKNOWN".
     * CONSTRUCT_CALL and DESTROY_CALL follow it, so no value moves: CONSTRUCT_CALL is the explicit lowering of Type(args) (the type name is the constructed
     * type, the one child is an ARGUMENT_LIST, filled with construct's own defaults as an ordinary call fills
     * them; a Type{...} literal of a type with construct has a second child, the LITERAL_VALUE that is its initial value);
     * DESTROY_CALL is the explicit cleanup the ir inserts at a scope exit of a destructible local (the
     * text is the qualified destroy function, "Type.name", and the one child is an IDENTIFIER naming the local
     * being destroyed). BACKEND_ATTRIBUTE follows them: the resolved backend of a function (see the sema doc, "Backend"), as the last
     * child of its FUNCTION_DECLARATION, after the body (the text is the member: scalar, vector_extension or intrinsics, never
     * auto), so the first two children of a function keep their places. A function with no backend has no such child. MACRO_CALL comes last: a compile-time macro that the
     * IR keeps for a later stage (build_value! and the macro that a shipped library registers). The text is the name, with the
     * alias of a namespace replaced by the namespace (marea.array.include_bytes), and the one child is the ARGUMENT_LIST of the
     * lowered arguments, in the order they were written. It carries the resolved type of the call. INCLUDED_BYTES is the value of a macro that gives the bytes of a file, when the
     * include resolver gave them: the text holds two lowercase hexadecimal digits for each byte, the one child is a STRING_LITERAL
     * whose text is the path of the file as the resolver gave it, and the resolved type is the array of uinteger8 that holds
     * them. The list ends with MAREA_IR_KIND_COUNT.
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
        MAREA_IR_KIND_CONSTRUCT_CALL = (MAREA_IR_KIND_CHARACTER_LITERAL + 2),
        MAREA_IR_KIND_DESTROY_CALL,
        MAREA_IR_KIND_BACKEND_ATTRIBUTE,
        MAREA_IR_KIND_MACRO_CALL,
        MAREA_IR_KIND_INCLUDED_BYTES,

        MAREA_IR_KIND_COUNT
    } MAREA_IrKind;

    /**
     * @brief Returns the stable upper-case name of an IR kind, for example "MODULE".
     * @param kind Kind to name.
     * @return The name, or "UNKNOWN" when kind is out of range or is the retired value after CHARACTER_LITERAL.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_Character8 *MAREA_CALL MAREA_IrKindGetName(MAREA_IrKind kind);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_IR_MAREAIRKIND_H */
