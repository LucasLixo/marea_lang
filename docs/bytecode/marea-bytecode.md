# Marea bytecode stage

## Contract

The `bytecode` stage holds the data model of the register bytecode and its two serializable forms, plus the one
function that lowers an IR module into it. A `MAREA_BytecodeModule` has a string pool, a type table, a function
table with the code of each function, and a read-only data section. The stage writes a module to a canonical binary
form and to a canonical text form, and reads both back. It has no global state, never prints and never reads the
disk: the caller gives it an arena (which owns every module and every output buffer) and a diagnostic sink in a
`MAREA_BytecodeContext`. A malformed binary or text is a diagnostic, never a crash and never an error code; a
construct `MAREA_BytecodeGenerate` cannot lower yet is the same, never an exception. The stage links `marea::ir`
(for `MAREA_BytecodeGenerate` only; the binary and text forms need nothing past `marea::base`). `vm`, the next
stage, reads a `MAREA_BytecodeModule` only: it never reads the IR. The CMake target is `Marea-Bytecode` (alias
`marea::bytecode`, plus a shared target `Marea-Bytecode-Shared` when `MAREA_OPTION_SHARED_LIBS` is on), built when
`MAREA_OPTION_TARGET_BYTECODE` is on. The option is on by default and depends on `MAREA_OPTION_TARGET_IR`.

Three laws hold for every module that passes the checks of the readers, and the tests run them over 200 generated
modules each: `Deserialize(Serialize(m))` equals `m`, `Assemble(Disassemble(m))` equals `m`, and the text to binary to
text path is stable (`Serialize(Assemble(Disassemble(m)))` is byte-identical to `Serialize(m)`).

## Use

```c
MAREA_BytecodeContext *context;
MAREA_BytecodeModule *module;
MAREA_BytecodeTypeInfo type = { .kind = MAREA_BYTECODE_TYPE_KIND_PRIMITIVE, .name = MAREA_BYTECODE_NONE,
                                .element = MAREA_BYTECODE_NONE, .size = 4, .alignment = 4 };
MAREA_Uinteger32 index;

MAREA_BytecodeContextCreate(arena, sink, &context);
MAREA_BytecodeModuleCreate(context, &module);
MAREA_BytecodeModuleAddType(module, &type, &index);

const MAREA_Byte *bytes;
MAREA_IntegerSize size;
MAREA_BytecodeSerialize(module, &bytes, &size);                 /* canonical binary, in the arena */

MAREA_BytecodeModule *back;
MAREA_BytecodeDeserialize(context, bytes, size, &back);         /* back is null on a malformed input */

const MAREA_Character8 *text;
MAREA_IntegerSize length;
MAREA_BytecodeDisassemble(module, &text, &length);              /* canonical text, with a NUL after it */
MAREA_BytecodeAssemble(context, text, length, &back);
```

Every builder call (`AddString`, `AddType`, `AddMember`, `AddFunction`, `AddParameter`, `AddInstruction`, `AddData`)
returns `MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER` for a null argument, `MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE`
for a type or function index that does not exist, and `MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY` when the arena
cannot grow (a request larger than the address space is the same result, never a crash). Every `Get` call reports a
null argument or an index past the count the same way. Every count accessor on a null module returns 0.
`MAREA_BytecodeModuleEquals` is true only for two non-null modules with the same content in the same order.

## Module model

- **Strings.** A byte string, any byte allowed, with a NUL after the last byte that the length does not count.
  `MAREA_BytecodeModuleAddString` deduplicates: the same bytes give the same index. The readers do not, so a pool
  with an equal string twice is read and written as it is.
- **Types.** One entry per type: kind (`primitive`, `structure`, `enum`, `array`, `vector`, `pointer`, `owned`,
  `slice`, `task`, `function`), a name (a string index or `MAREA_BYTECODE_NONE`), the size and the alignment in bytes,
  an element type (an index or `MAREA_BYTECODE_NONE`), a count (the length of an array) and a member list. A member
  has a name, a type and a value: the byte offset of a field of a structure, or the value of a variant of an enum. A
  type index is its position in the table. Index 0 has no special meaning yet.
- **Functions.** A name, a result type (an index or `MAREA_BYTECODE_NONE`), the register count, the size of the frame
  area, a flag word, a parameter list (a type and a flag word each) and the code of the function.
