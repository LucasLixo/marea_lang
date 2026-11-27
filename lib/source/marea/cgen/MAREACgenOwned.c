// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/cgen/MAREACgenInternal.h"

#include <string.h>

// INFO: the ownership of the values that hold storage of the allocator, which is a vector or a value that holds one.
// A value is taken over by a copy of its bytes that leaves zeros behind, so each storage has one owner and a destroy
// of a value that was taken does nothing. A local and a borrowed temporary are destroyed when their block ends and
// at each jump out of it.

// INFO: the destroy operator of a structure of the unit, or null when it declares none.
static MAREA_CgenFunction *MAREA_CALL MAREA_G_DestroyOf(const MAREA_CgenGenerator *generator, MAREA_Uinteger32 type)
{
    const MAREA_CgenType *entry = MAREA_CgenTypeAt(generator, type);

    if ((entry == MAREA_NULLPTR) || (entry->category != MAREA_CGEN_CATEGORY_STRUCTURE)) {
        return MAREA_NULLPTR;
    }

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < generator->functionCount; index = (index + MAREA_INTEGER_SIZE(1))) {
        MAREA_CgenFunction *function = &generator->functions[index];

        if ((MAREA_IrNodeIsDestroyOperator(function->node) == MAREA_BOOLEAN_TRUE) && (MAREA_CgenSameText(function->name, function->ownerLength, entry->name, entry->nameLength) == MAREA_BOOLEAN_TRUE)) {
            return function;
        }
    }

    return MAREA_NULLPTR;
}

