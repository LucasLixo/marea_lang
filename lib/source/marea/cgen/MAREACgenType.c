// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/cgen/MAREACgenInternal.h"

#include <string.h>

typedef struct MAREA_G_Scalar
{
    const MAREA_Character8 *name;
    const MAREA_Character8 *cName;
    const MAREA_Character8 *suffix;
    MAREA_Uinteger64 size;
} MAREA_G_Scalar;

// INFO: the index of a row is the kind of the scalar.
static const MAREA_G_Scalar MAREA_G_SCALARS[] = {
    { .name = "sinteger8", .cName = "int8_t", .suffix = "s8", .size = MAREA_UINTEGER_64(1) },
    { .name = "sinteger16", .cName = "int16_t", .suffix = "s16", .size = MAREA_UINTEGER_64(2) },
    { .name = "sinteger32", .cName = "int32_t", .suffix = "s32", .size = MAREA_UINTEGER_64(4) },
    { .name = "sinteger64", .cName = "int64_t", .suffix = "s64", .size = MAREA_UINTEGER_64(8) },
    { .name = "uinteger8", .cName = "uint8_t", .suffix = "u8", .size = MAREA_UINTEGER_64(1) },
    { .name = "uinteger16", .cName = "uint16_t", .suffix = "u16", .size = MAREA_UINTEGER_64(2) },
    { .name = "uinteger32", .cName = "uint32_t", .suffix = "u32", .size = MAREA_UINTEGER_64(4) },
    { .name = "uinteger64", .cName = "uint64_t", .suffix = "u64", .size = MAREA_UINTEGER_64(8) },
    { .name = "float32", .cName = "float", .suffix = "f32", .size = MAREA_UINTEGER_64(4) },
    { .name = "float64", .cName = "double", .suffix = "f64", .size = MAREA_UINTEGER_64(8) },
    { .name = "boolean", .cName = "bool", .suffix = "bool", .size = MAREA_UINTEGER_64(1) },
};

MAREA_Uinteger32 MAREA_CALL MAREA_CgenKindFromText(const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    if (text == MAREA_NULLPTR) {
        return MAREA_CGEN_KIND_NONE;
    }

    for (MAREA_Uinteger32 index = MAREA_UINTEGER_32(0); index < (MAREA_Uinteger32)MAREA_ARRAY_SIZE(MAREA_G_SCALARS); index = (index + MAREA_UINTEGER_32(1))) {
        if (MAREA_CgenSameText(MAREA_G_SCALARS[index].name, (MAREA_IntegerSize)strlen(MAREA_G_SCALARS[index].name), text, length) == MAREA_BOOLEAN_TRUE) {
            return index;
        }
    }

    return MAREA_CGEN_KIND_NONE;
}

MAREA_Boolean MAREA_CALL MAREA_CgenIsNumber(MAREA_Uinteger32 type)
{
    return (type < MAREA_CGEN_KIND_NUMBER_COUNT);
}

MAREA_Boolean MAREA_CALL MAREA_CgenIsInteger(MAREA_Uinteger32 type)
{
    return (type <= MAREA_CGEN_KIND_UINTEGER_64);
}

MAREA_Boolean MAREA_CALL MAREA_CgenIsSigned(MAREA_Uinteger32 type)
{
    return (type <= MAREA_CGEN_KIND_SINTEGER_64);
}

MAREA_Boolean MAREA_CALL MAREA_CgenIsFloat(MAREA_Uinteger32 type)
{
    return ((type == MAREA_CGEN_KIND_FLOAT_32) || (type == MAREA_CGEN_KIND_FLOAT_64));
}

const MAREA_Character8 *MAREA_CALL MAREA_CgenKindSuffix(MAREA_Uinteger32 kind)
{
    return ((kind <= MAREA_CGEN_KIND_BOOLEAN) ? MAREA_G_SCALARS[kind].suffix : "");
}

static MAREA_CgenType *MAREA_CALL MAREA_G_Entry(const MAREA_CgenGenerator *generator, MAREA_Uinteger32 type)
{
    if ((type < MAREA_CGEN_TYPE_FIRST_COMPOSITE) || ((MAREA_IntegerSize)(type - MAREA_CGEN_TYPE_FIRST_COMPOSITE) >= generator->typeCount)) {
        return MAREA_NULLPTR;
    }

    return &generator->typeTable[(type - MAREA_CGEN_TYPE_FIRST_COMPOSITE)];
}

const MAREA_CgenType *MAREA_CALL MAREA_CgenTypeAt(const MAREA_CgenGenerator *generator, MAREA_Uinteger32 type)
{
    return MAREA_G_Entry(generator, type);
}

MAREA_Uinteger8 MAREA_CALL MAREA_CgenCategoryOf(const MAREA_CgenGenerator *generator, MAREA_Uinteger32 type)
{
    if (type <= MAREA_CGEN_KIND_BOOLEAN) {
        return MAREA_CGEN_CATEGORY_SCALAR;
    }

    if (type == MAREA_CGEN_KIND_VOID) {
        return MAREA_CGEN_CATEGORY_VOID;
    }

    const MAREA_CgenType *entry = MAREA_G_Entry(generator, type);

    return (entry == MAREA_NULLPTR) ? MAREA_CGEN_CATEGORY_NONE : entry->category;
}

