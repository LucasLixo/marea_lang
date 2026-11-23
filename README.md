# Marea Lang

A statically-typed, high-performance scripting language for general use together with C11. Every type has a fixed
width and a full name, checked once at compile time and fully erased before code generation. Memory is scoped to
regions with static ownership rules, so there is no garbage collector and no lifetime annotation to write.

One intermediate representation compiles to bytecode for a small reference interpreter, portable for embedding, or
to C11 for maximum performance, and both give the same observable behavior. The compiler is modular, one C11 library
per stage, so a tool can use only the lexer, the parser, the type checker or the interpreter. There is no standard
library: every type beyond the language's own primitives is a library, written in C11 or in the language itself, and
a host can replace any library or runtime function from C11.
