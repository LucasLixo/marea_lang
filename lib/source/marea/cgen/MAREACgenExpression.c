// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/cgen/MAREACgenInternal.h"

#include <stdlib.h>
#include <string.h>

#if !defined(MAREA_CGEN_LITERAL_LIMIT)
    #define MAREA_CGEN_LITERAL_LIMIT MAREA_INTEGER_SIZE(128)
#endif

#if !defined(MAREA_CGEN_CONSTANT_DEPTH)
    #define MAREA_CGEN_CONSTANT_DEPTH MAREA_UINTEGER_32(32)
#endif

static MAREA_Boolean MAREA_CALL MAREA_G_ParseFloat(const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_Float64 *outValue)
{
    MAREA_Character8 buffer[MAREA_CGEN_LITERAL_LIMIT];
    MAREA_IntegerSize used = MAREA_INTEGER_SIZE(0);

    for (MAREA_IntegerSize position = MAREA_INTEGER_SIZE(0); position < length; position = (position + MAREA_INTEGER_SIZE(1))) {
        if (text[position] == '_') {
            continue;
        }

        if (used >= (MAREA_CGEN_LITERAL_LIMIT - MAREA_INTEGER_SIZE(1))) {
            return MAREA_BOOLEAN_FALSE;
        }

        buffer[used] = text[position];
        used = (used + MAREA_INTEGER_SIZE(1));
    }

    buffer[used] = '\0';

    if (used == MAREA_INTEGER_SIZE(0)) {
        return MAREA_BOOLEAN_FALSE;
    }

    MAREA_Character8 *end = MAREA_NULLPTR;

    *outValue = strtod(buffer, &end);

    return (*end == '\0');
}

// INFO: a hexadecimal floating constant holds every bit of the value, so the C compiler reads back the same number.
static const MAREA_Character8 *MAREA_CALL MAREA_G_FloatText(MAREA_CgenGenerator *generator, MAREA_Float64 value, MAREA_Boolean single)
{
    if (single == MAREA_BOOLEAN_TRUE) {
        value = (MAREA_Float64)((MAREA_Float32)value);
    }

    MAREA_Uinteger64 bits = MAREA_UINTEGER_64(0);

    memcpy(&bits, &value, sizeof(bits));

    const MAREA_Uinteger64 EXPONENT = ((bits >> MAREA_UINTEGER_32(52)) & MAREA_UINTEGER_64(0x7FF));
    const MAREA_Uinteger64 MANTISSA = (bits & MAREA_UINTEGER_64(0xFFFFFFFFFFFFF));

    const MAREA_Character8 *text = MAREA_NULLPTR;

    if (EXPONENT == MAREA_UINTEGER_64(0x7FF)) {
        text = (MANTISSA == MAREA_UINTEGER_64(0)) ? ((single == MAREA_BOOLEAN_TRUE) ? "__builtin_inff()" : "__builtin_inf()") : ((single == MAREA_BOOLEAN_TRUE) ? "__builtin_nanf(\"\")" : "__builtin_nan(\"\")");
    } else if ((EXPONENT == MAREA_UINTEGER_64(0)) && (MANTISSA == MAREA_UINTEGER_64(0))) {
        text = (single == MAREA_BOOLEAN_TRUE) ? "0x0p+0f" : "0x0p+0";
    } else {
        MAREA_Character8 digits[14];

        for (MAREA_Uinteger32 index = MAREA_UINTEGER_32(0); index < MAREA_UINTEGER_32(13); index = (index + MAREA_UINTEGER_32(1))) {
            digits[index] = "0123456789abcdef"[((MANTISSA >> (MAREA_UINTEGER_32(48) - (index * MAREA_UINTEGER_32(4)))) & MAREA_UINTEGER_64(0xF))];
        }

        digits[13] = '\0';

        const MAREA_Boolean SUBNORMAL = (EXPONENT == MAREA_UINTEGER_64(0));
        const MAREA_Boolean LOW = ((SUBNORMAL == MAREA_BOOLEAN_TRUE) || (EXPONENT < MAREA_UINTEGER_64(1023)));
        const MAREA_Uinteger64 MAGNITUDE = (SUBNORMAL == MAREA_BOOLEAN_TRUE) ? MAREA_UINTEGER_64(1022) : ((LOW == MAREA_BOOLEAN_TRUE) ? (MAREA_UINTEGER_64(1023) - EXPONENT) : (EXPONENT - MAREA_UINTEGER_64(1023)));

        text = MAREA_CGEN_JOIN(generator, ((SUBNORMAL == MAREA_BOOLEAN_TRUE) ? "0x0." : "0x1."), digits, ((LOW == MAREA_BOOLEAN_TRUE) ? "p-" : "p+"), MAREA_CgenNumber(generator, MAGNITUDE), ((single == MAREA_BOOLEAN_TRUE) ? "f" : ""));
    }

    return ((bits >> MAREA_UINTEGER_32(63)) != MAREA_UINTEGER_64(0)) ? MAREA_CGEN_JOIN(generator, "(-", text, ")") : text;
}

