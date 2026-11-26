// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

// INFO: the C definitions of the composite types: the layout of sema, written with explicit padding and asserted.
#include "marea/internal/cgen/MAREACgenInternal.h"

static MAREA_Uinteger64 MAREA_CALL MAREA_G_RoundUp(MAREA_Uinteger64 value, MAREA_Uinteger64 alignment)
{
    if (alignment <= MAREA_UINTEGER_64(1)) {
        return value;
    }

    return (((value + (alignment - MAREA_UINTEGER_64(1))) / alignment) * alignment);
}

// INFO: writes the fields of one C struct from a base offset, with the padding that places each one at its offset. outEnd receives the offset after the last field, from the base.
static void MAREA_CALL MAREA_G_WriteFields(MAREA_CgenGenerator *generator, const MAREA_CgenField *fields, MAREA_IntegerSize count, MAREA_Uinteger64 base, const MAREA_Character8 *indent, MAREA_Uinteger64 *ioPadding, MAREA_Uinteger64 *outEnd)
{
    MAREA_Uinteger64 position = MAREA_UINTEGER_64(0);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < count; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_Uinteger64 RELATIVE = (fields[index].offset - base);

        if (RELATIVE > position) {
            MAREA_CGEN_WRITE(generator->types, indent, "uint8_t marea_padding_", MAREA_CgenNumber(generator, *ioPadding), "[", MAREA_CgenNumber(generator, (RELATIVE - position)), "];\n");

            *ioPadding = (*ioPadding + MAREA_UINTEGER_64(1));
        }

        MAREA_CGEN_WRITE(generator->types, indent, MAREA_CgenTypeName(generator, fields[index].type), " ", MAREA_CgenMangled(generator, "f_", fields[index].name, fields[index].nameLength), ";\n");

        position = (RELATIVE + MAREA_CgenSizeOf(generator, fields[index].type));
    }

    *outEnd = position;
}

static void MAREA_CALL MAREA_G_WritePadding(MAREA_CgenGenerator *generator, const MAREA_Character8 *indent, MAREA_Uinteger64 bytes, MAREA_Uinteger64 *ioPadding)
{
    if (bytes == MAREA_UINTEGER_64(0)) {
        return;
    }

    MAREA_CGEN_WRITE(generator->types, indent, "uint8_t marea_padding_", MAREA_CgenNumber(generator, *ioPadding), "[", MAREA_CgenNumber(generator, bytes), "];\n");

    *ioPadding = (*ioPadding + MAREA_UINTEGER_64(1));
}

static void MAREA_CALL MAREA_G_WriteAssert(MAREA_CgenGenerator *generator, const MAREA_Character8 *expression, MAREA_Uinteger64 value, const MAREA_Character8 *what)
{
    MAREA_CGEN_WRITE(generator->types, "_Static_assert(", expression, " == ", MAREA_CgenNumber(generator, value), ", \"", what, " of the layout of sema\");\n");
}

static void MAREA_CALL MAREA_G_WriteSizeAsserts(MAREA_CgenGenerator *generator, const MAREA_CgenType *entry)
{
    MAREA_G_WriteAssert(generator, MAREA_CGEN_JOIN(generator, "sizeof(", entry->cName, ")"), entry->size, "size");
    MAREA_G_WriteAssert(generator, MAREA_CGEN_JOIN(generator, "_Alignof(", entry->cName, ")"), entry->alignment, "alignment");
}

// INFO: a field whose offset is not a multiple of its alignment can only be written in a packed struct.
static MAREA_Boolean MAREA_CALL MAREA_G_HasUnalignedField(const MAREA_CgenGenerator *generator, const MAREA_CgenField *fields, MAREA_IntegerSize count, MAREA_Uinteger64 *ioAlignment)
{
    MAREA_Boolean unaligned = MAREA_BOOLEAN_FALSE;

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < count; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_Uinteger64 ALIGNMENT = MAREA_CgenAlignmentOf(generator, fields[index].type);

        if (ALIGNMENT > *ioAlignment) {
            *ioAlignment = ALIGNMENT;
        }

        if ((fields[index].offset % ALIGNMENT) != MAREA_UINTEGER_64(0)) {
            unaligned = MAREA_BOOLEAN_TRUE;
        }
    }

    return unaligned;
}

