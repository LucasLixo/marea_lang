// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/cgen/MAREACgenInternal.h"

#include <string.h>

typedef struct MAREA_G_KindRow
{
    const MAREA_Character8 *name;
    MAREA_Uinteger8 kind;
    MAREA_Uinteger8 padding[7];
} MAREA_G_KindRow;

static const MAREA_G_KindRow MAREA_G_KIND_TABLE[] = {
    { .name = "sinteger8", .kind = MAREA_CGEN_KIND_SINTEGER_8 },
    { .name = "sinteger16", .kind = MAREA_CGEN_KIND_SINTEGER_16 },
    { .name = "sinteger32", .kind = MAREA_CGEN_KIND_SINTEGER_32 },
    { .name = "sinteger64", .kind = MAREA_CGEN_KIND_SINTEGER_64 },
    { .name = "uinteger8", .kind = MAREA_CGEN_KIND_UINTEGER_8 },
    { .name = "uinteger16", .kind = MAREA_CGEN_KIND_UINTEGER_16 },
    { .name = "uinteger32", .kind = MAREA_CGEN_KIND_UINTEGER_32 },
    { .name = "uinteger64", .kind = MAREA_CGEN_KIND_UINTEGER_64 },
    { .name = "float32", .kind = MAREA_CGEN_KIND_FLOAT_32 },
    { .name = "float64", .kind = MAREA_CGEN_KIND_FLOAT_64 },
    { .name = "boolean", .kind = MAREA_CGEN_KIND_BOOLEAN },
};

MAREA_Uinteger8 MAREA_CALL MAREA_CgenKindFromText(const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    if (text == MAREA_NULLPTR) {
        return MAREA_CGEN_KIND_NONE;
    }

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < (MAREA_IntegerSize)MAREA_ARRAY_SIZE(MAREA_G_KIND_TABLE); index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_G_KindRow *row = &MAREA_G_KIND_TABLE[index];

        if (((MAREA_IntegerSize)strlen(row->name) == length) && (memcmp(row->name, text, length) == 0)) {
            return row->kind;
        }
    }

    return MAREA_CGEN_KIND_NONE;
}

MAREA_Uinteger8 MAREA_CALL MAREA_CgenKindOfNode(const MAREA_IrNode *node)
{
    const MAREA_Character8 *resolved = MAREA_IrNodeGetResolvedType(node);

    if (resolved != MAREA_NULLPTR) {
        const MAREA_Uinteger8 KIND = MAREA_CgenKindFromText(resolved, MAREA_IrNodeGetResolvedTypeLength(node));

        if (KIND != MAREA_CGEN_KIND_NONE) {
            return KIND;
        }
    }

    return MAREA_CgenKindFromText(MAREA_IrNodeGetTypeName(node), MAREA_IrNodeGetTypeNameLength(node));
}

void MAREA_CALL MAREA_CgenWriteCType(MAREA_CgenBuffer *buffer, MAREA_Uinteger8 kind)
{
    switch (kind) {
    case MAREA_CGEN_KIND_SINTEGER_8:
        MAREA_CgenWriteText(buffer, "int8_t");
        return;
    case MAREA_CGEN_KIND_SINTEGER_16:
        MAREA_CgenWriteText(buffer, "int16_t");
        return;
    case MAREA_CGEN_KIND_SINTEGER_32:
        MAREA_CgenWriteText(buffer, "int32_t");
        return;
    case MAREA_CGEN_KIND_SINTEGER_64:
        MAREA_CgenWriteText(buffer, "int64_t");
        return;
    case MAREA_CGEN_KIND_UINTEGER_8:
        MAREA_CgenWriteText(buffer, "uint8_t");
        return;
    case MAREA_CGEN_KIND_UINTEGER_16:
        MAREA_CgenWriteText(buffer, "uint16_t");
        return;
    case MAREA_CGEN_KIND_UINTEGER_32:
        MAREA_CgenWriteText(buffer, "uint32_t");
        return;
    case MAREA_CGEN_KIND_UINTEGER_64:
        MAREA_CgenWriteText(buffer, "uint64_t");
        return;
    case MAREA_CGEN_KIND_FLOAT_32:
        MAREA_CgenWriteText(buffer, "float");
        return;
    case MAREA_CGEN_KIND_FLOAT_64:
        MAREA_CgenWriteText(buffer, "double");
        return;
    case MAREA_CGEN_KIND_BOOLEAN:
        MAREA_CgenWriteText(buffer, "bool");
        return;
    default:
        MAREA_CgenWriteText(buffer, "void");
        return;
    }
}

const MAREA_Character8 *MAREA_CALL MAREA_CgenKindSuffix(MAREA_Uinteger8 kind)
{
    switch (kind) {
    case MAREA_CGEN_KIND_SINTEGER_8:
        return "s8";
    case MAREA_CGEN_KIND_SINTEGER_16:
        return "s16";
    case MAREA_CGEN_KIND_SINTEGER_32:
        return "s32";
    case MAREA_CGEN_KIND_SINTEGER_64:
        return "s64";
    case MAREA_CGEN_KIND_UINTEGER_8:
        return "u8";
    case MAREA_CGEN_KIND_UINTEGER_16:
        return "u16";
    case MAREA_CGEN_KIND_UINTEGER_32:
        return "u32";
    default:
        return "u64";
    }
}

void MAREA_CALL MAREA_CgenWriteDefaultValue(MAREA_CgenBuffer *buffer, MAREA_Uinteger8 kind)
{
    switch (kind) {
    case MAREA_CGEN_KIND_FLOAT_32:
    case MAREA_CGEN_KIND_FLOAT_64:
        MAREA_CgenWriteText(buffer, "0.0");
        return;
    case MAREA_CGEN_KIND_BOOLEAN:
        MAREA_CgenWriteText(buffer, "false");
        return;
    default:
        MAREA_CgenWriteText(buffer, "0");
        return;
    }
}