- **Code.** One instruction is 64 bits: `opcode`, `a`, `b`, `c`, each 16 bits. The table of opcodes (46 values,
  `MAREA_BytecodeOpcode`) covers the scalar slices (move and constant loading, integer and float arithmetic with a
  `MAREA_BytecodeNumberKind` operand, the saturating family, bitwise and shift, convert, every comparison, jump and
  the two conditional jumps, call and return) and the composite family this slice adds (`zero`, `addr_of`, `field`,
  `load`, `store`, `copy_bytes`, `load_data_block`, see "Composite values" below), plus one `trap` opcode whose
  operand is a `MAREA_BytecodeTrapCode` (division by zero, a shift out of range, a negative exponent, a convert
  out of range, `unreachable!()`, a missing return, a stack overflow, an index out of range). A later slice (host
  calls, `fork_join`) adds an opcode as one more value and one more row of the name table of
  `MAREABytecodeOpcode.c`. `MAREABytecodeOpcode.h` documents the operand meaning of every opcode in full.
- **Data.** A list of read-only blobs, such as the bytes of an embedded file. A blob may be empty, and two equal blobs
  keep two indexes.
- **Open structures.** `MAREA_BytecodeTypeInfo`, `MAREA_BytecodeMemberInfo`, `MAREA_BytecodeFunctionInfo`,
  `MAREA_BytecodeParameterInfo` and `MAREA_BytecodeInstruction` are open to the public because they hold only numbers,
  so the caller fills one to add an entry and reads one back with no accessor for each field. A test checks the size
  and the offset of every field. `memberCount`, `parameterCount` and `instructionCount` are read-only fields: an add
  call ignores them.
- **References are checked late.** An add call checks no string, type or opcode reference. `Deserialize` and
  `Assemble` check the whole module after they read it: a string index in a type name, a member name, a function name;
  a type index in an element, a member, a result and a parameter; a type kind in the table; an opcode in the table.
  A module that the builder made with a bad reference serializes and disassembles as it is, and the reader rejects it.

## Binary format

All integers are little endian with a fixed width. There is no padding between fields and no alignment of a section:
a section starts where the one before it ends. The format version is 0.1 (`MAREA_BYTECODE_FORMAT_VERSION_MAJOR` and
`MAREA_BYTECODE_FORMAT_VERSION_MINOR`). A reader rejects another major version and a higher minor version.

```
magic          4 bytes   'M' 'B' 'C' 0
version        u16 major, u16 minor
flags          u32       0; any other bit is rejected
section_count  u32       5
section table  5 x { id u32, offset u64, size u64 }
sections       in table order
```

The table lists exactly five sections, with these ids, in this order, and no gap: the offset of a section is the
end of the one before it, the first one starts right after the table, and the last one ends at the end of the input.
Section id 3 and ids above 6 are not used yet (ids 3 (constants), 7 (exports), 8 (interface) and 9 (debug) of the
design are not in this version).

| Id | Section | Content |
|---|---|---|
| 1 | strings | `u32` count, then for each string `u32` length and the bytes |
| 2 | types | `u32` count, then for each type: `u8` kind, `u32` name, `u32` alignment, `u32` element, `u64` size, `u64` count, `u32` member count, and for each member `u32` name, `u32` type, `u64` value |
| 4 | functions | `u32` count, then for each function: `u32` name, `u32` result, `u32` register count, `u32` flags, `u64` frame area, `u32` parameter count, each parameter as `u32` type and `u32` flags, then `u32` code offset and `u32` code length (both in instructions) |
| 5 | code | the instructions of every function, in function order, each as four `u16` |
| 6 | data | `u32` count, then for each blob `u64` size and the bytes |

An absent name, element or result is `0xFFFFFFFF`. The code offset of a function is the sum of the code lengths of the
functions before it, so the code section is covered with no gap and no overlap; the code section size is a multiple
of 8. A count is checked against the bytes left in its section before anything is allocated, so a huge count in a
small input gives a diagnostic and not a huge allocation. A section must be used up exactly: bytes left after the last
item are a diagnostic. The same module always gives the same bytes: no timestamp, no host path and no pointer value is
written.

The section ids 1, 2, 4 and 5 follow the design table of the stage. The data section has id 6, the id that the design
gives to the module-level constants with their bytes, and it holds the read-only blobs.

## Text form

Line oriented, ASCII, one canonical spelling for each module. A line feed ends a line, a carriage return counts as a
blank, and the last line needs no line feed. The reader skips blank lines and comments (`;` to the end of the line,
except inside a string), and any blank or tab in front of or between the words. The writer writes no comment, no blank
line and no space other than one between two words and two in front of a member, a parameter or an instruction.

