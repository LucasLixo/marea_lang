// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include "marea/base/MAREAVectorRuntime.h"

#include <string.h>

// INFO: the largest alignment that sema computes with. A larger value is a gap for the backend, so the arithmetic below never overflows.
#define MAREA_G_ALIGNMENT_LIMIT MAREA_UINTEGER_64(4294967296)

typedef struct MAREA_G_PrimitiveWidth
{
    const MAREA_Character8 *name;
    MAREA_Uinteger64 size;
    MAREA_Boolean isInteger;
} MAREA_G_PrimitiveWidth;

// INFO: the width of a primitive that has one. Its alignment is its size. A string is not here, because it is a library type and its width is a choice of the library.
static const MAREA_G_PrimitiveWidth MAREA_G_WIDTHS[] = {
    { .name = "boolean", .size = 1, .isInteger = MAREA_BOOLEAN_FALSE },
    { .name = "character", .size = 1, .isInteger = MAREA_BOOLEAN_FALSE },
    { .name = "sinteger8", .size = 1, .isInteger = MAREA_BOOLEAN_TRUE },
    { .name = "uinteger8", .size = 1, .isInteger = MAREA_BOOLEAN_TRUE },
    { .name = "sinteger16", .size = 2, .isInteger = MAREA_BOOLEAN_TRUE },
    { .name = "uinteger16", .size = 2, .isInteger = MAREA_BOOLEAN_TRUE },
    { .name = "sinteger32", .size = 4, .isInteger = MAREA_BOOLEAN_TRUE },
    { .name = "uinteger32", .size = 4, .isInteger = MAREA_BOOLEAN_TRUE },
    { .name = "float32", .size = 4, .isInteger = MAREA_BOOLEAN_FALSE },
    { .name = "sinteger64", .size = 8, .isInteger = MAREA_BOOLEAN_TRUE },
    { .name = "uinteger64", .size = 8, .isInteger = MAREA_BOOLEAN_TRUE },
    { .name = "float64", .size = 8, .isInteger = MAREA_BOOLEAN_FALSE },
};

// INFO: the width of the tag of an enum that writes no tag, the type sinteger32.
#define MAREA_G_DEFAULT_TAG_SIZE MAREA_UINTEGER_64(4)

// INFO: how deep a structure or an enum may hold another one by value. A deeper chain, or a type that holds itself, is not decided.
#define MAREA_G_DEPTH_LIMIT MAREA_UINTEGER_64(8)

// INFO: how many declarations one measure may visit. A fan-out of nested structures grows with the power of the depth, so the budget keeps the cost bounded.
#define MAREA_G_MEASURE_BUDGET MAREA_UINTEGER_64(1048576)

// INFO: what one measure knows about a declaration. The layout is measured when the reason is none. The type is comparable when the convert check can use it: a primitive or a structure at every level, and a c structure only holds c structures.
typedef struct MAREA_G_Measure
{
    MAREA_Uinteger64 size;
    MAREA_Uinteger64 alignment;
    MAREA_Uinteger64 tagSize;
    MAREA_IntegerSize memberCount;
    MAREA_SemaMeasureReason reason;
    MAREA_Boolean cLayout;
    MAREA_Boolean comparable;
} MAREA_G_Measure;

typedef struct MAREA_G_State
{
    const MAREA_SemaContext *context;
    MAREA_Uinteger64 remaining;
} MAREA_G_State;

// INFO: where the next member goes. The offset and the alignment grow with each member, and a pack caps the alignment of a member.
typedef struct MAREA_G_Cursor
{
    MAREA_Uinteger64 offset;
    MAREA_Uinteger64 alignment;
    MAREA_Uinteger64 pack;
} MAREA_G_Cursor;

typedef enum MAREA_G_ShapeKind
{
    MAREA_G_SHAPE_KIND_PRIMITIVE = MAREA_UINTEGER_8(0),
    MAREA_G_SHAPE_KIND_STRUCTURE = MAREA_UINTEGER_8(1),
    MAREA_G_SHAPE_KIND_ENUM = MAREA_UINTEGER_8(2),
    MAREA_G_SHAPE_KIND_UNDECIDED = MAREA_UINTEGER_8(3),
} MAREA_G_ShapeKind;

