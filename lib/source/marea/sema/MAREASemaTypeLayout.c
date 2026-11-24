// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

static MAREA_Boolean MAREA_CALL MAREA_G_IsMeasured(const MAREA_SemaTypeLayout *layout)
{
    return (layout->reason == MAREA_SEMA_MEASURE_REASON_NONE) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: reads one number of a measured layout. A null layout or output is a null pointer, and an undecided layout has no number.
static MAREA_SemaTypedAstExcept MAREA_CALL MAREA_G_ReadNumber(const MAREA_SemaTypeLayout *layout, MAREA_Uinteger64 value, MAREA_Uinteger64 *out)
{
    if ((layout == MAREA_NULLPTR) || (out == MAREA_NULLPTR)) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    if (MAREA_G_IsMeasured(layout) == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_UNDECIDED;
    }

    *out = value;

    return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
}

MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypedNodeGetLayout(const MAREA_SemaTypedNode *node, const MAREA_SemaTypeLayout **out)
{
    if ((node == MAREA_NULLPTR) || (out == MAREA_NULLPTR)) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    if (node->layout == MAREA_NULLPTR) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NO_LAYOUT;
    }

    *out = node->layout;

    return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
}

MAREA_SemaMeasureVerdict MAREA_CALL MAREA_SemaTypeLayoutGetVerdict(const MAREA_SemaTypeLayout *layout)
{
    return ((layout != MAREA_NULLPTR) && (MAREA_G_IsMeasured(layout) == MAREA_BOOLEAN_TRUE)) ? MAREA_SEMA_MEASURE_VERDICT_MEASURED : MAREA_SEMA_MEASURE_VERDICT_UNDECIDED;
}

MAREA_SemaMeasureReason MAREA_CALL MAREA_SemaTypeLayoutGetReason(const MAREA_SemaTypeLayout *layout)
{
    return (layout == MAREA_NULLPTR) ? MAREA_SEMA_MEASURE_REASON_UNRESOLVED : layout->reason;
}

MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypeLayoutGetSize(const MAREA_SemaTypeLayout *layout, MAREA_Uinteger64 *out)
{
    return MAREA_G_ReadNumber(layout, (layout == MAREA_NULLPTR) ? 0 : layout->size, out);
}

MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypeLayoutGetAlignment(const MAREA_SemaTypeLayout *layout, MAREA_Uinteger64 *out)
{
    return MAREA_G_ReadNumber(layout, (layout == MAREA_NULLPTR) ? 0 : layout->alignment, out);
}

MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypeLayoutGetTagSize(const MAREA_SemaTypeLayout *layout, MAREA_Uinteger64 *out)
{
    return MAREA_G_ReadNumber(layout, (layout == MAREA_NULLPTR) ? 0 : layout->tagSize, out);
}

MAREA_IntegerSize MAREA_CALL MAREA_SemaTypeLayoutGetMemberCount(const MAREA_SemaTypeLayout *layout)
{
    return (layout == MAREA_NULLPTR) ? 0 : layout->memberCount;
}

MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypeLayoutGetMemberOffset(const MAREA_SemaTypeLayout *layout, MAREA_IntegerSize index, MAREA_Uinteger64 *out)
{
    const MAREA_SemaTypedAstExcept RESULT = MAREA_G_ReadNumber(layout, 0, out);

    if (RESULT != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
        return RESULT;
    }

    if (index >= layout->memberCount) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    *out = layout->offsets[index];

    return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
}

MAREA_IntegerSize MAREA_CALL MAREA_SemaTypeLayoutGetVariantFieldCount(const MAREA_SemaTypeLayout *layout, MAREA_IntegerSize variantIndex)
{
    if ((layout == MAREA_NULLPTR) || (layout->variantFieldCounts == MAREA_NULLPTR) || (variantIndex >= layout->memberCount)) {
        return 0;
    }

    return layout->variantFieldCounts[variantIndex];
}

MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypeLayoutGetVariantFieldOffset(const MAREA_SemaTypeLayout *layout, MAREA_IntegerSize variantIndex, MAREA_IntegerSize fieldIndex, MAREA_Uinteger64 *out)
{
    const MAREA_SemaTypedAstExcept RESULT = MAREA_G_ReadNumber(layout, 0, out);

    if (RESULT != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
        return RESULT;
    }

    if (fieldIndex >= MAREA_SemaTypeLayoutGetVariantFieldCount(layout, variantIndex)) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    // INFO: the offsets of the fields follow the payload offsets of the variants, one run for each variant in order.
    MAREA_IntegerSize slot = layout->memberCount;

    for (MAREA_IntegerSize variant = 0; variant < variantIndex; ++variant) {
        slot += layout->variantFieldCounts[variant];
    }

    *out = layout->offsets[(slot + fieldIndex)];

    return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
}