// INFO: the alignment is written only where C would not give it, since a member with an explicit alignment inside a packed type raises a warning. 8 is always written: some targets align a 64-bit member of a structure to 4.
static void MAREA_CALL MAREA_G_WriteOpening(MAREA_CgenGenerator *generator, const MAREA_CgenType *entry, MAREA_Uinteger64 natural)
{
    const MAREA_Character8 *aligned = ((entry->alignment != natural) || (entry->alignment >= MAREA_UINTEGER_64(8))) ? MAREA_CGEN_JOIN(generator, ", __aligned__(", MAREA_CgenNumber(generator, entry->alignment), ")") : "";

    MAREA_CGEN_WRITE(generator->types, "typedef struct __attribute__((__may_alias__", ((entry->packed == MAREA_BOOLEAN_TRUE) ? ", __packed__" : ""), aligned, ")) ", entry->cName, " {\n");
}

void MAREA_CALL MAREA_CgenWriteStructureDefinition(MAREA_CgenGenerator *generator, MAREA_CgenType *entry)
{
    MAREA_Uinteger64 widest = MAREA_UINTEGER_64(1);

    const MAREA_Boolean UNALIGNED = MAREA_G_HasUnalignedField(generator, entry->fields, entry->memberCount, &widest);

    entry->packed = ((UNALIGNED == MAREA_BOOLEAN_TRUE) || (entry->alignment < widest) || ((entry->size % widest) != MAREA_UINTEGER_64(0)));

    if (entry->packed == MAREA_BOOLEAN_TRUE) {
        generator->usesPacked = MAREA_BOOLEAN_TRUE;
    }

    MAREA_G_WriteOpening(generator, entry, ((entry->packed == MAREA_BOOLEAN_TRUE) ? MAREA_UINTEGER_64(1) : widest));

    MAREA_Uinteger64 padding = MAREA_UINTEGER_64(0);
    MAREA_Uinteger64 end = MAREA_UINTEGER_64(0);

    MAREA_G_WriteFields(generator, entry->fields, entry->memberCount, MAREA_UINTEGER_64(0), "    ", &padding, &end);

    // INFO: C has no empty struct, so a structure with no field holds one byte that nothing reads.
    if (entry->memberCount == MAREA_INTEGER_SIZE(0)) {
        MAREA_CgenWriteText(generator->types, "    uint8_t marea_empty;\n");
    } else if (entry->size > end) {
        MAREA_G_WritePadding(generator, "    ", (entry->size - end), &padding);
    }

    MAREA_CGEN_WRITE(generator->types, "} ", entry->cName, ";\n");

    if (entry->memberCount == MAREA_INTEGER_SIZE(0)) {
        return;
    }

    MAREA_G_WriteSizeAsserts(generator, entry);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < entry->memberCount; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_Character8 *name = MAREA_CgenMangled(generator, "f_", entry->fields[index].name, entry->fields[index].nameLength);

        MAREA_G_WriteAssert(generator, MAREA_CGEN_JOIN(generator, "offsetof(", entry->cName, ", ", name, ")"), entry->fields[index].offset, "offset");
    }
}

// INFO: the size of the union of the variants as C lays it out, and the alignment it asks for.
static MAREA_Uinteger64 MAREA_CALL MAREA_G_UnionSize(const MAREA_CgenGenerator *generator, const MAREA_CgenType *entry, MAREA_Uinteger64 payload, MAREA_Boolean packed, MAREA_Uinteger64 *outAlignment)
{
    MAREA_Uinteger64 size = MAREA_UINTEGER_64(0);
    MAREA_Uinteger64 alignment = MAREA_UINTEGER_64(1);

    for (MAREA_IntegerSize variant = MAREA_INTEGER_SIZE(0); variant < entry->memberCount; variant = (variant + MAREA_INTEGER_SIZE(1))) {
        const MAREA_CgenVariant *row = &entry->variants[variant];

        MAREA_Uinteger64 end = MAREA_UINTEGER_64(0);
        MAREA_Uinteger64 widest = MAREA_UINTEGER_64(1);

        for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < row->fieldCount; index = (index + MAREA_INTEGER_SIZE(1))) {
            const MAREA_Uinteger64
FIELD_END
= ((row->fields[index].offset - payload) + MAREA_CgenSizeOf(generator, row->fields[index].type));
const MAREA_Uinteger64 ALIGNMENT = MAREA_CgenAlignmentOf(generator, row->fields[index].type);

end = (FIELD_END > end) ?
FIELD_END : end;
widest = (ALIGNMENT > widest) ? ALIGNMENT : widest;
}

end = (packed == MAREA_BOOLEAN_TRUE) ? end : MAREA_G_RoundUp(end, widest);

size = (end > size) ? end : size;
alignment = (widest > alignment) ? widest : alignment;
}