// INFO: the bits of an integer constant cut to its width and read with its sign, the way the vm normalizes them.
static const MAREA_Character8 *MAREA_CALL MAREA_G_IntegerText(MAREA_CgenGenerator *generator, MAREA_Uinteger32 kind, MAREA_Uinteger64 bits)
{
    const MAREA_Uinteger32 WIDTH = (MAREA_UINTEGER_32(8) << (kind & MAREA_UINTEGER_32(3)));

    if (WIDTH < MAREA_UINTEGER_32(64)) {
        const MAREA_Uinteger64 MASK = ((MAREA_UINTEGER_64(1) << WIDTH) - MAREA_UINTEGER_64(1));

        bits = (bits & MASK);

        if ((MAREA_CgenIsSigned(kind) == MAREA_BOOLEAN_TRUE) && ((bits >> (WIDTH - MAREA_UINTEGER_32(1))) != MAREA_UINTEGER_64(0))) {
            bits = (bits | (~MASK));
        }
    }

    const MAREA_Character8 *cType = MAREA_CgenTypeName(generator, kind);

    if (MAREA_CgenIsSigned(kind) == MAREA_BOOLEAN_FALSE) {
        return MAREA_CGEN_JOIN(generator, "((", cType, ")", MAREA_CgenNumber(generator, bits), "ULL)");
    }

    if (bits == (MAREA_UINTEGER_64(1) << MAREA_UINTEGER_32(63))) {
        return "((int64_t)(-9223372036854775807LL - 1))";
    }

    if ((bits >> MAREA_UINTEGER_32(63)) != MAREA_UINTEGER_64(0)) {
        return MAREA_CGEN_JOIN(generator, "((", cType, ")-", MAREA_CgenNumber(generator, (MAREA_UINTEGER_64(0) - bits)), "LL)");
    }

    return MAREA_CGEN_JOIN(generator, "((", cType, ")", MAREA_CgenNumber(generator, bits), "LL)");
}

