// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/cgen/MAREACgenInternal.h"

#include "marea/parse/MAREAAst.h"

#include <string.h>

static MAREA_Boolean MAREA_CALL MAREA_G_IsShiftAssignment(MAREA_TokenKind token)
{
    return ((token == MAREA_TOKEN_KIND_SHIFT_LEFT_EQUAL) || (token == MAREA_TOKEN_KIND_SHIFT_RIGHT_EQUAL));
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteLocal(MAREA_CgenGenerator *generator, const MAREA_IrNode *node)
{
    const MAREA_IrNode *initial = MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(0));

    if (initial == MAREA_NULLPTR) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A local has no initial value.");
    }

    MAREA_Uinteger32 type = MAREA_CGEN_KIND_NONE;

    MAREA_CGEN_TRY(MAREA_CgenTypeOfNode(generator, node, &type));

    if (MAREA_CgenIsValueType(generator, type) == MAREA_BOOLEAN_FALSE) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A local has a type that cgen does not write.");
    }

    MAREA_CgenValue value = { 0 };

    MAREA_CGEN_TRY(MAREA_CgenWriteOwned(generator, initial, type, &value));

    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *name = MAREA_CgenLocalName(generator, MAREA_IrNodeGetText(node), MAREA_IrNodeGetTextLength(node));

    // INFO: the value is computed before the name is declared, so it reads the local that the new one hides.
    if (strstr(value.text, name) != MAREA_NULLPTR) {
        MAREA_CgenSnapshot(generator, &value);
    }

    MAREA_CGEN_WRITE(generator->buffer, MAREA_CgenTypeName(generator, type), " ", name, " = ", value.text, ";\n(void)", name, ";\n");

    if (MAREA_CgenNeedsDrop(generator, type) == MAREA_BOOLEAN_TRUE) {
        MAREA_CGEN_TRY(MAREA_CgenPushOwned(generator, name, type));
    }

    return MAREA_CgenPushSymbol(generator, MAREA_IrNodeGetText(node), MAREA_IrNodeGetTextLength(node), name, type, (MAREA_CgenIsAggregate(generator, type) == MAREA_BOOLEAN_FALSE), MAREA_BOOLEAN_FALSE);
}

// INFO: the data of an enum is written under its tag; a variant without the field is the unreachable trap.
static void MAREA_CALL MAREA_G_WriteVariantStore(MAREA_CgenGenerator *generator, const MAREA_CgenValue *base, const MAREA_IrNode *member, MAREA_Uinteger32 type, const MAREA_Character8 *value)
{
    const MAREA_CgenType *entry = MAREA_CgenTypeAt(generator, base->type);

    const MAREA_Character8 *field = MAREA_CgenMangled(generator, "f_", MAREA_IrNodeGetText(member), MAREA_IrNodeGetTextLength(member));

    MAREA_CGEN_WRITE(generator->buffer, "switch ((", base->text, ").marea_tag) {\n");

    for (MAREA_IntegerSize variant = MAREA_INTEGER_SIZE(0); variant < entry->memberCount; variant = (variant + MAREA_INTEGER_SIZE(1))) {
        for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < entry->variants[variant].fieldCount; index = (index + MAREA_INTEGER_SIZE(1))) {
            const MAREA_CgenField *row = &entry->variants[variant].fields[index];

            if ((row->type == type) && (MAREA_CgenSameText(row->name, row->nameLength, MAREA_IrNodeGetText(member), MAREA_IrNodeGetTextLength(member)) == MAREA_BOOLEAN_TRUE)) {
                MAREA_CGEN_WRITE(generator->buffer, "case ", MAREA_CgenNumber(generator, variant), ":\n(", base->text, ").marea_data.v_", MAREA_CgenNumber(generator, variant), ".", field, " = ", value, ";\nbreak;\n");
            }
        }
    }

    MAREA_CgenWriteText(generator->buffer, "default:\n");
    MAREA_CgenWriteTrap(generator, "MAREA_RT_TRAP_UNREACHABLE");
    MAREA_CgenWriteText(generator->buffer, "}\n");
}