MAREA_Boolean MAREA_CALL MAREA_CgenIsValueType(const MAREA_CgenGenerator *generator, MAREA_Uinteger32 type)
{
    if (type <= MAREA_CGEN_KIND_BOOLEAN) {
        return MAREA_BOOLEAN_TRUE;
    }

    const MAREA_CgenType *entry = MAREA_G_Entry(generator, type);

    return ((entry != MAREA_NULLPTR) && (entry->done == MAREA_BOOLEAN_TRUE) && (entry->failed == MAREA_BOOLEAN_FALSE));
}

MAREA_Boolean MAREA_CALL MAREA_CgenIsAggregate(const MAREA_CgenGenerator *generator, MAREA_Uinteger32 type)
{
    const MAREA_CgenType *entry = MAREA_G_Entry(generator, type);

    if (entry == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    return ((entry->category != MAREA_CGEN_CATEGORY_ENUM) || (entry->wrapped == MAREA_BOOLEAN_TRUE));
}

const MAREA_Character8 *MAREA_CALL MAREA_CgenTypeName(const MAREA_CgenGenerator *generator, MAREA_Uinteger32 type)
{
    if (type <= MAREA_CGEN_KIND_BOOLEAN) {
        return MAREA_G_SCALARS[type].cName;
    }

    const MAREA_CgenType *entry = MAREA_G_Entry(generator, type);

    return (entry == MAREA_NULLPTR) ? "void" : entry->cName;
}

MAREA_Uinteger64 MAREA_CALL MAREA_CgenSizeOf(const MAREA_CgenGenerator *generator, MAREA_Uinteger32 type)
{
    if (type <= MAREA_CGEN_KIND_BOOLEAN) {
        return MAREA_G_SCALARS[type].size;
    }

    const MAREA_CgenType *entry = MAREA_G_Entry(generator, type);

    return (entry == MAREA_NULLPTR) ? MAREA_UINTEGER_64(0) : entry->size;
}

MAREA_Uinteger64 MAREA_CALL MAREA_CgenAlignmentOf(const MAREA_CgenGenerator *generator, MAREA_Uinteger32 type)
{
    if (type <= MAREA_CGEN_KIND_BOOLEAN) {
        return MAREA_G_SCALARS[type].size;
    }

    const MAREA_CgenType *entry = MAREA_G_Entry(generator, type);

    return ((entry == MAREA_NULLPTR) || (entry->alignment == MAREA_UINTEGER_64(0))) ? MAREA_UINTEGER_64(1) : entry->alignment;
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_AddType(MAREA_CgenGenerator *generator, MAREA_Uinteger8 category, const MAREA_Character8 *name, MAREA_IntegerSize nameLength, const MAREA_Character8 *cName, MAREA_Uinteger32 *outType)
{
    MAREA_Null items = generator->typeTable;

    MAREA_CGEN_TRY(MAREA_CgenGrow(generator, &items, &generator->typeCapacity, generator->typeCount, sizeof(MAREA_CgenType), MAREA_ALIGNOF(MAREA_CgenType)));

    generator->typeTable = items;

    MAREA_CgenType *entry = &generator->typeTable[generator->typeCount];

    memset(entry, 0, sizeof(*entry));

    entry->category = category;
    entry->name = name;
    entry->nameLength = nameLength;
    entry->cName = cName;

    *outType = (MAREA_CGEN_TYPE_FIRST_COMPOSITE + (MAREA_Uinteger32)generator->typeCount);

    generator->typeCount = (generator->typeCount + MAREA_INTEGER_SIZE(1));

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

static MAREA_Uinteger32 MAREA_CALL MAREA_G_FindNamed(const MAREA_CgenGenerator *generator, MAREA_Uinteger8 category, const MAREA_Character8 *name, MAREA_IntegerSize nameLength)
{
    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < generator->typeCount; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_CgenType *entry = &generator->typeTable[index];

        if ((entry->category == category) && (MAREA_CgenSameText(entry->name, entry->nameLength, name, nameLength) == MAREA_BOOLEAN_TRUE)) {
            return (MAREA_CGEN_TYPE_FIRST_COMPOSITE + (MAREA_Uinteger32)index);
        }
    }

    return MAREA_CGEN_KIND_NONE;
}

// INFO: the layout of sema of a declaration of the unit, or null when sema did not measure it.
static const MAREA_IrLayout *MAREA_CALL MAREA_G_MeasuredLayout(const MAREA_CgenGenerator *generator, const MAREA_IrNode *declaration)
{
    const MAREA_IrLayout *layout = MAREA_NULLPTR;

    if (MAREA_IrModuleFindLayout(generator->module, MAREA_IrNodeGetText(declaration), MAREA_IrNodeGetTextLength(declaration), &layout) != MAREA_IR_LAYOUT_EXCEPT_SUCCESS) {
        return MAREA_NULLPTR;
    }

    return (MAREA_IrLayoutGetVerdict(layout) == MAREA_SEMA_MEASURE_VERDICT_MEASURED) ? layout : MAREA_NULLPTR;
}

static MAREA_IntegerSize MAREA_CALL MAREA_G_CountChildren(const MAREA_IrNode *node, MAREA_IrKind kind)
{
    MAREA_IntegerSize count = MAREA_INTEGER_SIZE(0);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < MAREA_IrNodeGetChildCount(node); index = (index + MAREA_INTEGER_SIZE(1))) {
        if (MAREA_IrNodeGetKind(MAREA_CgenChildAt(node, index)) == kind) {
            count = (count + MAREA_INTEGER_SIZE(1));
        }
    }

    return count;
}

// INFO: reads the fields of one declaration into the list, the offsets from the layout. A field that cgen cannot write, or a structure that holds itself, sets failed.
static MAREA_CgenExcept MAREA_CALL MAREA_G_ReadFields(MAREA_CgenGenerator *generator, const MAREA_IrNode *declaration, MAREA_IrKind kind, MAREA_CgenField *fields, MAREA_Boolean *ioFailed)
{
    MAREA_IntegerSize used = MAREA_INTEGER_SIZE(0);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < MAREA_IrNodeGetChildCount(declaration); index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_IrNode *child = MAREA_CgenChildAt(declaration, index);

        if (MAREA_IrNodeGetKind(child) != kind) {
            continue;
        }

        MAREA_Uinteger32 type = MAREA_CGEN_KIND_NONE;

        MAREA_CGEN_TRY(MAREA_CgenTypeOfNode(generator, child, &type));

        if (MAREA_CgenIsValueType(generator, type) == MAREA_BOOLEAN_FALSE) {
            *ioFailed = MAREA_BOOLEAN_TRUE;
        }

        fields[used].declaration = child;
        fields[used].name = MAREA_IrNodeGetText(child);
        fields[used].nameLength = MAREA_IrNodeGetTextLength(child);
        fields[used].type = type;

        used = (used + MAREA_INTEGER_SIZE(1));
    }

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_DefineStructure(MAREA_CgenGenerator *generator, const MAREA_IrNode *declaration, MAREA_Uinteger32 *outType)
{
    const MAREA_Character8 *name = MAREA_IrNodeGetText(declaration);
    const MAREA_IntegerSize LENGTH = MAREA_IrNodeGetTextLength(declaration);

    const MAREA_Uinteger32 FOUND = MAREA_G_FindNamed(generator, MAREA_CGEN_CATEGORY_STRUCTURE, name, LENGTH);

    if (FOUND != MAREA_CGEN_KIND_NONE) {
        *outType = FOUND;

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    MAREA_Uinteger32 type = MAREA_CGEN_KIND_NONE;

    MAREA_CGEN_TRY(MAREA_G_AddType(generator, MAREA_CGEN_CATEGORY_STRUCTURE, name, LENGTH, MAREA_CgenMangled(generator, "marea_s_", name, LENGTH), &type));

    *outType = type;

    const MAREA_IrLayout *layout = MAREA_G_MeasuredLayout(generator, declaration);

    const MAREA_IntegerSize COUNT = MAREA_G_CountChildren(declaration, MAREA_IR_KIND_FIELD_DECLARATION);

    MAREA_Null memory = MAREA_NULLPTR;

    MAREA_CGEN_TRY(MAREA_CgenAllocate(generator, (COUNT * sizeof(MAREA_CgenField)), MAREA_ALIGNOF(MAREA_CgenField), &memory));

    MAREA_CgenField *fields = memory;

    MAREA_Boolean failed = ((layout == MAREA_NULLPTR) || (MAREA_IrLayoutGetMemberCount(layout) != COUNT));

    MAREA_CGEN_TRY(MAREA_G_ReadFields(generator, declaration, MAREA_IR_KIND_FIELD_DECLARATION, fields, &failed));

    MAREA_Uinteger64 size = MAREA_UINTEGER_64(0);
    MAREA_Uinteger64 alignment = MAREA_UINTEGER_64(0);

    if ((failed == MAREA_BOOLEAN_FALSE) && ((MAREA_IrLayoutGetSize(layout, &size) != MAREA_IR_LAYOUT_EXCEPT_SUCCESS) || (MAREA_IrLayoutGetAlignment(layout, &alignment) != MAREA_IR_LAYOUT_EXCEPT_SUCCESS))) {
        failed = MAREA_BOOLEAN_TRUE;
    }

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); (failed == MAREA_BOOLEAN_FALSE) && (index < COUNT); index = (index + MAREA_INTEGER_SIZE(1))) {
        if (MAREA_IrLayoutGetMemberOffset(layout, index, &fields[index].offset) != MAREA_IR_LAYOUT_EXCEPT_SUCCESS) {
            failed = MAREA_BOOLEAN_TRUE;
        }
    }

    // INFO: the fields may have added types, so the entry is read again.
    MAREA_CgenType *entry = MAREA_G_Entry(generator, type);

    entry->fields = fields;
    entry->memberCount = COUNT;
    entry->size = size;
    entry->alignment = alignment;
    entry->failed = failed;

    if (failed == MAREA_BOOLEAN_FALSE) {
        MAREA_CgenWriteStructureDefinition(generator, entry);
    }

    entry->done = MAREA_BOOLEAN_TRUE;

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

// INFO: the tag is the written integer type of the enum when it has the size of the layout, else the unsigned integer of that size.
static MAREA_Uinteger32 MAREA_CALL MAREA_G_TagKind(const MAREA_IrNode *declaration, MAREA_Uinteger64 size)
{
    const MAREA_Uinteger32 WRITTEN = MAREA_CgenKindFromText(MAREA_IrNodeGetTypeName(declaration), MAREA_IrNodeGetTypeNameLength(declaration));

    if ((MAREA_CgenIsInteger(WRITTEN) == MAREA_BOOLEAN_TRUE) && (MAREA_G_SCALARS[WRITTEN].size == size)) {
        return WRITTEN;
    }

    if (size == MAREA_UINTEGER_64(1)) {
        return MAREA_CGEN_KIND_UINTEGER_8;
    }

    if (size == MAREA_UINTEGER_64(2)) {
        return MAREA_CGEN_KIND_UINTEGER_16;
    }

    return (size == MAREA_UINTEGER_64(4)) ? MAREA_CGEN_KIND_UINTEGER_32 : MAREA_CGEN_KIND_UINTEGER_64;
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_ReadVariants(MAREA_CgenGenerator *generator, const MAREA_IrNode *declaration, const MAREA_IrLayout *layout, MAREA_CgenVariant *variants, MAREA_Boolean *ioFailed)
{
    MAREA_IntegerSize used = MAREA_INTEGER_SIZE(0);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < MAREA_IrNodeGetChildCount(declaration); index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_IrNode *child = MAREA_CgenChildAt(declaration, index);

        if (MAREA_IrNodeGetKind(child) != MAREA_IR_KIND_VARIANT_DECLARATION) {
            continue;
        }

        const MAREA_IntegerSize COUNT = MAREA_G_CountChildren(child, MAREA_IR_KIND_VARIANT_FIELD);

        MAREA_Null memory = MAREA_NULLPTR;

        MAREA_CGEN_TRY(MAREA_CgenAllocate(generator, (COUNT * sizeof(MAREA_CgenField)), MAREA_ALIGNOF(MAREA_CgenField), &memory));

        MAREA_CgenVariant *variant = &variants[used];

        variant->name = MAREA_IrNodeGetText(child);
        variant->nameLength = MAREA_IrNodeGetTextLength(child);
        variant->fields = memory;
        variant->fieldCount = COUNT;

        MAREA_CGEN_TRY(MAREA_G_ReadFields(generator, child, MAREA_IR_KIND_VARIANT_FIELD, variant->fields, ioFailed));

        for (MAREA_IntegerSize field = MAREA_INTEGER_SIZE(0); (*ioFailed == MAREA_BOOLEAN_FALSE) && (field < COUNT); field = (field + MAREA_INTEGER_SIZE(1))) {
            if (MAREA_IrLayoutGetVariantFieldOffset(layout, used, field, &variant->fields[field].offset) != MAREA_IR_LAYOUT_EXCEPT_SUCCESS) {
                *ioFailed = MAREA_BOOLEAN_TRUE;
            }
        }

        used = (used + MAREA_INTEGER_SIZE(1));
    }

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_DefineEnum(MAREA_CgenGenerator *generator, const MAREA_IrNode *declaration, MAREA_Uinteger32 *outType)
{
    const MAREA_Character8 *name = MAREA_IrNodeGetText(declaration);
    const MAREA_IntegerSize LENGTH = MAREA_IrNodeGetTextLength(declaration);

    const MAREA_Uinteger32 FOUND = MAREA_G_FindNamed(generator, MAREA_CGEN_CATEGORY_ENUM, name, LENGTH);

    if (FOUND != MAREA_CGEN_KIND_NONE) {
        *outType = FOUND;

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    MAREA_Uinteger32 type = MAREA_CGEN_KIND_NONE;

    MAREA_CGEN_TRY(MAREA_G_AddType(generator, MAREA_CGEN_CATEGORY_ENUM, name, LENGTH, MAREA_CgenMangled(generator, "marea_e_", name, LENGTH), &type));

    *outType = type;

    const MAREA_IrLayout *layout = MAREA_G_MeasuredLayout(generator, declaration);

    const MAREA_IntegerSize COUNT = MAREA_G_CountChildren(declaration, MAREA_IR_KIND_VARIANT_DECLARATION);

    MAREA_Null memory = MAREA_NULLPTR;

    MAREA_CGEN_TRY(MAREA_CgenAllocate(generator, (COUNT * sizeof(MAREA_CgenVariant)), MAREA_ALIGNOF(MAREA_CgenVariant), &memory));

    MAREA_CgenVariant *variants = memory;

    MAREA_Boolean failed = ((layout == MAREA_NULLPTR) || (MAREA_IrLayoutGetMemberCount(layout) != COUNT) || (COUNT == MAREA_INTEGER_SIZE(0)));

    MAREA_CGEN_TRY(MAREA_G_ReadVariants(generator, declaration, layout, variants, &failed));

    MAREA_Uinteger64 size = MAREA_UINTEGER_64(0);
    MAREA_Uinteger64 alignment = MAREA_UINTEGER_64(0);
    MAREA_Uinteger64 tag = MAREA_UINTEGER_64(0);
    MAREA_Uinteger64 payload = MAREA_UINTEGER_64(0);

    if ((failed == MAREA_BOOLEAN_FALSE) && ((MAREA_IrLayoutGetSize(layout, &size) != MAREA_IR_LAYOUT_EXCEPT_SUCCESS) || (MAREA_IrLayoutGetAlignment(layout, &alignment) != MAREA_IR_LAYOUT_EXCEPT_SUCCESS) || (MAREA_IrLayoutGetTagSize(layout, &tag) != MAREA_IR_LAYOUT_EXCEPT_SUCCESS) || (MAREA_IrLayoutGetMemberOffset(layout, MAREA_INTEGER_SIZE(0), &payload) != MAREA_IR_LAYOUT_EXCEPT_SUCCESS))) {
        failed = MAREA_BOOLEAN_TRUE;
    }

    MAREA_CgenType *entry = MAREA_G_Entry(generator, type);

    entry->variants = variants;
    entry->memberCount = COUNT;
    entry->size = size;
    entry->alignment = alignment;
    entry->element = MAREA_G_TagKind(declaration, tag);
    entry->failed = failed;

    if (failed == MAREA_BOOLEAN_FALSE) {
        MAREA_CgenWriteEnumDefinition(generator, entry, payload);
    }

    entry->done = MAREA_BOOLEAN_TRUE;

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_DefineArray(MAREA_CgenGenerator *generator, MAREA_Uinteger32 element, MAREA_Uinteger64 count, MAREA_Uinteger32 *outType)
{
    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < generator->typeCount; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_CgenType *entry = &generator->typeTable[index];

        if ((entry->category == MAREA_CGEN_CATEGORY_ARRAY) && (entry->element == element) && (entry->count == count)) {
            *outType = (MAREA_CGEN_TYPE_FIRST_COMPOSITE + (MAREA_Uinteger32)index);

            return MAREA_CGEN_EXCEPT_SUCCESS;
        }
    }

    const MAREA_Character8 *cName = MAREA_CGEN_JOIN(generator, "marea_a", MAREA_CgenNumber(generator, generator->typeCount));

    MAREA_CGEN_TRY(MAREA_G_AddType(generator, MAREA_CGEN_CATEGORY_ARRAY, "", MAREA_INTEGER_SIZE(0), cName, outType));

    MAREA_CgenType *entry = MAREA_G_Entry(generator, *outType);

    entry->element = element;
    entry->count = count;
    entry->size = (count * MAREA_CgenSizeOf(generator, element));
    entry->alignment = MAREA_CgenAlignmentOf(generator, element);
    entry->done = MAREA_BOOLEAN_TRUE;

    MAREA_CgenWriteArrayDefinition(generator, entry);

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

// INFO: the descriptor of a full path, or null when the registry has none.
static const MAREA_TypeDescriptor *MAREA_CALL MAREA_G_Describe(const MAREA_CgenGenerator *generator, const MAREA_Character8 *name, MAREA_IntegerSize length)
{
    MAREA_IntegerSize dot = length;

    while ((dot > MAREA_INTEGER_SIZE(0)) && (name[(dot - MAREA_INTEGER_SIZE(1))] != '.')) {
        dot = (dot - MAREA_INTEGER_SIZE(1));
    }

    if ((generator->context->registry == MAREA_NULLPTR) || (dot == MAREA_INTEGER_SIZE(0))) {
        return MAREA_NULLPTR;
    }

    return MAREA_TypeRegistryFind(generator->context->registry, name, (dot - MAREA_INTEGER_SIZE(1)), (name + dot), (length - dot));
}

// INFO: the full path that a spelling of sema stands for: itself, a spelling that the module shows, or a namespace alias that the module shows, in front of the rest of the spelling.
static const MAREA_Character8 *MAREA_CALL MAREA_G_FullPath(MAREA_CgenGenerator *generator, const MAREA_Character8 *head, MAREA_IntegerSize length, MAREA_IntegerSize *outLength)
{
    *outLength = length;

    if (MAREA_G_Describe(generator, head, length) != MAREA_NULLPTR) {
        return head;
    }

    const MAREA_CgenAlias *prefix = MAREA_NULLPTR;

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < generator->aliasCount; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_CgenAlias *alias = &generator->aliases[index];

        if (MAREA_CgenSameText(alias->from, alias->fromLength, head, length) == MAREA_BOOLEAN_TRUE) {
            *outLength = alias->toLength;

            return alias->to;
        }

        if ((length > alias->fromLength) && (head[alias->fromLength] == '.') && (memcmp(head, alias->from, alias->fromLength) == 0) && ((prefix == MAREA_NULLPTR) || (alias->fromLength > prefix->fromLength))) {
            prefix = alias;
        }
    }

    if (prefix == MAREA_NULLPTR) {
        return head;
    }

    const MAREA_Character8 *full = MAREA_CGEN_JOIN(generator, MAREA_CgenCopyText(generator, prefix->to, prefix->toLength), MAREA_CgenCopyText(generator, (head + prefix->fromLength), (length - prefix->fromLength)));

    *outLength = (MAREA_IntegerSize)strlen(full);

    return full;
}

static void MAREA_CALL MAREA_G_Trim(const MAREA_Character8 **ioText, MAREA_IntegerSize *ioLength)
{
    while ((*ioLength > MAREA_INTEGER_SIZE(0)) && ((*ioText)[0] == ' ')) {
        *ioText = (*ioText + 1);
        *ioLength = (*ioLength - MAREA_INTEGER_SIZE(1));
    }

    while ((*ioLength > MAREA_INTEGER_SIZE(0)) && ((*ioText)[(*ioLength - MAREA_INTEGER_SIZE(1))] == ' ')) {
        *ioLength = (*ioLength - MAREA_INTEGER_SIZE(1));
    }
}

// INFO: the two top-level type arguments of a resolved spelling such as array<array<float32,4>,2>.
static MAREA_Boolean MAREA_CALL MAREA_G_TwoArguments(const MAREA_Character8 *text, MAREA_IntegerSize length, const MAREA_Character8 **outFirst, MAREA_IntegerSize *outFirstLength, const MAREA_Character8 **outSecond, MAREA_IntegerSize *outSecondLength)
{
    MAREA_IntegerSize open = MAREA_INTEGER_SIZE(0);

    while ((open < length) && (text[open] != '<')) {
        open = (open + MAREA_INTEGER_SIZE(1));
    }

    if ((open >= length) || (text[(length - MAREA_INTEGER_SIZE(1))] != '>')) {
        return MAREA_BOOLEAN_FALSE;
    }

    MAREA_IntegerSize comma = MAREA_INTEGER_SIZE(0);
    MAREA_Sinteger32 depth = MAREA_SINTEGER_32(0);

    for (MAREA_IntegerSize index = (open + MAREA_INTEGER_SIZE(1)); index < (length - MAREA_INTEGER_SIZE(1)); index = (index + MAREA_INTEGER_SIZE(1))) {
        if ((text[index] == '<') || (text[index] == '(')) {
            depth = (depth + MAREA_SINTEGER_32(1));
        } else if ((text[index] == '>') || (text[index] == ')')) {
            depth = (depth - MAREA_SINTEGER_32(1));
        } else if ((text[index] == ',') && (depth == MAREA_SINTEGER_32(0))) {
            if (comma != MAREA_INTEGER_SIZE(0)) {
                return MAREA_BOOLEAN_FALSE;
            }

            comma = index;
        }
    }

    if (comma == MAREA_INTEGER_SIZE(0)) {
        return MAREA_BOOLEAN_FALSE;
    }

    *outFirst = (text + open + MAREA_INTEGER_SIZE(1));
    *outFirstLength = (comma - open - MAREA_INTEGER_SIZE(1));
    *outSecond = (text + comma + MAREA_INTEGER_SIZE(1));
    *outSecondLength = (length - comma - MAREA_INTEGER_SIZE(2));

    MAREA_G_Trim(outFirst, outFirstLength);
    MAREA_G_Trim(outSecond, outSecondLength);

    return MAREA_BOOLEAN_TRUE;
}

// INFO: an array of a registered inline run: an index protocol, no role, a copy class and two type arguments, the element and a literal count. This is the shape that sema measures, so cgen writes it as a C array.
static MAREA_CgenExcept MAREA_CALL MAREA_G_ArrayType(MAREA_CgenGenerator *generator, const MAREA_Character8 *head, MAREA_IntegerSize headLength, const MAREA_Character8 *resolved, MAREA_IntegerSize resolvedLength, MAREA_Uinteger32 *outType)
{
    if (resolved == MAREA_NULLPTR) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    MAREA_IntegerSize fullLength = MAREA_INTEGER_SIZE(0);

    const MAREA_Character8 *full = MAREA_G_FullPath(generator, head, headLength, &fullLength);

    const MAREA_TypeDescriptor *descriptor = MAREA_G_Describe(generator, full, fullLength);

    if ((descriptor == MAREA_NULLPTR) || (descriptor->hasIndexProtocol == MAREA_BOOLEAN_FALSE) || (descriptor->roles != MAREA_TYPE_ROLE_NONE) || (descriptor->ownershipClass != MAREA_TYPE_OWNERSHIP_CLASS_COPY) || (descriptor->typeParameterCount != MAREA_INTEGER_SIZE(2))) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *first = MAREA_NULLPTR;
    const MAREA_Character8 *second = MAREA_NULLPTR;
    MAREA_IntegerSize firstLength = MAREA_INTEGER_SIZE(0);
    MAREA_IntegerSize secondLength = MAREA_INTEGER_SIZE(0);

    MAREA_Uinteger64 count = MAREA_UINTEGER_64(0);

    if ((MAREA_G_TwoArguments(resolved, resolvedLength, &first, &firstLength, &second, &secondLength) == MAREA_BOOLEAN_FALSE) || (MAREA_CgenParseInteger(second, secondLength, &count) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    MAREA_Uinteger32 element = MAREA_CGEN_KIND_NONE;

    MAREA_CGEN_TRY(MAREA_CgenTypeOfText(generator, MAREA_NULLPTR, MAREA_INTEGER_SIZE(0), first, firstLength, &element));

    if (MAREA_CgenIsValueType(generator, element) == MAREA_BOOLEAN_FALSE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    return MAREA_G_DefineArray(generator, element, count, outType);
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenTypeOfText(MAREA_CgenGenerator *generator, const MAREA_Character8 *head, MAREA_IntegerSize headLength, const MAREA_Character8 *resolved, MAREA_IntegerSize resolvedLength, MAREA_Uinteger32 *outType)
{
    *outType = MAREA_CGEN_KIND_NONE;

    if ((head == MAREA_NULLPTR) || (headLength == MAREA_INTEGER_SIZE(0))) {
        head = resolved;
        headLength = MAREA_INTEGER_SIZE(0);

        while ((resolved != MAREA_NULLPTR) && (headLength < resolvedLength) && (resolved[headLength] != '<') && (resolved[headLength] != '(')) {
            headLength = (headLength + MAREA_INTEGER_SIZE(1));
        }
    }

    if ((head == MAREA_NULLPTR) || (headLength == MAREA_INTEGER_SIZE(0))) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    if (MAREA_CgenSameText(head, headLength, "Self", MAREA_INTEGER_SIZE(4)) == MAREA_BOOLEAN_TRUE) {
        if (generator->selfName == MAREA_NULLPTR) {
            return MAREA_CGEN_EXCEPT_SUCCESS;
        }

        head = generator->selfName;
        headLength = generator->selfLength;
    }

    const MAREA_Uinteger32 SCALAR = MAREA_CgenKindFromText(head, headLength);

    if (SCALAR != MAREA_CGEN_KIND_NONE) {
        *outType = SCALAR;

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    if (MAREA_CgenSameText(head, headLength, "void", MAREA_INTEGER_SIZE(4)) == MAREA_BOOLEAN_TRUE) {
        *outType = MAREA_CGEN_KIND_VOID;

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    const MAREA_IntegerSize SKIP = MAREA_CgenNamespaceSkip(generator, head, headLength);

    const MAREA_IrNode *structure = MAREA_CgenFindDeclaration(generator, MAREA_IR_KIND_STRUCTURE_DECLARATION, (head + SKIP), (headLength - SKIP));

    if (structure != MAREA_NULLPTR) {
        return MAREA_G_DefineStructure(generator, structure, outType);
    }

    const MAREA_IrNode *enumeration = MAREA_CgenFindDeclaration(generator, MAREA_IR_KIND_ENUM_DECLARATION, (head + SKIP), (headLength - SKIP));

    if (enumeration != MAREA_NULLPTR) {
        return MAREA_G_DefineEnum(generator, enumeration, outType);
    }

    return MAREA_G_ArrayType(generator, head, headLength, resolved, resolvedLength, outType);
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenTypeOfNode(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 *outType)
{
    return MAREA_CgenTypeOfText(generator, MAREA_IrNodeGetTypeName(node), MAREA_IrNodeGetTypeNameLength(node), MAREA_IrNodeGetResolvedType(node), MAREA_IrNodeGetResolvedTypeLength(node), outType);
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_AddAlias(MAREA_CgenGenerator *generator, const MAREA_Character8 *from, MAREA_IntegerSize fromLength, const MAREA_Character8 *to, MAREA_IntegerSize toLength)
{
    if (MAREA_CgenSameText(from, fromLength, to, toLength) == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < generator->aliasCount; index = (index + MAREA_INTEGER_SIZE(1))) {
        if (MAREA_CgenSameText(generator->aliases[index].from, generator->aliases[index].fromLength, from, fromLength) == MAREA_BOOLEAN_TRUE) {
            return MAREA_CGEN_EXCEPT_SUCCESS;
        }
    }

    MAREA_Null items = generator->aliases;

    MAREA_CGEN_TRY(MAREA_CgenGrow(generator, &items, &generator->aliasCapacity, generator->aliasCount, sizeof(MAREA_CgenAlias), MAREA_ALIGNOF(MAREA_CgenAlias)));

    generator->aliases = items;

    MAREA_CgenAlias *alias = &generator->aliases[generator->aliasCount];

    alias->from = from;
    alias->fromLength = fromLength;
    alias->to = to;
    alias->toLength = toLength;

    generator->aliasCount = (generator->aliasCount + MAREA_INTEGER_SIZE(1));

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

static MAREA_IntegerSize MAREA_CALL MAREA_G_LastDot(const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    MAREA_IntegerSize position = length;

    while (position > MAREA_INTEGER_SIZE(0)) {
        position = (position - MAREA_INTEGER_SIZE(1));

        if (text[position] == '.') {
            return position;
        }
    }

    return length;
}

// INFO: a spelling and its full path also show the alias of their namespace, once the segments they share are cut.
static MAREA_CgenExcept MAREA_CALL MAREA_G_AddAliases(MAREA_CgenGenerator *generator, const MAREA_Character8 *from, MAREA_IntegerSize fromLength, const MAREA_Character8 *to, MAREA_IntegerSize toLength)
{
    MAREA_CGEN_TRY(MAREA_G_AddAlias(generator, from, fromLength, to, toLength));

    for (;;) {
        const MAREA_IntegerSize FROM_DOT = MAREA_G_LastDot(from, fromLength);
        const MAREA_IntegerSize TO_DOT = MAREA_G_LastDot(to, toLength);

        if ((FROM_DOT == fromLength) || (TO_DOT == toLength) || (MAREA_CgenSameText((from + FROM_DOT), (fromLength - FROM_DOT), (to + TO_DOT), (toLength - TO_DOT)) == MAREA_BOOLEAN_FALSE)) {
            break;
        }

        fromLength = FROM_DOT;
        toLength = TO_DOT;
    }

    return MAREA_G_AddAlias(generator, from, fromLength, to, toLength);
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenCollectAliases(MAREA_CgenGenerator *generator, const MAREA_IrNode *node)
{
    const MAREA_Character8 *head = MAREA_IrNodeGetTypeName(node);
    const MAREA_Character8 *resolved = MAREA_IrNodeGetResolvedType(node);

    if ((head != MAREA_NULLPTR) && (resolved != MAREA_NULLPTR)) {
        const MAREA_IntegerSize LENGTH = MAREA_IrNodeGetResolvedTypeLength(node);

        MAREA_IntegerSize spelled = MAREA_INTEGER_SIZE(0);

        while ((spelled < LENGTH) && (resolved[spelled] != '<') && (resolved[spelled] != '(')) {
            spelled = (spelled + MAREA_INTEGER_SIZE(1));
        }

        if ((spelled > MAREA_INTEGER_SIZE(0)) && ((spelled == LENGTH) || (resolved[spelled] != '('))) {
            MAREA_CGEN_TRY(MAREA_G_AddAliases(generator, resolved, spelled, head, MAREA_IrNodeGetTypeNameLength(node)));
        }
    }

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < MAREA_IrNodeGetChildCount(node); index = (index + MAREA_INTEGER_SIZE(1))) {
        MAREA_CGEN_TRY(MAREA_CgenCollectAliases(generator, MAREA_CgenChildAt(node, index)));
    }

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenDeclareTypes(MAREA_CgenGenerator *generator)
{
    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < MAREA_IrNodeGetChildCount(generator->module); index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_IrNode *child = MAREA_CgenChildAt(generator->module, index);

        const MAREA_IrKind KIND = MAREA_IrNodeGetKind(child);

        if (((KIND != MAREA_IR_KIND_STRUCTURE_DECLARATION) && (KIND != MAREA_IR_KIND_ENUM_DECLARATION)) || (MAREA_G_MeasuredLayout(generator, child) == MAREA_NULLPTR)) {
            continue;
        }

        MAREA_Uinteger32 type = MAREA_CGEN_KIND_NONE;

        MAREA_CGEN_TRY(MAREA_CgenTypeOfText(generator, MAREA_IrNodeGetText(child), MAREA_IrNodeGetTextLength(child), MAREA_NULLPTR, MAREA_INTEGER_SIZE(0), &type));
    }

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

MAREA_Boolean MAREA_CALL MAREA_CgenFindVariant(const MAREA_CgenGenerator *generator, MAREA_Uinteger32 type, const MAREA_Character8 *name, MAREA_IntegerSize length, MAREA_IntegerSize *outVariant)
{
    const MAREA_CgenType *entry = MAREA_G_Entry(generator, type);

    if ((entry == MAREA_NULLPTR) || (entry->category != MAREA_CGEN_CATEGORY_ENUM)) {
        return MAREA_BOOLEAN_FALSE;
    }

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < entry->memberCount; index = (index + MAREA_INTEGER_SIZE(1))) {
        if (MAREA_CgenSameText(entry->variants[index].name, entry->variants[index].nameLength, name, length) == MAREA_BOOLEAN_TRUE) {
            *outVariant = index;

            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

const MAREA_CgenField *MAREA_CALL MAREA_CgenFindField(const MAREA_CgenGenerator *generator, MAREA_Uinteger32 type, const MAREA_Character8 *name, MAREA_IntegerSize length)
{
    const MAREA_CgenType *entry = MAREA_G_Entry(generator, type);

    if ((entry == MAREA_NULLPTR) || (entry->category != MAREA_CGEN_CATEGORY_STRUCTURE)) {
        return MAREA_NULLPTR;
    }

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < entry->memberCount; index = (index + MAREA_INTEGER_SIZE(1))) {
        if (MAREA_CgenSameText(entry->fields[index].name, entry->fields[index].nameLength, name, length) == MAREA_BOOLEAN_TRUE) {
            return &entry->fields[index];
        }
    }

    return MAREA_NULLPTR;
}