static void MAREA_CALL MAREA_G_SetValue(MAREA_CgenValue *outValue, const MAREA_Character8 *text, MAREA_Uinteger32 type, MAREA_Boolean stable, MAREA_Boolean unaligned)
{
    outValue->text = text;
    outValue->type = type;
    outValue->stable = stable;
    outValue->unaligned = unaligned;
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteNumberLiteral(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_Boolean negate, MAREA_Uinteger32 expected, MAREA_CgenValue *outValue)
{
    const MAREA_Uinteger32 OWN = MAREA_CgenKindFromText(MAREA_IrNodeGetResolvedType(node), MAREA_IrNodeGetResolvedTypeLength(node));

    const MAREA_Uinteger32 WANTED = (MAREA_CgenIsNumber(OWN) == MAREA_BOOLEAN_TRUE) ? OWN : expected;

    if (MAREA_IrNodeGetKind(node) == MAREA_IR_KIND_INTEGER_LITERAL) {
        MAREA_Uinteger64 value = MAREA_UINTEGER_64(0);

        if (MAREA_CgenParseInteger(MAREA_IrNodeGetText(node), MAREA_IrNodeGetTextLength(node), &value) == MAREA_BOOLEAN_FALSE) {
            return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "An integer literal does not fit 64 bits.");
        }

        const MAREA_Uinteger32 KIND = (MAREA_CgenIsNumber(WANTED) == MAREA_BOOLEAN_TRUE) ? WANTED : (MAREA_Uinteger32)MAREA_CGEN_KIND_SINTEGER_32;

        if (MAREA_CgenIsFloat(KIND) == MAREA_BOOLEAN_TRUE) {
            const MAREA_Float64 NUMBER = (MAREA_Float64)value;

            MAREA_G_SetValue(outValue, MAREA_G_FloatText(generator, ((negate == MAREA_BOOLEAN_TRUE) ? -NUMBER : NUMBER), (KIND == MAREA_CGEN_KIND_FLOAT_32)), KIND, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_FALSE);

            return MAREA_CGEN_EXCEPT_SUCCESS;
        }

        MAREA_G_SetValue(outValue, MAREA_G_IntegerText(generator, KIND, ((negate == MAREA_BOOLEAN_TRUE) ? (MAREA_UINTEGER_64(0) - value) : value)), KIND, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_FALSE);

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    MAREA_Float64 number = MAREA_FLOAT_64(0.0);

    if (MAREA_G_ParseFloat(MAREA_IrNodeGetText(node), MAREA_IrNodeGetTextLength(node), &number) == MAREA_BOOLEAN_FALSE) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A float literal cannot be read.");
    }

    const MAREA_Uinteger32 KIND = (MAREA_CgenIsFloat(WANTED) == MAREA_BOOLEAN_TRUE) ? WANTED : (MAREA_Uinteger32)MAREA_CGEN_KIND_FLOAT_64;

    MAREA_G_SetValue(outValue, MAREA_G_FloatText(generator, ((negate == MAREA_BOOLEAN_TRUE) ? -number : number), (KIND == MAREA_CGEN_KIND_FLOAT_32)), KIND, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_FALSE);

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteModuleScoped(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, const MAREA_Character8 *owner, MAREA_IntegerSize ownerLength, MAREA_Uinteger32 expected, MAREA_CgenValue *outValue)
{
    const MAREA_IntegerSize SYMBOLS = generator->symbolCount;
    const MAREA_Character8 *selfName = generator->selfName;
    const MAREA_IntegerSize SELF_LENGTH = generator->selfLength;

    generator->symbolCount = MAREA_INTEGER_SIZE(0);
    generator->selfName = (ownerLength > MAREA_INTEGER_SIZE(0)) ? owner : MAREA_NULLPTR;
    generator->selfLength = ownerLength;

    const MAREA_CgenExcept EXCEPT = (expected == MAREA_CGEN_KIND_NONE) ? MAREA_CgenWriteValue(generator, node, expected, outValue) : MAREA_CgenWriteTyped(generator, node, expected, outValue);

    generator->symbolCount = SYMBOLS;
    generator->selfName = selfName;
    generator->selfLength = SELF_LENGTH;

    return EXCEPT;
}

// INFO: a constant is built where it is read, so each reader gets a fresh value, as in the vm.
MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteConstant(MAREA_CgenGenerator *generator, const MAREA_IrNode *constant, MAREA_CgenValue *outValue)
{
    const MAREA_IrNode *value = MAREA_CgenChildAt(constant, MAREA_INTEGER_SIZE(0));

    if ((value == MAREA_NULLPTR) || (generator->constantDepth >= MAREA_CGEN_CONSTANT_DEPTH)) {
        return MAREA_CgenGeneratorFail(generator, constant, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A constant has no value, or reads constants too deep.");
    }

    const MAREA_Character8 *name = MAREA_IrNodeGetText(constant);

    MAREA_IntegerSize owner = MAREA_IrNodeGetTextLength(constant);

    while ((owner > MAREA_INTEGER_SIZE(0)) && (name[(owner - MAREA_INTEGER_SIZE(1))] != '.')) {
        owner = (owner - MAREA_INTEGER_SIZE(1));
    }

    owner = (owner > MAREA_INTEGER_SIZE(0)) ? (owner - MAREA_INTEGER_SIZE(1)) : MAREA_INTEGER_SIZE(0);

    const MAREA_Character8 *selfName = generator->selfName;
    const MAREA_IntegerSize SELF_LENGTH = generator->selfLength;

    generator->selfName = (owner > MAREA_INTEGER_SIZE(0)) ? name : MAREA_NULLPTR;
    generator->selfLength = owner;

    MAREA_Uinteger32 type = MAREA_CGEN_KIND_NONE;

    const MAREA_CgenExcept TYPED = MAREA_CgenTypeOfNode(generator, constant, &type);

    generator->selfName = selfName;
    generator->selfLength = SELF_LENGTH;

    MAREA_CGEN_TRY(TYPED);

    if (MAREA_CgenIsValueType(generator, type) == MAREA_BOOLEAN_FALSE) {
        return MAREA_CgenGeneratorFail(generator, constant, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A constant has a type that cgen does not write.");
    }

    generator->constantDepth = (generator->constantDepth + MAREA_UINTEGER_32(1));

    const MAREA_CgenExcept EXCEPT = MAREA_CgenWriteModuleScoped(generator, value, name, owner, type, outValue);

    generator->constantDepth = (generator->constantDepth - MAREA_UINTEGER_32(1));

    return EXCEPT;
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteIdentifier(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_CgenValue *outValue)
{
    const MAREA_CgenSymbol *symbol = MAREA_CgenFindSymbol(generator, MAREA_IrNodeGetText(node), MAREA_IrNodeGetTextLength(node));

    if (symbol != MAREA_NULLPTR) {
        if (symbol->type == MAREA_CGEN_KIND_NONE) {
            return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "The part of a partition is not a value yet.");
        }

        MAREA_G_SetValue(outValue, symbol->cText, symbol->type, symbol->stable, symbol->unaligned);

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    const MAREA_IrNode *constant = MAREA_CgenFindConstant(generator, MAREA_IrNodeGetText(node), MAREA_IrNodeGetTextLength(node));

    if (constant != MAREA_NULLPTR) {
        return MAREA_CgenWriteConstant(generator, constant, outValue);
    }

    return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNRESOLVED_NAME, "A name is not a local, a parameter or a constant of the module.");
}

static const MAREA_Character8 *MAREA_CALL MAREA_G_DottedName(MAREA_CgenGenerator *generator, const MAREA_IrNode *node)
{
    if (MAREA_IrNodeGetKind(node) != MAREA_IR_KIND_MEMBER_ACCESS) {
        return MAREA_CgenCopyText(generator, MAREA_IrNodeGetText(node), MAREA_IrNodeGetTextLength(node));
    }

    const MAREA_IrNode *member = MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(1));

    return MAREA_CGEN_JOIN(generator, MAREA_G_DottedName(generator, MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(0))), ".", MAREA_CgenCopyText(generator, MAREA_IrNodeGetText(member), MAREA_IrNodeGetTextLength(member)));
}

static MAREA_Boolean MAREA_CALL MAREA_G_NamesAType(const MAREA_CgenGenerator *generator, const MAREA_Character8 *name, MAREA_IntegerSize length)
{
    return ((MAREA_CgenFindDeclaration(generator, MAREA_IR_KIND_STRUCTURE_DECLARATION, name, length) != MAREA_NULLPTR) || (MAREA_CgenFindDeclaration(generator, MAREA_IR_KIND_ENUM_DECLARATION, name, length) != MAREA_NULLPTR));
}

// INFO: a name of a namespace has no resolved type and is no local and no type, as the bytecode stage reads it.
const MAREA_Character8 *MAREA_CALL MAREA_CgenQualifiedName(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_IntegerSize *outLength)
{
    *outLength = MAREA_INTEGER_SIZE(0);

    const MAREA_IrNode *root = node;

    while (MAREA_IrNodeGetKind(root) == MAREA_IR_KIND_MEMBER_ACCESS) {
        root = MAREA_CgenChildAt(root, MAREA_INTEGER_SIZE(0));
    }

    if ((root == node) || (MAREA_IrNodeGetKind(root) != MAREA_IR_KIND_IDENTIFIER) || (MAREA_IrNodeGetResolvedType(root) != MAREA_NULLPTR)) {
        return MAREA_NULLPTR;
    }

    const MAREA_Character8 *text = MAREA_IrNodeGetText(root);
    const MAREA_IntegerSize LENGTH = MAREA_IrNodeGetTextLength(root);

    if ((MAREA_CgenFindSymbol(generator, text, LENGTH) != MAREA_NULLPTR) || (MAREA_CgenSameText(text, LENGTH, "Self", MAREA_INTEGER_SIZE(4)) == MAREA_BOOLEAN_TRUE) || (MAREA_G_NamesAType(generator, text, LENGTH) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_NULLPTR;
    }

    const MAREA_Character8 *name = MAREA_G_DottedName(generator, node);

    *outLength = (MAREA_IntegerSize)strlen(name);

    return name;
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenQualifierType(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 *outType)
{
    *outType = MAREA_CGEN_KIND_NONE;

    const MAREA_Character8 *name = MAREA_NULLPTR;
    MAREA_IntegerSize length = MAREA_INTEGER_SIZE(0);

    if (MAREA_IrNodeGetKind(node) == MAREA_IR_KIND_IDENTIFIER) {
        name = MAREA_IrNodeGetText(node);
        length = MAREA_IrNodeGetTextLength(node);

        if ((MAREA_IrNodeGetResolvedType(node) != MAREA_NULLPTR) || (MAREA_CgenFindSymbol(generator, name, length) != MAREA_NULLPTR)) {
            return MAREA_CGEN_EXCEPT_SUCCESS;
        }

        if ((MAREA_CgenSameText(name, length, "Self", MAREA_INTEGER_SIZE(4)) == MAREA_BOOLEAN_TRUE) && (generator->selfName != MAREA_NULLPTR)) {
            name = generator->selfName;
            length = generator->selfLength;
        }
    } else if (MAREA_IrNodeGetKind(node) == MAREA_IR_KIND_MEMBER_ACCESS) {
        name = MAREA_CgenQualifiedName(generator, node, &length);
    }

    if (name == MAREA_NULLPTR) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    const MAREA_IntegerSize SKIP = MAREA_CgenNamespaceSkip(generator, name, length);

    if (MAREA_G_NamesAType(generator, (name + SKIP), (length - SKIP)) == MAREA_BOOLEAN_FALSE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    return MAREA_CgenTypeOfText(generator, name, length, MAREA_NULLPTR, MAREA_INTEGER_SIZE(0), outType);
}

// INFO: the data of a variant is read through the tag, so another variant is the unreachable trap and never a read of other bytes. A composite field is reached by address, so a write through it changes the value.
static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteEnumField(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, const MAREA_CgenValue *base, const MAREA_IrNode *member, MAREA_CgenValue *outValue)
{
    const MAREA_CgenType *entry = MAREA_CgenTypeAt(generator, base->type);

    const MAREA_CgenVariant *variants = entry->variants;
    const MAREA_IntegerSize COUNT = entry->memberCount;
    const MAREA_Boolean PACKED = entry->packed;

    MAREA_Uinteger32 type = MAREA_CGEN_KIND_NONE;

    for (MAREA_IntegerSize variant = MAREA_INTEGER_SIZE(0); (type == MAREA_CGEN_KIND_NONE) && (variant < COUNT); variant = (variant + MAREA_INTEGER_SIZE(1))) {
        for (MAREA_IntegerSize field = MAREA_INTEGER_SIZE(0); field < variants[variant].fieldCount; field = (field + MAREA_INTEGER_SIZE(1))) {
            if (MAREA_CgenSameText(variants[variant].fields[field].name, variants[variant].fields[field].nameLength, MAREA_IrNodeGetText(member), MAREA_IrNodeGetTextLength(member)) == MAREA_BOOLEAN_TRUE) {
                type = variants[variant].fields[field].type;

                break;
            }
        }
    }

    if (type == MAREA_CGEN_KIND_NONE) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNRESOLVED_NAME, "No variant of the enum has a field of this name.");
    }

    const MAREA_Boolean AGGREGATE = MAREA_CgenIsAggregate(generator, type);

    const MAREA_Character8 *name = MAREA_CgenTemporaryName(generator);
    const MAREA_Character8 *field = MAREA_CgenMangled(generator, "f_", MAREA_IrNodeGetText(member), MAREA_IrNodeGetTextLength(member));

    MAREA_CGEN_WRITE(generator->buffer, MAREA_CgenTypeName(generator, type), ((AGGREGATE == MAREA_BOOLEAN_TRUE) ? " *" : " "), name, ((AGGREGATE == MAREA_BOOLEAN_TRUE) ? " = NULL;\n" : " = 0;\n"));
    MAREA_CGEN_WRITE(generator->buffer, "switch ((", base->text, ").marea_tag) {\n");

    for (MAREA_IntegerSize variant = MAREA_INTEGER_SIZE(0); variant < COUNT; variant = (variant + MAREA_INTEGER_SIZE(1))) {
        for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < variants[variant].fieldCount; index = (index + MAREA_INTEGER_SIZE(1))) {
            if ((variants[variant].fields[index].type == type) && (MAREA_CgenSameText(variants[variant].fields[index].name, variants[variant].fields[index].nameLength, MAREA_IrNodeGetText(member), MAREA_IrNodeGetTextLength(member)) == MAREA_BOOLEAN_TRUE)) {
                MAREA_CGEN_WRITE(generator->buffer, "case ", MAREA_CgenNumber(generator, variant), ":\n", name, ((AGGREGATE == MAREA_BOOLEAN_TRUE) ? " = &" : " = "), "(", base->text, ").marea_data.v_", MAREA_CgenNumber(generator, variant), ".", field, ";\nbreak;\n");
            }
        }
    }

    MAREA_CgenWriteText(generator->buffer, "default:\n");
    MAREA_CgenWriteTrap(generator, "MAREA_RT_TRAP_UNREACHABLE");
    MAREA_CgenWriteText(generator->buffer, "}\n");

    if (AGGREGATE == MAREA_BOOLEAN_TRUE) {
        MAREA_G_SetValue(outValue, MAREA_CGEN_JOIN(generator, "(*", name, ")"), type, MAREA_BOOLEAN_FALSE, ((base->unaligned == MAREA_BOOLEAN_TRUE) || (PACKED == MAREA_BOOLEAN_TRUE)));
    } else {
        MAREA_G_SetValue(outValue, name, type, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_FALSE);
    }

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteMember(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, const MAREA_CgenValue *base, MAREA_CgenValue *outPlace)
{
    const MAREA_IrNode *member = MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(1));

    const MAREA_Uinteger8 CATEGORY = MAREA_CgenCategoryOf(generator, base->type);

    if (CATEGORY == MAREA_CGEN_CATEGORY_STRUCTURE) {
        const MAREA_CgenField *field = MAREA_CgenFindField(generator, base->type, MAREA_IrNodeGetText(member), MAREA_IrNodeGetTextLength(member));

        if (field == MAREA_NULLPTR) {
            return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNRESOLVED_NAME, "A structure has no field of this name.");
        }

        const MAREA_Boolean UNALIGNED = ((base->unaligned == MAREA_BOOLEAN_TRUE) || (MAREA_CgenTypeAt(generator, base->type)->packed == MAREA_BOOLEAN_TRUE));

        MAREA_G_SetValue(outPlace, MAREA_CGEN_JOIN(generator, "(", base->text, ").", MAREA_CgenMangled(generator, "f_", field->name, field->nameLength)), field->type, MAREA_BOOLEAN_FALSE, UNALIGNED);

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    if ((CATEGORY == MAREA_CGEN_CATEGORY_ENUM) && (MAREA_CgenIsAggregate(generator, base->type) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_G_WriteEnumField(generator, node, base, member, outPlace);
    }

    return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "This value has no member that cgen reads.");
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteMemberPlace(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_CgenValue *outPlace)
{
    MAREA_CgenValue base = { 0 };

    MAREA_CGEN_TRY(MAREA_CgenWritePlace(generator, MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(0)), MAREA_CGEN_KIND_NONE, &base));

    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    return MAREA_CgenWriteMember(generator, node, &base, outPlace);
}

// INFO: the index is checked when the place is reached, before the value that is stored is computed. A negative index reads as a very large one, so it is past the count too.
static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteIndexPlace(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_CgenValue *outPlace)
{
    MAREA_CgenValue base = { 0 };

    MAREA_CGEN_TRY(MAREA_CgenWritePlace(generator, MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(0)), MAREA_CGEN_KIND_NONE, &base));

    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    if (MAREA_CgenCategoryOf(generator, base.type) != MAREA_CGEN_CATEGORY_ARRAY) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "Only an array of a registered inline run is indexed.");
    }

    const MAREA_Uinteger64 COUNT = MAREA_CgenTypeAt(generator, base.type)->count;
    const MAREA_Uinteger32 ELEMENT = MAREA_CgenTypeAt(generator, base.type)->element;

    MAREA_CgenValue index = { 0 };

    MAREA_CGEN_TRY(MAREA_CgenWriteValue(generator, MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(1)), MAREA_CGEN_KIND_NONE, &index));

    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    if (MAREA_CgenIsInteger(index.type) == MAREA_BOOLEAN_FALSE) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "An index is an integer.");
    }

    // INFO: an array of no element has no index in range, and a test against zero would never be false.
    if (COUNT == MAREA_UINTEGER_64(0)) {
        MAREA_CGEN_WRITE(generator->buffer, "(void)(", index.text, ");\n");
        MAREA_CgenWriteEmptyElement(generator, ELEMENT, outPlace);

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *name = MAREA_CgenTemporaryName(generator);

    MAREA_CGEN_WRITE(generator->buffer, "uint64_t ", name, " = (uint64_t)(", index.text, ");\nif (", name, " >= ", MAREA_CgenNumber(generator, COUNT), "ULL) {\n");
    MAREA_CgenWriteTrap(generator, "MAREA_RT_TRAP_INDEX_OUT_OF_BOUNDS");
    MAREA_CgenWriteText(generator->buffer, "}\n");

    MAREA_G_SetValue(outPlace, MAREA_CGEN_JOIN(generator, "(", base.text, ").items[", name, "]"), ELEMENT, MAREA_BOOLEAN_FALSE, base.unaligned);

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

// INFO: a member access names a variant or a constant of a type, a constant of a namespace or a part of a value.
static MAREA_CgenExcept MAREA_CALL MAREA_G_ReadQualified(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_Boolean *outQualified, MAREA_CgenValue *outValue)
{
    const MAREA_IrNode *base = MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(0));
    const MAREA_IrNode *member = MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(1));

    *outQualified = MAREA_BOOLEAN_TRUE;

    MAREA_Uinteger32 type = MAREA_CGEN_KIND_NONE;

    MAREA_CGEN_TRY(MAREA_CgenQualifierType(generator, base, &type));

    const MAREA_Character8 *name = MAREA_NULLPTR;

    if (type != MAREA_CGEN_KIND_NONE) {
        MAREA_IntegerSize variant = MAREA_INTEGER_SIZE(0);

        if (MAREA_CgenFindVariant(generator, type, MAREA_IrNodeGetText(member), MAREA_IrNodeGetTextLength(member), &variant) == MAREA_BOOLEAN_TRUE) {
            return MAREA_CgenWriteVariant(generator, node, type, variant, MAREA_NULLPTR, outValue);
        }

        const MAREA_CgenType *entry = MAREA_CgenTypeAt(generator, type);

        name = MAREA_CGEN_JOIN(generator, MAREA_CgenCopyText(generator, entry->name, entry->nameLength), ".", MAREA_CgenCopyText(generator, MAREA_IrNodeGetText(member), MAREA_IrNodeGetTextLength(member)));
    } else {
        MAREA_IntegerSize length = MAREA_INTEGER_SIZE(0);

        name = MAREA_CgenQualifiedName(generator, node, &length);
    }

    if (name == MAREA_NULLPTR) {
        *outQualified = MAREA_BOOLEAN_FALSE;

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    const MAREA_IrNode *constant = MAREA_CgenFindConstant(generator, name, (MAREA_IntegerSize)strlen(name));

    if (constant == MAREA_NULLPTR) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNRESOLVED_NAME, "A qualified name is no variant and no constant of the module.");
    }

    return MAREA_CgenWriteConstant(generator, constant, outValue);
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenWritePlace(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 expected, MAREA_CgenValue *outPlace)
{
    MAREA_G_SetValue(outPlace, "0", MAREA_CGEN_KIND_NONE, MAREA_BOOLEAN_FALSE, MAREA_BOOLEAN_FALSE);

    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    if (node == MAREA_NULLPTR) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "An expression is missing.");
    }

    if (MAREA_IrNodeGetKind(node) == MAREA_IR_KIND_INDEX_ACCESS) {
        return MAREA_G_WriteIndexPlace(generator, node, outPlace);
    }

    if (MAREA_IrNodeGetKind(node) == MAREA_IR_KIND_MEMBER_ACCESS) {
        MAREA_Boolean qualified = MAREA_BOOLEAN_FALSE;

        MAREA_CGEN_TRY(MAREA_G_ReadQualified(generator, node, &qualified, outPlace));

        if (qualified == MAREA_BOOLEAN_TRUE) {
            return MAREA_CGEN_EXCEPT_SUCCESS;
        }

        return MAREA_G_WriteMemberPlace(generator, node, outPlace);
    }

    MAREA_CGEN_TRY(MAREA_CgenWriteValue(generator, node, expected, outPlace));

    if ((generator->functionFailed == MAREA_BOOLEAN_FALSE) && (outPlace->stable == MAREA_BOOLEAN_FALSE) && (MAREA_CgenIsAggregate(generator, outPlace->type) == MAREA_BOOLEAN_FALSE) && (MAREA_IrNodeGetKind(node) != MAREA_IR_KIND_IDENTIFIER)) {
        MAREA_CgenSnapshot(generator, outPlace);
    }

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