MAREA_Boolean MAREA_CALL MAREA_CgenNeedsDrop(const MAREA_CgenGenerator *generator, MAREA_Uinteger32 type)
{
    const MAREA_CgenType *entry = MAREA_CgenTypeAt(generator, type);

    if (entry == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    if (entry->category == MAREA_CGEN_CATEGORY_VECTOR) {
        return MAREA_BOOLEAN_TRUE;
    }

    if (entry->category == MAREA_CGEN_CATEGORY_ARRAY) {
        return ((entry->count > MAREA_UINTEGER_64(0)) && (MAREA_CgenNeedsDrop(generator, entry->element) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    }

    if (entry->category == MAREA_CGEN_CATEGORY_STRUCTURE) {
        for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < entry->memberCount; index = (index + MAREA_INTEGER_SIZE(1))) {
            if (MAREA_CgenNeedsDrop(generator, entry->fields[index].type) == MAREA_BOOLEAN_TRUE) {
                return MAREA_BOOLEAN_TRUE;
            }
        }

        return MAREA_BOOLEAN_FALSE;
    }

    for (MAREA_IntegerSize variant = MAREA_INTEGER_SIZE(0); (entry->category == MAREA_CGEN_CATEGORY_ENUM) && (variant < entry->memberCount); variant = (variant + MAREA_INTEGER_SIZE(1))) {
        for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < entry->variants[variant].fieldCount; index = (index + MAREA_INTEGER_SIZE(1))) {
            if (MAREA_CgenNeedsDrop(generator, entry->variants[variant].fields[index].type) == MAREA_BOOLEAN_TRUE) {
                return MAREA_BOOLEAN_TRUE;
            }
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

MAREA_Boolean MAREA_CALL MAREA_CgenElementNeedsDrop(const MAREA_CgenGenerator *generator, MAREA_Uinteger32 type)
{
    if ((MAREA_CgenNeedsDrop(generator, type) == MAREA_BOOLEAN_TRUE) || (MAREA_G_DestroyOf(generator, type) != MAREA_NULLPTR)) {
        return MAREA_BOOLEAN_TRUE;
    }

    const MAREA_CgenType *entry = MAREA_CgenTypeAt(generator, type);

    return ((entry != MAREA_NULLPTR) && (entry->category == MAREA_CGEN_CATEGORY_ARRAY) && (entry->count > MAREA_UINTEGER_64(0))) ? MAREA_CgenElementNeedsDrop(generator, entry->element) : MAREA_BOOLEAN_FALSE;
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_DropArray(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, const MAREA_Character8 *place, MAREA_Uinteger32 type, MAREA_Boolean isElement)
{
    const MAREA_Uinteger64 COUNT = MAREA_CgenTypeAt(generator, type)->count;
    const MAREA_Uinteger32 ELEMENT = MAREA_CgenTypeAt(generator, type)->element;

    const MAREA_Character8 *index = MAREA_CgenTemporaryName(generator);

    MAREA_CGEN_WRITE(generator->buffer, "for (uint64_t ", index, " = ", MAREA_CgenNumber(generator, COUNT), "ULL; ", index, " > 0u; ", index, " -= 1u) {\n");
    MAREA_CGEN_TRY(MAREA_CgenWriteDrop(generator, node, MAREA_CGEN_JOIN(generator, "(", place, ").items[", index, " - 1u]"), ELEMENT, isElement));
    MAREA_CgenWriteText(generator->buffer, "}\n");

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_DropVector(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, const MAREA_Character8 *place, MAREA_Uinteger32 type)
{
    const MAREA_Uinteger32 ELEMENT = MAREA_CgenTypeAt(generator, type)->element;

    if (MAREA_CgenElementNeedsDrop(generator, ELEMENT) == MAREA_BOOLEAN_TRUE) {
        const MAREA_Character8 *index = MAREA_CgenTemporaryName(generator);

        MAREA_CGEN_WRITE(generator->buffer, "for (uint64_t ", index, " = (", place, ").size; ", index, " > 0u; ", index, " -= 1u) {\n");
        MAREA_CGEN_TRY(MAREA_CgenWriteDrop(generator, node, MAREA_CgenElementText(generator, place, ELEMENT, MAREA_CGEN_JOIN(generator, index, " - 1u")), ELEMENT, MAREA_BOOLEAN_TRUE));
        MAREA_CgenWriteText(generator->buffer, "}\n");
    }

    MAREA_CgenWriteText(generator->buffer, "{\n");

    const MAREA_Character8 *context = MAREA_CgenWriteContext(generator);

    MAREA_CGEN_WRITE(generator->buffer, "MAREA_VectorRelease(&", context, ", &(", place, "));\n}\n");

    generator->usesVector = MAREA_BOOLEAN_TRUE;

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_DropFields(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, const MAREA_Character8 *place, const MAREA_CgenField *fields, MAREA_IntegerSize count)
{
    for (MAREA_IntegerSize index = count; index > MAREA_INTEGER_SIZE(0); index = (index - MAREA_INTEGER_SIZE(1))) {
        const MAREA_CgenField *field = &fields[(index - MAREA_INTEGER_SIZE(1))];

        if (MAREA_CgenNeedsDrop(generator, field->type) == MAREA_BOOLEAN_TRUE) {
            MAREA_CGEN_TRY(MAREA_CgenWriteDrop(generator, node, MAREA_CGEN_JOIN(generator, "(", place, ").", MAREA_CgenMangled(generator, "f_", field->name, field->nameLength)), field->type, MAREA_BOOLEAN_FALSE));
        }
    }

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_DropEnum(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, const MAREA_Character8 *place, MAREA_Uinteger32 type)
{
    const MAREA_CgenVariant *variants = MAREA_CgenTypeAt(generator, type)->variants;
    const MAREA_IntegerSize COUNT = MAREA_CgenTypeAt(generator, type)->memberCount;

    MAREA_CGEN_WRITE(generator->buffer, "switch ((", place, ").marea_tag) {\n");

    for (MAREA_IntegerSize variant = MAREA_INTEGER_SIZE(0); variant < COUNT; variant = (variant + MAREA_INTEGER_SIZE(1))) {
        MAREA_CGEN_WRITE(generator->buffer, "case ", MAREA_CgenNumber(generator, variant), ":\n");

        if (variants[variant].fieldCount > MAREA_INTEGER_SIZE(0)) {
            MAREA_CGEN_TRY(MAREA_G_DropFields(generator, node, MAREA_CGEN_JOIN(generator, "(", place, ").marea_data.v_", MAREA_CgenNumber(generator, variant)), variants[variant].fields, variants[variant].fieldCount));
        }

        MAREA_CgenWriteText(generator->buffer, "break;\n");
    }

    MAREA_CgenWriteText(generator->buffer, "default:\nbreak;\n}\n");

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteDrop(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, const MAREA_Character8 *place, MAREA_Uinteger32 type, MAREA_Boolean isElement)
{
    // INFO: a trap runs no destroy operator; it only gives storage back.
    MAREA_CgenFunction *destroy = ((isElement == MAREA_BOOLEAN_TRUE) && (generator->unwinding == MAREA_BOOLEAN_FALSE)) ? MAREA_G_DestroyOf(generator, type) : MAREA_NULLPTR;

    if (destroy != MAREA_NULLPTR) {
        MAREA_CgenValue ignored = { 0 };

        MAREA_CGEN_TRY(MAREA_CgenWriteUnitCall(generator, node, destroy, MAREA_NULLPTR, MAREA_NULLPTR, &ignored));
    }

    const MAREA_Boolean NEEDED = (isElement == MAREA_BOOLEAN_TRUE) ? MAREA_CgenElementNeedsDrop(generator, type) : MAREA_CgenNeedsDrop(generator, type);

    if (NEEDED == MAREA_BOOLEAN_FALSE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    switch (MAREA_CgenCategoryOf(generator, type)) {
    case MAREA_CGEN_CATEGORY_VECTOR:
        return MAREA_G_DropVector(generator, node, place, type);
    case MAREA_CGEN_CATEGORY_ARRAY:
        return MAREA_G_DropArray(generator, node, place, type, isElement);
    case MAREA_CGEN_CATEGORY_STRUCTURE:
        return MAREA_G_DropFields(generator, node, place, MAREA_CgenTypeAt(generator, type)->fields, MAREA_CgenTypeAt(generator, type)->memberCount);
    case MAREA_CGEN_CATEGORY_ENUM:
        return (MAREA_CgenNeedsDrop(generator, type) == MAREA_BOOLEAN_TRUE) ? MAREA_G_DropEnum(generator, node, place, type) : MAREA_CGEN_EXCEPT_SUCCESS;
    default:
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_CopyVector(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, const MAREA_Character8 *target, const MAREA_Character8 *source, MAREA_Uinteger32 type)
{
    const MAREA_Uinteger32 ELEMENT = MAREA_CgenTypeAt(generator, type)->element;

    const MAREA_Character8 *index = MAREA_CgenTemporaryName(generator);

    MAREA_CGEN_WRITE(generator->buffer, "if ((", source, ").size > 0u) {\n");

    const MAREA_Character8 *context = MAREA_CgenWriteContext(generator);

    MAREA_CGEN_WRITE(generator->buffer, "MAREA_VectorReserve(&", context, ", &(", target, "), (", source, ").size, ", MAREA_CgenStrideText(generator, ELEMENT), ");\n");

    MAREA_CgenWriteMemoryGuard(generator, context);

    MAREA_CGEN_WRITE(generator->buffer, "}\nfor (uint64_t ", index, " = 0; ", index, " < (", source, ").size; ", index, " += 1u) {\n");
    MAREA_CGEN_TRY(MAREA_CgenWriteDeepCopy(generator, node, MAREA_CgenElementText(generator, target, ELEMENT, index), MAREA_CgenElementText(generator, source, ELEMENT, index), ELEMENT));
    MAREA_CGEN_WRITE(generator->buffer, "(", target, ").size = ", index, " + 1u;\n}\n");

    generator->usesVector = MAREA_BOOLEAN_TRUE;

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteDeepCopy(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, const MAREA_Character8 *target, const MAREA_Character8 *source, MAREA_Uinteger32 type)
{
    if (MAREA_CgenNeedsDrop(generator, type) == MAREA_BOOLEAN_FALSE) {
        MAREA_CgenStore(generator, target, type, source);

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    const MAREA_Uinteger8 CATEGORY = MAREA_CgenCategoryOf(generator, type);

    // INFO: the target is zero, which is an empty vector, so the copy of a vector starts from it.
    if (CATEGORY == MAREA_CGEN_CATEGORY_VECTOR) {
        MAREA_CGEN_WRITE(generator->buffer, "memset(&(", target, "), 0, sizeof(", target, "));\n");

        return MAREA_G_CopyVector(generator, node, target, source, type);
    }

    if (CATEGORY == MAREA_CGEN_CATEGORY_ARRAY) {
        const MAREA_Uinteger64 COUNT = MAREA_CgenTypeAt(generator, type)->count;
        const MAREA_Uinteger32 ELEMENT = MAREA_CgenTypeAt(generator, type)->element;

        const MAREA_Character8 *index = MAREA_CgenTemporaryName(generator);

        MAREA_CGEN_WRITE(generator->buffer, "for (uint64_t ", index, " = 0; ", index, " < ", MAREA_CgenNumber(generator, COUNT), "ULL; ", index, " += 1u) {\n");
        MAREA_CGEN_TRY(MAREA_CgenWriteDeepCopy(generator, node, MAREA_CGEN_JOIN(generator, "(", target, ").items[", index, "]"), MAREA_CGEN_JOIN(generator, "(", source, ").items[", index, "]"), ELEMENT));
        MAREA_CgenWriteText(generator->buffer, "}\n");

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    if (CATEGORY != MAREA_CGEN_CATEGORY_STRUCTURE) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A copy of an enum that holds a vector is not written.");
    }

    MAREA_CgenStore(generator, target, type, source);

    const MAREA_CgenField *fields = MAREA_CgenTypeAt(generator, type)->fields;
    const MAREA_IntegerSize COUNT = MAREA_CgenTypeAt(generator, type)->memberCount;

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        if (MAREA_CgenNeedsDrop(generator, fields[index].type) == MAREA_BOOLEAN_TRUE) {
            const MAREA_Character8 *name = MAREA_CgenMangled(generator, "f_", fields[index].name, fields[index].nameLength);

            MAREA_CGEN_TRY(MAREA_CgenWriteDeepCopy(generator, node, MAREA_CGEN_JOIN(generator, "(", target, ").", name), MAREA_CGEN_JOIN(generator, "(", source, ").", name), fields[index].type));
        }
    }

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenPushOwned(MAREA_CgenGenerator *generator, const MAREA_Character8 *text, MAREA_Uinteger32 type)
{
    MAREA_Null items = generator->owned;

    MAREA_CGEN_TRY(MAREA_CgenGrow(generator, &items, &generator->ownedCapacity, generator->ownedCount, sizeof(MAREA_CgenOwned), MAREA_ALIGNOF(MAREA_CgenOwned)));

    generator->owned = items;
    generator->owned[generator->ownedCount] = (MAREA_CgenOwned){ .text = text, .type = type };
    generator->ownedCount = (generator->ownedCount + MAREA_INTEGER_SIZE(1));

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteDrops(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_IntegerSize mark)
{
    for (MAREA_IntegerSize index = generator->ownedCount; (index > mark) && (generator->functionFailed == MAREA_BOOLEAN_FALSE); index = (index - MAREA_INTEGER_SIZE(1))) {
        const MAREA_CgenOwned OWNED = generator->owned[(index - MAREA_INTEGER_SIZE(1))];

        MAREA_CGEN_TRY(MAREA_CgenWriteDrop(generator, node, OWNED.text, OWNED.type, MAREA_BOOLEAN_FALSE));
    }

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

// INFO: the temporary lives in the frame of the function, so a destroy outside the block that builds it still reaches it. A loop builds it again, so the old value goes first.
MAREA_CgenExcept MAREA_CALL MAREA_CgenHold(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_CgenValue *ioValue)
{
    if (MAREA_CgenNeedsDrop(generator, ioValue->type) == MAREA_BOOLEAN_FALSE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *name = MAREA_CgenTemporaryName(generator);

    MAREA_CGEN_WRITE(generator->prelude, MAREA_CgenTypeName(generator, ioValue->type), " ", name, ";\nmemset(&", name, ", 0, sizeof(", name, "));\n");

    MAREA_CGEN_TRY(MAREA_CgenWriteDrop(generator, node, name, ioValue->type, MAREA_BOOLEAN_FALSE));

    MAREA_CgenStore(generator, name, ioValue->type, ioValue->text);

    ioValue->text = name;
    ioValue->stable = MAREA_BOOLEAN_FALSE;
    ioValue->unaligned = MAREA_BOOLEAN_FALSE;

    return MAREA_CgenPushOwned(generator, name, ioValue->type);
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteOwned(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 expected, MAREA_CgenValue *outValue)
{
    const MAREA_IrKind KIND = MAREA_IrNodeGetKind(node);

    const MAREA_Boolean IS_PLACE = ((KIND == MAREA_IR_KIND_IDENTIFIER) || (KIND == MAREA_IR_KIND_MEMBER_ACCESS) || (KIND == MAREA_IR_KIND_INDEX_ACCESS)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    if ((IS_PLACE == MAREA_BOOLEAN_FALSE) || (MAREA_CgenNeedsDrop(generator, expected) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_CgenWriteTyped(generator, node, expected, outValue);
    }

    MAREA_CgenValue place = { 0 };

    MAREA_CGEN_TRY(MAREA_CgenWritePlace(generator, node, expected, &place));

    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    if (place.type != expected) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A value has another type than the place that takes it.");
    }

    *outValue = place;

    MAREA_CgenSnapshot(generator, outValue);
    MAREA_CGEN_WRITE(generator->buffer, "memset(&(", place.text, "), 0, sizeof(", place.text, "));\n");

    return MAREA_CGEN_EXCEPT_SUCCESS;
}
