// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_SEMA_MAREASEMALAYOUT_H)
    #define MAREA_INCLUDE_SEMA_MAREASEMALAYOUT_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/sema/MAREASemaTypedAst.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Opaque layout of one structure or one enum: its verdict, its size and alignment in bytes, and the
     * offset of each field (or of the payload of each enum variant). The record is made by
     * MAREA_SemaBuildTypedAst and is valid for the arena lifetime.
     *
     * The layout is the same for both values of the layout key (c and marea): the fields in the order written, each
     * at the next multiple of its own alignment, and the size rounded up to the largest alignment. pack caps the
     * alignment of a member and align raises the alignment of the whole type. The value marea stays reserved so a
     * later change may reorder the fields.
     *
     * An enum is its tag, an integer type (sinteger32 when none is written), and then its data as a union: the size
     * of the largest variant, at the first offset after the tag that fits the largest alignment of a variant. The
     * alignment of the enum is the largest one among the tag and the variants.
     *
     * Every offset counts from the start of the value of the structure or of the enum.
     */
    typedef struct MAREA_SemaTypeLayout MAREA_SemaTypeLayout;

    /**
     * @brief Whether sema measured a layout. A later change may turn UNDECIDED into MEASURED for a type that
     * sema cannot measure today, without a change of this format.
     */
    typedef enum MAREA_SemaMeasureVerdict
    {
        MAREA_SEMA_MEASURE_VERDICT_MEASURED = MAREA_UINTEGER_8(0),
        MAREA_SEMA_MEASURE_VERDICT_UNDECIDED = MAREA_UINTEGER_8(1),
    } MAREA_SemaMeasureVerdict;

    /**
     * @brief Why a layout is undecided. A type that holds an undecided type has the reason of that type.
     */
    typedef enum MAREA_SemaMeasureReason
    {
        MAREA_SEMA_MEASURE_REASON_NONE = MAREA_UINTEGER_8(0),
        MAREA_SEMA_MEASURE_REASON_LIBRARY_TYPE = MAREA_UINTEGER_8(1),
        MAREA_SEMA_MEASURE_REASON_GENERIC = MAREA_UINTEGER_8(2),
        MAREA_SEMA_MEASURE_REASON_FUNCTION_TYPE = MAREA_UINTEGER_8(3),
        MAREA_SEMA_MEASURE_REASON_OPAQUE = MAREA_UINTEGER_8(4),
        MAREA_SEMA_MEASURE_REASON_INVALID_ATTRIBUTE = MAREA_UINTEGER_8(5),
        MAREA_SEMA_MEASURE_REASON_TOO_DEEP = MAREA_UINTEGER_8(6),
        MAREA_SEMA_MEASURE_REASON_UNRESOLVED = MAREA_UINTEGER_8(7),
    } MAREA_SemaMeasureReason;

    /**
     * @brief Returns the layout of a typed STRUCTURE_DECLARATION or ENUM_DECLARATION node.
     * @param node Typed node to query. Must not be null.
     * @param out Receives the layout, valid for the arena lifetime. Must not be null.
     * @return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS, ERROR_NULL_POINTER when node or out is null, or
     * ERROR_NO_LAYOUT when the node is not a structure or an enum declaration.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypedNodeGetLayout(const MAREA_SemaTypedNode *node, const MAREA_SemaTypeLayout **out);

    /**
     * @brief Tells whether a layout was measured.
     * @param layout Layout to query. Null returns MAREA_SEMA_MEASURE_VERDICT_UNDECIDED.
     * @return The verdict.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_SemaMeasureVerdict MAREA_CALL MAREA_SemaTypeLayoutGetVerdict(const MAREA_SemaTypeLayout *layout);

    /**
     * @brief Tells why a layout is undecided.
     * @param layout Layout to query. Null returns MAREA_SEMA_MEASURE_REASON_UNRESOLVED.
     * @return MAREA_SEMA_MEASURE_REASON_NONE for a measured layout, else the reason.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_SemaMeasureReason MAREA_CALL MAREA_SemaTypeLayoutGetReason(const MAREA_SemaTypeLayout *layout);

    /**
     * @brief Returns the size of the type in bytes, with its tail padding.
     * @param layout Layout to query. Must not be null.
     * @param out Receives the size. Must not be null.
     * @return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS, ERROR_NULL_POINTER when layout or out is null, or
     * ERROR_UNDECIDED when the layout is undecided.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypeLayoutGetSize(const MAREA_SemaTypeLayout *layout, MAREA_Uinteger64 *out);

    /**
     * @brief Returns the alignment of the type in bytes.
     * @param layout Layout to query. Must not be null.
     * @param out Receives the alignment. Must not be null.
     * @return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS, ERROR_NULL_POINTER when layout or out is null, or
     * ERROR_UNDECIDED when the layout is undecided.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypeLayoutGetAlignment(const MAREA_SemaTypeLayout *layout, MAREA_Uinteger64 *out);

    /**
     * @brief Returns the size of the tag of an enum in bytes. The tag sits at offset 0.
     * @param layout Layout to query. Must not be null.
     * @param out Receives the tag size, 0 for a structure. Must not be null.
     * @return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS, ERROR_NULL_POINTER when layout or out is null, or
     * ERROR_UNDECIDED when the layout is undecided.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypeLayoutGetTagSize(const MAREA_SemaTypeLayout *layout, MAREA_Uinteger64 *out);

    /**
     * @brief Returns how many members the type declares: the fields of a structure or the variants of an enum.
     * The count is known even when the layout is undecided.
     * @param layout Layout to query. Null returns 0.
     * @return The member count.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_IntegerSize MAREA_CALL MAREA_SemaTypeLayoutGetMemberCount(const MAREA_SemaTypeLayout *layout);

    /**
     * @brief Returns the offset of one member: the offset of a field of a structure, or the offset of the payload
     * of a variant of an enum (the same for every variant, because the data is a union).
     * @param layout Layout to query. Must not be null.
     * @param index Zero-based member index, in the order written.
     * @param out Receives the offset. Must not be null.
     * @return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS, ERROR_NULL_POINTER when layout or out is null or index is not
     * less than the member count, or ERROR_UNDECIDED when the layout is undecided.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypeLayoutGetMemberOffset(const MAREA_SemaTypeLayout *layout, MAREA_IntegerSize index, MAREA_Uinteger64 *out);

    /**
     * @brief Returns how many fields one variant of an enum has.
     * @param layout Layout to query. Null returns 0.
     * @param variantIndex Zero-based variant index.
     * @return The field count, 0 for a structure and for an index that is out of range.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_IntegerSize MAREA_CALL MAREA_SemaTypeLayoutGetVariantFieldCount(const MAREA_SemaTypeLayout *layout, MAREA_IntegerSize variantIndex);

    /**
     * @brief Returns the offset of one field of one variant of an enum, from the start of the enum value. The
     * fields of a variant follow the rules of a structure, starting at the payload offset of the variant.
     * @param layout Layout to query. Must not be null.
     * @param variantIndex Zero-based variant index.
     * @param fieldIndex Zero-based field index inside the variant.
     * @param out Receives the offset. Must not be null.
     * @return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS, ERROR_NULL_POINTER when layout or out is null or an index is out
     * of range, or ERROR_UNDECIDED when the layout is undecided.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypeLayoutGetVariantFieldOffset(const MAREA_SemaTypeLayout *layout, MAREA_IntegerSize variantIndex, MAREA_IntegerSize fieldIndex, MAREA_Uinteger64 *out);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_SEMA_MAREASEMALAYOUT_H */
