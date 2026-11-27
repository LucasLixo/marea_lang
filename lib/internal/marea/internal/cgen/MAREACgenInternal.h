// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INTERNAL_CGEN_MAREACGENINTERNAL_H)
    #define MAREA_INTERNAL_CGEN_MAREACGENINTERNAL_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREAArena.h"
    #include "marea/base/MAREADiagnostic.h"
    #include "marea/base/MAREATypeRegistry.h"
    #include "marea/cgen/MAREACgen.h"
    #include "marea/ir/MAREAIrKind.h"
    #include "marea/ir/MAREAIrLayout.h"
    #include "marea/ir/MAREAIrNode.h"
    #include "marea/lex/MAREATokenKind.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Passes the failure of an expression that returns a MAREA_CgenExcept up to the caller.
     */
    #if !defined(MAREA_CGEN_TRY)
        #define MAREA_CGEN_TRY(expression)                              \
            do {                                                        \
                const MAREA_CgenExcept marea_try_except = (expression); \
                if (marea_try_except != MAREA_CGEN_EXCEPT_SUCCESS) {    \
                    return marea_try_except;                            \
                }                                                       \
            } while (0)
    #endif

    /**
     * @brief Joins C strings into one NUL terminated string of the arena of the generator, for example
     * MAREA_CGEN_JOIN(generator, "f_", name). A failed allocation gives an empty string and marks the generator.
     */
    #if !defined(MAREA_CGEN_JOIN)
        #define MAREA_CGEN_JOIN(generator, ...) MAREA_CgenJoin((generator), (const MAREA_Character8 *const[]){ __VA_ARGS__ }, (sizeof((const MAREA_Character8 *const[]){ __VA_ARGS__ }) / sizeof(const MAREA_Character8 *)))
    #endif

    /**
     * @brief The first types of the type table: a scalar type has its own number, which is also the id of its type.
     * The integer and float kinds share the order MAREA_BytecodeGeneratorKind already uses, so the two stages read the
     * same written type the same way. Every other type (a structure, an enum, an array) is an id from
     * MAREA_CGEN_TYPE_FIRST_COMPOSITE up, in the order cgen meets it.
     */
    typedef enum MAREA_CgenKind
    {
        MAREA_CGEN_KIND_SINTEGER_8 = MAREA_UINTEGER_8(0),
        MAREA_CGEN_KIND_SINTEGER_16 = MAREA_UINTEGER_8(1),
        MAREA_CGEN_KIND_SINTEGER_32 = MAREA_UINTEGER_8(2),
        MAREA_CGEN_KIND_SINTEGER_64 = MAREA_UINTEGER_8(3),
        MAREA_CGEN_KIND_UINTEGER_8 = MAREA_UINTEGER_8(4),
        MAREA_CGEN_KIND_UINTEGER_16 = MAREA_UINTEGER_8(5),
        MAREA_CGEN_KIND_UINTEGER_32 = MAREA_UINTEGER_8(6),
        MAREA_CGEN_KIND_UINTEGER_64 = MAREA_UINTEGER_8(7),
        MAREA_CGEN_KIND_FLOAT_32 = MAREA_UINTEGER_8(8),
        MAREA_CGEN_KIND_FLOAT_64 = MAREA_UINTEGER_8(9),
        MAREA_CGEN_KIND_NUMBER_COUNT = MAREA_UINTEGER_8(10),
        MAREA_CGEN_KIND_BOOLEAN = MAREA_UINTEGER_8(10),
        MAREA_CGEN_KIND_VOID = MAREA_UINTEGER_8(11),
        MAREA_CGEN_KIND_NONE = MAREA_UINTEGER_8(12),
        MAREA_CGEN_TYPE_FIRST_COMPOSITE = MAREA_UINTEGER_8(13),
    } MAREA_CgenKind;

    /**
     * @brief What a type is.
     */
    typedef enum MAREA_CgenCategory
    {
        MAREA_CGEN_CATEGORY_NONE = MAREA_UINTEGER_8(0),
        MAREA_CGEN_CATEGORY_SCALAR = MAREA_UINTEGER_8(1),
        MAREA_CGEN_CATEGORY_VOID = MAREA_UINTEGER_8(2),
        MAREA_CGEN_CATEGORY_STRUCTURE = MAREA_UINTEGER_8(3),
        MAREA_CGEN_CATEGORY_ENUM = MAREA_UINTEGER_8(4),
        MAREA_CGEN_CATEGORY_ARRAY = MAREA_UINTEGER_8(5),
        MAREA_CGEN_CATEGORY_VECTOR = MAREA_UINTEGER_8(6),
    } MAREA_CgenCategory;

    /**
     * @brief How a parameter takes its argument: a copy of the value, or the address of the value (a borrow).
     */
    typedef enum MAREA_CgenPassing
    {
        MAREA_CGEN_PASSING_VALUE = MAREA_UINTEGER_8(0),
        MAREA_CGEN_PASSING_BORROW = MAREA_UINTEGER_8(1),
        MAREA_CGEN_PASSING_MUTABLE_BORROW = MAREA_UINTEGER_8(2),
    } MAREA_CgenPassing;

    /**
     * @brief Growable text buffer, the arena-backed form every writer in this stage appends to. The same shape
     * the ir dump already uses, kept private to this stage.
     */
    typedef struct MAREA_CgenBuffer
    {
        MAREA_Arena *arena;
        MAREA_Character8 *data;
        MAREA_IntegerSize length;
        MAREA_IntegerSize capacity;
        MAREA_Boolean failed;
    } MAREA_CgenBuffer;

    /**
     * @brief One alias that a written type of the module shows: a spelling that sema kept and the full path that the
     * ir gave for it. The spelling of an element type of an array is read through these.
     */
    typedef struct MAREA_CgenAlias
    {
        const MAREA_Character8 *from;
        const MAREA_Character8 *to;
        MAREA_IntegerSize fromLength;
        MAREA_IntegerSize toLength;
    } MAREA_CgenAlias;

    /**
     * @brief One field of a structure or of a variant of an enum: its name, its type and its byte offset from the
     * start of the value, as the layout table of the module gives it.
     */
    typedef struct MAREA_CgenField
    {
        MAREA_Uinteger64 offset;
        const MAREA_Character8 *name;
        const MAREA_IrNode *declaration;
        MAREA_IntegerSize nameLength;
        MAREA_Uinteger32 type;
        MAREA_Uinteger8 padding[4];
    } MAREA_CgenField;

    /**
     * @brief One variant of an enum: its name and its fields. Its number is its position in the enum.
     */
    typedef struct MAREA_CgenVariant
    {
        const MAREA_Character8 *name;
        MAREA_CgenField *fields;
        MAREA_IntegerSize nameLength;
        MAREA_IntegerSize fieldCount;
    } MAREA_CgenVariant;

    /**
     * @brief One composite type of the type table. memberCount counts the fields of a structure or the variants of an
     * enum, element is the element of an array or of a vector or the tag of an enum, count the length of an array. cName is the C
     * name of the type. wrapped is set for an enum written as a C structure (it has data or padding), packed for a
     * type whose members can be misaligned. A type that failed has no definition and its users report it.
     */
    typedef struct MAREA_CgenType
    {
        MAREA_Uinteger64 size;
        MAREA_Uinteger64 alignment;
        MAREA_Uinteger64 count;
        const MAREA_Character8 *name;
        const MAREA_Character8 *cName;
        MAREA_CgenField *fields;
        MAREA_CgenVariant *variants;
        MAREA_IntegerSize nameLength;
        MAREA_IntegerSize memberCount;
        MAREA_Uinteger32 element;
        MAREA_Uinteger8 category;
        MAREA_Boolean wrapped;
        MAREA_Boolean packed;
        MAREA_Boolean failed;
        MAREA_Boolean done;
        // INFO: a structure that is a tuple: its fields are named by their places, and its type name is empty.
        MAREA_Boolean isTuple;
        MAREA_Uinteger8 padding[2];
    } MAREA_CgenType;

    /**
     * @brief One parameter of a function of the unit.
     */
    typedef struct MAREA_CgenParameter
    {
        const MAREA_Character8 *cName;
        const MAREA_IrNode *node;
        MAREA_Uinteger32 type;
        MAREA_Uinteger8 passing;
        MAREA_Uinteger8 padding[3];
    } MAREA_CgenParameter;

    /**
     * @brief One function of the ir module, known before any body is written, so a call expression finds its C name,
     * its parameters and its result without a second pass. ownerLength is the length of "Type" in a "Type.name"
     * function of an implementation block, 0 for a plain function. A function whose signature cgen cannot write is
     * not supported and a call of it fails. A function that failed has no definition; callees holds the functions it
     * calls, so a caller of a failed function fails too and the written program links.
     */
    typedef struct MAREA_CgenFunction
    {
        const MAREA_IrNode *node;
        const MAREA_Character8 *name;
        const MAREA_Character8 *cName;
        const MAREA_Character8 *body;
        MAREA_CgenParameter *parameters;
        MAREA_IntegerSize *callees;
        MAREA_IntegerSize nameLength;
        MAREA_IntegerSize ownerLength;
        MAREA_IntegerSize parameterCount;
        MAREA_IntegerSize calleeCount;
        MAREA_IntegerSize calleeCapacity;
        MAREA_Uinteger32 result;
        MAREA_Boolean supported;
        MAREA_Boolean failed;
        MAREA_Uinteger8 padding[2];
    } MAREA_CgenFunction;

    /**
     * @brief One name in scope while a function body is written: a parameter or a local, the C expression that names
     * its storage and its type. A stable symbol never changes while an expression is computed. An unaligned symbol
     * names storage inside a packed type. A symbol of type MAREA_CGEN_KIND_NONE is the part of a partition.
     */
    typedef struct MAREA_CgenSymbol
    {
        const MAREA_Character8 *name;
        const MAREA_Character8 *cText;
        MAREA_IntegerSize length;
        MAREA_Uinteger32 type;
        MAREA_Boolean stable;
        MAREA_Boolean unaligned;
        MAREA_Uinteger8 padding[2];
    } MAREA_CgenSymbol;

    /**
     * @brief The result of an expression: the C text that names it and its type. A stable value is a literal, a local
     * passed by value or a temporary that nothing else changes. A composite value is the C name of storage, and an
     * unaligned one is inside a packed type, so its address is never passed on.
     */
    typedef struct MAREA_CgenValue
    {
        const MAREA_Character8 *text;
        MAREA_Uinteger32 type;
        MAREA_Boolean stable;
        MAREA_Boolean unaligned;
        MAREA_Uinteger8 padding[2];
    } MAREA_CgenValue;

    /**
     * @brief One value that is destroyed when its block ends: the C text of its storage and its type.
     */
    typedef struct MAREA_CgenOwned
    {
        const MAREA_Character8 *text;
        MAREA_Uinteger32 type;
        MAREA_Uinteger8 padding[4];
    } MAREA_CgenOwned;

    /**
     * @brief The body of the context of MAREACgen.h: what the caller gave.
     */
    struct MAREA_CgenContext
    {
        MAREA_Arena *arena;
        MAREA_DiagnosticSink *sink;
        const MAREA_TypeRegistry *registry;
        MAREA_CgenHostResolveFunction resolve;
        MAREA_Null userData;
        const MAREA_Character8 **includes;
        const MAREA_Character8 *allocate;
        const MAREA_Character8 *release;
        MAREA_IntegerSize includeCount;
        MAREA_IntegerSize includeCapacity;
    };

    /**
     * @brief State shared by every writer of this stage while one module is generated.
     */
    typedef struct MAREA_CgenGenerator
    {
        MAREA_CgenContext *context;
        MAREA_Arena *arena;
        MAREA_DiagnosticSink *sink;
        const MAREA_IrNode *module;
        const MAREA_Character8 *namespaceText;
        MAREA_CgenBuffer *buffer;
        MAREA_CgenBuffer *types;
        // INFO: the names of the included files that are defined already.
        const MAREA_Character8 **included;
        MAREA_IntegerSize includedCount;
        MAREA_IntegerSize includedCapacity;
        MAREA_CgenType *typeTable;
        MAREA_CgenFunction *functions;
        const MAREA_IrNode **constants;
        const MAREA_IrNode **destroys;
        MAREA_CgenSymbol *symbols;
        MAREA_CgenAlias *aliases;
        // INFO: the values in scope that are destroyed at the end of their block, and the count of them at the start of the innermost loop.
        MAREA_CgenOwned *owned;
        // INFO: the declarations at the top of the function that is written: the temporaries that outlive a block.
        MAREA_CgenBuffer *prelude;
        MAREA_IntegerSize ownedCount;
        MAREA_IntegerSize ownedCapacity;
        MAREA_IntegerSize loopMark;
        MAREA_CgenFunction *current;
        const MAREA_Character8 *selfName;
        MAREA_IntegerSize namespaceLength;
        MAREA_IntegerSize selfLength;
        MAREA_IntegerSize typeCount;
        MAREA_IntegerSize typeCapacity;
        MAREA_IntegerSize functionCount;
        MAREA_IntegerSize functionCapacity;
        MAREA_IntegerSize constantCount;
        MAREA_IntegerSize constantCapacity;
        MAREA_IntegerSize destroyCount;
        MAREA_IntegerSize destroyCapacity;
        MAREA_IntegerSize symbolCount;
        MAREA_IntegerSize symbolCapacity;
        MAREA_IntegerSize aliasCount;
        MAREA_IntegerSize aliasCapacity;
        MAREA_Uinteger32 temporaryCount;
        MAREA_Uinteger32 constantDepth;
        MAREA_Boolean functionFailed;
        MAREA_Boolean outOfMemory;
        MAREA_Boolean usesMath;
        MAREA_Boolean usesPacked;
        MAREA_Boolean usesContext;
        MAREA_Boolean usesVector;
        // INFO: set while the destroys of a trap are written, which run whatever they raise.
        MAREA_Boolean unwinding;
        MAREA_Uinteger8 padding[1];
    } MAREA_CgenGenerator;

    /**
     * @brief Reserves at least extra bytes of room, growing the arena allocation when needed. One extra byte is
     * always kept free for the final NUL.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_CgenBufferReserve(MAREA_CgenBuffer * buffer, MAREA_IntegerSize extra);

    /**
     * @brief Appends raw bytes to a buffer. A no-op once the buffer has failed.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_CgenWrite(MAREA_CgenBuffer * buffer, const MAREA_Character8 *text, MAREA_IntegerSize length);

    /**
     * @brief Appends a NUL-terminated C string to a buffer.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_CgenWriteText(MAREA_CgenBuffer * buffer, const MAREA_Character8 *text);

    /**
     * @brief Appends NUL-terminated C strings to a buffer, for example MAREA_CGEN_WRITE(buffer, "(", name, ")").
     */
    #if !defined(MAREA_CGEN_WRITE)
        #define MAREA_CGEN_WRITE(buffer, ...) MAREA_CgenWriteTexts((buffer), (const MAREA_Character8 *const[]){ __VA_ARGS__ }, (sizeof((const MAREA_Character8 *const[]){ __VA_ARGS__ }) / sizeof(const MAREA_Character8 *)))
    #endif

    /**
     * @brief The body of MAREA_CGEN_WRITE. A null part is skipped.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_CgenWriteTexts(MAREA_CgenBuffer * buffer, const MAREA_Character8 *const *parts, MAREA_IntegerSize count);

    /**
     * @brief Ends a buffer with a NUL and gives its text.
     * @return The text, or an empty text when the buffer failed.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_Character8 *MAREA_CALL MAREA_CgenFinish(MAREA_CgenBuffer * buffer);

    /**
     * @brief The body of MAREA_CGEN_JOIN.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_Character8 *MAREA_CALL MAREA_CgenJoin(MAREA_CgenGenerator * generator, const MAREA_Character8 *const *parts, MAREA_IntegerSize count);

    /**
     * @brief A NUL terminated copy of a text of a given length.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_Character8 *MAREA_CALL MAREA_CgenCopyText(MAREA_CgenGenerator * generator, const MAREA_Character8 *text, MAREA_IntegerSize length);

    /**
     * @brief The decimal text of a number in the arena of the generator.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_Character8 *MAREA_CALL MAREA_CgenNumber(MAREA_CgenGenerator * generator, MAREA_Uinteger64 value);

    /**
     * @brief A name made from raw source text (an identifier or a type name), sanitized into a valid C11 identifier:
     * every byte that is not a letter, a digit or an underscore becomes an underscore, and the prefix goes in front so
     * a mangled name never collides with a C keyword or with a name this stage writes for another purpose.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_Character8 *MAREA_CALL MAREA_CgenMangled(MAREA_CgenGenerator * generator, const MAREA_Character8 *prefix, const MAREA_Character8 *text, MAREA_IntegerSize length);

    /**
     * @brief The C name of a local or a parameter: v_ and the name, and h_ and the rest of the name for a hidden
     * local of the ir, whose name starts with '$', so the two never collide.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_Character8 *MAREA_CALL MAREA_CgenLocalName(MAREA_CgenGenerator * generator, const MAREA_Character8 *text, MAREA_IntegerSize length);

    /**
     * @brief A new temporary name, marea_t and a number that is unique in the function.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_Character8 *MAREA_CALL MAREA_CgenTemporaryName(MAREA_CgenGenerator * generator);

    /**
     * @brief Gives a child of a node, or null when there is none at the index.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_IrNode *MAREA_CALL MAREA_CgenChildAt(const MAREA_IrNode *node, MAREA_IntegerSize index);

    /**
     * @brief Tells whether the text of a node is the given C string.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Boolean MAREA_CALL MAREA_CgenTextIs(const MAREA_IrNode *node, const MAREA_Character8 *expected);

    /**
     * @brief Tells whether two texts are equal.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Boolean MAREA_CALL MAREA_CgenSameText(const MAREA_Character8 *left, MAREA_IntegerSize leftLength, const MAREA_Character8 *right, MAREA_IntegerSize rightLength);

    /**
     * @brief Skips the namespace of the unit, and the dot after it, at the start of a name.
     * @return The number of bytes to skip, 0 when the name does not start with the namespace.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_IntegerSize MAREA_CALL MAREA_CgenNamespaceSkip(const MAREA_CgenGenerator *generator, const MAREA_Character8 *text, MAREA_IntegerSize length);

    /**
     * @brief Grows an array of the arena so that it holds at least one more item.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenGrow(MAREA_CgenGenerator * generator, MAREA_Null * ioItems, MAREA_IntegerSize * ioCapacity, MAREA_IntegerSize count, MAREA_IntegerSize itemSize, MAREA_IntegerSize alignment);

    /**
     * @brief Gives zeroed memory of the arena of the generator.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenAllocate(MAREA_CgenGenerator * generator, MAREA_IntegerSize size, MAREA_IntegerSize alignment, MAREA_Null * outMemory);

    /**
     * @brief Records one diagnostic and marks the function being written as failed, so no body is written for it
     * (the module as a whole still succeeds; see MAREA_CgenGenerateModule). Only the first fault of a function is
     * recorded.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenGeneratorFail(MAREA_CgenGenerator * generator, const MAREA_IrNode *node, MAREA_CgenDiagnosticCode code, const MAREA_Character8 *message);

    /**
     * @brief Reads an integer as the lexer accepts it: decimal, 0x or 0b, with '_' between digits.
     * @return MAREA_BOOLEAN_FALSE when the text is no integer or does not fit 64 bits.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Boolean MAREA_CALL MAREA_CgenParseInteger(const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_Uinteger64 *outValue);

    /**
     * @brief Finds a parameter or a local in scope by its source name.
     * @return The symbol, or null when no name in scope matches. It is valid until the next push.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_CgenSymbol *MAREA_CALL MAREA_CgenFindSymbol(const MAREA_CgenGenerator *generator, const MAREA_Character8 *text, MAREA_IntegerSize length);

    /**
     * @brief Pushes one parameter or local into scope.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenPushSymbol(MAREA_CgenGenerator * generator, const MAREA_Character8 *text, MAREA_IntegerSize length, const MAREA_Character8 *cText, MAREA_Uinteger32 type, MAREA_Boolean stable, MAREA_Boolean unaligned);

    /**
     * @brief Finds a function of the module by name, with the namespace of this unit skipped when it was written. A
     * function of an implementation block is named "Type.name".
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenFunction *MAREA_CALL MAREA_CgenFindFunction(const MAREA_CgenGenerator *generator, const MAREA_Character8 *text, MAREA_IntegerSize length);

    /**
     * @brief Finds the function "Type.member" of the module.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenFunction *MAREA_CALL MAREA_CgenFindMember(const MAREA_CgenGenerator *generator, const MAREA_Character8 *owner, MAREA_IntegerSize ownerLength, const MAREA_Character8 *member, MAREA_IntegerSize memberLength);

    /**
     * @brief Finds a constant of the module by name, with the namespace of the unit skipped.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_IrNode *MAREA_CALL MAREA_CgenFindConstant(const MAREA_CgenGenerator *generator, const MAREA_Character8 *text, MAREA_IntegerSize length);

    /**
     * @brief Finds a declaration of the module with the given kind and name.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_IrNode *MAREA_CALL MAREA_CgenFindDeclaration(const MAREA_CgenGenerator *generator, MAREA_IrKind kind, const MAREA_Character8 *text, MAREA_IntegerSize length);

    /**
     * @brief Reads the kind a scalar type name stands for.
     * @return The kind, or MAREA_CGEN_KIND_NONE when the text names no scalar type.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Uinteger32 MAREA_CALL MAREA_CgenKindFromText(const MAREA_Character8 *text, MAREA_IntegerSize length);

    /**
     * @brief Tells whether a type id is a number.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Boolean MAREA_CALL MAREA_CgenIsNumber(MAREA_Uinteger32 type);

    /**
     * @brief Tells whether a type id is an integer.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Boolean MAREA_CALL MAREA_CgenIsInteger(MAREA_Uinteger32 type);

    /**
     * @brief Tells whether a type id is a signed integer.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Boolean MAREA_CALL MAREA_CgenIsSigned(MAREA_Uinteger32 type);

    /**
     * @brief Tells whether a type id is a float.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Boolean MAREA_CALL MAREA_CgenIsFloat(MAREA_Uinteger32 type);

    /**
     * @brief Returns the suffix of the runtime helper functions of a scalar kind (s8, u32, f64, bool).
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_Character8 *MAREA_CALL MAREA_CgenKindSuffix(MAREA_Uinteger32 kind);

    /**
     * @brief What a type id is.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Uinteger8 MAREA_CALL MAREA_CgenCategoryOf(const MAREA_CgenGenerator *generator, MAREA_Uinteger32 type);

    /**
     * @brief The entry of a composite type.
     * @return The entry, or null for a scalar type, void or none. It is valid until the next type is added.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_CgenType *MAREA_CALL MAREA_CgenTypeAt(const MAREA_CgenGenerator *generator, MAREA_Uinteger32 type);

    /**
     * @brief Tells whether cgen can write a value of the type: a scalar, or a composite type that did not fail.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Boolean MAREA_CALL MAREA_CgenIsValueType(const MAREA_CgenGenerator *generator, MAREA_Uinteger32 type);

    /**
     * @brief Tells whether a value of the type is a C aggregate: a structure, an array or an enum with data.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Boolean MAREA_CALL MAREA_CgenIsAggregate(const MAREA_CgenGenerator *generator, MAREA_Uinteger32 type);

    /**
     * @brief The C name of a type.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_Character8 *MAREA_CALL MAREA_CgenTypeName(const MAREA_CgenGenerator *generator, MAREA_Uinteger32 type);

    /**
     * @brief The size in bytes of a value of the type.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Uinteger64 MAREA_CALL MAREA_CgenSizeOf(const MAREA_CgenGenerator *generator, MAREA_Uinteger32 type);

    /**
     * @brief The alignment in bytes of a value of the type, at least 1.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Uinteger64 MAREA_CALL MAREA_CgenAlignmentOf(const MAREA_CgenGenerator *generator, MAREA_Uinteger32 type);

    /**
     * @brief Writes the C definition of a structure into the types buffer, with the layout of entry, which it asserts.
     * Sets the packed flag of the entry.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_CgenWriteStructureDefinition(MAREA_CgenGenerator * generator, MAREA_CgenType * entry);

    /**
     * @brief Writes the C definition of an enum into the types buffer: its tag type alone, or a tag and a union of the
     * data of each variant, at the payload offset. Sets the wrapped and packed flags of the entry.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_CgenWriteEnumDefinition(MAREA_CgenGenerator * generator, MAREA_CgenType * entry, MAREA_Uinteger64 payload);

    /**
     * @brief Writes the C definition of an array into the types buffer: a structure that holds the elements.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_CgenWriteArrayDefinition(MAREA_CgenGenerator * generator, const MAREA_CgenType *entry);

    /**
     * @brief The type of a written type: a scalar, void, Self, a structure or an enum of the unit, or an array of a
     * registered inline run. The head is the type name of the ir (a full path), resolved the full spelling of sema.
     * outType receives MAREA_CGEN_KIND_NONE when cgen has no form for the type.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenTypeOfText(MAREA_CgenGenerator * generator, const MAREA_Character8 *head, MAREA_IntegerSize headLength, const MAREA_Character8 *resolved, MAREA_IntegerSize resolvedLength, MAREA_Uinteger32 *outType);

    /**
     * @brief The type of a node with a written or a resolved type.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenTypeOfNode(MAREA_CgenGenerator * generator, const MAREA_IrNode *node, MAREA_Uinteger32 *outType);

    /**
     * @brief True when the resolved type of a node is a raw pointer, a type whose descriptor has the raw pointer
     * role. Gives the text of its element type, which points into the resolved type. False for a null node.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Boolean MAREA_CALL MAREA_CgenPointee(MAREA_CgenGenerator * generator, const MAREA_IrNode *node, const MAREA_Character8 **outElement, MAREA_IntegerSize *outElementLength);

    /**
     * @brief MAREA_CgenPointee for a caller that does not need the element.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Boolean MAREA_CALL MAREA_CgenIsPointer(MAREA_CgenGenerator * generator, const MAREA_IrNode *node);

    /**
     * @brief Reads the aliases that the written types of a tree show.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenCollectAliases(MAREA_CgenGenerator * generator, const MAREA_IrNode *node);

    /**
     * @brief Adds every structure and enum of the module that has a measured layout to the type table.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenDeclareTypes(MAREA_CgenGenerator * generator);

    /**
     * @brief Writes the runtime of the generated program: its includes, the trap codes and the helper functions that
     * give every operation the outcome of the vm without C undefined behavior.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_CgenWritePreamble(MAREA_CgenBuffer * buffer, MAREA_Boolean math, MAREA_Boolean packed);

    /**
     * @brief Writes "if (*marea_trap != MAREA_RT_TRAP_NONE) { return ZERO; }", which stops the function as soon as an
     * operation or a call of it trapped, the way the vm stops the run.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_CgenWriteTrapGuard(MAREA_CgenGenerator * generator);

    /**
     * @brief Writes the return of the zero value of the current function, or "return;" for no result.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_CgenWriteZeroReturn(MAREA_CgenGenerator * generator);

    /**
     * @brief Writes the setting of a trap of the runtime and the return of the function.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_CgenWriteTrap(MAREA_CgenGenerator * generator, const MAREA_Character8 *trap);

    /**
     * @brief Writes "TYPE marea_tN = EXPRESSION;" and gives the new temporary as a stable value.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_CgenBind(MAREA_CgenGenerator * generator, MAREA_Uinteger32 type, const MAREA_Character8 *expression, MAREA_CgenValue *outValue);

    /**
     * @brief Writes the declaration of a temporary of a type, with every byte zero, and gives it.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_CgenZeroTemporary(MAREA_CgenGenerator * generator, MAREA_Uinteger32 type, MAREA_CgenValue * outValue);

    /**
     * @brief Writes the store of a value into storage of its type: an assignment, or a copy of the bytes.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_CgenStore(MAREA_CgenGenerator * generator, const MAREA_Character8 *target, MAREA_Uinteger32 type, const MAREA_Character8 *value);

    /**
     * @brief Copies a value into a new temporary, so a later change of its storage cannot reach it.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_CgenSnapshot(MAREA_CgenGenerator * generator, MAREA_CgenValue * ioValue);

    /**
     * @brief Writes the statements that compute an expression and gives its value.
     * @param expected The type the context of the expression asks for, or MAREA_CGEN_KIND_NONE.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteValue(MAREA_CgenGenerator * generator, const MAREA_IrNode *node, MAREA_Uinteger32 expected, MAREA_CgenValue *outValue);

    /**
     * @brief Writes the statements that reach the storage an expression names, or computes the value into a temporary
     * when the expression names none, and gives the storage. Every index of the path is checked and held in a
     * temporary, so the storage stays the same.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenWritePlace(MAREA_CgenGenerator * generator, const MAREA_IrNode *node, MAREA_Uinteger32 expected, MAREA_CgenValue *outPlace);

    /**
     * @brief Gives the storage of one member of a value that is already reached: a field of a structure, or the data
     * field of the variants of an enum, read through the tag. node is the MEMBER_ACCESS.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteMember(MAREA_CgenGenerator * generator, const MAREA_IrNode *node, const MAREA_CgenValue *base, MAREA_CgenValue *outPlace);

    /**
     * @brief Writes the value of an expression and reports it when its type is not the expected one.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteTyped(MAREA_CgenGenerator * generator, const MAREA_IrNode *node, MAREA_Uinteger32 expected, MAREA_CgenValue *outValue);

    /**
     * @brief Writes a binary operator on two values of one type, with the trap guard of an operator that can trap.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteOperator(MAREA_CgenGenerator * generator, const MAREA_IrNode *node, MAREA_TokenKind operatorKind, const MAREA_CgenValue *left, const MAREA_CgenValue *right, MAREA_CgenValue *outValue);

    /**
     * @brief The operator of a compound assignment, for example PLUS for PLUS_EQUAL.
     * @return The operator, or MAREA_TOKEN_KIND_ERROR when the token is no compound assignment.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_TokenKind MAREA_CALL MAREA_CgenPlainOperator(MAREA_TokenKind compound);

    /**
     * @brief Tells whether a type is the generated structure of a tuple.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Boolean MAREA_CALL MAREA_CgenIsTuple(const MAREA_CgenGenerator *generator, MAREA_Uinteger32 type);

    /**
     * @brief The place of the element that the literal index of an INDEX_ACCESS names in a tuple value.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteTupleIndex(MAREA_CgenGenerator * generator, const MAREA_IrNode *node, const MAREA_CgenValue *base, MAREA_CgenValue *outPlace);

    /**
     * @brief Writes == or != of two tuples of one type, element by element.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteTupleComparison(MAREA_CgenGenerator * generator, const MAREA_IrNode *node, MAREA_Uinteger32 type, MAREA_CgenValue *outValue);

    /**
     * @brief Writes the call of swap on a tuple.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteTupleCall(MAREA_CgenGenerator * generator, const MAREA_IrNode *node, const MAREA_CgenValue *receiver, const MAREA_IrNode *member, const MAREA_IrNode *arguments, MAREA_CgenValue *outValue);

    /**
     * @brief Writes a comparison of two values.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteComparison(MAREA_CgenGenerator * generator, const MAREA_IrNode *node, MAREA_CgenValue *outValue);

    /**
     * @brief Writes a unary or a binary expression, with and and or short circuiting.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteUnary(MAREA_CgenGenerator * generator, const MAREA_IrNode *node, MAREA_Uinteger32 expected, MAREA_CgenValue *outValue);

    /**
     * @brief Writes a binary expression.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteBinary(MAREA_CgenGenerator * generator, const MAREA_IrNode *node, MAREA_Uinteger32 expected, MAREA_CgenValue *outValue);

    /**
     * @brief Writes an integer or a float literal of the type of the literal, the expected type or the default type.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteNumberLiteral(MAREA_CgenGenerator * generator, const MAREA_IrNode *node, MAREA_Boolean negate, MAREA_Uinteger32 expected, MAREA_CgenValue *outValue);

    /**
     * @brief Writes a convert call: a value of another number type, with a trap when it does not fit.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteConvert(MAREA_CgenGenerator * generator, const MAREA_IrNode *node, MAREA_CgenValue *outValue);

    /**
     * @brief Writes a structure literal or an array literal into a new temporary.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteLiteralValue(MAREA_CgenGenerator * generator, const MAREA_IrNode *node, MAREA_Uinteger32 expected, MAREA_CgenValue *outValue);

    /**
     * @brief Writes a value of one variant of an enum, with the arguments in order and then the defaults.
     * @param arguments The ARGUMENT_LIST of the constructor call, or null for a variant written with no call.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteVariant(MAREA_CgenGenerator * generator, const MAREA_IrNode *node, MAREA_Uinteger32 type, MAREA_IntegerSize variant, const MAREA_IrNode *arguments, MAREA_CgenValue *outValue);

    /**
     * @brief Writes a CONSTRUCT_CALL: the zero value or the literal, then the construct operator of the type.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteConstruct(MAREA_CgenGenerator * generator, const MAREA_IrNode *node, MAREA_CgenValue *outValue);

    /**
     * @brief Finds the number of the variant of an enum type with the given name.
     * @return MAREA_BOOLEAN_FALSE when the type is no enum or has no variant of that name.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Boolean MAREA_CALL MAREA_CgenFindVariant(const MAREA_CgenGenerator *generator, MAREA_Uinteger32 type, const MAREA_Character8 *name, MAREA_IntegerSize length, MAREA_IntegerSize *outVariant);

    /**
     * @brief Finds a field of a structure type by name.
     * @return The field, or null when the type is no structure or has no field of that name.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_CgenField *MAREA_CALL MAREA_CgenFindField(const MAREA_CgenGenerator *generator, MAREA_Uinteger32 type, const MAREA_Character8 *name, MAREA_IntegerSize length);

    /**
     * @brief Reads the type that a base of a member access names (Type.member), or MAREA_CGEN_KIND_NONE when the base
     * is a value or a namespace.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenQualifierType(MAREA_CgenGenerator * generator, const MAREA_IrNode *node, MAREA_Uinteger32 *outType);

    /**
     * @brief Builds the dotted name of a chain of member accesses whose root is a namespace, not a value.
     * @return The name, or null when the chain starts at a value.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_Character8 *MAREA_CALL MAREA_CgenQualifiedName(MAREA_CgenGenerator * generator, const MAREA_IrNode *node, MAREA_IntegerSize *outLength);

    /**
     * @brief Writes a call: of a function of the unit, of a method, of a variant constructor, of a built-in or of a
     * C function of the host.
     * @param valueUsed Whether the result of the call is read.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteCall(MAREA_CgenGenerator * generator, const MAREA_IrNode *node, MAREA_Boolean valueUsed, MAREA_CgenValue *outValue);

    /**
     * @brief Writes a call of a function of the unit. receiver is the storage of self in a method call, already
     * reached, or null; the arguments are an ARGUMENT_LIST, or null for none.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteUnitCall(MAREA_CgenGenerator * generator, const MAREA_IrNode *node, MAREA_CgenFunction *function, const MAREA_CgenValue *receiver, const MAREA_IrNode *arguments, MAREA_CgenValue *outValue);

    /**
     * @brief Writes a call of a C function of the host, or the trap of a missing one.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteHostCall(MAREA_CgenGenerator * generator, const MAREA_IrNode *node, const MAREA_Character8 *name, MAREA_IntegerSize length, MAREA_Boolean valueUsed, MAREA_CgenValue *outValue);

    /**
     * @brief Writes one ir statement as one or more C11 statements.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteStatement(MAREA_CgenGenerator * generator, const MAREA_IrNode *node);

    /**
     * @brief Writes every statement of a BLOCK node inside a fresh C11 scope ("{" ... "}"), with the symbols the
     * block pushes dropped from scope again once it ends.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteBlock(MAREA_CgenGenerator * generator, const MAREA_IrNode *node);

    /**
     * @brief Writes the constant of the module into the current function, in the scope of the module.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteConstant(MAREA_CgenGenerator * generator, const MAREA_IrNode *constant, MAREA_CgenValue *outValue);

    /**
     * @brief Writes an expression in the scope of the module: no local is visible and Self is the type of the owner.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteModuleScoped(MAREA_CgenGenerator * generator, const MAREA_IrNode *node, const MAREA_Character8 *owner, MAREA_IntegerSize ownerLength, MAREA_Uinteger32 expected, MAREA_CgenValue *outValue);

    /**
     * @brief Writes the trap of an element that does not exist and gives a zero value that is never read.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_CgenWriteEmptyElement(MAREA_CgenGenerator * generator, MAREA_Uinteger32 element, MAREA_CgenValue * outPlace);

    /**
     * @brief Writes the static call filled of the array type that a CALL_EXPRESSION gives.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteArrayFilled(MAREA_CgenGenerator * generator, const MAREA_IrNode *node, const MAREA_IrNode *arguments, MAREA_CgenValue *outValue);

    /**
     * @brief Writes a call of a member of an array, whose place is already written.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteArrayCall(MAREA_CgenGenerator * generator, const MAREA_IrNode *node, const MAREA_CgenValue *receiver, const MAREA_IrNode *member, const MAREA_IrNode *arguments, MAREA_CgenValue *outValue);

    /**
     * @brief Writes the value of an INCLUDED_BYTES node: a static array that holds the file, defined once for each
     * file of the module.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteIncludedBytes(MAREA_CgenGenerator * generator, const MAREA_IrNode *node, MAREA_CgenValue *outValue);

    /**
     * @brief Tells whether a value of a type holds storage of the allocator: a vector, or a value that holds one.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Boolean MAREA_CALL MAREA_CgenNeedsDrop(const MAREA_CgenGenerator *generator, MAREA_Uinteger32 type);

    /**
     * @brief Tells whether an element of a container must be destroyed: it holds storage, or its type declares the
     * destroy operator.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Boolean MAREA_CALL MAREA_CgenElementNeedsDrop(const MAREA_CgenGenerator *generator, MAREA_Uinteger32 type);

    /**
     * @brief Writes the destroy of the value at a place. With isElement the destroy operator of the type runs first,
     * which for a local the ir writes itself. A destroy of a value that was taken does nothing.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteDrop(MAREA_CgenGenerator * generator, const MAREA_IrNode *node, const MAREA_Character8 *place, MAREA_Uinteger32 type, MAREA_Boolean isElement);

    /**
     * @brief Writes a deep copy of a value into storage that holds none yet.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteDeepCopy(MAREA_CgenGenerator * generator, const MAREA_IrNode *node, const MAREA_Character8 *target, const MAREA_Character8 *source, MAREA_Uinteger32 type);

    /**
     * @brief Puts a value among those that are destroyed when the current block ends.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenPushOwned(MAREA_CgenGenerator * generator, const MAREA_Character8 *text, MAREA_Uinteger32 type);

    /**
     * @brief Writes the destroys of the owned values from a mark up, the last one first. They stay in the list.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteDrops(MAREA_CgenGenerator * generator, const MAREA_IrNode *node, MAREA_IntegerSize mark);

    /**
     * @brief Moves a temporary that holds storage, and that nothing takes over, into storage of the function that is
     * destroyed with the block. Any other value stays as it is.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenHold(MAREA_CgenGenerator * generator, const MAREA_IrNode *node, MAREA_CgenValue *ioValue);

    /**
     * @brief Writes a value that the caller takes over: as MAREA_CgenWriteTyped, and a name, a field or an element
     * that holds storage leaves zeros behind.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteOwned(MAREA_CgenGenerator * generator, const MAREA_IrNode *node, MAREA_Uinteger32 expected, MAREA_CgenValue *outValue);

    /**
     * @brief The C text of the element of a vector at an index that is known to exist.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_Character8 *MAREA_CALL MAREA_CgenElementText(MAREA_CgenGenerator * generator, const MAREA_Character8 *vector, MAREA_Uinteger32 element, const MAREA_Character8 *index);

    /**
     * @brief The two C arguments that give the stride and the alignment of an element to the runtime of a vector.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_Character8 *MAREA_CALL MAREA_CgenStrideText(MAREA_CgenGenerator * generator, MAREA_Uinteger32 element);

    /**
     * @brief Declares a context of one runtime call in the current block and gives its name.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_Character8 *MAREA_CALL MAREA_CgenWriteContext(MAREA_CgenGenerator * generator);

    /**
     * @brief Writes the out of memory trap of a context that the runtime raised a trap on.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_CgenWriteMemoryGuard(MAREA_CgenGenerator * generator, const MAREA_Character8 *context);

    /**
     * @brief Writes the checked place of an element of a vector, for an INDEX_ACCESS.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteVectorIndex(MAREA_CgenGenerator * generator, const MAREA_IrNode *node, const MAREA_CgenValue *base, MAREA_CgenValue *outPlace);

    /**
     * @brief Writes a LITERAL_VALUE of a vector.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteVectorLiteral(MAREA_CgenGenerator * generator, const MAREA_IrNode *node, MAREA_Uinteger32 type, MAREA_CgenValue *outValue);

    /**
     * @brief Writes a call of a member of a vector, whose place is already written. With no use of the result a
     * member that gives an element up destroys it.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteVectorCall(MAREA_CgenGenerator * generator, const MAREA_IrNode *node, const MAREA_CgenValue *receiver, const MAREA_IrNode *member, const MAREA_IrNode *arguments, MAREA_Boolean valueUsed, MAREA_CgenValue *outValue);

MAREA_END_EXTERN_C

#endif /* MAREA_INTERNAL_CGEN_MAREACGENINTERNAL_H */
