// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/ir/MAREAIrInternal.h"

#include <string.h>

static const MAREA_IrLayoutTable *MAREA_CALL MAREA_G_TableOf(const MAREA_IrNode *module)
{
    if ((module == MAREA_NULLPTR) || (module->kind != MAREA_IR_KIND_MODULE)) {
        return MAREA_NULLPTR;
    }

    return module->layouts;
}

MAREA_IntegerSize MAREA_CALL MAREA_IrModuleGetLayoutCount(const MAREA_IrNode *module)
{
    const MAREA_IrLayoutTable *table = MAREA_G_TableOf(module);

    return (table == MAREA_NULLPTR) ? MAREA_INTEGER_SIZE(0) : table->count;
}

MAREA_IrLayoutExcept MAREA_CALL MAREA_IrModuleGetLayoutAt(const MAREA_IrNode *module, MAREA_IntegerSize index, const MAREA_IrLayout **out)
{
    if ((module == MAREA_NULLPTR) || (out == MAREA_NULLPTR)) {
        return MAREA_IR_LAYOUT_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_IrLayoutTable *table = MAREA_G_TableOf(module);

    if ((table == MAREA_NULLPTR) || (index >= table->count)) {
        return MAREA_IR_LAYOUT_EXCEPT_ERROR_OUT_OF_RANGE;
    }

    *out = &table->entries[index];

    return MAREA_IR_LAYOUT_EXCEPT_SUCCESS;
}

MAREA_IrLayoutExcept MAREA_CALL MAREA_IrModuleFindLayout(const MAREA_IrNode *module, const MAREA_Character8 *name, MAREA_IntegerSize nameLength, const MAREA_IrLayout **out)
{
    if ((module == MAREA_NULLPTR) || (name == MAREA_NULLPTR) || (out == MAREA_NULLPTR)) {
        return MAREA_IR_LAYOUT_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_IrLayoutTable *table = MAREA_G_TableOf(module);

    const MAREA_IntegerSize COUNT = (table == MAREA_NULLPTR) ? MAREA_INTEGER_SIZE(0) : table->count;

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_IrLayout *entry = &table->entries[index];

        if ((entry->nameLength == nameLength) && (memcmp(entry->name, name, nameLength) == 0)) {
            *out = entry;

            return MAREA_IR_LAYOUT_EXCEPT_SUCCESS;
        }
    }

    return MAREA_IR_LAYOUT_EXCEPT_ERROR_NOT_FOUND;
}

MAREA_IrLayoutKind MAREA_CALL MAREA_IrLayoutGetKind(const MAREA_IrLayout *layout)
{
    return (layout == MAREA_NULLPTR) ? MAREA_IR_LAYOUT_KIND_COUNT : layout->kind;
}

const MAREA_Character8 *MAREA_CALL MAREA_IrLayoutGetName(const MAREA_IrLayout *layout)
{
    return (layout == MAREA_NULLPTR) ? MAREA_NULLPTR : layout->name;
}

MAREA_IntegerSize MAREA_CALL MAREA_IrLayoutGetNameLength(const MAREA_IrLayout *layout)
{
    return (layout == MAREA_NULLPTR) ? MAREA_INTEGER_SIZE(0) : layout->nameLength;
}

MAREA_SemaMeasureVerdict MAREA_CALL MAREA_IrLayoutGetVerdict(const MAREA_IrLayout *layout)
{
    return (layout == MAREA_NULLPTR) ? MAREA_SEMA_MEASURE_VERDICT_UNDECIDED : layout->verdict;
}

MAREA_SemaMeasureReason MAREA_CALL MAREA_IrLayoutGetReason(const MAREA_IrLayout *layout)
{
    return (layout == MAREA_NULLPTR) ? MAREA_SEMA_MEASURE_REASON_UNRESOLVED : layout->reason;
}

// INFO: reads one number of a measured entry. A null entry or output is a null pointer, and an undecided entry has no number.
static MAREA_IrLayoutExcept MAREA_CALL MAREA_G_ReadNumber(const MAREA_IrLayout *layout, MAREA_Uinteger64 value, MAREA_Uinteger64 *out)
{
    if ((layout == MAREA_NULLPTR) || (out == MAREA_NULLPTR)) {
        return MAREA_IR_LAYOUT_EXCEPT_ERROR_NULL_POINTER;
    }

    if (layout->verdict != MAREA_SEMA_MEASURE_VERDICT_MEASURED) {
        return MAREA_IR_LAYOUT_EXCEPT_ERROR_UNDECIDED;
    }

    *out = value;

    return MAREA_IR_LAYOUT_EXCEPT_SUCCESS;
}

MAREA_IrLayoutExcept MAREA_CALL MAREA_IrLayoutGetSize(const MAREA_IrLayout *layout, MAREA_Uinteger64 *out)
{
    return MAREA_G_ReadNumber(layout, (layout == MAREA_NULLPTR) ? MAREA_UINTEGER_64(0) : layout->size, out);
}

MAREA_IrLayoutExcept MAREA_CALL MAREA_IrLayoutGetAlignment(const MAREA_IrLayout *layout, MAREA_Uinteger64 *out)
{
    return MAREA_G_ReadNumber(layout, (layout == MAREA_NULLPTR) ? MAREA_UINTEGER_64(0) : layout->alignment, out);
}

MAREA_IrLayoutExcept MAREA_CALL MAREA_IrLayoutGetTagSize(const MAREA_IrLayout *layout, MAREA_Uinteger64 *out)
{
    return MAREA_G_ReadNumber(layout, (layout == MAREA_NULLPTR) ? MAREA_UINTEGER_64(0) : layout->tagSize, out);
}

MAREA_IntegerSize MAREA_CALL MAREA_IrLayoutGetMemberCount(const MAREA_IrLayout *layout)
{
    return (layout == MAREA_NULLPTR) ? MAREA_INTEGER_SIZE(0) : layout->memberCount;
}

MAREA_IrLayoutExcept MAREA_CALL MAREA_IrLayoutGetMemberOffset(const MAREA_IrLayout *layout, MAREA_IntegerSize index, MAREA_Uinteger64 *out)
{
    const MAREA_IrLayoutExcept RESULT = MAREA_G_ReadNumber(layout, MAREA_UINTEGER_64(0), out);

    if (RESULT != MAREA_IR_LAYOUT_EXCEPT_SUCCESS) {
        return RESULT;
    }

    if (index >= layout->memberCount) {
        return MAREA_IR_LAYOUT_EXCEPT_ERROR_OUT_OF_RANGE;
    }

    *out = layout->offsets[index];

    return MAREA_IR_LAYOUT_EXCEPT_SUCCESS;
}

MAREA_IntegerSize MAREA_CALL MAREA_IrLayoutGetVariantFieldCount(const MAREA_IrLayout *layout, MAREA_IntegerSize variantIndex)
{
    if ((layout == MAREA_NULLPTR) || (layout->variantFieldCounts == MAREA_NULLPTR) || (variantIndex >= layout->memberCount)) {
        return MAREA_INTEGER_SIZE(0);
    }

    return layout->variantFieldCounts[variantIndex];
}

MAREA_IrLayoutExcept MAREA_CALL MAREA_IrLayoutGetVariantFieldOffset(const MAREA_IrLayout *layout, MAREA_IntegerSize variantIndex, MAREA_IntegerSize fieldIndex, MAREA_Uinteger64 *out)
{
    const MAREA_IrLayoutExcept RESULT = MAREA_G_ReadNumber(layout, MAREA_UINTEGER_64(0), out);

    if (RESULT != MAREA_IR_LAYOUT_EXCEPT_SUCCESS) {
        return RESULT;
    }

    if (fieldIndex >= MAREA_IrLayoutGetVariantFieldCount(layout, variantIndex)) {
        return MAREA_IR_LAYOUT_EXCEPT_ERROR_OUT_OF_RANGE;
    }

    // INFO: the offsets of the fields follow the payload offsets of the variants, one run for each variant in order.
    MAREA_IntegerSize slot = layout->memberCount;

    for (MAREA_IntegerSize variant = MAREA_INTEGER_SIZE(0); variant < variantIndex; variant = (variant + MAREA_INTEGER_SIZE(1))) {
        slot = (slot + layout->variantFieldCounts[variant]);
    }

    *out = layout->offsets[(slot + fieldIndex)];

    return MAREA_IR_LAYOUT_EXCEPT_SUCCESS;
}

// INFO: makes an entry undecided and drops every number it holds, so that it keeps no half of a measure.
static void MAREA_CALL MAREA_G_MakeUndecided(MAREA_IrLayout *entry, MAREA_SemaMeasureReason reason)
{
    entry->verdict = MAREA_SEMA_MEASURE_VERDICT_UNDECIDED;
    entry->reason = reason;
    entry->size = MAREA_UINTEGER_64(0);
    entry->alignment = MAREA_UINTEGER_64(0);
    entry->tagSize = MAREA_UINTEGER_64(0);
    entry->offsets = MAREA_NULLPTR;
    entry->variantFieldCounts = MAREA_NULLPTR;
}

// INFO: copies the numbers of a measured layout of sema. Gives false when sema refuses one, which a measured layout never does.
static MAREA_Boolean MAREA_CALL MAREA_G_CopyNumbers(const MAREA_SemaTypeLayout *source, MAREA_IrLayout *entry)
{
    if ((MAREA_SemaTypeLayoutGetSize(source, &entry->size) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) || (MAREA_SemaTypeLayoutGetAlignment(source, &entry->alignment) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) || (MAREA_SemaTypeLayoutGetTagSize(source, &entry->tagSize) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS)) {
        return MAREA_BOOLEAN_FALSE;
    }

    for (MAREA_IntegerSize member = MAREA_INTEGER_SIZE(0); member < entry->memberCount; member = (member + MAREA_INTEGER_SIZE(1))) {
        if (MAREA_SemaTypeLayoutGetMemberOffset(source, member, &entry->offsets[member]) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
            return MAREA_BOOLEAN_FALSE;
        }
    }

    if (entry->variantFieldCounts == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_TRUE;
    }

    MAREA_IntegerSize slot = entry->memberCount;

    for (MAREA_IntegerSize variant = MAREA_INTEGER_SIZE(0); variant < entry->memberCount; variant = (variant + MAREA_INTEGER_SIZE(1))) {
        for (MAREA_IntegerSize field = MAREA_INTEGER_SIZE(0); field < entry->variantFieldCounts[variant]; field = (field + MAREA_INTEGER_SIZE(1))) {
            if (MAREA_SemaTypeLayoutGetVariantFieldOffset(source, variant, field, &entry->offsets[slot]) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
                return MAREA_BOOLEAN_FALSE;
            }

            slot = (slot + MAREA_INTEGER_SIZE(1));
        }
    }

    return MAREA_BOOLEAN_TRUE;
}

// INFO: reserves the slots of a measured enum or structure: one for each member, and for an enum one for each field of each variant.
static MAREA_IrExcept MAREA_CALL MAREA_G_ReserveSlots(MAREA_IrContext *context, const MAREA_SemaTypeLayout *source, MAREA_IrLayout *entry)
{
    MAREA_IntegerSize slotCount = entry->memberCount;

    MAREA_Null memory = MAREA_NULLPTR;

    if ((entry->kind == MAREA_IR_LAYOUT_KIND_ENUM) && (entry->memberCount > MAREA_INTEGER_SIZE(0))) {
        if (MAREA_ArenaAllocate(context->arena, (entry->memberCount * sizeof(MAREA_IntegerSize)), MAREA_ALIGNOF(MAREA_IntegerSize), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
            return MAREA_IR_EXCEPT_ERROR_OUT_OF_MEMORY;
        }

        entry->variantFieldCounts = memory;

        for (MAREA_IntegerSize variant = MAREA_INTEGER_SIZE(0); variant < entry->memberCount; variant = (variant + MAREA_INTEGER_SIZE(1))) {
            entry->variantFieldCounts[variant] = MAREA_SemaTypeLayoutGetVariantFieldCount(source, variant);
            slotCount = (slotCount + entry->variantFieldCounts[variant]);
        }
    }

    if (slotCount == MAREA_INTEGER_SIZE(0)) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    if (MAREA_ArenaAllocate(context->arena, (slotCount * sizeof(MAREA_Uinteger64)), MAREA_ALIGNOF(MAREA_Uinteger64), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_IR_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    entry->offsets = memory;

    return MAREA_IR_EXCEPT_SUCCESS;
}

// INFO: fills one entry from a lowered declaration. The numbers are the ones sema recorded; nothing is measured here.
static MAREA_IrExcept MAREA_CALL MAREA_G_FillEntry(MAREA_IrContext *context, const MAREA_IrLayoutSource *source, MAREA_IrLayout *entry)
{
    const MAREA_AstKind DECLARATION_KIND = MAREA_IrTypedKind(source->typed);

    entry->name = source->node->text;
    entry->nameLength = (MAREA_IntegerSize)source->node->textLength;
    entry->kind = (DECLARATION_KIND == MAREA_AST_KIND_ENUM_DECLARATION) ? MAREA_IR_LAYOUT_KIND_ENUM : MAREA_IR_LAYOUT_KIND_STRUCTURE;
    entry->memberCount = MAREA_INTEGER_SIZE(0);

    MAREA_G_MakeUndecided(entry, MAREA_SEMA_MEASURE_REASON_UNRESOLVED);

    // INFO: the layout of an opaque structure belongs to C, so sema records none for it.
    if (DECLARATION_KIND == MAREA_AST_KIND_OPAQUE_DECLARATION) {
        entry->reason = MAREA_SEMA_MEASURE_REASON_OPAQUE;

        return MAREA_IR_EXCEPT_SUCCESS;
    }

    const MAREA_SemaTypeLayout *layout = MAREA_NULLPTR;

    if (MAREA_SemaTypedNodeGetLayout(source->typed, &layout) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    entry->memberCount = MAREA_SemaTypeLayoutGetMemberCount(layout);

    if (MAREA_SemaTypeLayoutGetVerdict(layout) != MAREA_SEMA_MEASURE_VERDICT_MEASURED) {
        entry->reason = MAREA_SemaTypeLayoutGetReason(layout);

        return MAREA_IR_EXCEPT_SUCCESS;
    }

    const MAREA_IrExcept RESERVED = MAREA_G_ReserveSlots(context, layout, entry);

    if (RESERVED != MAREA_IR_EXCEPT_SUCCESS) {
        return RESERVED;
    }

    if (MAREA_G_CopyNumbers(layout, entry) == MAREA_BOOLEAN_FALSE) {
        MAREA_G_MakeUndecided(entry, MAREA_SEMA_MEASURE_REASON_UNRESOLVED);

        return MAREA_IR_EXCEPT_SUCCESS;
    }

    entry->verdict = MAREA_SEMA_MEASURE_VERDICT_MEASURED;
    entry->reason = MAREA_SEMA_MEASURE_REASON_NONE;

    return MAREA_IR_EXCEPT_SUCCESS;
}

MAREA_IrExcept MAREA_CALL MAREA_IrLayoutTableBuild(MAREA_IrContext *context, MAREA_IrNode *module, const MAREA_IrLayoutSource *sources, MAREA_IntegerSize count)
{
    if ((context == MAREA_NULLPTR) || (module == MAREA_NULLPTR)) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(context->arena, sizeof(MAREA_IrLayoutTable), MAREA_ALIGNOF(MAREA_IrLayoutTable), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_IR_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_IrLayoutTable *table = memory;

    table->entries = MAREA_NULLPTR;
    table->count = MAREA_INTEGER_SIZE(0);

    if (count > MAREA_INTEGER_SIZE(0)) {
        if (MAREA_ArenaAllocate(context->arena, (count * sizeof(MAREA_IrLayout)), MAREA_ALIGNOF(MAREA_IrLayout), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
            return MAREA_IR_EXCEPT_ERROR_OUT_OF_MEMORY;
        }

        table->entries = memory;
    }

    // INFO: the sources and the children of the module are in the same order. A source whose node is no child any more was dropped, and it gets no entry.
    MAREA_IntegerSize cursor = MAREA_INTEGER_SIZE(0);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < count; index = (index + MAREA_INTEGER_SIZE(1))) {
        MAREA_IntegerSize found = cursor;

        while ((found < module->childCount) && (module->children[found] != sources[index].node)) {
            found = (found + MAREA_INTEGER_SIZE(1));
        }

        if (found == module->childCount) {
            continue;
        }

        cursor = (found + MAREA_INTEGER_SIZE(1));

        const MAREA_IrExcept FILLED = MAREA_G_FillEntry(context, &sources[index], &table->entries[table->count]);

        if (FILLED != MAREA_IR_EXCEPT_SUCCESS) {
            return FILLED;
        }

        table->count = (table->count + MAREA_INTEGER_SIZE(1));
    }

    module->layouts = table;

    return MAREA_IR_EXCEPT_SUCCESS;
}