```
.marea-bytecode 0.1
.strings
s0 "Point"
s1 "q\"\\\x0a\xff"
.types
t0 primitive name none size 4 align 4 element none count 0
t1 structure name s0 size 4 align 4 element none count 0
  member name s1 type t0 value 0
.functions
f0 name s1 result t0 registers 3 area 16 flags 1
  param type t1 flags 1
  ins nop 1 2 3
  ins return_void 0 0 0
.data
d0 00ff10
d1 -
```

- The first line is the header with the version, then the four section lines `.strings`, `.types`, `.functions` and
  `.data`, each exactly once and in that order, even when a section has no item.
- The items of a section are numbered from 0 with the letter of the section (`s`, `t`, `f`, `d`), and the number must
  be the next one. A reference is a letter and a number (`s1`, `t0`), or `none` where a name, an element or a result
  may be absent.
- A string is in double quotes with printable ASCII only. `\"` and `\\` are escapes, and `\xNN` (two hex digits, either
  case on input and lower case on output) writes any other byte.
- A type line has the fields `name`, `size`, `align`, `element` and `count` in that order. The kind is one of the ten
  words of the type table. A `member` line belongs to the last type, a `param` line and an `ins` line to the last
  function. Numbers are decimal digits with no sign: 16 bits for an instruction operand, 32 bits for `align`,
  `registers`, `flags` and a parameter `flags`, 64 bits for the others.
- A blob is its bytes as hex pairs, or `-` when it is empty.

## Lowering from the IR

`MAREA_BytecodeGenerate(context, irModule, &outModule)` reads one `MAREA_IR_KIND_MODULE` node and writes one
`MAREA_BytecodeModule`, or leaves `outModule` null with a diagnostic for the first construct it cannot lower.
Covers the scalar slices (integer and float expressions, locals, `return`, `if`, `while`, `switch`, `for` range,
calls with scalar parameters, a borrow of a primitive, which the language already passes by value) and, this
slice, a `structure` local with no `construct` or `destroy` operator (see "Composite values" below): `array`,
`enum` with data, `string`, a lifecycle call to a user `construct` or `destroy`, a `structure` as a parameter, a
call argument or a result, host call or `fork_join` is still `UNSUPPORTED_TYPE` or `UNSUPPORTED_CONSTRUCT`, left
to a later slice.

- **Registers.** A function keeps a flat list of virtual registers, numbered from 0, freed back to the mark of
  their own block when it ends (so two sibling blocks reuse the same numbers) and never reused while still read.
  A parameter takes the first registers, in order. `MAREA_BytecodeFunctionInfo.registerCount` is the widest point
  the function reached, which may be higher than the number any single statement still holds.
- **Scalar-only slice.** Only a primitive number kind and `boolean` lower; every other type gives
  `UNSUPPORTED_TYPE` (5033) for a parameter, a local or a result. `for each` always gives `UNWALKABLE_COLLECTION`
  (5034) in this slice, because no collection type is a bytecode value yet (B2 of the plan): the counted walk B3
  describes needs an aggregate the vm cannot represent before the composites slice.
  `do ... while`, `switch` and a `for` range desugar in the IR itself (see `docs/ir/marea-ir.md`) into `while`,
  `if` and a hidden local and lower with no code of their own: lowering the one general `while`, `if`,
  `BINARY_EXPRESSION` and `LOCAL_DECLARATION` already covers all three.
- **Constants.** A module-level `CONSTANT_DECLARATION` is inlined at every read, not kept as a bytecode register
  or a module constant; a constant that reads another constant is followed up to 32 levels deep. `build_value!`
  lowers to its default, because this stage has no build settings.
