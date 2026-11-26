// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/cgen/MAREACgenInternal.h"

// INFO: writes the entry of a literal at one position of an array. The position is checked here when it is a constant, since sema accepts only a literal count.
static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteElement(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, const MAREA_Character8 *target, MAREA_Uinteger32 type, MAREA_Uinteger64 position, const MAREA_IrNode *value)
{
    if (position >= MAREA_CgenTypeAt(generator, type)->count) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A literal has more elements than its array holds.");
    }

    const MAREA_Uinteger32 ELEMENT = MAREA_CgenTypeAt(generator, type)->element;

    MAREA_CgenValue element = { 0 };

    MAREA_CGEN_TRY(MAREA_CgenWriteTyped(generator, value, ELEMENT, &element));

    MAREA_CgenStore(generator, MAREA_CGEN_JOIN(generator, "(", target, ").items[", MAREA_CgenNumber(generator, position), "]"), ELEMENT, element.text);

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

// INFO: an indexed entry with an index that is not a literal is checked when the program runs.
static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteIndexedElement(MAREA_CgenGenerator *generator, const MAREA_IrNode *entry, const MAREA_Character8 *target, MAREA_Uinteger32 type, const MAREA_IrNode *indexNode)
{
    const MAREA_Uinteger64 COUNT = MAREA_CgenTypeAt(generator, type)->count;
    const MAREA_Uinteger32 ELEMENT = MAREA_CgenTypeAt(generator, type)->element;

    MAREA_CgenValue index = { 0 };

    MAREA_CGEN_TRY(MAREA_CgenWriteValue(generator, indexNode, MAREA_CGEN_KIND_NONE, &index));

    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    if (MAREA_CgenIsInteger(index.type) == MAREA_BOOLEAN_FALSE) {
        return MAREA_CgenGeneratorFail(generator, entry, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "An index is an integer.");
    }

    const MAREA_Character8 *name = MAREA_CgenTemporaryName(generator);

    MAREA_CGEN_WRITE(generator->buffer, "uint64_t ", name, " = (uint64_t)(", index.text, ");\nif (", name, " >= ", MAREA_CgenNumber(generator, COUNT), "ULL) {\n");
    MAREA_CgenWriteTrap(generator, "MAREA_RT_TRAP_INDEX_OUT_OF_BOUNDS");
    MAREA_CgenWriteText(generator->buffer, "}\n");

    MAREA_CgenValue element = { 0 };

    MAREA_CGEN_TRY(MAREA_CgenWriteTyped(generator, MAREA_CgenChildAt(entry, MAREA_INTEGER_SIZE(1)), ELEMENT, &element));

    MAREA_CgenStore(generator, MAREA_CGEN_JOIN(generator, "(", target, ").items[", name, "]"), ELEMENT, element.text);

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteFieldEntry(MAREA_CgenGenerator *generator, const MAREA_IrNode *entry, const MAREA_Character8 *target, MAREA_Uinteger32 type)
{
    const MAREA_CgenField *field = MAREA_CgenFindField(generator, type, MAREA_IrNodeGetText(entry), MAREA_IrNodeGetTextLength(entry));

    if (field == MAREA_NULLPTR) {
        return MAREA_CgenGeneratorFail(generator, entry, MAREA_CGEN_DIAGNOSTIC_CODE_UNRESOLVED_NAME, "A structure has no field of this name.");
    }

    const MAREA_Uinteger32 FIELD_TYPE = field->type;

    const MAREA_Character8 *name = MAREA_CgenMangled(generator, "f_", field->name, field->nameLength);

    MAREA_CgenValue value = { 0 };

    MAREA_CGEN_TRY(MAREA_CgenWriteTyped(generator, MAREA_CgenChildAt(entry, MAREA_INTEGER_SIZE(0)), FIELD_TYPE, &value));

    MAREA_CgenStore(generator, MAREA_CGEN_JOIN(generator, "(", target, ").", name), FIELD_TYPE, value.text);

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteLiteralValue(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 expected, MAREA_CgenValue *outValue)
{
    MAREA_Uinteger32 type = expected;

    if ((expected == MAREA_CGEN_KIND_NONE) || (expected <= MAREA_CGEN_KIND_VOID)) {
        MAREA_CGEN_TRY(MAREA_CgenTypeOfNode(generator, node, &type));
    }

    const MAREA_Uinteger8 CATEGORY = MAREA_CgenCategoryOf(generator, type);

    if ((MAREA_CgenIsValueType(generator, type) == MAREA_BOOLEAN_FALSE) || ((CATEGORY != MAREA_CGEN_CATEGORY_STRUCTURE) && (CATEGORY != MAREA_CGEN_CATEGORY_ARRAY))) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A literal makes a type that cgen does not write.");
    }

    MAREA_CgenValue made = { 0 };

    MAREA_CgenZeroTemporary(generator, type, &made);

    MAREA_Uinteger64 position = MAREA_UINTEGER_64(0);
    MAREA_Boolean positionKnown = MAREA_BOOLEAN_TRUE;

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); (generator->functionFailed == MAREA_BOOLEAN_FALSE) && (index < MAREA_IrNodeGetChildCount(node)); index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_IrNode *entry = MAREA_CgenChildAt(node, index);

        if ((MAREA_IrNodeGetKind(entry) == MAREA_IR_KIND_FIELD_ENTRY) && (CATEGORY == MAREA_CGEN_CATEGORY_STRUCTURE)) {
            MAREA_CGEN_TRY(MAREA_G_WriteFieldEntry(generator, entry, made.text, type));
        } else if ((MAREA_IrNodeGetKind(entry) == MAREA_IR_KIND_VALUE_ENTRY) && (CATEGORY == MAREA_CGEN_CATEGORY_ARRAY) && (positionKnown == MAREA_BOOLEAN_TRUE)) {
            MAREA_CGEN_TRY(MAREA_G_WriteElement(generator, entry, made.text, type, position, MAREA_CgenChildAt(entry, MAREA_INTEGER_SIZE(0))));

            position = (position + MAREA_UINTEGER_64(1));
        } else if ((MAREA_IrNodeGetKind(entry) == MAREA_IR_KIND_INDEX_ENTRY) && (CATEGORY == MAREA_CGEN_CATEGORY_ARRAY)) {
            const MAREA_IrNode *indexNode = MAREA_CgenChildAt(entry, MAREA_INTEGER_SIZE(0));

            if ((MAREA_IrNodeGetKind(indexNode) == MAREA_IR_KIND_INTEGER_LITERAL) && (MAREA_CgenParseInteger(MAREA_IrNodeGetText(indexNode), MAREA_IrNodeGetTextLength(indexNode), &position) == MAREA_BOOLEAN_TRUE)) {
                MAREA_CGEN_TRY(MAREA_G_WriteElement(generator, entry, made.text, type, position, MAREA_CgenChildAt(entry, MAREA_INTEGER_SIZE(1))));

                position = (position + MAREA_UINTEGER_64(1));
                positionKnown = MAREA_BOOLEAN_TRUE;
            } else {
                MAREA_CGEN_TRY(MAREA_G_WriteIndexedElement(generator, entry, made.text, type, indexNode));

                positionKnown = MAREA_BOOLEAN_FALSE;
            }
        } else {
            return MAREA_CgenGeneratorFail(generator, entry, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A literal holds an entry that is not written.");
        }
    }

    *outValue = made;

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

// INFO: the arguments run first, in order, then the defaults of the fields that no argument gives, which sema evaluates in the scope of the module.
MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteVariant(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 type, MAREA_IntegerSize variant, const MAREA_IrNode *arguments, MAREA_CgenValue *outValue)
{
    if (MAREA_CgenIsValueType(generator, type) == MAREA_BOOLEAN_FALSE) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "The enum has no layout that cgen writes.");
    }

    const MAREA_CgenType *entry = MAREA_CgenTypeAt(generator, type);

    const MAREA_CgenField *fields = entry->variants[variant].fields;
    const MAREA_IntegerSize COUNT = entry->variants[variant].fieldCount;
    const MAREA_Uinteger32 TAG = entry->element;
    const MAREA_Character8 *name = entry->name;
    const MAREA_IntegerSize NAME_LENGTH = entry->nameLength;

    const MAREA_IntegerSize GIVEN = (arguments == MAREA_NULLPTR) ? MAREA_INTEGER_SIZE(0) : MAREA_IrNodeGetChildCount(arguments);

    if (GIVEN > COUNT) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A variant takes fewer values than the call gives.");
    }

    const MAREA_Character8 *tagText = MAREA_CGEN_JOIN(generator, "((", MAREA_CgenTypeName(generator, TAG), ")", MAREA_CgenNumber(generator, variant), ")");

    if (MAREA_CgenIsAggregate(generator, type) == MAREA_BOOLEAN_FALSE) {
        outValue->text = MAREA_CGEN_JOIN(generator, "((", MAREA_CgenTypeName(generator, type), ")", MAREA_CgenNumber(generator, variant), ")");
        outValue->type = type;
        outValue->stable = MAREA_BOOLEAN_TRUE;
        outValue->unaligned = MAREA_BOOLEAN_FALSE;

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    MAREA_Null memory = MAREA_NULLPTR;

    MAREA_CGEN_TRY(MAREA_CgenAllocate(generator, (GIVEN * sizeof(MAREA_CgenValue)), MAREA_ALIGNOF(MAREA_CgenValue), &memory));

    MAREA_CgenValue *values = memory;

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < GIVEN; index = (index + MAREA_INTEGER_SIZE(1))) {
        MAREA_CGEN_TRY(MAREA_CgenWriteTyped(generator, MAREA_CgenChildAt(arguments, index), fields[index].type, &values[index]));

        if ((generator->functionFailed == MAREA_BOOLEAN_FALSE) && (values[index].stable == MAREA_BOOLEAN_FALSE)) {
            MAREA_CgenSnapshot(generator, &values[index]);
        }
    }

    MAREA_CgenValue made = { 0 };

    MAREA_CgenZeroTemporary(generator, type, &made);

    MAREA_CGEN_WRITE(generator->buffer, "(", made.text, ").marea_tag = ", tagText, ";\n");

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); (generator->functionFailed == MAREA_BOOLEAN_FALSE) && (index < COUNT); index = (index + MAREA_INTEGER_SIZE(1))) {
        MAREA_CgenValue value = { 0 };

        if (index < GIVEN) {
            value = values[index];
        } else {
            const MAREA_IrNode *given = MAREA_CgenChildAt(fields[index].declaration, MAREA_INTEGER_SIZE(0));

            if (given == MAREA_NULLPTR) {
                return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A field of a variant has no value and no default.");
            }

            MAREA_CGEN_TRY(MAREA_CgenWriteModuleScoped(generator, given, name, NAME_LENGTH, fields[index].type, &value));
        }

        MAREA_CgenStore(generator, MAREA_CGEN_JOIN(generator, "(", made.text, ").marea_data.v_", MAREA_CgenNumber(generator, variant), ".", MAREA_CgenMangled(generator, "f_", fields[index].name, fields[index].nameLength)), fields[index].type, value.text);
    }

    *outValue = made;

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsDestroy(const MAREA_CgenGenerator *generator, const MAREA_CgenFunction *function)
{
    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < generator->destroyCount; index = (index + MAREA_INTEGER_SIZE(1))) {
        if (MAREA_CgenSameText(MAREA_IrNodeGetText(generator->destroys[index]), MAREA_IrNodeGetTextLength(generator->destroys[index]), function->name, function->nameLength) == MAREA_BOOLEAN_TRUE) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

// INFO: the shape of a construct operator: a function of the type with no result and no self, where every parameter has a default, that is not the destroy of the type. The ir does not name it.
static MAREA_Boolean MAREA_CALL MAREA_G_IsConstructShape(const MAREA_CgenGenerator *generator, const MAREA_CgenFunction *function, const MAREA_CgenType *type, MAREA_IntegerSize count)
{
    if ((function->ownerLength != type->nameLength) || (MAREA_CgenSameText(function->name, function->ownerLength, type->name, type->nameLength) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_BOOLEAN_FALSE;
    }

    if ((MAREA_IrNodeGetResolvedType(function->node) != MAREA_NULLPTR) || (function->parameterCount != count) || (MAREA_G_IsDestroy(generator, function) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_IrNode *parameters = MAREA_CgenChildAt(function->node, MAREA_INTEGER_SIZE(0));

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < count; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_IrNode *parameter = MAREA_CgenChildAt(parameters, index);

        if ((MAREA_IrNodeGetChildCount(parameter) == MAREA_INTEGER_SIZE(0)) || (MAREA_CgenTextIs(parameter, "self") == MAREA_BOOLEAN_TRUE)) {
            return MAREA_BOOLEAN_FALSE;
        }
    }

    return MAREA_BOOLEAN_TRUE;
}

// INFO: Type(args) and Type{...} of a type with construct make the zero value or the literal first, then run the construct operator. With no operator and no argument the value is only zeros.
MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteConstruct(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_CgenValue *outValue)
{
    MAREA_Uinteger32 type = MAREA_CGEN_KIND_NONE;

    MAREA_CGEN_TRY(MAREA_CgenTypeOfNode(generator, node, &type));

    const MAREA_Uinteger8 CATEGORY = MAREA_CgenCategoryOf(generator, type);

    if ((MAREA_CgenIsValueType(generator, type) == MAREA_BOOLEAN_FALSE) || ((CATEGORY != MAREA_CGEN_CATEGORY_STRUCTURE) && (CATEGORY != MAREA_CGEN_CATEGORY_ENUM))) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A constructed type has no form that cgen writes.");
    }

    const MAREA_IrNode *arguments = MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(0));
    const MAREA_IrNode *literal = MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(1));

    const MAREA_IntegerSize COUNT = (arguments == MAREA_NULLPTR) ? MAREA_INTEGER_SIZE(0) : MAREA_IrNodeGetChildCount(arguments);

    MAREA_CgenFunction *construct = MAREA_NULLPTR;
    MAREA_IntegerSize found = MAREA_INTEGER_SIZE(0);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < generator->functionCount; index = (index + MAREA_INTEGER_SIZE(1))) {
        if (MAREA_G_IsConstructShape(generator, &generator->functions[index], MAREA_CgenTypeAt(generator, type), COUNT) == MAREA_BOOLEAN_TRUE) {
            construct = &generator->functions[index];
            found = (found + MAREA_INTEGER_SIZE(1));
        }
    }

    // INFO: with no argument and no literal, a function of the shape could be any function of the type.
    const MAREA_Boolean NEEDED = ((literal != MAREA_NULLPTR) || (COUNT > MAREA_INTEGER_SIZE(0)));

    if ((found > MAREA_INTEGER_SIZE(1)) || (NEEDED != (found == MAREA_INTEGER_SIZE(1)))) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "The construct operator of the type cannot be told apart from its other functions.");
    }

    MAREA_CgenValue made = { 0 };

    if (literal != MAREA_NULLPTR) {
        MAREA_CGEN_TRY(MAREA_CgenWriteLiteralValue(generator, literal, type, &made));
    } else {
        MAREA_CgenZeroTemporary(generator, type, &made);
    }

    if (construct != MAREA_NULLPTR) {
        MAREA_CgenValue ignored = { 0 };

        MAREA_CGEN_TRY(MAREA_CgenWriteUnitCall(generator, node, construct, MAREA_NULLPTR, arguments, &ignored));
    }

    *outValue = made;

    return MAREA_CGEN_EXCEPT_SUCCESS;
}