static MAREA_Uinteger32 MAREA_CALL MAREA_G_VariantFieldType(const MAREA_CgenGenerator *generator, MAREA_Uinteger32 type, const MAREA_IrNode *member)
{
    const MAREA_CgenType *entry = MAREA_CgenTypeAt(generator, type);

    for (MAREA_IntegerSize variant = MAREA_INTEGER_SIZE(0); variant < entry->memberCount; variant = (variant + MAREA_INTEGER_SIZE(1))) {
        for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < entry->variants[variant].fieldCount; index = (index + MAREA_INTEGER_SIZE(1))) {
            const MAREA_CgenField *row = &entry->variants[variant].fields[index];

            if (MAREA_CgenSameText(row->name, row->nameLength, MAREA_IrNodeGetText(member), MAREA_IrNodeGetTextLength(member)) == MAREA_BOOLEAN_TRUE) {
                return row->type;
            }
        }
    }

    return MAREA_CGEN_KIND_NONE;
}

// INFO: the right side of a compound assignment runs before the target is read, and a fault of the operator stops the run before the store. ioValue holds the right side and receives the result.
static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteCombined(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, const MAREA_CgenValue *current, MAREA_CgenValue *ioValue)
{
    const MAREA_TokenKind PLAIN = MAREA_CgenPlainOperator(MAREA_IrNodeGetOperator(node));

    if (PLAIN == MAREA_TOKEN_KIND_ERROR) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "An assignment operator is not written.");
    }

    const MAREA_CgenValue RIGHT = *ioValue;

    return MAREA_CgenWriteOperator(generator, node, PLAIN, current, &RIGHT, ioValue);
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteEnumAssignment(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, const MAREA_CgenValue *base)
{
    const MAREA_IrNode *target = MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(0));
    const MAREA_IrNode *member = MAREA_CgenChildAt(target, MAREA_INTEGER_SIZE(1));

    const MAREA_Uinteger32 TYPE = MAREA_G_VariantFieldType(generator, base->type, member);

    if (TYPE == MAREA_CGEN_KIND_NONE) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNRESOLVED_NAME, "No variant of the enum has a field of this name.");
    }

    const MAREA_TokenKind OPERATOR = MAREA_IrNodeGetOperator(node);

    MAREA_CgenValue value = { 0 };

    if (OPERATOR == MAREA_TOKEN_KIND_EQUAL) {
        MAREA_CGEN_TRY(MAREA_CgenWriteTyped(generator, MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(1)), TYPE, &value));
    } else {
        MAREA_CGEN_TRY(MAREA_CgenWriteValue(generator, MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(1)), ((MAREA_G_IsShiftAssignment(OPERATOR) == MAREA_BOOLEAN_TRUE) ? (MAREA_Uinteger32)MAREA_CGEN_KIND_NONE : TYPE), &value));
    }

    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    if (value.stable == MAREA_BOOLEAN_FALSE) {
        MAREA_CgenSnapshot(generator, &value);
    }

    if (OPERATOR != MAREA_TOKEN_KIND_EQUAL) {
        MAREA_CgenValue current = { 0 };

        MAREA_CGEN_TRY(MAREA_CgenWriteMember(generator, target, base, &current));
        MAREA_CGEN_TRY(MAREA_G_WriteCombined(generator, node, &current, &value));

        if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
            return MAREA_CGEN_EXCEPT_SUCCESS;
        }
    }

    MAREA_G_WriteVariantStore(generator, base, member, TYPE, value.text);

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteStore(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, const MAREA_CgenValue *place)
{
    const MAREA_IrNode *source = MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(1));

    const MAREA_TokenKind OPERATOR = MAREA_IrNodeGetOperator(node);

    MAREA_CgenValue value = { 0 };

    if (OPERATOR == MAREA_TOKEN_KIND_EQUAL) {
        MAREA_CGEN_TRY(MAREA_CgenWriteOwned(generator, source, place->type, &value));

        // INFO: the new value is taken before the old one is destroyed, since it may read the target.
        if ((generator->functionFailed == MAREA_BOOLEAN_FALSE) && (MAREA_CgenNeedsDrop(generator, place->type) == MAREA_BOOLEAN_TRUE)) {
            MAREA_CgenSnapshot(generator, &value);
            MAREA_CGEN_TRY(MAREA_CgenWriteDrop(generator, node, place->text, place->type, MAREA_BOOLEAN_FALSE));
        }

        if (generator->functionFailed == MAREA_BOOLEAN_FALSE) {
            MAREA_CgenStore(generator, place->text, place->type, value.text);
        }

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    MAREA_CGEN_TRY(MAREA_CgenWriteValue(generator, source, ((MAREA_G_IsShiftAssignment(OPERATOR) == MAREA_BOOLEAN_TRUE) ? (MAREA_Uinteger32)MAREA_CGEN_KIND_NONE : place->type), &value));

    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    MAREA_CGEN_TRY(MAREA_G_WriteCombined(generator, node, place, &value));

    if (generator->functionFailed == MAREA_BOOLEAN_FALSE) {
        MAREA_CgenStore(generator, place->text, place->type, value.text);
    }

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

// INFO: tells whether the place of a target is inside the storage of a vector, which a call in the value may move.
static MAREA_CgenExcept MAREA_CALL MAREA_G_ThroughVector(MAREA_CgenGenerator *generator, const MAREA_IrNode *target, MAREA_Boolean *outThrough)
{
    *outThrough = MAREA_BOOLEAN_FALSE;

    for (const MAREA_IrNode *node = target; (MAREA_IrNodeGetKind(node) == MAREA_IR_KIND_INDEX_ACCESS) || (MAREA_IrNodeGetKind(node) == MAREA_IR_KIND_MEMBER_ACCESS); node = MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(0))) {
        if (MAREA_IrNodeGetKind(node) != MAREA_IR_KIND_INDEX_ACCESS) {
            continue;
        }

        MAREA_Uinteger32 type = MAREA_CGEN_KIND_NONE;

        MAREA_CGEN_TRY(MAREA_CgenTypeOfNode(generator, MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(0)), &type));

        if (MAREA_CgenCategoryOf(generator, type) == MAREA_CGEN_CATEGORY_VECTOR) {
            *outThrough = MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

// INFO: a target in a vector: the value runs first, then the target is reached, so it is never an old address of the storage.
static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteVectorAssignment(MAREA_CgenGenerator *generator, const MAREA_IrNode *node)
{
    const MAREA_IrNode *target = MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(0));
    const MAREA_IrNode *source = MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(1));

    const MAREA_TokenKind OPERATOR = MAREA_IrNodeGetOperator(node);

    MAREA_Uinteger32 type = MAREA_CGEN_KIND_NONE;

    MAREA_CGEN_TRY(MAREA_CgenTypeOfNode(generator, target, &type));

    if (MAREA_CgenIsValueType(generator, type) == MAREA_BOOLEAN_FALSE) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "An assigned element has a type that cgen does not write.");
    }

    MAREA_CgenValue value = { 0 };

    if (OPERATOR == MAREA_TOKEN_KIND_EQUAL) {
        MAREA_CGEN_TRY(MAREA_CgenWriteOwned(generator, source, type, &value));
    } else {
        MAREA_CGEN_TRY(MAREA_CgenWriteValue(generator, source, ((MAREA_G_IsShiftAssignment(OPERATOR) == MAREA_BOOLEAN_TRUE) ? (MAREA_Uinteger32)MAREA_CGEN_KIND_NONE : type), &value));
    }

    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    MAREA_CgenSnapshot(generator, &value);

    MAREA_CgenValue place = { 0 };

    MAREA_CGEN_TRY(MAREA_CgenWritePlace(generator, target, MAREA_CGEN_KIND_NONE, &place));

    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    if (OPERATOR != MAREA_TOKEN_KIND_EQUAL) {
        MAREA_CGEN_TRY(MAREA_G_WriteCombined(generator, node, &place, &value));
    } else if (MAREA_CgenNeedsDrop(generator, place.type) == MAREA_BOOLEAN_TRUE) {
        MAREA_CGEN_TRY(MAREA_CgenWriteDrop(generator, node, place.text, place.type, MAREA_BOOLEAN_FALSE));
    }

    if (generator->functionFailed == MAREA_BOOLEAN_FALSE) {
        MAREA_CgenStore(generator, place.text, place.type, value.text);
    }

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