- **Calls.** A call whose callee is the bare name of a function of the same unit (the IR's own rule, see
  "Call-default filling" of `docs/ir/marea-ir.md`) is found in a function table the generator fills before any
  body is lowered, so a function may call one declared after it, or itself. The call opcode's window is one
  result register plus one register per argument, right above the registers already in use; the callee's own
  parameters land in that window. A callee outside the unit, through a member access, or with the wrong
  argument count (a gap of the IR's own default filling) is `UNRESOLVED_NAME` or `UNSUPPORTED_CONSTRUCT`.
  `convert<T>(x)` is the one built-in call this slice lowers, to the `convert` opcode.
- **Composite values.** A local whose resolved type names a `structure` of the same unit, with every field
  measured by sema's layout (`MAREA_IrLayout`, read through `MAREA_IrModuleFindLayout`), lowers to the type table
  (one `MAREA_BYTECODE_TYPE_KIND_STRUCTURE` row, cached by name, with one member per field carrying its own type
  and its byte offset from the layout) and to a value in the function's own frame memory area
  (`MAREA_BytecodeFunctionInfo.frameArea`, reserved with `MAREA_BytecodeGeneratorAllocateFrame`, a bump allocator
  at `MAREA_BYTECODE_GENERATOR_FRAME_LIMIT` bytes). The local's register holds the address `addr_of` gave it, not
  the bytes: `field` adds a constant byte offset to an address, and `load`/`store` move one scalar through it
  (`MAREA_BYTECODE_NUMBER_KIND_UINTEGER_8` for a `boolean`). A plain `LITERAL_VALUE` (a type with no `construct`)
  lowers to `zero` over the whole size, the language's zero-default rule, then one `field` and `load`/`store` or
  recursive `MAREA_BytecodeGeneratorConstructAggregate` call per `FIELD_ENTRY` (a nested structure field is built
  in the same pass). `copy name` and `move name` both lower to `copy_bytes` from the named local's address;
  `move` also `zero`s the source's bytes afterward, the same instruction the zero rule uses. A `MEMBER_ACCESS`
  chain (`a.b.c`) reads a field, following a nested structure's address through as many `field` steps as the
  chain needs. `load_data_block` copies a read-only blob into a reserved range of the frame area, for a future
  `include_bytes!`. None of this crosses a call: a `structure` parameter, call argument or result, a
  `CONSTRUCT_CALL` or `DESTROY_CALL` to a user `construct` or `destroy` (which would need the address of the
  value as a hidden argument), an `enum` with data, an `array` or a `tuple` are a later slice (see Gaps); the vm's
  own mechanics for all of them (the frame memory area, the bounds check an index needs, a tagged union's tag and
  payload) are already proven directly against hand-built modules in `tests/source/vm/MAREAVm.cpp`, ahead of the
  generator wiring.

## Diagnostics

Every diagnostic is an error. A binary error has the byte offset in `position.offset` and line 0. A text error has the
line and the column from 1, and the offset; an error that belongs to the whole input (an empty input, a missing section
at the end, a reference found by the final check) has line 0. A generate diagnostic (5030 and above) gives the source
line, column and offset of the IR node it was raised at.

| Code | Name | Example |
|---|---|---|
| 5001 | `BAD_MAGIC` | the first four bytes are not `MBC` and a zero byte |
| 5002 | `UNSUPPORTED_VERSION` | major version other than 0, or a minor version above 1, in the binary or in the header line |
| 5003 | `TRUNCATED` | an input shorter than the header, than the section table, or than the end of a section |
| 5004 | `BAD_SECTION_TABLE` | a section count other than 5, a wrong id, an offset that is not the end of the section before, a size that overflows, a byte after the last section |
| 5005 | `BAD_REFERENCE` | a string index, a type index, an element, a member type or a parameter type past its table |
| 5006 | `UNKNOWN_OPCODE` | an opcode that is not in the table, in the binary, or an unknown name after `ins` |
| 5007 | `MALFORMED_SECTION` | a count or a length larger than the bytes left in the section, bytes left after the last item, a code range that does not follow the one before it, a type kind above 9 |
| 5008 | `BAD_FLAGS` | a flag bit set in the header |
| 5020 | `TEXT_SYNTAX` | no header line, a missing or extra word, an unknown type kind, a `member`, `param` or `ins` line in the wrong place or with no owner, an odd or non-hex blob |
| 5021 | `TEXT_BAD_SECTION` | an unknown, repeated, out of order or missing section, an item before the first section |
| 5022 | `TEXT_BAD_INDEX` | an item that does not carry the next number, or a reference with the wrong letter |
| 5023 | `TEXT_BAD_NUMBER` | a number with a non-digit, with no digit, or above the limit of its field |
| 5024 | `TEXT_BAD_STRING` | a string with no opening or closing quote, an unknown escape, a character that is not printable ASCII |
| 5030 | `UNSUPPORTED_CONSTRUCT` | `fork_join`, a `CONSTRUCT_CALL` or a `DESTROY_CALL` (a user `construct` or `destroy`), a structure literal that holds something that is not a named field, a call with the wrong argument count, a call this slice cannot resolve to a function of the unit |
| 5031 | `UNRESOLVED_NAME` | a name that is not a local, a parameter, a module constant or a function of the unit; a structure field name a `MEMBER_ACCESS` or a `FIELD_ENTRY` does not declare |
| 5032 | `OPERAND_OVERFLOW` | a function needs more than 65535 registers, a jump farther than 32767 instructions, a module needs more than 65535 constants, the frame area needs more bytes than this slice allows |
| 5033 | `UNSUPPORTED_TYPE` | a parameter, a call argument or a result of a `structure`; a `string`, an `enum` with data, an `array` or a `tuple` anywhere; a structure field of such a type; a `MEMBER_ACCESS` of a nested structure (reads only a scalar field) |
| 5034 | `UNWALKABLE_COLLECTION` | a `for each`, because no collection type is a bytecode value in this slice |

The range 5000 to 5999 is reserved for this stage. The messages are fixed texts and do not name the value.

## Gaps

- A `structure` with a user `construct` or `destroy` operator (`CONSTRUCT_CALL`, `DESTROY_CALL`) is not lowered:
  calling it needs the address of the value as a hidden receiver argument, and no call in this slice passes a
  structure. `tests/language/accept/memory/L-MEMORY-010.marea` through `-015.marea` already exercise the sema and
  ir side of this (each with an `-- output:` comment that names the expected construct and destroy order) and
  still only reach `UNSUPPORTED_CONSTRUCT` here; wiring them is the next composites apply's first item.
- A `structure` as a parameter, a call argument or a function result is `UNSUPPORTED_TYPE`: passing or returning
  one needs the same hidden-address convention as a lifecycle call. A local-only structure (built, read, copied
  and moved inside one function) is this slice's whole scope.
- An `enum` with data (a tagged union payload), `array`, `vector`, `tuple` and `string` are not lowered from the
  IR: no accepted program can write `array<T, N>` or `tuple<T, U>` yet (no shipped library registers them, wave
  5), and no accepted program constructs a data-carrying enum variant (`Shape.Circle(2.0)`) either, so the
  generator gives `UNSUPPORTED_TYPE` for all four. The vm's own mechanics for a tagged union (the tag at offset
  0, a payload at a fixed offset, a dispatch on the tag) and for a bounds-checked, compiler-known array are
  proven directly against hand-built modules in `tests/source/vm/MAREAVm.cpp` (`EnumTaggedUnionConstructReadAndSwitchDispatch`,
  `IndexOutOfRangeTrapsOnABoundsCheckedArray`), standing in for the fixture descriptors the apply's own plan names,
  ahead of the generator wiring a later apply adds once the IR shape of a variant call is settled.
