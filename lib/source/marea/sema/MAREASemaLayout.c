// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <string.h>

// INFO: the largest alignment that sema computes with. A larger value is a gap for the backend, so the arithmetic below never overflows.
#define MAREA_G_ALIGNMENT_LIMIT MAREA_UINTEGER_64(4294967296)

typedef struct MAREA_G_PrimitiveWidth
{
    const MAREA_Character8 *name;
    MAREA_Uinteger64 size;
} MAREA_G_PrimitiveWidth;

// INFO: the width of a primitive that has one. Its alignment is its size. A string is not here, because its width is a choice of the backend.
static const MAREA_G_PrimitiveWidth MAREA_G_WIDTHS[] = {
    { .name = "boolean", .size = 1 },
    { .name = "integer8", .size = 1 },
    { .name = "unsigned8", .size = 1 },
    { .name = "integer16", .size = 2 },
    { .name = "unsigned16", .size = 2 },
    { .name = "integer32", .size = 4 },
    { .name = "unsigned32", .size = 4 },
    { .name = "float32", .size = 4 },
    { .name = "integer64", .size = 8 },
    { .name = "unsigned64", .size = 8 },
    { .name = "float64", .size = 8 },
};

typedef struct MAREA_G_Measure
{
    MAREA_Uinteger64 size;
    MAREA_Uinteger64 alignment;
    MAREA_IntegerSize fieldCount;
    MAREA_Boolean cLayout;
} MAREA_G_Measure;