typedef struct MAREA_G_FieldShape
{
    const MAREA_Character8 *name;
    MAREA_IntegerSize length;
    const MAREA_AstNode *declaration;
    MAREA_Uinteger64 size;
    MAREA_Uinteger64 alignment;
    MAREA_Uinteger64 explicitAlignment;
    // INFO: a field that is an inline run holds repeat values of the shape in a row; isRun tells it apart from one value.
    MAREA_Uinteger64 repeat;
    MAREA_G_ShapeKind kind;
    MAREA_SemaMeasureReason reason;
    MAREA_Boolean isRun;
} MAREA_G_FieldShape;

static MAREA_Uinteger64 MAREA_CALL MAREA_G_RoundUp(MAREA_Uinteger64 value, MAREA_Uinteger64 alignment)
{
    return (((value + alignment) - 1) / alignment) * alignment;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsKind(const MAREA_AstNode *node, MAREA_AstKind kind)
{
    return ((node != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(node) == kind)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsField(const MAREA_AstNode *node)
{
    return MAREA_G_IsKind(node, MAREA_AST_KIND_FIELD_DECLARATION);
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsType(const MAREA_AstNode *node)
{
    return ((MAREA_G_IsKind(node, MAREA_AST_KIND_NAMED_TYPE) == MAREA_BOOLEAN_TRUE) || (MAREA_G_IsKind(node, MAREA_AST_KIND_FUNCTION_TYPE) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_IntegerSize MAREA_CALL MAREA_G_CountChildren(const MAREA_AstNode *node, MAREA_AstKind kind)
{
    MAREA_IntegerSize count = 0;

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(node); ++index) {
        count += (MAREA_G_IsKind(MAREA_SemaChildAt(node, index), kind) == MAREA_BOOLEAN_TRUE) ? 1 : 0;
    }

    return count;
}

// INFO: the members of a declaration: the variants of an enum, else the fields of a structure.
static MAREA_IntegerSize MAREA_CALL MAREA_G_CountMembers(const MAREA_AstNode *declaration)
{
    return MAREA_G_CountChildren(declaration, (MAREA_G_IsKind(declaration, MAREA_AST_KIND_ENUM_DECLARATION) == MAREA_BOOLEAN_TRUE) ? MAREA_AST_KIND_VARIANT_DECLARATION : MAREA_AST_KIND_FIELD_DECLARATION);
}

static const MAREA_G_PrimitiveWidth *MAREA_CALL MAREA_G_FindWidth(const MAREA_Character8 *name, MAREA_IntegerSize length)
{
    for (MAREA_IntegerSize index = 0; index < MAREA_ARRAY_SIZE(MAREA_G_WIDTHS); ++index) {
        if (MAREA_SemaTextEquals(name, length, MAREA_G_WIDTHS[index].name, strlen(MAREA_G_WIDTHS[index].name)) == MAREA_BOOLEAN_TRUE) {
            return &MAREA_G_WIDTHS[index];
        }
    }

    return MAREA_NULLPTR;
}

// INFO: sorts one written type: a primitive that has a width, a structure or an enum, or a type that sema does not measure, with the reason. A library type is not measured on purpose: its layout comes from the library.
static void MAREA_CALL MAREA_G_ClassifyType(const MAREA_SemaContext *context, const MAREA_AstNode *type, MAREA_Uinteger64 depth, MAREA_G_FieldShape *outShape)
{
    outShape->name = MAREA_NULLPTR;
    outShape->length = 0;
    outShape->declaration = MAREA_NULLPTR;
    outShape->size = 0;
    outShape->alignment = 0;
    outShape->kind = MAREA_G_SHAPE_KIND_UNDECIDED;
    outShape->reason = MAREA_SEMA_MEASURE_REASON_LIBRARY_TYPE;
    outShape->repeat = 1;
    outShape->isRun = MAREA_BOOLEAN_FALSE;

    if (MAREA_G_IsKind(type, MAREA_AST_KIND_FUNCTION_TYPE) == MAREA_BOOLEAN_TRUE) {
        outShape->reason = MAREA_SEMA_MEASURE_REASON_FUNCTION_TYPE;

        return;
    }

    // INFO: a registered inline run with a measured element and a literal count is measured from its descriptor: the
    // size is the count times the element size and the alignment is the element's. A run of runs multiplies the
    // counts. Any other registered type has the layout its library gives, which sema does not see.
    const MAREA_TypeDescriptor *descriptor = MAREA_NULLPTR;

    if (MAREA_SemaDescriptorOfType(context, type, &descriptor) == MAREA_BOOLEAN_TRUE) {
        MAREA_SemaInlineRun run = { 0 };

        const MAREA_Boolean IS_RUN = MAREA_SemaInlineRunOfType(context, type, &run);

        // INFO: a growable run is its header of three words, whatever its element is.
        if ((IS_RUN == MAREA_BOOLEAN_TRUE) && (run.isGrowable == MAREA_BOOLEAN_TRUE)) {
            outShape->kind = MAREA_G_SHAPE_KIND_PRIMITIVE;
            outShape->reason = MAREA_SEMA_MEASURE_REASON_NONE;
            outShape->size = sizeof(MAREA_VectorHeader);
            outShape->alignment = MAREA_ALIGNOF(MAREA_VectorHeader);

            return;
        }

        if ((depth >= MAREA_G_DEPTH_LIMIT) || (IS_RUN == MAREA_BOOLEAN_FALSE) || (run.hasCount == MAREA_BOOLEAN_FALSE)) {
            return;
        }

        MAREA_G_ClassifyType(context, run.element, (depth + 1), outShape);

        if (outShape->kind == MAREA_G_SHAPE_KIND_UNDECIDED) {
            return;
        }

        if ((outShape->kind == MAREA_G_SHAPE_KIND_PRIMITIVE) && (outShape->alignment == 0)) {
            outShape->alignment = outShape->size;
        }

        outShape->repeat = ((outShape->isRun == MAREA_BOOLEAN_TRUE) ? outShape->repeat : 1) * run.count;
        outShape->isRun = MAREA_BOOLEAN_TRUE;

        return;
    }

    const MAREA_SemaTypeClass CLASS = MAREA_SemaClassifyType(context, type, &outShape->declaration, &outShape->name, &outShape->length);

    if (CLASS == MAREA_SEMA_TYPE_CLASS_GENERIC) {
        outShape->reason = MAREA_SEMA_MEASURE_REASON_GENERIC;

        return;
    }

    if (CLASS == MAREA_SEMA_TYPE_CLASS_USER) {
        outShape->reason = MAREA_SEMA_MEASURE_REASON_UNRESOLVED;

        if (MAREA_G_IsKind(outShape->declaration, MAREA_AST_KIND_STRUCTURE_DECLARATION) == MAREA_BOOLEAN_TRUE) {
            outShape->kind = MAREA_G_SHAPE_KIND_STRUCTURE;
            outShape->reason = MAREA_SEMA_MEASURE_REASON_NONE;
        } else if (MAREA_G_IsKind(outShape->declaration, MAREA_AST_KIND_ENUM_DECLARATION) == MAREA_BOOLEAN_TRUE) {
            const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, outShape->name, outShape->length);

            // INFO: an enum bound by an import belongs to another file, whose names and file attributes this context cannot see, so it is not measured.
            const MAREA_Boolean IS_IMPORTED = ((entry != MAREA_NULLPTR) && (entry->kind == MAREA_SEMA_NAME_KIND_IMPORTED_SYMBOL)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

            outShape->kind = (IS_IMPORTED == MAREA_BOOLEAN_TRUE) ? MAREA_G_SHAPE_KIND_UNDECIDED : MAREA_G_SHAPE_KIND_ENUM;
            outShape->reason = (IS_IMPORTED == MAREA_BOOLEAN_TRUE) ? MAREA_SEMA_MEASURE_REASON_UNRESOLVED : MAREA_SEMA_MEASURE_REASON_NONE;
        } else if (MAREA_G_IsKind(outShape->declaration, MAREA_AST_KIND_OPAQUE_DECLARATION) == MAREA_BOOLEAN_TRUE) {
            outShape->reason = MAREA_SEMA_MEASURE_REASON_OPAQUE;
        }

        return;
    }

    const MAREA_G_PrimitiveWidth *width = (CLASS == MAREA_SEMA_TYPE_CLASS_PRIMITIVE) ? MAREA_G_FindWidth(outShape->name, outShape->length) : MAREA_NULLPTR;

    if (width != MAREA_NULLPTR) {
        outShape->kind = MAREA_G_SHAPE_KIND_PRIMITIVE;
        outShape->reason = MAREA_SEMA_MEASURE_REASON_NONE;
        outShape->size = width->size;
    }
}

// INFO: reads the shape of a field of a structure: its type, and its own align. A field align that sema cannot use is not decided.
static void MAREA_CALL MAREA_G_FieldShapeOf(const MAREA_SemaContext *context, const MAREA_AstNode *field, MAREA_G_FieldShape *outShape)
{
    MAREA_G_ClassifyType(context, MAREA_SemaChildAt(field, (MAREA_AstNodeGetChildCount(field) - 1)), 0, outShape);

    outShape->explicitAlignment = 0;

    const MAREA_AstNode *item = MAREA_SemaAttributeFind(MAREA_NULLPTR, field, "align");

    if ((item != MAREA_NULLPTR) && ((MAREA_SemaAttributeReadInteger(item, &outShape->explicitAlignment) == MAREA_BOOLEAN_FALSE) || (outShape->explicitAlignment == 0) || (outShape->explicitAlignment > MAREA_G_ALIGNMENT_LIMIT))) {
        outShape->kind = MAREA_G_SHAPE_KIND_UNDECIDED;
        outShape->reason = MAREA_SEMA_MEASURE_REASON_INVALID_ATTRIBUTE;
    }
}

// INFO: reads an optional integer attribute. A value that is there and that sema cannot use is not decided. Zero is no value for pack or align.
static MAREA_Boolean MAREA_CALL MAREA_G_ReadLimit(const MAREA_SemaContext *context, const MAREA_AstNode *declaration, const MAREA_Character8 *key, MAREA_Uinteger64 *outValue)
{
    *outValue = 0;

    const MAREA_AstNode *item = MAREA_SemaAttributeFind(context->root, declaration, key);

    if (item == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_TRUE;
    }

    return ((MAREA_SemaAttributeReadInteger(item, outValue) == MAREA_BOOLEAN_TRUE) && (*outValue > 0) && (*outValue <= MAREA_G_ALIGNMENT_LIMIT)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsCLayout(const MAREA_SemaContext *context, const MAREA_AstNode *declaration)
{
    const MAREA_AstNode *item = MAREA_SemaAttributeFind(context->root, declaration, "layout");

    const MAREA_AstNode *value = MAREA_SemaChildAt(item, 1);

    const MAREA_AstNode *member = ((value != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(value) == MAREA_AST_KIND_QUALIFIED_NAME) && (MAREA_AstNodeGetChildCount(value) == 1)) ? MAREA_SemaChildAt(value, 0) : MAREA_NULLPTR;

    return ((member != MAREA_NULLPTR) && (MAREA_SemaTextEquals(MAREA_AstNodeGetText(member), MAREA_AstNodeGetTextLength(member), "c", 1) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: puts one member at the next multiple of its alignment: its natural one, capped by the pack, then raised by its own align. Returns the offset.
static MAREA_Uinteger64 MAREA_CALL MAREA_G_Place(MAREA_G_Cursor *cursor, MAREA_Uinteger64 size, MAREA_Uinteger64 natural, MAREA_Uinteger64 explicitAlignment)
{
    MAREA_Uinteger64 alignment = ((cursor->pack > 0) && (natural > cursor->pack)) ? cursor->pack : natural;

    alignment = (explicitAlignment > alignment) ? explicitAlignment : alignment;

    const MAREA_Uinteger64 OFFSET = MAREA_G_RoundUp(cursor->offset, alignment);

    cursor->offset = (OFFSET + size);
    cursor->alignment = (alignment > cursor->alignment) ? alignment : cursor->alignment;

    return OFFSET;
}

static void MAREA_CALL MAREA_G_MeasureDeclaration(MAREA_G_State *state, const MAREA_AstNode *declaration, MAREA_Uinteger64 depth, MAREA_Uinteger64 *slots, MAREA_G_Measure *outMeasure);

// INFO: puts a member of a given shape in the parent. A structure or an enum held by value is measured one level deeper. Returns false, with the reason in the parent, when the member is not decided.
static MAREA_Boolean MAREA_CALL MAREA_G_PlaceShape(MAREA_G_State *state, MAREA_G_Measure *parent, MAREA_G_Cursor *cursor, const MAREA_G_FieldShape *shape, MAREA_Uinteger64 depth, MAREA_Uinteger64 *outOffset)
{
    if (shape->kind == MAREA_G_SHAPE_KIND_UNDECIDED) {
        parent->reason = shape->reason;

        return MAREA_BOOLEAN_FALSE;
    }

    MAREA_Uinteger64 size = shape->size;

    MAREA_Uinteger64 natural = (shape->alignment > 0) ? shape->alignment : shape->size;

    if (shape->kind != MAREA_G_SHAPE_KIND_PRIMITIVE) {
        MAREA_G_Measure inner = { 0 };

        MAREA_G_MeasureDeclaration(state, shape->declaration, (depth + 1), MAREA_NULLPTR, &inner);

        if (inner.reason != MAREA_SEMA_MEASURE_REASON_NONE) {
            parent->reason = inner.reason;

            return MAREA_BOOLEAN_FALSE;
        }

        size = inner.size;
        natural = inner.alignment;

        // INFO: the convert check compares primitives and structures only, and only a c structure inside a c structure has a size to compare.
        if ((shape->kind == MAREA_G_SHAPE_KIND_ENUM) || (inner.comparable == MAREA_BOOLEAN_FALSE) || ((parent->cLayout == MAREA_BOOLEAN_TRUE) && (inner.cLayout == MAREA_BOOLEAN_FALSE))) {
            parent->comparable = MAREA_BOOLEAN_FALSE;
        }
    }

    // INFO: the convert check does not compare an inline run.
    if (shape->isRun == MAREA_BOOLEAN_TRUE) {
        parent->comparable = MAREA_BOOLEAN_FALSE;
    }

    *outOffset = MAREA_G_Place(cursor, (size * shape->repeat), natural, shape->explicitAlignment);

    return MAREA_BOOLEAN_TRUE;
}

// INFO: the layout of the rules of C, for both values of the layout key: each field at the next multiple of its alignment, a pack caps that alignment, an align raises the one of the structure or of a field. The fields stay in the order written. The offset of each field goes to slots when it is given.
static void MAREA_CALL MAREA_G_MeasureStructure(MAREA_G_State *state, const MAREA_AstNode *structure, MAREA_Uinteger64 depth, MAREA_Uinteger64 pack, MAREA_Uinteger64 align, MAREA_Uinteger64 *slots, MAREA_G_Measure *outMeasure)
{
    MAREA_G_Cursor cursor = { .offset = 0, .alignment = 1, .pack = pack };

    MAREA_IntegerSize slot = 0;

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(structure); ++index) {
        const MAREA_AstNode *child = MAREA_SemaChildAt(structure, index);

        if (MAREA_G_IsKind(child, MAREA_AST_KIND_GENERIC_PARAMETERS) == MAREA_BOOLEAN_TRUE) {
            outMeasure->reason = MAREA_SEMA_MEASURE_REASON_GENERIC;

            return;
        }

        if (MAREA_G_IsField(child) == MAREA_BOOLEAN_FALSE) {
            continue;
        }

        MAREA_G_FieldShape shape = { 0 };

        MAREA_Uinteger64 offset = 0;

        MAREA_G_FieldShapeOf(state->context, child, &shape);

        if (MAREA_G_PlaceShape(state, outMeasure, &cursor, &shape, depth, &offset) == MAREA_BOOLEAN_FALSE) {
            return;
        }

        if (slots != MAREA_NULLPTR) {
            slots[slot] = offset;
        }

        slot += 1;
    }

    outMeasure->alignment = (align > cursor.alignment) ? align : cursor.alignment;
    outMeasure->size = MAREA_G_RoundUp(cursor.offset, outMeasure->alignment);
}

// INFO: the type of a variant field is its first type child, because a default value may follow it.
static const MAREA_AstNode *MAREA_CALL MAREA_G_VariantFieldType(const MAREA_AstNode *field)
{
    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(field); ++index) {
        const MAREA_AstNode *child = MAREA_SemaChildAt(field, index);

        if (MAREA_G_IsType(child) == MAREA_BOOLEAN_TRUE) {
            return child;
        }
    }

    return MAREA_NULLPTR;
}

// INFO: reads the width of the tag of an enum: the written integer type, else sinteger32. The tag is the first type child of the enum.
static MAREA_Boolean MAREA_CALL MAREA_G_ReadTagSize(const MAREA_SemaContext *context, const MAREA_AstNode *enumeration, MAREA_Uinteger64 *outSize)
{
    *outSize = MAREA_G_DEFAULT_TAG_SIZE;

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(enumeration); ++index) {
        const MAREA_AstNode *child = MAREA_SemaChildAt(enumeration, index);

        if (MAREA_G_IsType(child) == MAREA_BOOLEAN_FALSE) {
            continue;
        }

        MAREA_G_FieldShape shape = { 0 };

        MAREA_G_ClassifyType(context, child, 0, &shape);

        const MAREA_G_PrimitiveWidth *width = (shape.kind == MAREA_G_SHAPE_KIND_PRIMITIVE) ? MAREA_G_FindWidth(shape.name, shape.length) : MAREA_NULLPTR;

        if ((width == MAREA_NULLPTR) || (width->isInteger == MAREA_BOOLEAN_FALSE)) {
            return MAREA_BOOLEAN_FALSE;
        }

        *outSize = width->size;

        return MAREA_BOOLEAN_TRUE;
    }

    return MAREA_BOOLEAN_TRUE;
}

// INFO: an enum is its tag at offset 0, then its data as a union at the first offset after the tag that fits the largest alignment of a variant. The union is as large as the largest variant, and each variant is laid out as a structure. The slots hold the payload offset of each variant, then the offset of each variant field.
static void MAREA_CALL MAREA_G_MeasureEnum(MAREA_G_State *state, const MAREA_AstNode *enumeration, MAREA_Uinteger64 depth, MAREA_Uinteger64 pack, MAREA_Uinteger64 align, MAREA_Uinteger64 *slots, MAREA_G_Measure *outMeasure)
{
    MAREA_Uinteger64 tagSize = 0;

    if (MAREA_G_ReadTagSize(state->context, enumeration, &tagSize) == MAREA_BOOLEAN_FALSE) {
        outMeasure->reason = MAREA_SEMA_MEASURE_REASON_UNRESOLVED;

        return;
    }

    MAREA_Uinteger64 payloadAlignment = 1;

    MAREA_Uinteger64 payloadSize = 0;

    MAREA_IntegerSize fieldSlot = outMeasure->memberCount;

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(enumeration); ++index) {
        const MAREA_AstNode *variant = MAREA_SemaChildAt(enumeration, index);

        if (MAREA_G_IsKind(variant, MAREA_AST_KIND_VARIANT_DECLARATION) == MAREA_BOOLEAN_FALSE) {
            continue;
        }

        MAREA_G_Cursor cursor = { .offset = 0, .alignment = 1, .pack = pack };

        for (MAREA_IntegerSize fieldIndex = 0; fieldIndex < MAREA_AstNodeGetChildCount(variant); ++fieldIndex) {
            const MAREA_AstNode *field = MAREA_SemaChildAt(variant, fieldIndex);

            if (MAREA_G_IsKind(field, MAREA_AST_KIND_VARIANT_FIELD) == MAREA_BOOLEAN_FALSE) {
                continue;
            }

            MAREA_G_FieldShape shape = { 0 };

            MAREA_Uinteger64 offset = 0;

            MAREA_G_ClassifyType(state->context, MAREA_G_VariantFieldType(field), 0, &shape);

            if (MAREA_G_PlaceShape(state, outMeasure, &cursor, &shape, depth, &offset) == MAREA_BOOLEAN_FALSE) {
                return;
            }

            if (slots != MAREA_NULLPTR) {
                slots[fieldSlot] = offset;
            }

            fieldSlot += 1;
        }

        const MAREA_Uinteger64 VARIANT_SIZE = MAREA_G_RoundUp(cursor.offset, cursor.alignment);

        payloadSize = (VARIANT_SIZE > payloadSize) ? VARIANT_SIZE : payloadSize;
        payloadAlignment = (cursor.alignment > payloadAlignment) ? cursor.alignment : payloadAlignment;
    }

    const MAREA_Uinteger64 TAG_ALIGNMENT = ((pack > 0) && (tagSize > pack)) ? pack : tagSize;

    const MAREA_Uinteger64 PAYLOAD_OFFSET = MAREA_G_RoundUp(tagSize, payloadAlignment);

    MAREA_Uinteger64 alignment = (TAG_ALIGNMENT > payloadAlignment) ? TAG_ALIGNMENT : payloadAlignment;

    alignment = (align > alignment) ? align : alignment;

    outMeasure->tagSize = tagSize;
    outMeasure->alignment = alignment;
    outMeasure->size = MAREA_G_RoundUp((PAYLOAD_OFFSET + payloadSize), alignment);

    if (slots == MAREA_NULLPTR) {
        return;
    }

    // INFO: every variant field was placed from the start of its payload, so the payload offset moves them to the start of the value.
    for (MAREA_IntegerSize slot = 0; slot < outMeasure->memberCount; ++slot) {
        slots[slot] = PAYLOAD_OFFSET;
    }

    for (MAREA_IntegerSize slot = outMeasure->memberCount; slot < fieldSlot; ++slot) {
        slots[slot] += PAYLOAD_OFFSET;
    }
}

static void MAREA_CALL MAREA_G_MeasureDeclaration(MAREA_G_State *state, const MAREA_AstNode *declaration, MAREA_Uinteger64 depth, MAREA_Uinteger64 *slots, MAREA_G_Measure *outMeasure)
{
    const MAREA_Boolean IS_ENUM = MAREA_G_IsKind(declaration, MAREA_AST_KIND_ENUM_DECLARATION);

    outMeasure->size = 0;
    outMeasure->alignment = 1;
    outMeasure->tagSize = 0;
    outMeasure->memberCount = MAREA_G_CountMembers(declaration);
    outMeasure->reason = MAREA_SEMA_MEASURE_REASON_NONE;
    outMeasure->cLayout = MAREA_G_IsCLayout(state->context, declaration);
    outMeasure->comparable = (IS_ENUM == MAREA_BOOLEAN_TRUE) ? MAREA_BOOLEAN_FALSE : MAREA_BOOLEAN_TRUE;

    if ((depth > MAREA_G_DEPTH_LIMIT) || (state->remaining == 0)) {
        outMeasure->reason = MAREA_SEMA_MEASURE_REASON_TOO_DEEP;

        return;
    }

    state->remaining -= 1;

    MAREA_Uinteger64 pack = 0;

    MAREA_Uinteger64 align = 0;

    if ((MAREA_G_ReadLimit(state->context, declaration, "pack", &pack) == MAREA_BOOLEAN_FALSE) || (MAREA_G_ReadLimit(state->context, declaration, "align", &align) == MAREA_BOOLEAN_FALSE)) {
        outMeasure->reason = MAREA_SEMA_MEASURE_REASON_INVALID_ATTRIBUTE;

        return;
    }

    if (IS_ENUM == MAREA_BOOLEAN_TRUE) {
        MAREA_G_MeasureEnum(state, declaration, depth, pack, align, slots, outMeasure);
    } else {
        MAREA_G_MeasureStructure(state, declaration, depth, pack, align, slots, outMeasure);
    }
}

// INFO: the measure that the convert check uses: false when the structure is not decided or when a field is an enum, which the check does not compare.
static MAREA_Boolean MAREA_CALL MAREA_G_MeasureComparable(const MAREA_SemaContext *context, const MAREA_AstNode *structure, MAREA_Uinteger64 depth, MAREA_G_Measure *outMeasure)
{
    MAREA_G_State state = { .context = context, .remaining = MAREA_G_MEASURE_BUDGET };

    MAREA_G_MeasureDeclaration(&state, structure, depth, MAREA_NULLPTR, outMeasure);

    return ((outMeasure->reason == MAREA_SEMA_MEASURE_REASON_NONE) && (outMeasure->comparable == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

MAREA_Boolean MAREA_CALL MAREA_SemaMeasureWrittenType(const MAREA_SemaContext *context, const MAREA_AstNode *type, MAREA_Uinteger64 *outSize, MAREA_Uinteger64 *outAlignment)
{
    if ((context == MAREA_NULLPTR) || (type == MAREA_NULLPTR) || (outSize == MAREA_NULLPTR) || (outAlignment == MAREA_NULLPTR)) {
        return MAREA_BOOLEAN_FALSE;
    }

    MAREA_G_FieldShape shape = { 0 };

    MAREA_G_ClassifyType(context, type, 0, &shape);

    if (shape.kind == MAREA_G_SHAPE_KIND_PRIMITIVE) {
        *outSize = shape.size * shape.repeat;
        *outAlignment = (shape.alignment > 0) ? shape.alignment : shape.size;

        return MAREA_BOOLEAN_TRUE;
    }

    if ((shape.kind != MAREA_G_SHAPE_KIND_STRUCTURE) && (shape.kind != MAREA_G_SHAPE_KIND_ENUM)) {
        return MAREA_BOOLEAN_FALSE;
    }

    MAREA_G_State state = { .context = context, .remaining = MAREA_G_MEASURE_BUDGET };

    MAREA_G_Measure measure = { 0 };

    MAREA_G_MeasureDeclaration(&state, shape.declaration, 0, MAREA_NULLPTR, &measure);

    if (measure.reason != MAREA_SEMA_MEASURE_REASON_NONE) {
        return MAREA_BOOLEAN_FALSE;
    }

    *outSize = measure.size * shape.repeat;
    *outAlignment = measure.alignment;

    return MAREA_BOOLEAN_TRUE;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaMeasureDeclaration(MAREA_SemaContext *context, const MAREA_AstNode *declaration, const MAREA_SemaTypeLayout **outLayout)
{
    if ((context == MAREA_NULLPTR) || (declaration == MAREA_NULLPTR) || (outLayout == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_Boolean IS_ENUM = MAREA_G_IsKind(declaration, MAREA_AST_KIND_ENUM_DECLARATION);

    if ((IS_ENUM == MAREA_BOOLEAN_FALSE) && (MAREA_G_IsKind(declaration, MAREA_AST_KIND_STRUCTURE_DECLARATION) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_IntegerSize MEMBER_COUNT = MAREA_G_CountMembers(declaration);

    MAREA_IntegerSize slotCount = MEMBER_COUNT;

    for (MAREA_IntegerSize index = 0; ((IS_ENUM == MAREA_BOOLEAN_TRUE) && (index < MAREA_AstNodeGetChildCount(declaration))); ++index) {
        slotCount += MAREA_G_CountChildren(MAREA_SemaChildAt(declaration, index), MAREA_AST_KIND_VARIANT_FIELD);
    }

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(context->arena, sizeof(MAREA_SemaTypeLayout), MAREA_ALIGNOF(MAREA_SemaTypeLayout), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_SemaTypeLayout *layout = memory;

    layout->offsets = MAREA_NULLPTR;
    layout->variantFieldCounts = MAREA_NULLPTR;

    if (slotCount > 0) {
        if (MAREA_ArenaAllocate(context->arena, (slotCount * sizeof(MAREA_Uinteger64)), MAREA_ALIGNOF(MAREA_Uinteger64), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
            return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
        }

        layout->offsets = memory;

        memset(layout->offsets, 0, (slotCount * sizeof(MAREA_Uinteger64)));
    }

    if ((IS_ENUM == MAREA_BOOLEAN_TRUE) && (MEMBER_COUNT > 0)) {
        if (MAREA_ArenaAllocate(context->arena, (MEMBER_COUNT * sizeof(MAREA_IntegerSize)), MAREA_ALIGNOF(MAREA_IntegerSize), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
            return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
        }

        layout->variantFieldCounts = memory;

        MAREA_IntegerSize variant = 0;

        for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(declaration); ++index) {
            const MAREA_AstNode *child = MAREA_SemaChildAt(declaration, index);

            if (MAREA_G_IsKind(child, MAREA_AST_KIND_VARIANT_DECLARATION) == MAREA_BOOLEAN_TRUE) {
                layout->variantFieldCounts[variant] = MAREA_G_CountChildren(child, MAREA_AST_KIND_VARIANT_FIELD);
                variant += 1;
            }
        }
    }

    MAREA_G_State state = { .context = context, .remaining = MAREA_G_MEASURE_BUDGET };

    MAREA_G_Measure measure = { 0 };

    MAREA_G_MeasureDeclaration(&state, declaration, 0, layout->offsets, &measure);

    layout->size = measure.size;
    layout->alignment = measure.alignment;
    layout->tagSize = measure.tagSize;
    layout->memberCount = measure.memberCount;
    layout->reason = measure.reason;
    *outLayout = layout;

    return MAREA_SEMA_EXCEPT_SUCCESS;
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

        MAREA_G_FieldShapeOf(context, MAREA_G_FieldAt(from, position), &fromShape);
        MAREA_G_FieldShapeOf(context, MAREA_G_FieldAt(to, position), &toShape);

        // INFO: both fields were measured already and none is an enum, so each one is a primitive or a structure here.
        if ((fromShape.kind != toShape.kind) || (fromShape.explicitAlignment != toShape.explicitAlignment)) {
            return MAREA_SEMA_LAYOUT_VERDICT_INCOMPATIBLE;
        }

        if (fromShape.kind == MAREA_G_SHAPE_KIND_PRIMITIVE) {
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

    if ((MAREA_G_MeasureComparable(context, from, depth, &fromMeasure) == MAREA_BOOLEAN_FALSE) || (MAREA_G_MeasureComparable(context, to, depth, &toMeasure) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_SEMA_LAYOUT_VERDICT_UNDECIDED;
    }

    if ((fromMeasure.memberCount != toMeasure.memberCount) || (fromMeasure.alignment != toMeasure.alignment)) {
        return MAREA_SEMA_LAYOUT_VERDICT_INCOMPATIBLE;
    }

    const MAREA_SemaLayoutVerdict FIELDS = MAREA_G_SameFields(context, from, to, fromMeasure.memberCount, depth);

    if (FIELDS != MAREA_SEMA_LAYOUT_VERDICT_COMPATIBLE) {
        return FIELDS;
    }

    if (fromMeasure.cLayout != toMeasure.cLayout) {
        return MAREA_SEMA_LAYOUT_VERDICT_UNDECIDED;
    }

    // INFO: two marea layouts that have the same fields, the same pack and the same align come from the same input, so the backend gives them the same layout. Sema has no size to compare for them.
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