- `for each` always gives `UNWALKABLE_COLLECTION`, even over a type sema can type the value of (an `array`, a
  `vector`, a `slice`): no such type is a bytecode value yet, so there is nothing to build a counted loop from.
  B3 of the plan (the counted loop the bytecode builds itself) waits on a later slice.
- `owned<T>` storage and a `convert` result that exposes padding are not zero-filled: the zero-default rule's
  other two creation paths (`docs/ir/marea-ir.md`, "Zero-default creation"), neither reachable by an accepted
  program yet.
- The frame memory area is a fixed-capacity bump allocator (`MAREA_BYTECODE_GENERATOR_FRAME_LIMIT`, 4096 bytes),
  matched by the vm's own `MAREA_VM_FRAME_MEMORY_LIMIT`: a function whose locals need more is `OPERAND_OVERFLOW`,
  a slice limit, not a language rule. A structure's own byte layout (field order, offset, size, alignment) comes
  straight from sema's layout table with no re-derivation here, so a future `cgen` backend must compute the same
  layout for the two backends to agree (the parity goal of the design).
- A call outside the unit (through an import, an alias, or a member access) is not resolved: the generator's
  function table holds only the functions the IR module itself declares. `UNRESOLVED_NAME` covers it, the same
  code a plain undeclared name gives; the two are not told apart, because no accepted program reaches the first
  one yet (sema already rejects an undeclared callee before the IR is built).
- The binary has no constants section, no exports, no interface hash and no debug section, so the module cannot
  state a C module API yet (load, unload, interface hash, find by name). The design numbers sections 3, 7, 8 and 9
  for these; they are unused in version 0.1.
- No type index is reserved for `void`, and the module does not check that a structure's size and member offsets agree,
  or that an array's count and element give its size. The layout comes from sema later.
- A register number, a frame area and a code offset in a function are not checked against each other at build
  time by the binary and text readers; the `vm` stage bounds every register and jump target of its own accord
  at run time instead, so a malformed module still cannot read or write past a frame.
- The byte order is little endian on every host: the writer and the reader compose the bytes one by one, so no host
  byte order is assumed. The build and the tests ran on Linux only.
- A module is limited by the format to 2^32 - 1 strings, types, functions, blobs and instructions, and the writer
  returns `MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY` above that. This limit is not tested, because no test can build a
  module that large.
- The string pool lookup in `AddString` is linear, so building a pool of n strings costs n squared comparisons.
