// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_SEMA_MAREASEMA_H)
    #define MAREA_INCLUDE_SEMA_MAREASEMA_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREAArena.h"
    #include "marea/base/MAREADiagnostic.h"
    #include "marea/base/MAREATypeRegistry.h"
    #include "marea/parse/MAREAAst.h"
    #include "marea/sema/MAREASemaImportResolver.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Result of a sema operation. A rule violation is not an exception: it is a diagnostic.
     */
    typedef enum MAREA_SemaExcept
    {
        MAREA_SEMA_EXCEPT_SUCCESS = MAREA_UINTEGER_8(0),
        MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER = MAREA_UINTEGER_8(1),
        MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY = MAREA_UINTEGER_8(2),
    } MAREA_SemaExcept;

    /**
     * @brief Diagnostic code emitted by sema, in the 3000-3999 range reserved for this stage. Every code is
     * an error.
     */
    typedef enum MAREA_SemaDiagnosticCode
    {
        MAREA_SEMA_DIAGNOSTIC_CODE_UNRESOLVED_IMPORT = MAREA_UINTEGER_32(3001),
        MAREA_SEMA_DIAGNOSTIC_CODE_NAMESPACE_PATH_MISMATCH = MAREA_UINTEGER_32(3002),
        MAREA_SEMA_DIAGNOSTIC_CODE_NAME_ALREADY_DECLARED = MAREA_UINTEGER_32(3003),
        MAREA_SEMA_DIAGNOSTIC_CODE_UNDECLARED_NAME = MAREA_UINTEGER_32(3004),
        MAREA_SEMA_DIAGNOSTIC_CODE_UNKNOWN_ATTRIBUTE_KEY = MAREA_UINTEGER_32(3005),
        MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_ATTRIBUTE_VALUE = MAREA_UINTEGER_32(3006),
        MAREA_SEMA_DIAGNOSTIC_CODE_CONFLICTING_ATTRIBUTES = MAREA_UINTEGER_32(3007),
        MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_ALIGNMENT = MAREA_UINTEGER_32(3008),
        MAREA_SEMA_DIAGNOSTIC_CODE_UNKNOWN_TYPE = MAREA_UINTEGER_32(3009),
        MAREA_SEMA_DIAGNOSTIC_CODE_ARGUMENT_COUNT_MISMATCH = MAREA_UINTEGER_32(3010),
        MAREA_SEMA_DIAGNOSTIC_CODE_TYPE_MISMATCH = MAREA_UINTEGER_32(3011),
        MAREA_SEMA_DIAGNOSTIC_CODE_DUPLICATE_MEMBER = MAREA_UINTEGER_32(3012),
        MAREA_SEMA_DIAGNOSTIC_CODE_CONTRACT_NOT_SATISFIED = MAREA_UINTEGER_32(3013),
        MAREA_SEMA_DIAGNOSTIC_CODE_GENERIC_ARGUMENT_MISMATCH = MAREA_UINTEGER_32(3014),
        MAREA_SEMA_DIAGNOSTIC_CODE_INSTANTIATION_TOO_DEEP = MAREA_UINTEGER_32(3015),
        MAREA_SEMA_DIAGNOSTIC_CODE_ASSIGNMENT_NEEDS_COPY_OR_MOVE = MAREA_UINTEGER_32(3016),
        MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_BORROW = MAREA_UINTEGER_32(3017),
        MAREA_SEMA_DIAGNOSTIC_CODE_REFERENCE_ESCAPES_SCOPE = MAREA_UINTEGER_32(3018),
        MAREA_SEMA_DIAGNOSTIC_CODE_EXCLUSIVE_BORROW_CONFLICT = MAREA_UINTEGER_32(3019),
        MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_SPAWN_ARGUMENT = MAREA_UINTEGER_32(3020),
        MAREA_SEMA_DIAGNOSTIC_CODE_NON_EXHAUSTIVE_SWITCH = MAREA_UINTEGER_32(3021),
        MAREA_SEMA_DIAGNOSTIC_CODE_MISSING_RETURN = MAREA_UINTEGER_32(3022),
        // INFO: 3023 is retired (the fixed main signature check) and stays reserved, never reused.
        MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_PARALLEL_JUMP = MAREA_UINTEGER_32(3024),
        MAREA_SEMA_DIAGNOSTIC_CODE_DUPLICATE_SWITCH_ARM = MAREA_UINTEGER_32(3025),
        MAREA_SEMA_DIAGNOSTIC_CODE_JUMP_OUTSIDE_LOOP = MAREA_UINTEGER_32(3026),
        MAREA_SEMA_DIAGNOSTIC_CODE_LIFECYCLE_OPERATOR_CALLED_DIRECTLY = MAREA_UINTEGER_32(3027),
        MAREA_SEMA_DIAGNOSTIC_CODE_LIFECYCLE_OPERATOR_NEEDS_CONCRETE_TYPE = MAREA_UINTEGER_32(3028),
        MAREA_SEMA_DIAGNOSTIC_CODE_LIFECYCLE_OPERATOR_BAD_SHAPE = MAREA_UINTEGER_32(3029),
        MAREA_SEMA_DIAGNOSTIC_CODE_DEFAULT_PARAMETER_BEFORE_REQUIRED = MAREA_UINTEGER_32(3030),
        MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_ENUM_TAG_TYPE = MAREA_UINTEGER_32(3031),
        MAREA_SEMA_DIAGNOSTIC_CODE_CANNOT_BORROW_CONSTANT_AS_MUTABLE = MAREA_UINTEGER_32(3032),
        MAREA_SEMA_DIAGNOSTIC_CODE_LITERAL_OUT_OF_RANGE = MAREA_UINTEGER_32(3033),
        MAREA_SEMA_DIAGNOSTIC_CODE_DIVISION_BY_ZERO = MAREA_UINTEGER_32(3034),
        MAREA_SEMA_DIAGNOSTIC_CODE_SHIFT_COUNT_TOO_LARGE = MAREA_UINTEGER_32(3035),
        MAREA_SEMA_DIAGNOSTIC_CODE_NEGATE_UNSIGNED = MAREA_UINTEGER_32(3036),
        MAREA_SEMA_DIAGNOSTIC_CODE_SIGNED_DIVISION_OVERFLOW = MAREA_UINTEGER_32(3037),
        MAREA_SEMA_DIAGNOSTIC_CODE_BITWISE_OPERAND_NOT_INTEGER = MAREA_UINTEGER_32(3038),
        MAREA_SEMA_DIAGNOSTIC_CODE_LOGICAL_OPERAND_NOT_BOOLEAN = MAREA_UINTEGER_32(3039),
        MAREA_SEMA_DIAGNOSTIC_CODE_ASSIGNMENT_TO_CONSTANT = MAREA_UINTEGER_32(3040),
        MAREA_SEMA_DIAGNOSTIC_CODE_RAW_POINTER_OUTSIDE_UNSAFE = MAREA_UINTEGER_32(3041),
        MAREA_SEMA_DIAGNOSTIC_CODE_NON_BOOLEAN_CONDITION = MAREA_UINTEGER_32(3043),
        MAREA_SEMA_DIAGNOSTIC_CODE_UNKNOWN_FIELD = MAREA_UINTEGER_32(3044),
        MAREA_SEMA_DIAGNOSTIC_CODE_RECURSIVE_TYPE = MAREA_UINTEGER_32(3045),
        MAREA_SEMA_DIAGNOSTIC_CODE_CYCLIC_TYPE_ALIAS = MAREA_UINTEGER_32(3046),
        MAREA_SEMA_DIAGNOSTIC_CODE_EMPTY_ENUM = MAREA_UINTEGER_32(3047),
        MAREA_SEMA_DIAGNOSTIC_CODE_UNKNOWN_ENUM_VARIANT = MAREA_UINTEGER_32(3048),
        MAREA_SEMA_DIAGNOSTIC_CODE_UNKNOWN_TYPE_FUNCTION = MAREA_UINTEGER_32(3049),
        MAREA_SEMA_DIAGNOSTIC_CODE_OPERATOR_SIGNATURE_MISMATCH = MAREA_UINTEGER_32(3050),
        MAREA_SEMA_DIAGNOSTIC_CODE_BOUND_OPERATION_NOT_GIVEN = MAREA_UINTEGER_32(3051),
        MAREA_SEMA_DIAGNOSTIC_CODE_OVERLAPPING_IMPLEMENTATION = MAREA_UINTEGER_32(3052),
        MAREA_SEMA_DIAGNOSTIC_CODE_FUNCTION_TYPE_MISMATCH = MAREA_UINTEGER_32(3053),
        MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_RESULT_USE = MAREA_UINTEGER_32(3054),
        MAREA_SEMA_DIAGNOSTIC_CODE_CYCLIC_CONSTANT = MAREA_UINTEGER_32(3055),
        MAREA_SEMA_DIAGNOSTIC_CODE_RESERVED_MACRO_NAME = MAREA_UINTEGER_32(3056),
        MAREA_SEMA_DIAGNOSTIC_CODE_OPAQUE_TYPE_MISUSE = MAREA_UINTEGER_32(3057),
        MAREA_SEMA_DIAGNOSTIC_CODE_OPAQUE_FIELD_BY_VALUE = MAREA_UINTEGER_32(3058),
        MAREA_SEMA_DIAGNOSTIC_CODE_COMPILE_TIME_ASSERTION_FAILED = MAREA_UINTEGER_32(3059),
        MAREA_SEMA_DIAGNOSTIC_CODE_UNKNOWN_MACRO = MAREA_UINTEGER_32(3060),
        MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_MACRO_ARGUMENT = MAREA_UINTEGER_32(3061),
        MAREA_SEMA_DIAGNOSTIC_CODE_PARALLEL_CONSTRUCT_OUTSIDE_FORK_JOIN = MAREA_UINTEGER_32(3062),
        MAREA_SEMA_DIAGNOSTIC_CODE_SPAWN_NEEDS_CALL = MAREA_UINTEGER_32(3063),
        MAREA_SEMA_DIAGNOSTIC_CODE_PARTITION_SOURCE_NOT_ARRAY = MAREA_UINTEGER_32(3064),
        MAREA_SEMA_DIAGNOSTIC_CODE_LOCAL_WITHOUT_INITIAL_VALUE = MAREA_UINTEGER_32(3065),
        MAREA_SEMA_DIAGNOSTIC_CODE_PRIVATE_SYMBOL_ACCESS = MAREA_UINTEGER_32(3066),
        MAREA_SEMA_DIAGNOSTIC_CODE_PRIVATE_TYPE_ACCESS = MAREA_UINTEGER_32(3067),
        MAREA_SEMA_DIAGNOSTIC_CODE_PRIVATE_FIELD_ACCESS = MAREA_UINTEGER_32(3068),
        MAREA_SEMA_DIAGNOSTIC_CODE_WRITE_THROUGH_READ_ONLY_BORROW = MAREA_UINTEGER_32(3069),
        MAREA_SEMA_DIAGNOSTIC_CODE_USE_OF_MOVED_VALUE = MAREA_UINTEGER_32(3070),
        MAREA_SEMA_DIAGNOSTIC_CODE_VALUE_MOVED_AGAIN = MAREA_UINTEGER_32(3071),
        MAREA_SEMA_DIAGNOSTIC_CODE_USE_OF_PARTIALLY_MOVED_VALUE = MAREA_UINTEGER_32(3072),
        MAREA_SEMA_DIAGNOSTIC_CODE_VALUE_CHANGED_WHILE_BORROWED = MAREA_UINTEGER_32(3073),
        MAREA_SEMA_DIAGNOSTIC_CODE_CONFLICTING_PARALLEL_ACCESS = MAREA_UINTEGER_32(3074),
        MAREA_SEMA_DIAGNOSTIC_CODE_PARTITION_BODY_WRITES_OUTER_NAME = MAREA_UINTEGER_32(3075),
        MAREA_SEMA_DIAGNOSTIC_CODE_PARTITION_SOURCE_USED_IN_BODY = MAREA_UINTEGER_32(3076),
        MAREA_SEMA_DIAGNOSTIC_CODE_MISSING_FIELD = MAREA_UINTEGER_32(3077),
        MAREA_SEMA_DIAGNOSTIC_CODE_UNIMPORTED_NAMESPACE = MAREA_UINTEGER_32(3078),
        MAREA_SEMA_DIAGNOSTIC_CODE_UNRESOLVED_REQUIRE = MAREA_UINTEGER_32(3079),
        MAREA_SEMA_DIAGNOSTIC_CODE_AMBIGUOUS_NAMESPACE = MAREA_UINTEGER_32(3080),
        MAREA_SEMA_DIAGNOSTIC_CODE_RESERVED_NAMESPACE_ROOT = MAREA_UINTEGER_32(3081),
        // INFO: an element type with no measured layout held by value (decision 28 of the 2026-11-24 plan,
        // file 14_UB_C_BORDER.md): a registered type whose descriptor holds its type argument by value
        // (hasIndexProtocol true, an array-shaped library type) needs that argument to have a measured
        // layout.
        MAREA_SEMA_DIAGNOSTIC_CODE_ELEMENT_WITHOUT_MEASURED_LAYOUT = MAREA_UINTEGER_32(3082),
        // INFO: an explicit backend of marea.math that the target description or the compiler cannot give
        // (decision 18 of the 2026-11-24 plan, file 17_BACKEND_ATTRIBUTE_WHEN.md). The plan proposed 3078 for it,
        // but 3078 to 3082 were taken by the require work, so the next free number is used.
        MAREA_SEMA_DIAGNOSTIC_CODE_BACKEND_UNAVAILABLE = MAREA_UINTEGER_32(3083),
        // INFO: the backend key written in a file whose namespace is not marea.math or below it.
        MAREA_SEMA_DIAGNOSTIC_CODE_ATTRIBUTE_NOT_ALLOWED_HERE = MAREA_UINTEGER_32(3084),
        // INFO: a known key written at a place that the placement matrix of the plan (11_ATTRIBUTES_MAP.md,
        // section 6) does not give it, and an export on a generic function.
        MAREA_SEMA_DIAGNOSTIC_CODE_MISPLACED_ATTRIBUTE = MAREA_UINTEGER_32(3085),
        // INFO: a value that the build gives to build_value! and that does not fit the type of the call.
        MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_BUILD_VALUE = MAREA_UINTEGER_32(3086),
        // INFO: an array literal whose positional values differ from the count of the array.
        MAREA_SEMA_DIAGNOSTIC_CODE_LITERAL_COUNT_MISMATCH = MAREA_UINTEGER_32(3087),
        // INFO: a constant index of an array that is not below its count.
        MAREA_SEMA_DIAGNOSTIC_CODE_CONSTANT_INDEX_OUT_OF_RANGE = MAREA_UINTEGER_32(3088),
        // INFO: an index of a tuple that is no integer literal, so the type of the element is not known.
        MAREA_SEMA_DIAGNOSTIC_CODE_TUPLE_INDEX_NOT_CONSTANT = MAREA_UINTEGER_32(3089),
        // INFO: a for each over a tuple, whose elements have different types.
        MAREA_SEMA_DIAGNOSTIC_CODE_NOT_ITERABLE = MAREA_UINTEGER_32(3090),
    } MAREA_SemaDiagnosticCode;

    /**
     * @brief The target description that the caller gives to sema: what a when(...) predicate and the
     * availability of a backend are judged against. A plain struct, caller owned, not opaque, because the
     * caller fills every field (CMake, or a host with no CMake) and sema only reads it during a check, never
     * copies or frees it. A text is NUL terminated and may be null, and a null text, a zero pointer width and
     * an empty feature list mean "not answered": a predicate on it is false. Context: the caller of sema
     * knows the target and sema never reads the environment.
     */
    typedef struct MAREA_ALIGNED(8) MAREA_SemaTarget
    {
        const MAREA_Character8 *architecture;
        const MAREA_Character8 *operatingSystem;
        const MAREA_Character8 *compiler;
        const MAREA_Character8 *endianness;
        const MAREA_Character8 *const *features;
        MAREA_IntegerSize featureCount;
        MAREA_Uinteger32 pointerWidth;
    } MAREA_SemaTarget;

    /**
     * @brief One name and text pair that the build gives to build_value!. Both texts are NUL terminated.
     */
    typedef struct MAREA_ALIGNED(8) MAREA_SemaSetting
    {
        const MAREA_Character8 *name;
        const MAREA_Character8 *value;
    } MAREA_SemaSetting;

    /**
     * @brief The build settings that the caller gives to sema: the pairs that build_value! reads. A plain
     * struct, caller owned, not opaque, for the same reason as MAREA_SemaTarget. A name that no pair holds
     * gives the default of the macro call.
     */
    typedef struct MAREA_ALIGNED(8) MAREA_SemaSettings
    {
        const MAREA_SemaSetting *pairs;
        MAREA_IntegerSize count;
    } MAREA_SemaSettings;

    /**
     * @brief A file that include_bytes! names, as the caller of sema read it: its bytes, their number, and the path
     * of the file that the C compiler of the generated text can open. path is not NUL terminated; with no path the
     * length is 0 and the C backend cannot write the file. A plain struct, filled by the load function of the
     * caller; sema copies the bytes and the path into its arena.
     */
    typedef struct MAREA_ALIGNED(8) MAREA_SemaIncludedFile
    {
        const MAREA_Byte *bytes;
        MAREA_IntegerSize size;
        const MAREA_Character8 *path;
        MAREA_IntegerSize pathLength;
    } MAREA_SemaIncludedFile;

    /**
     * @brief Function the caller of sema gives so that a program can take the bytes of a file. Sema never reads the
     * disk itself.
     * @param userData Pointer the caller keeps with the function.
     * @param path File path as written in the call, relative to the source file, not NUL terminated.
     * @param pathLength Length of path in bytes.
     * @param outFile Receives the file. Read only when the function returns true. Its pointers must stay valid
     * until the next call of the function.
     * @return True when the file was read, false when it was not.
     */
    typedef MAREA_Boolean(MAREA_CALL * MAREA_SemaIncludeLoadFunction)(MAREA_Null userData, const MAREA_Character8 *path, MAREA_IntegerSize pathLength, MAREA_SemaIncludedFile *outFile);

    /**
     * @brief The include resolver that the caller gives to sema: the one way a registered macro such as
     * include_bytes! learns the size of a file, because sema never reads the disk. A plain struct, caller
     * owned, not opaque, for the same reason as MAREA_SemaTarget.
     */
    typedef struct MAREA_ALIGNED(8) MAREA_SemaIncludeResolver
    {
        MAREA_MacroIncludeSizeFunction size;
        MAREA_Null userData;
        // INFO: gives the bytes of a file, whose number must be the one that size gives. May be null: include_bytes!
        // is then checked by its size alone, and no backend can write its value.
        MAREA_SemaIncludeLoadFunction load;
    } MAREA_SemaIncludeResolver;

    /**
     * @brief Opaque context that drives MAREA_SemaCheckCompilationUnit: the arena, diagnostic sink and
     * import resolver it uses. Never prints, never touches global state, never reads the disk.
     */
    typedef struct MAREA_SemaContext MAREA_SemaContext;

    /**
     * @brief Creates a sema context.
     * @param arena Arena for the working data of the checks. Must not be null.
     * @param sink Diagnostic sink that diagnostics are appended to. Must not be null.
     * @param resolver Import resolver that answers every import. Must not be null.
     * @param outContext Receives the context. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER when an argument is null, or
     * MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY when the arena cannot grow.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_SemaExcept MAREA_CALL MAREA_SemaContextCreate(MAREA_Arena * arena, MAREA_DiagnosticSink * sink, MAREA_SemaImportResolver * resolver, MAREA_SemaContext * *outContext);

    /**
     * @brief Returns the arena the context was created with.
     * @param context Context to query. Null returns null.
     * @return The arena, or null when context is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Arena *MAREA_CALL MAREA_SemaContextGetArena(const MAREA_SemaContext *context);

    /**
     * @brief Returns the diagnostic sink the context was created with.
     * @param context Context to query. Null returns null.
     * @return The sink, or null when context is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_DiagnosticSink *MAREA_CALL MAREA_SemaContextGetDiagnostics(const MAREA_SemaContext *context);

    /**
     * @brief Returns the import resolver the context was created with.
     * @param context Context to query. Null returns null.
     * @return The resolver, or null when context is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_SemaImportResolver *MAREA_CALL MAREA_SemaContextGetImportResolver(const MAREA_SemaContext *context);

    /**
     * @brief Sets the resolver that answers every require: the registry of the names that the host registered
     * on the C side. A context created without one answers a require through the import resolver, so a host
     * with one resolver needs no call. With a registry set, an import never reaches a name that only the
     * registry knows, and a require never reaches a project file that only the import resolver knows.
     * @param context Context to change. Must not be null.
     * @param registry Resolver for the registered names, or null to answer a require through the import
     * resolver again. Its signature function gives the signatures of the registered members.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, or MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER when context is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_SemaExcept MAREA_CALL MAREA_SemaContextSetRequireResolver(MAREA_SemaContext * context, MAREA_SemaImportResolver * registry);

    /**
     * @brief Returns the registry that answers every require.
     * @param context Context to query. Null returns null.
     * @return The registry, or null when none was set or context is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_SemaImportResolver *MAREA_CALL MAREA_SemaContextGetRequireResolver(const MAREA_SemaContext *context);

    /**
     * @brief Sets the type registry sema reads roles and ownership from: the one place a library's
     * type descriptors reach sema, so a NAMED_TYPE whose full path the registry answers is classified by
     * its roles and its ownership class, never by its name. A context with none classifies every such
     * path as it did with no library at all: an ordinary namespace member, EXTERNAL or unknown.
     * @param context Context to change. Must not be null.
     * @param registry Registry to read, or null to stop reading one.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, or MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER when context is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_SemaExcept MAREA_CALL MAREA_SemaContextSetTypeRegistry(MAREA_SemaContext * context, const MAREA_TypeRegistry *registry);

    /**
     * @brief Returns the type registry the context was given.
     * @param context Context to query. Null returns null.
     * @return The registry, or null when none was set or context is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_TypeRegistry *MAREA_CALL MAREA_SemaContextGetTypeRegistry(const MAREA_SemaContext *context);

    /**
     * @brief Sets the target description that a when(...) predicate and a backend are judged against. A
     * context with none answers no predicate (every one is false), resolves the automatic backend to scalar
     * and reports an explicit vector_extension or intrinsics backend as unavailable.
     * @param context Context to change. Must not be null.
     * @param target Description to read, or null to clear it. The caller keeps it valid while the context
     * checks.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, or MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER when context is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_SemaExcept MAREA_CALL MAREA_SemaContextSetTarget(MAREA_SemaContext * context, const MAREA_SemaTarget *target);

    /**
     * @brief Returns the target description the context was given.
     * @param context Context to query. Null returns null.
     * @return The target, or null when none was set or context is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_SemaTarget *MAREA_CALL MAREA_SemaContextGetTarget(const MAREA_SemaContext *context);

    /**
     * @brief Says that the compilation unit is a source file that the language ships. Sema then lets the namespace
     * declaration of the unit start with the reserved segment c or marea. No other rule changes, and an import or a
     * require is checked as before. The default is false: a file of the project must never set this input, only the
     * caller that checks a file of a shipped library does.
     * @param context Context to change. Must not be null.
     * @param value MAREA_BOOLEAN_TRUE for a shipped file, MAREA_BOOLEAN_FALSE for a file of the project.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, or MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER when context is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_SemaExcept MAREA_CALL MAREA_SemaContextSetShippedLibrarySource(MAREA_SemaContext * context, MAREA_Boolean value);

    /**
     * @brief Sets the build settings that build_value! reads. A context with none gives every
     * build_value! call its default.
     * @param context Context to change. Must not be null.
     * @param settings Settings to read, or null to clear them. The caller keeps them valid while the
     * context checks.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, or MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER when context is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_SemaExcept MAREA_CALL MAREA_SemaContextSetSettings(MAREA_SemaContext * context, const MAREA_SemaSettings *settings);

    /**
     * @brief Returns the build settings the context was given.
     * @param context Context to query. Null returns null.
     * @return The settings, or null when none were set or context is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_SemaSettings *MAREA_CALL MAREA_SemaContextGetSettings(const MAREA_SemaContext *context);

    /**
     * @brief Sets the include resolver that a registered macro asks about a file. A context with none
     * reports every file as missing, so a macro that needs one is INVALID_MACRO_ARGUMENT.
     * @param context Context to change. Must not be null.
     * @param resolver Resolver to use, or null to clear it. The caller keeps it valid while the context
     * checks.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, or MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER when context is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_SemaExcept MAREA_CALL MAREA_SemaContextSetIncludeResolver(MAREA_SemaContext * context, const MAREA_SemaIncludeResolver *resolver);

    /**
     * @brief Returns the include resolver the context was given.
     * @param context Context to query. Null returns null.
     * @return The resolver, or null when none was set or context is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_SemaIncludeResolver *MAREA_CALL MAREA_SemaContextGetIncludeResolver(const MAREA_SemaContext *context);

    /**
     * @brief Checks one compilation unit. Every violation is reported to the sink. An ERROR_NODE left by
     * the parser is skipped: no diagnostic is positioned under it.
     * @param context Context that owns the arena, sink and resolver. Must not be null.
     * @param root COMPILATION_UNIT node from MAREA_ParseCompilationUnit. Must not be null.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER when an argument is null, or
     * MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY when the arena cannot grow.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckCompilationUnit(MAREA_SemaContext * context, const MAREA_AstNode *root);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_SEMA_MAREASEMA_H */