// INFO: the target is reached first, with the check of each index, then the value runs.
static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteAssignment(MAREA_CgenGenerator *generator, const MAREA_IrNode *node)
{
    const MAREA_IrNode *target = MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(0));

    if ((target == MAREA_NULLPTR) || (MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(1)) == MAREA_NULLPTR)) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "An assignment has no target or no value.");
    }

    MAREA_Boolean through = MAREA_BOOLEAN_FALSE;

    MAREA_CGEN_TRY(MAREA_G_ThroughVector(generator, target, &through));

    if (through == MAREA_BOOLEAN_TRUE) {
        return MAREA_G_WriteVectorAssignment(generator, node);
    }

    MAREA_CgenValue place = { 0 };

    if (MAREA_IrNodeGetKind(target) == MAREA_IR_KIND_IDENTIFIER) {
        const MAREA_CgenSymbol *symbol = MAREA_CgenFindSymbol(generator, MAREA_IrNodeGetText(target), MAREA_IrNodeGetTextLength(target));

        if ((symbol == MAREA_NULLPTR) || (symbol->type == MAREA_CGEN_KIND_NONE)) {
            return MAREA_CgenGeneratorFail(generator, target, MAREA_CGEN_DIAGNOSTIC_CODE_UNRESOLVED_NAME, "An assigned name is not a local or a parameter.");
        }

        place.text = symbol->cText;
        place.type = symbol->type;
        place.unaligned = symbol->unaligned;
    } else if (MAREA_IrNodeGetKind(target) == MAREA_IR_KIND_MEMBER_ACCESS) {
        MAREA_CgenValue base = { 0 };

        MAREA_CGEN_TRY(MAREA_CgenWritePlace(generator, MAREA_CgenChildAt(target, MAREA_INTEGER_SIZE(0)), MAREA_CGEN_KIND_NONE, &base));

        if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
            return MAREA_CGEN_EXCEPT_SUCCESS;
        }

        if ((MAREA_CgenCategoryOf(generator, base.type) == MAREA_CGEN_CATEGORY_ENUM) && (MAREA_CgenIsAggregate(generator, base.type) == MAREA_BOOLEAN_TRUE)) {
            return MAREA_G_WriteEnumAssignment(generator, node, &base);
        }

        MAREA_CGEN_TRY(MAREA_CgenWriteMember(generator, target, &base, &place));
    } else if (MAREA_IrNodeGetKind(target) == MAREA_IR_KIND_INDEX_ACCESS) {
        MAREA_CGEN_TRY(MAREA_CgenWritePlace(generator, target, MAREA_CGEN_KIND_NONE, &place));
    } else {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "Only a local, a parameter, a field or an element is assigned.");
    }

    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    return MAREA_G_WriteStore(generator, node, &place);
}

