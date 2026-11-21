# Marea Lang

A statically-typed, high-performance scripting language for general use with C. It has fixed-width
types that are checked once and erased at compile time, and scoped memory with static ownership rules, so there is no
garbage collector and no lifetime annotation. One IR compiles to bytecode for a small reference interpreter or to C11
for maximum speed, and both give the same behavior. The compiler is modular, one C11 library per stage, so a tool can
use only the lexer, the parser, the type checker or the interpreter. One C macro registers a function for the language
and a host can replace library functions from C.