*outAlignment = (packed == MAREA_BOOLEAN_TRUE) ? MAREA_UINTEGER_64(1) : alignment;

return (packed == MAREA_BOOLEAN_TRUE) ? size : MAREA_G_RoundUp(size, alignment);
}

static MAREA_Boolean MAREA_CALL MAREA_G_HasData(const MAREA_CgenType *entry)
{
    for (MAREA_IntegerSize variant = MAREA_INTEGER_SIZE(0); variant < entry->memberCount; variant = (variant + MAREA_INTEGER_SIZE(1))) {
        if (entry->variants[variant].fieldCount > MAREA_INTEGER_SIZE(0)) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_NeedsPacking(const MAREA_CgenGenerator *generator, const MAREA_CgenType *entry, MAREA_Uinteger64 payload)
{
    const MAREA_Uinteger64 TAG = MAREA_CgenSizeOf(generator, entry->element);

    MAREA_Uinteger64 widest = TAG;

    MAREA_Boolean unaligned = MAREA_BOOLEAN_FALSE;

    for (MAREA_IntegerSize variant = MAREA_INTEGER_SIZE(0); variant < entry->memberCount; variant = (variant + MAREA_INTEGER_SIZE(1))) {
        if (MAREA_G_HasUnalignedField(generator, entry->variants[variant].fields, entry->variants[variant].fieldCount, &widest) == MAREA_BOOLEAN_TRUE) {
            unaligned = MAREA_BOOLEAN_TRUE;
        }
    }

    MAREA_Uinteger64 alignment = MAREA_UINTEGER_64(1);

    const MAREA_Uinteger64 UNION_SIZE = MAREA_G_UnionSize(generator, entry, payload, MAREA_BOOLEAN_FALSE, &alignment);

    if (MAREA_G_HasData(entry) == MAREA_BOOLEAN_TRUE) {
        return ((unaligned == MAREA_BOOLEAN_TRUE) || ((payload % alignment) != MAREA_UINTEGER_64(0)) || (entry->alignment < widest) || (entry->size < MAREA_G_RoundUp((payload + UNION_SIZE), entry->alignment)));
    }

    return ((entry->alignment < TAG) || ((entry->size % TAG) != MAREA_UINTEGER_64(0)));
}

static void MAREA_CALL MAREA_G_WriteUnion(MAREA_CgenGenerator *generator, const MAREA_CgenType *entry, MAREA_Uinteger64 payload)
{
    MAREA_CgenWriteText(generator->types, "    union {\n");

    for (MAREA_IntegerSize variant = MAREA_INTEGER_SIZE(0); variant < entry->memberCount; variant = (variant + MAREA_INTEGER_SIZE(1))) {
        const MAREA_CgenVariant *row = &entry->variants[variant];

        if (row->fieldCount == MAREA_INTEGER_SIZE(0)) {
            continue;
        }

        MAREA_CGEN_WRITE(generator->types, "        struct ", ((entry->packed == MAREA_BOOLEAN_TRUE) ? "__attribute__((__packed__)) " : ""), "{\n");

        MAREA_Uinteger64 padding = MAREA_UINTEGER_64(0);
        MAREA_Uinteger64 end = MAREA_UINTEGER_64(0);

        MAREA_G_WriteFields(generator, row->fields, row->fieldCount, payload, "            ", &padding, &end);

        MAREA_CGEN_WRITE(generator->types, "        } v_", MAREA_CgenNumber(generator, variant), ";\n");
    }

    MAREA_CgenWriteText(generator->types, "    } marea_data;\n");
}

void MAREA_CALL MAREA_CgenWriteEnumDefinition(MAREA_CgenGenerator *generator, MAREA_CgenType *entry, MAREA_Uinteger64 payload)
{
    const MAREA_Uinteger64 TAG = MAREA_CgenSizeOf(generator, entry->element);

    const MAREA_Boolean DATA = MAREA_G_HasData(entry);

    entry->wrapped = ((DATA == MAREA_BOOLEAN_TRUE) || (entry->size != TAG) || (entry->alignment != TAG));

    if (entry->wrapped == MAREA_BOOLEAN_FALSE) {
        MAREA_CGEN_WRITE(generator->types, "typedef ", MAREA_CgenTypeName(generator, entry->element), " ", entry->cName, ";\n");

        MAREA_G_WriteSizeAsserts(generator, entry);

        return;
    }

    entry->packed = MAREA_G_NeedsPacking(generator, entry, payload);

    if (entry->packed == MAREA_BOOLEAN_TRUE) {
        generator->usesPacked = MAREA_BOOLEAN_TRUE;
    }

    MAREA_Uinteger64 unionAlignment = MAREA_UINTEGER_64(1);

    MAREA_IGNORE(MAREA_G_UnionSize(generator, entry, payload, MAREA_BOOLEAN_FALSE, &unionAlignment));

    const MAREA_Uinteger64 WIDEST = (unionAlignment > TAG) ? unionAlignment : TAG;

    MAREA_G_WriteOpening(generator, entry, ((entry->packed == MAREA_BOOLEAN_TRUE) ? MAREA_UINTEGER_64(1) : WIDEST));

    MAREA_CGEN_WRITE(generator->types, "    ", MAREA_CgenTypeName(generator, entry->element), " marea_tag;\n");

    MAREA_Uinteger64 padding = MAREA_UINTEGER_64(0);

    if (DATA == MAREA_BOOLEAN_TRUE) {
        MAREA_G_WritePadding(generator, "    ", (payload - TAG), &padding);
        MAREA_G_WriteUnion(generator, entry, payload);

        MAREA_Uinteger64 alignment = MAREA_UINTEGER_64(1);

        const MAREA_Uinteger64 UNION_SIZE = MAREA_G_UnionSize(generator, entry, payload, entry->packed, &alignment);

        if (entry->size > MAREA_G_RoundUp((payload + UNION_SIZE), entry->alignment)) {
            MAREA_G_WritePadding(generator, "    ", (entry->size - (payload + UNION_SIZE)), &padding);
        }
    } else if (entry->size > TAG) {
        MAREA_G_WritePadding(generator, "    ", (entry->size - TAG), &padding);
    }

    MAREA_CGEN_WRITE(generator->types, "} ", entry->cName, ";\n");

    MAREA_G_WriteSizeAsserts(generator, entry);

    if (DATA == MAREA_BOOLEAN_FALSE) {
        return;
    }

    MAREA_G_WriteAssert(generator, MAREA_CGEN_JOIN(generator, "offsetof(", entry->cName, ", marea_data)"), payload, "offset");

    for (MAREA_IntegerSize variant = MAREA_INTEGER_SIZE(0); variant < entry->memberCount; variant = (variant + MAREA_INTEGER_SIZE(1))) {
        const MAREA_CgenVariant *row = &entry->variants[variant];

        for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < row->fieldCount; index = (index + MAREA_INTEGER_SIZE(1))) {
            const MAREA_Character8 *name = MAREA_CgenMangled(generator, "f_", row->fields[index].name, row->fields[index].nameLength);

            MAREA_G_WriteAssert(generator, MAREA_CGEN_JOIN(generator, "offsetof(", entry->cName, ", marea_data.v_", MAREA_CgenNumber(generator, variant), ".", name, ")"), row->fields[index].offset, "offset");
        }
    }
}

void MAREA_CALL MAREA_CgenWriteArrayDefinition(MAREA_CgenGenerator *generator, const MAREA_CgenType *entry)
{
    MAREA_CGEN_WRITE(generator->types, "typedef struct __attribute__((__may_alias__)) ", entry->cName, " {\n    ", MAREA_CgenTypeName(generator, entry->element), " items[", MAREA_CgenNumber(generator, entry->count), "ULL];\n} ", entry->cName, ";\n");
}