// INFO: the parts of a fork_join run one after the other, so a spawn that no one keeps is its call.
static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteExpressionStatement(MAREA_CgenGenerator *generator, const MAREA_IrNode *node)
{
    const MAREA_IrNode *expression = MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(0));

    if ((MAREA_IrNodeGetKind(expression) == MAREA_IR_KIND_UNARY_EXPRESSION) && (MAREA_IrNodeGetOperator(expression) == MAREA_TOKEN_KIND_KEYWORD_SPAWN)) {
        expression = MAREA_CgenChildAt(expression, MAREA_INTEGER_SIZE(0));
    }

    MAREA_CgenValue value = { 0 };

    if (MAREA_IrNodeGetKind(expression) == MAREA_IR_KIND_CALL_EXPRESSION) {
        MAREA_CGEN_TRY(MAREA_CgenWriteCall(generator, expression, MAREA_BOOLEAN_FALSE, &value));
    } else {
        MAREA_CGEN_TRY(MAREA_CgenWriteValue(generator, expression, MAREA_CGEN_KIND_NONE, &value));
    }

    // INFO: a result that nothing takes over is destroyed at once.
    if ((generator->functionFailed == MAREA_BOOLEAN_FALSE) && (MAREA_CgenNeedsDrop(generator, value.type) == MAREA_BOOLEAN_TRUE) && (MAREA_IrNodeGetKind(expression) == MAREA_IR_KIND_CALL_EXPRESSION)) {
        MAREA_CGEN_TRY(MAREA_CgenWriteDrop(generator, node, value.text, value.type, MAREA_BOOLEAN_FALSE));
    }

    if ((generator->functionFailed == MAREA_BOOLEAN_FALSE) && (value.type != MAREA_CGEN_KIND_VOID)) {
        MAREA_CGEN_WRITE(generator->buffer, "(void)(", value.text, ");\n");
    }

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteReturn(MAREA_CgenGenerator *generator, const MAREA_IrNode *node)
{
    if (MAREA_IrNodeGetChildCount(node) == MAREA_INTEGER_SIZE(0)) {
        MAREA_CGEN_TRY(MAREA_CgenWriteDrops(generator, node, MAREA_INTEGER_SIZE(0)));
        MAREA_CgenWriteText(generator->buffer, "return;\n");

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    MAREA_CgenValue value = { 0 };

    MAREA_CGEN_TRY(MAREA_CgenWriteOwned(generator, MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(0)), generator->current->result, &value));

    // INFO: the result is held before the values of the function are destroyed, since it may read them.
    if ((generator->functionFailed == MAREA_BOOLEAN_FALSE) && (generator->ownedCount > MAREA_INTEGER_SIZE(0))) {
        MAREA_CgenSnapshot(generator, &value);
        MAREA_CGEN_TRY(MAREA_CgenWriteDrops(generator, node, MAREA_INTEGER_SIZE(0)));
    }

    if (generator->functionFailed == MAREA_BOOLEAN_FALSE) {
        MAREA_CGEN_WRITE(generator->buffer, "return ", value.text, ";\n");
    }

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

// INFO: the test of each elseif runs inside the else of the one before, so it only runs when that one is false.
static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteArm(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, const MAREA_IrNode *arm, MAREA_IntegerSize position)
{
    MAREA_CgenValue condition = { 0 };

    MAREA_CGEN_TRY(MAREA_CgenWriteTyped(generator, MAREA_CgenChildAt(arm, MAREA_INTEGER_SIZE(0)), MAREA_CGEN_KIND_BOOLEAN, &condition));

    MAREA_CGEN_WRITE(generator->buffer, "if (", condition.text, ") ");

    MAREA_CGEN_TRY(MAREA_CgenWriteBlock(generator, MAREA_CgenChildAt(arm, MAREA_INTEGER_SIZE(1))));

    if (position >= MAREA_IrNodeGetChildCount(node)) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    const MAREA_IrNode *clause = MAREA_CgenChildAt(node, position);

    if (MAREA_IrNodeGetKind(clause) == MAREA_IR_KIND_ELSE_CLAUSE) {
        MAREA_CgenWriteText(generator->buffer, "else ");

        return MAREA_CgenWriteBlock(generator, MAREA_CgenChildAt(clause, MAREA_INTEGER_SIZE(0)));
    }

    MAREA_CgenWriteText(generator->buffer, "else {\n");

    MAREA_CGEN_TRY(MAREA_G_WriteArm(generator, node, clause, (position + MAREA_INTEGER_SIZE(1))));

    MAREA_CgenWriteText(generator->buffer, "}\n");

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

// INFO: the test runs at the start of each turn, with its own faults, so a continue runs it again.
static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteWhile(MAREA_CgenGenerator *generator, const MAREA_IrNode *node)
{
    const MAREA_IrNode *condition = MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(0));

    MAREA_CgenWriteText(generator->buffer, "for (;;) {\n");

    if ((MAREA_IrNodeGetKind(condition) != MAREA_IR_KIND_BOOLEAN_LITERAL) || (MAREA_CgenTextIs(condition, "true") == MAREA_BOOLEAN_FALSE)) {
        MAREA_CgenValue test = { 0 };

        MAREA_CGEN_TRY(MAREA_CgenWriteTyped(generator, condition, MAREA_CGEN_KIND_BOOLEAN, &test));

        MAREA_CGEN_WRITE(generator->buffer, "if (!(", test.text, ")) {\nbreak;\n}\n");
    }

    const MAREA_IntegerSize LOOP_MARK = generator->loopMark;

    generator->loopMark = generator->ownedCount;

    MAREA_CGEN_TRY(MAREA_CgenWriteBlock(generator, MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(1))));

    generator->loopMark = LOOP_MARK;

    MAREA_CgenWriteText(generator->buffer, "}\n");

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

// INFO: the body runs once for each part, from part 0 up, and the part is no value yet, as in the vm.
static MAREA_CgenExcept MAREA_CALL MAREA_G_WritePartition(MAREA_CgenGenerator *generator, const MAREA_IrNode *node)
{
    MAREA_CgenWriteText(generator->buffer, "{\n");

    MAREA_CgenValue parts = { 0 };

    MAREA_CGEN_TRY(MAREA_CgenWriteValue(generator, MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(1)), MAREA_CGEN_KIND_SINTEGER_32, &parts));

    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    if (MAREA_CgenIsInteger(parts.type) == MAREA_BOOLEAN_FALSE) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A part count is an integer.");
    }

    MAREA_CgenSnapshot(generator, &parts);

    const MAREA_Character8 *suffix = MAREA_CgenKindSuffix(parts.type);
    const MAREA_Character8 *cType = MAREA_CgenTypeName(generator, parts.type);

    MAREA_CGEN_WRITE(generator->buffer, "if (marea_rt_", ((MAREA_CgenIsSigned(parts.type) == MAREA_BOOLEAN_TRUE) ? "le_" : "eq_"), suffix, "(", parts.text, ", 0)) {\n");
    MAREA_CgenWriteTrap(generator, "MAREA_RT_TRAP_INVALID_PARTITION_COUNT");
    MAREA_CgenWriteText(generator->buffer, "}\n");

    const MAREA_Character8 *index = MAREA_CgenTemporaryName(generator);

    MAREA_CGEN_WRITE(generator->buffer, "for (", cType, " ", index, " = 0; marea_rt_lt_", suffix, "(", index, ", ", parts.text, "); ", index, " = (", cType, ")(", index, " + 1)) {\n");

    const MAREA_IntegerSize MARK = generator->symbolCount;

    MAREA_CGEN_TRY(MAREA_CgenPushSymbol(generator, MAREA_IrNodeGetText(node), MAREA_IrNodeGetTextLength(node), "0", MAREA_CGEN_KIND_NONE, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_FALSE));
    MAREA_CGEN_TRY(MAREA_CgenWriteBlock(generator, MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(2))));

    generator->symbolCount = MARK;

    MAREA_CgenWriteText(generator->buffer, "}\n}\n");

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteForEachBody(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, const MAREA_CgenValue *collection, const MAREA_Character8 *index)
{
    const MAREA_IntegerSize CHILDREN = MAREA_IrNodeGetChildCount(node);

    const MAREA_Uinteger32 ELEMENT = MAREA_CgenTypeAt(generator, collection->type)->element;

    const MAREA_Character8 *element = (MAREA_CgenCategoryOf(generator, collection->type) == MAREA_CGEN_CATEGORY_VECTOR) ? MAREA_CgenElementText(generator, collection->text, ELEMENT, index) : MAREA_CGEN_JOIN(generator, "(", collection->text, ").items[", index, "]");

    if (CHILDREN == MAREA_INTEGER_SIZE(3)) {
        const MAREA_IrNode *key = MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(0));

        MAREA_Uinteger32 type = MAREA_CGEN_KIND_NONE;

        MAREA_CGEN_TRY(MAREA_CgenTypeOfNode(generator, key, &type));

        if (MAREA_CgenIsInteger(type) == MAREA_BOOLEAN_FALSE) {
            return MAREA_CgenGeneratorFail(generator, key, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "The key of an element is an integer.");
        }

        const MAREA_Character8 *name = MAREA_CgenLocalName(generator, MAREA_IrNodeGetText(key), MAREA_IrNodeGetTextLength(key));

        MAREA_CGEN_WRITE(generator->buffer, MAREA_CgenTypeName(generator, type), " ", name, " = (", MAREA_CgenTypeName(generator, type), ")", index, ";\n(void)", name, ";\n");
        MAREA_CGEN_TRY(MAREA_CgenPushSymbol(generator, MAREA_IrNodeGetText(key), MAREA_IrNodeGetTextLength(key), name, type, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_FALSE));
    }

    // INFO: a mutable loop names each element itself, so a write reaches the collection; a plain one copies it, but an element that holds storage is named too, since a copy of its bytes would be a second owner.
    if (((MAREA_IrNodeGetFlags(node) & (MAREA_Uinteger32)MAREA_AST_FLAG_MUTABLE) != MAREA_UINTEGER_32(0)) || (MAREA_CgenNeedsDrop(generator, ELEMENT) == MAREA_BOOLEAN_TRUE)) {
        MAREA_CGEN_TRY(MAREA_CgenPushSymbol(generator, MAREA_IrNodeGetText(node), MAREA_IrNodeGetTextLength(node), element, ELEMENT, MAREA_BOOLEAN_FALSE, collection->unaligned));
    } else {
        const MAREA_Character8 *name = MAREA_CgenLocalName(generator, MAREA_IrNodeGetText(node), MAREA_IrNodeGetTextLength(node));

        MAREA_CGEN_WRITE(generator->buffer, MAREA_CgenTypeName(generator, ELEMENT), " ", name, " = ", element, ";\n(void)", name, ";\n");
        MAREA_CGEN_TRY(MAREA_CgenPushSymbol(generator, MAREA_IrNodeGetText(node), MAREA_IrNodeGetTextLength(node), name, ELEMENT, (MAREA_CgenIsAggregate(generator, ELEMENT) == MAREA_BOOLEAN_FALSE), MAREA_BOOLEAN_FALSE));
    }

    return MAREA_CgenWriteBlock(generator, MAREA_CgenChildAt(node, (CHILDREN - MAREA_INTEGER_SIZE(1))));
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteForEach(MAREA_CgenGenerator *generator, const MAREA_IrNode *node)
{
    const MAREA_IntegerSize CHILDREN = MAREA_IrNodeGetChildCount(node);

    if ((CHILDREN < MAREA_INTEGER_SIZE(2)) || (CHILDREN > MAREA_INTEGER_SIZE(3))) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A for each has no collection or no body.");
    }

    MAREA_CgenWriteText(generator->buffer, "{\n");

    MAREA_CgenValue collection = { 0 };

    MAREA_CGEN_TRY(MAREA_CgenWritePlace(generator, MAREA_CgenChildAt(node, (CHILDREN - MAREA_INTEGER_SIZE(2))), MAREA_CGEN_KIND_NONE, &collection));

    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    const MAREA_Uinteger8 CATEGORY = MAREA_CgenCategoryOf(generator, collection.type);

    if ((CATEGORY != MAREA_CGEN_CATEGORY_ARRAY) && (CATEGORY != MAREA_CGEN_CATEGORY_VECTOR)) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "Only an array or a vector is walked by a for each.");
    }

    const MAREA_Uinteger64 COUNT = MAREA_CgenTypeAt(generator, collection.type)->count;

    // INFO: the body of a loop over no element never runs, so it is not written.
    if ((CATEGORY == MAREA_CGEN_CATEGORY_ARRAY) && (COUNT == MAREA_UINTEGER_64(0))) {
        MAREA_CGEN_WRITE(generator->buffer, "(void)(", collection.text, ");\n}\n");

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    // INFO: the count of a vector is read once, when the loop starts.
    const MAREA_Character8 *last = MAREA_CGEN_JOIN(generator, MAREA_CgenNumber(generator, COUNT), "ULL");

    if (CATEGORY == MAREA_CGEN_CATEGORY_VECTOR) {
        last = MAREA_CgenTemporaryName(generator);

        MAREA_CGEN_WRITE(generator->buffer, "uint64_t ", last, " = (", collection.text, ").size;\n");
    }

    const MAREA_Character8 *index = MAREA_CgenTemporaryName(generator);

    MAREA_CGEN_WRITE(generator->buffer, "for (uint64_t ", index, " = 0; ", index, " < ", last, "; ", index, " += 1u) {\n");

    const MAREA_IntegerSize MARK = generator->symbolCount;
    const MAREA_IntegerSize LOOP_MARK = generator->loopMark;

    generator->loopMark = generator->ownedCount;

    MAREA_CGEN_TRY(MAREA_G_WriteForEachBody(generator, node, &collection, index));

    generator->symbolCount = MARK;
    generator->loopMark = LOOP_MARK;

    MAREA_CgenWriteText(generator->buffer, "}\n}\n");

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

// INFO: the destroy of a local at the end of its scope, which the ir inserts, takes no parameter.
static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteDestroy(MAREA_CgenGenerator *generator, const MAREA_IrNode *node)
{
    MAREA_CgenFunction *function = MAREA_CgenFindFunction(generator, MAREA_IrNodeGetText(node), MAREA_IrNodeGetTextLength(node));

    if (function == MAREA_NULLPTR) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNRESOLVED_NAME, "A destroy names no function of this unit.");
    }

    // INFO: an array is destroyed element by element, so the operator runs once for each element of its arrays.
    const MAREA_IrNode *local = MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(0));

    const MAREA_CgenSymbol *symbol = (local == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_CgenFindSymbol(generator, MAREA_IrNodeGetText(local), MAREA_IrNodeGetTextLength(local));

    MAREA_Uinteger64 times = MAREA_UINTEGER_64(1);

    for (MAREA_Uinteger32 type = (symbol == MAREA_NULLPTR) ? (MAREA_Uinteger32)MAREA_CGEN_KIND_NONE : symbol->type; MAREA_CgenCategoryOf(generator, type) == MAREA_CGEN_CATEGORY_ARRAY; type = MAREA_CgenTypeAt(generator, type)->element) {
        times = (times * MAREA_CgenTypeAt(generator, type)->count);
    }

    MAREA_CgenValue ignored = { 0 };

    if (times == MAREA_UINTEGER_64(1)) {
        return MAREA_CgenWriteUnitCall(generator, node, function, MAREA_NULLPTR, MAREA_NULLPTR, &ignored);
    }

    if (times == MAREA_UINTEGER_64(0)) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *index = MAREA_CgenTemporaryName(generator);

    MAREA_CGEN_WRITE(generator->buffer, "for (uint64_t ", index, " = 0; ", index, " < ", MAREA_CgenNumber(generator, times), "ULL; ", index, " += 1u) {\n");
    MAREA_CGEN_TRY(MAREA_CgenWriteUnitCall(generator, node, function, MAREA_NULLPTR, MAREA_NULLPTR, &ignored));
    MAREA_CgenWriteText(generator->buffer, "}\n");

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteStatement(MAREA_CgenGenerator *generator, const MAREA_IrNode *node)
{
    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    switch (MAREA_IrNodeGetKind(node)) {
    case MAREA_IR_KIND_LOCAL_DECLARATION:
        return MAREA_G_WriteLocal(generator, node);
    case MAREA_IR_KIND_BLOCK:
        return MAREA_CgenWriteBlock(generator, node);
    case MAREA_IR_KIND_SCOPE_BLOCK:
    case MAREA_IR_KIND_FORK_JOIN_BLOCK:
        // INFO: the join of a fork_join is its end, because every spawn runs to its end where it stands.
        return MAREA_CgenWriteBlock(generator, MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(0)));
    case MAREA_IR_KIND_ASSIGNMENT:
        return MAREA_G_WriteAssignment(generator, node);
    case MAREA_IR_KIND_EXPRESSION_STATEMENT:
        return MAREA_G_WriteExpressionStatement(generator, node);
    case MAREA_IR_KIND_RETURN_STATEMENT:
        return MAREA_G_WriteReturn(generator, node);
    case MAREA_IR_KIND_IF_STATEMENT:
        return MAREA_G_WriteArm(generator, node, node, MAREA_INTEGER_SIZE(2));
    case MAREA_IR_KIND_WHILE_STATEMENT:
        return MAREA_G_WriteWhile(generator, node);
    case MAREA_IR_KIND_FOR_EACH_STATEMENT:
        return MAREA_G_WriteForEach(generator, node);
    case MAREA_IR_KIND_PARTITION_STATEMENT:
        return MAREA_G_WritePartition(generator, node);
    case MAREA_IR_KIND_DESTROY_CALL:
        return MAREA_G_WriteDestroy(generator, node);
    case MAREA_IR_KIND_BREAK_STATEMENT:
        MAREA_CGEN_TRY(MAREA_CgenWriteDrops(generator, node, generator->loopMark));
        MAREA_CgenWriteText(generator->buffer, "break;\n");

        return MAREA_CGEN_EXCEPT_SUCCESS;
    case MAREA_IR_KIND_CONTINUE_STATEMENT:
        MAREA_CGEN_TRY(MAREA_CgenWriteDrops(generator, node, generator->loopMark));
        MAREA_CgenWriteText(generator->buffer, "continue;\n");

        return MAREA_CGEN_EXCEPT_SUCCESS;
    case MAREA_IR_KIND_UNREACHABLE_STATEMENT:
        MAREA_CgenWriteTrap(generator, "MAREA_RT_TRAP_UNREACHABLE");

        return MAREA_CGEN_EXCEPT_SUCCESS;
    default:
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A statement is not written yet.");
    }
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteBlock(MAREA_CgenGenerator *generator, const MAREA_IrNode *node)
{
    const MAREA_IntegerSize MARK = generator->symbolCount;
    const MAREA_IntegerSize OWNED_MARK = generator->ownedCount;
    const MAREA_IntegerSize COUNT = MAREA_IrNodeGetChildCount(node);

    MAREA_CgenWriteText(generator->buffer, "{\n");

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); (generator->functionFailed == MAREA_BOOLEAN_FALSE) && (index < COUNT); index = (index + MAREA_INTEGER_SIZE(1))) {
        MAREA_CGEN_TRY(MAREA_CgenWriteStatement(generator, MAREA_CgenChildAt(node, index)));
    }

    MAREA_CGEN_TRY(MAREA_CgenWriteDrops(generator, node, OWNED_MARK));

    generator->symbolCount = MARK;
    generator->ownedCount = OWNED_MARK;

    MAREA_CgenWriteText(generator->buffer, "}\n");

    return MAREA_CGEN_EXCEPT_SUCCESS;
}