// INFO: a scalar read from a member or an element goes into a temporary, so a later operand cannot change it.
static MAREA_CgenExcept MAREA_CALL MAREA_G_ReadPlace(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_CgenValue *outValue)
{
    MAREA_CGEN_TRY(MAREA_CgenWritePlace(generator, node, MAREA_CGEN_KIND_NONE, outValue));

    if ((generator->functionFailed == MAREA_BOOLEAN_FALSE) && (outValue->stable == MAREA_BOOLEAN_FALSE) && (MAREA_CgenIsAggregate(generator, outValue->type) == MAREA_BOOLEAN_FALSE)) {
        MAREA_CgenSnapshot(generator, outValue);
    }

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

// INFO: this stage has no build settings, so build_value! is its default, as in the bytecode stage.
static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteMacro(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 expected, MAREA_CgenValue *outValue)
{
    const MAREA_IrNode *arguments = MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(0));

    if ((MAREA_CgenTextIs(node, "build_value") == MAREA_BOOLEAN_TRUE) && (arguments != MAREA_NULLPTR) && (MAREA_IrNodeGetChildCount(arguments) == MAREA_INTEGER_SIZE(3))) {
        return MAREA_CgenWriteValue(generator, MAREA_CgenChildAt(arguments, MAREA_INTEGER_SIZE(2)), expected, outValue);
    }

    return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A macro of a library is not written.");
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteValue(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 expected, MAREA_CgenValue *outValue)
{
    MAREA_G_SetValue(outValue, "0", MAREA_CGEN_KIND_NONE, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_FALSE);

    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    if (node == MAREA_NULLPTR) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "An expression is missing.");
    }

    switch (MAREA_IrNodeGetKind(node)) {
    case MAREA_IR_KIND_INTEGER_LITERAL:
    case MAREA_IR_KIND_FLOAT_LITERAL:
        return MAREA_CgenWriteNumberLiteral(generator, node, MAREA_BOOLEAN_FALSE, expected, outValue);
    case MAREA_IR_KIND_BOOLEAN_LITERAL:
        MAREA_G_SetValue(outValue, ((MAREA_CgenTextIs(node, "true") == MAREA_BOOLEAN_TRUE) ? "true" : "false"), MAREA_CGEN_KIND_BOOLEAN, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_FALSE);

        return MAREA_CGEN_EXCEPT_SUCCESS;
    case MAREA_IR_KIND_IDENTIFIER:
        return MAREA_G_WriteIdentifier(generator, node, outValue);
    case MAREA_IR_KIND_UNARY_EXPRESSION:
        return MAREA_CgenWriteUnary(generator, node, expected, outValue);
    case MAREA_IR_KIND_BINARY_EXPRESSION:
        return MAREA_CgenWriteBinary(generator, node, expected, outValue);
    case MAREA_IR_KIND_CALL_EXPRESSION:
        return MAREA_CgenWriteCall(generator, node, MAREA_BOOLEAN_TRUE, outValue);
    case MAREA_IR_KIND_MEMBER_ACCESS:
    case MAREA_IR_KIND_INDEX_ACCESS:
        return MAREA_G_ReadPlace(generator, node, outValue);
    case MAREA_IR_KIND_LITERAL_VALUE:
        return MAREA_CgenWriteLiteralValue(generator, node, expected, outValue);
    case MAREA_IR_KIND_CONSTRUCT_CALL:
        return MAREA_CgenWriteConstruct(generator, node, outValue);
    case MAREA_IR_KIND_INCLUDED_BYTES:
        return MAREA_CgenWriteIncludedBytes(generator, node, outValue);
    case MAREA_IR_KIND_MACRO_CALL:
        return MAREA_G_WriteMacro(generator, node, expected, outValue);
    case MAREA_IR_KIND_STRING_LITERAL:
    case MAREA_IR_KIND_CHARACTER_LITERAL:
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A string and a character are not written yet.");
    default:
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "An expression is not written yet.");
    }
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteTyped(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 expected, MAREA_CgenValue *outValue)
{
    MAREA_CGEN_TRY(MAREA_CgenWriteValue(generator, node, expected, outValue));

    if ((generator->functionFailed == MAREA_BOOLEAN_FALSE) && (outValue->type != expected)) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A value has another type than the place that takes it.");
    }

    return MAREA_CGEN_EXCEPT_SUCCESS;
}