static MAREA_Uinteger64 MAREA_CALL MAREA_G_RoundUp(MAREA_Uinteger64 value, MAREA_Uinteger64 alignment)
{
    return (((value + alignment) - 1) / alignment) * alignment;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsField(const MAREA_AstNode *node)
{
    return ((node != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(node) == MAREA_AST_KIND_FIELD_DECLARATION)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: how deep a structure may hold another one by value. A deeper chain, or a structure that holds itself, is not decided.
#define MAREA_G_DEPTH_LIMIT MAREA_UINTEGER_64(8)

typedef struct MAREA_G_FieldShape
{
    const MAREA_Character8 *name;
    MAREA_IntegerSize length;
    const MAREA_AstNode *declaration;
    MAREA_Uinteger64 size;
    MAREA_Uinteger64 explicitAlignment;
    MAREA_Boolean nested;
} MAREA_G_FieldShape;

// INFO: reads the shape of a field: a primitive that has a width, or a structure of the file. Anything else has no width in sema, so the read says so. A field align is read here too.
static MAREA_Boolean MAREA_CALL MAREA_G_FieldShapeOf(const MAREA_SemaContext *context, const MAREA_AstNode *field, MAREA_G_FieldShape *outShape)
{
    outShape->explicitAlignment = 0;
    outShape->nested = MAREA_BOOLEAN_FALSE;
    outShape->declaration = MAREA_NULLPTR;
    outShape->size = 0;

    const MAREA_AstNode *item = MAREA_SemaAttributeFind(MAREA_NULLPTR, field, "align");

    if ((item != MAREA_NULLPTR) && ((MAREA_SemaAttributeReadInteger(item, &outShape->explicitAlignment) == MAREA_BOOLEAN_FALSE) || (outShape->explicitAlignment == 0) || (outShape->explicitAlignment > MAREA_G_ALIGNMENT_LIMIT))) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_SemaTypeClass CLASS = MAREA_SemaClassifyType(context, MAREA_SemaChildAt(field, (MAREA_AstNodeGetChildCount(field) - 1)), &outShape->declaration, &outShape->name, &outShape->length);

    if ((CLASS == MAREA_SEMA_TYPE_CLASS_USER) && (outShape->declaration != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(outShape->declaration) == MAREA_AST_KIND_STRUCTURE_DECLARATION)) {
        outShape->nested = MAREA_BOOLEAN_TRUE;

        return MAREA_BOOLEAN_TRUE;
    }

    if (CLASS != MAREA_SEMA_TYPE_CLASS_PRIMITIVE) {
        return MAREA_BOOLEAN_FALSE;
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_ARRAY_SIZE(MAREA_G_WIDTHS); ++index) {
        if (MAREA_SemaTextEquals(outShape->name, outShape->length, MAREA_G_WIDTHS[index].name, strlen(MAREA_G_WIDTHS[index].name)) == MAREA_BOOLEAN_TRUE) {
            outShape->size = MAREA_G_WIDTHS[index].size;

            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

// INFO: reads an optional integer attribute. A value that is there and that sema cannot use is not decided. Zero is no value for pack or align.
static MAREA_Boolean MAREA_CALL MAREA_G_ReadLimit(const MAREA_SemaContext *context, const MAREA_AstNode *structure, const MAREA_Character8 *key, MAREA_Uinteger64 *outValue)
{
    *outValue = 0;

    const MAREA_AstNode *item = MAREA_SemaAttributeFind(context->root, structure, key);

    if (item == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_TRUE;
    }

    return ((MAREA_SemaAttributeReadInteger(item, outValue) == MAREA_BOOLEAN_TRUE) && (*outValue > 0) && (*outValue <= MAREA_G_ALIGNMENT_LIMIT)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsCLayout(const MAREA_SemaContext *context, const MAREA_AstNode *structure)
{
    const MAREA_AstNode *item = MAREA_SemaAttributeFind(context->root, structure, "layout");

    const MAREA_AstNode *value = MAREA_SemaChildAt(item, 1);

    const MAREA_AstNode *member = ((value != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(value) == MAREA_AST_KIND_QUALIFIED_NAME) && (MAREA_AstNodeGetChildCount(value) == 1)) ? MAREA_SemaChildAt(value, 0) : MAREA_NULLPTR;

    return ((member != MAREA_NULLPTR) && (MAREA_SemaTextEquals(MAREA_AstNodeGetText(member), MAREA_AstNodeGetTextLength(member), "c", 1) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: the layout of the rules of C: each field at the next multiple of its alignment, a pack caps that alignment, an align raises the one of the structure or of a field. A structure inside a c structure must be c too, because only then has it a size. Returns false when sema cannot size the structure.
static MAREA_Boolean MAREA_CALL MAREA_G_MeasureStructure(const MAREA_SemaContext *context, const MAREA_AstNode *structure, MAREA_Uinteger64 depth, MAREA_G_Measure *outMeasure)
{
    if (depth > MAREA_G_DEPTH_LIMIT) {
        return MAREA_BOOLEAN_FALSE;
    }

    MAREA_Uinteger64 pack = 0;

    MAREA_Uinteger64 align = 0;

    if ((MAREA_G_ReadLimit(context, structure, "pack", &pack) == MAREA_BOOLEAN_FALSE) || (MAREA_G_ReadLimit(context, structure, "align", &align) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_BOOLEAN_FALSE;
    }

    MAREA_Uinteger64 offset = 0;

    outMeasure->alignment = 1;
    outMeasure->fieldCount = 0;
    outMeasure->cLayout = MAREA_G_IsCLayout(context, structure);

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(structure); ++index) {
        const MAREA_AstNode *child = MAREA_SemaChildAt(structure, index);

        if ((child != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(child) == MAREA_AST_KIND_GENERIC_PARAMETERS)) {
            return MAREA_BOOLEAN_FALSE;
        }

        if (MAREA_G_IsField(child) == MAREA_BOOLEAN_FALSE) {
            continue;
        }

        MAREA_G_FieldShape shape = { 0 };

        if (MAREA_G_FieldShapeOf(context, child, &shape) == MAREA_BOOLEAN_FALSE) {
            return MAREA_BOOLEAN_FALSE;
        }

        MAREA_Uinteger64 size = shape.size;

        MAREA_Uinteger64 natural = shape.size;

        if (shape.nested == MAREA_BOOLEAN_TRUE) {
            MAREA_G_Measure inner = { 0 };

            if ((MAREA_G_MeasureStructure(context, shape.declaration, (depth + 1), &inner) == MAREA_BOOLEAN_FALSE) || ((outMeasure->cLayout == MAREA_BOOLEAN_TRUE) && (inner.cLayout == MAREA_BOOLEAN_FALSE))) {
                return MAREA_BOOLEAN_FALSE;
            }

            size = inner.size;
            natural = inner.alignment;
        }

        MAREA_Uinteger64 fieldAlignment = ((pack > 0) && (natural > pack)) ? pack : natural;

        fieldAlignment = (shape.explicitAlignment > fieldAlignment) ? shape.explicitAlignment : fieldAlignment;

        offset = (MAREA_G_RoundUp(offset, fieldAlignment) + size);
        outMeasure->alignment = (fieldAlignment > outMeasure->alignment) ? fieldAlignment : outMeasure->alignment;
        outMeasure->fieldCount += 1;
    }

    outMeasure->alignment = (align > outMeasure->alignment) ? align : outMeasure->alignment;
    outMeasure->size = MAREA_G_RoundUp(offset, outMeasure->alignment);

    return MAREA_BOOLEAN_TRUE;
}

static const MAREA_AstNode *MAREA_CALL MAREA_G_FieldAt(const MAREA_AstNode *structure, MAREA_IntegerSize position)
{
    MAREA_IntegerSize seen = 0;

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(structure); ++index) {
        const MAREA_AstNode *child = MAREA_SemaChildAt(structure, index);

        if (MAREA_G_IsField(child) == MAREA_BOOLEAN_FALSE) {
            continue;
        }

        if (seen == position) {
            return child;
        }

        seen += 1;
    }

    return MAREA_NULLPTR;
}

static MAREA_SemaLayoutVerdict MAREA_CALL MAREA_G_CompareDepth(const MAREA_SemaContext *context, const MAREA_AstNode *from, const MAREA_AstNode *to, MAREA_Uinteger64 depth);

// INFO: the fields in the same place must have the same primitive by exact name, or the same structure layout, and the same field align. A nested pair that is not decided is not decided here either.
static MAREA_SemaLayoutVerdict MAREA_CALL MAREA_G_SameFields(const MAREA_SemaContext *context, const MAREA_AstNode *from, const MAREA_AstNode *to, MAREA_IntegerSize count, MAREA_Uinteger64 depth)
{
    MAREA_SemaLayoutVerdict result = MAREA_SEMA_LAYOUT_VERDICT_COMPATIBLE;

    for (MAREA_IntegerSize position = 0; position < count; ++position) {
        MAREA_G_FieldShape fromShape = { 0 };

        MAREA_G_FieldShape toShape = { 0 };

        // INFO: both fields were measured already, so the reads cannot fail here.
        if ((MAREA_G_FieldShapeOf(context, MAREA_G_FieldAt(from, position), &fromShape) == MAREA_BOOLEAN_FALSE) || (MAREA_G_FieldShapeOf(context, MAREA_G_FieldAt(to, position), &toShape) == MAREA_BOOLEAN_FALSE) || (fromShape.nested != toShape.nested) || (fromShape.explicitAlignment != toShape.explicitAlignment)) {
            return MAREA_SEMA_LAYOUT_VERDICT_INCOMPATIBLE;
        }

        if (fromShape.nested == MAREA_BOOLEAN_FALSE) {
            if (MAREA_SemaTextEquals(fromShape.name, fromShape.length, toShape.name, toShape.length) == MAREA_BOOLEAN_FALSE) {
                return MAREA_SEMA_LAYOUT_VERDICT_INCOMPATIBLE;
            }

            continue;
        }

        const MAREA_SemaLayoutVerdict INNER = MAREA_G_CompareDepth(context, fromShape.declaration, toShape.declaration, (depth + 1));

        if (INNER == MAREA_SEMA_LAYOUT_VERDICT_INCOMPATIBLE) {
            return MAREA_SEMA_LAYOUT_VERDICT_INCOMPATIBLE;
        }

        result = (INNER == MAREA_SEMA_LAYOUT_VERDICT_UNDECIDED) ? MAREA_SEMA_LAYOUT_VERDICT_UNDECIDED : result;
    }

    return result;
}

static MAREA_SemaLayoutVerdict MAREA_CALL MAREA_G_CompareDepth(const MAREA_SemaContext *context, const MAREA_AstNode *from, const MAREA_AstNode *to, MAREA_Uinteger64 depth)
{
    if (from == to) {
        return MAREA_SEMA_LAYOUT_VERDICT_COMPATIBLE;
    }

    MAREA_G_Measure fromMeasure = { 0 };

    MAREA_G_Measure toMeasure = { 0 };

    if ((MAREA_G_MeasureStructure(context, from, depth, &fromMeasure) == MAREA_BOOLEAN_FALSE) || (MAREA_G_MeasureStructure(context, to, depth, &toMeasure) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_SEMA_LAYOUT_VERDICT_UNDECIDED;
    }

    if ((fromMeasure.fieldCount != toMeasure.fieldCount) || (fromMeasure.alignment != toMeasure.alignment)) {
        return MAREA_SEMA_LAYOUT_VERDICT_INCOMPATIBLE;
    }

    const MAREA_SemaLayoutVerdict FIELDS = MAREA_G_SameFields(context, from, to, fromMeasure.fieldCount, depth);

    if (FIELDS != MAREA_SEMA_LAYOUT_VERDICT_COMPATIBLE) {
        return FIELDS;
    }

    if (fromMeasure.cLayout != toMeasure.cLayout) {
        return MAREA_SEMA_LAYOUT_VERDICT_UNDECIDED;
    }

    // INFO: two auto layouts that have the same fields, the same pack and the same align come from the same input, so the backend gives them the same layout. Sema has no size to compare for them.
    if (fromMeasure.cLayout == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_LAYOUT_VERDICT_COMPATIBLE;
    }

    return (fromMeasure.size == toMeasure.size) ? MAREA_SEMA_LAYOUT_VERDICT_COMPATIBLE : MAREA_SEMA_LAYOUT_VERDICT_INCOMPATIBLE;
}

MAREA_SemaLayoutVerdict MAREA_CALL MAREA_SemaCompareStructureLayout(const MAREA_SemaContext *context, const MAREA_AstNode *from, const MAREA_AstNode *to)
{
    if ((context == MAREA_NULLPTR) || (from == MAREA_NULLPTR) || (to == MAREA_NULLPTR)) {
        return MAREA_SEMA_LAYOUT_VERDICT_UNDECIDED;
    }

    return MAREA_G_CompareDepth(context, from, to, 0);
}
