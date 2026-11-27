// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/cgen/MAREACgenInternal.h"

#include <string.h>

// INFO: FNV-1a over the bytes of one part, with a zero byte after it, so the parts do not run together.
static MAREA_Uinteger64 MAREA_CALL MAREA_G_Hash(MAREA_Uinteger64 hash, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index <= length; index = (index + MAREA_INTEGER_SIZE(1))) {
        hash = (hash ^ ((index < length) ? (MAREA_Uinteger64)(MAREA_Uinteger8)text[index] : MAREA_UINTEGER_64(0)));
        hash = (hash * MAREA_UINTEGER_64(1099511628211));
    }

    return hash;
}

// INFO: the symbol is made from the namespace of the unit, the path and the bytes, so two units never share one and two uses of one file in a unit do.
static const MAREA_Character8 *MAREA_CALL MAREA_G_SymbolName(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, const MAREA_IrNode *path)
{
    static const MAREA_Character8 DIGITS[] = "0123456789abcdef";

    MAREA_Uinteger64 hash = MAREA_UINTEGER_64(14695981039346656037);

    hash = MAREA_G_Hash(hash, MAREA_IrNodeGetText(generator->module), MAREA_IrNodeGetTextLength(generator->module));
    hash = MAREA_G_Hash(hash, MAREA_IrNodeGetText(path), MAREA_IrNodeGetTextLength(path));
    hash = MAREA_G_Hash(hash, MAREA_IrNodeGetText(node), MAREA_IrNodeGetTextLength(node));

    MAREA_Character8 digits[16];

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < MAREA_INTEGER_SIZE(16); index = (index + MAREA_INTEGER_SIZE(1))) {
        digits[index] = DIGITS[((hash >> (MAREA_UINTEGER_64(60) - (MAREA_UINTEGER_64(4) * index))) & MAREA_UINTEGER_64(15))];
    }

    return MAREA_CGEN_JOIN(generator, "marea_included_", MAREA_CgenCopyText(generator, digits, MAREA_INTEGER_SIZE(16)));
}

// INFO: writes a path as the inside of a string literal. Each byte that is not plain printable ASCII becomes an octal escape, which both C and the assembler read.
static const MAREA_Character8 *MAREA_CALL MAREA_G_Escape(MAREA_CgenGenerator *generator, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_CgenAllocate(generator, ((length * MAREA_INTEGER_SIZE(4)) + MAREA_INTEGER_SIZE(1)), MAREA_INTEGER_SIZE(1), &memory) != MAREA_CGEN_EXCEPT_SUCCESS) {
        return "";
    }

    MAREA_Character8 *escaped = memory;

    MAREA_IntegerSize used = MAREA_INTEGER_SIZE(0);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < length; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_Uinteger8 BYTE = (MAREA_Uinteger8)text[index];

        if ((BYTE >= 32) && (BYTE < 127) && (BYTE != '"') && (BYTE != '\\') && (BYTE != '?')) {
            escaped[used++] = (MAREA_Character8)BYTE;
        } else {
            escaped[used++] = '\\';
            escaped[used++] = (MAREA_Character8)('0' + (BYTE >> 6));
            escaped[used++] = (MAREA_Character8)('0' + ((BYTE >> 3) & 7));
            escaped[used++] = (MAREA_Character8)('0' + (BYTE & 7));
        }
    }

    escaped[used] = '\0';

    return escaped;
}

// INFO: the assembler reads the path from a string of its own, inside the C string of the asm statement, so each mark of the first escape is escaped again.
static const MAREA_Character8 *MAREA_CALL MAREA_G_EscapeForAssembler(MAREA_CgenGenerator *generator, const MAREA_Character8 *text)
{
    const MAREA_IntegerSize LENGTH = (MAREA_IntegerSize)strlen(text);

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_CgenAllocate(generator, ((LENGTH * MAREA_INTEGER_SIZE(2)) + MAREA_INTEGER_SIZE(1)), MAREA_INTEGER_SIZE(1), &memory) != MAREA_CGEN_EXCEPT_SUCCESS) {
        return "";
    }

    MAREA_Character8 *escaped = memory;

    MAREA_IntegerSize used = MAREA_INTEGER_SIZE(0);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < LENGTH; index = (index + MAREA_INTEGER_SIZE(1))) {
        if (text[index] == '\\') {
            escaped[used++] = '\\';
        }

        escaped[used++] = text[index];
    }

    escaped[used] = '\0';

    return escaped;
}

// INFO: the C compiler puts the file in a read only section of the object; the .if stops the build when the file no longer has the size that the program was checked with. Emscripten has no such directive, so there the program loads the file from its virtual file system before main, and a file that is missing or has another size aborts it.
static void MAREA_CALL MAREA_G_WriteDefinition(MAREA_CgenGenerator *generator, const MAREA_Character8 *symbol, const MAREA_Character8 *type, const MAREA_Character8 *path, const MAREA_Character8 *count)
{
    const MAREA_Character8 *label = MAREA_CGEN_JOIN(generator, "MAREA_LABEL_TEXT(__USER_LABEL_PREFIX__) \"", symbol);

    MAREA_CGEN_WRITE(generator->types, "#if defined(__EMSCRIPTEN__)\n#include <stdio.h>\n#include <stdlib.h>\n");
    MAREA_CGEN_WRITE(generator->types, "static ", type, " ", symbol, ";\n");
    MAREA_CGEN_WRITE(generator->types, "__attribute__((__constructor__)) static void ", symbol, "_load(void)\n{\n    FILE *file = fopen(\"", path, "\", \"rb\");\n");
    MAREA_CGEN_WRITE(generator->types, "    if ((file == NULL) || (fread(&", symbol, ", 1, ", count, "ULL, file) != ", count, "ULL) || (fgetc(file) != EOF)) {\n");
    MAREA_CGEN_WRITE(generator->types, "        fputs(\"marea: an included file is missing or has another size: \" \"", path, "\" \"\\n\", stderr);\n        abort();\n    }\n    fclose(file);\n}\n#else\n");
    MAREA_CGEN_WRITE(generator->types, "#if !defined(MAREA_LABEL_TEXT)\n#define MAREA_LABEL_TEXT_OF(prefix) #prefix\n#define MAREA_LABEL_TEXT(prefix) MAREA_LABEL_TEXT_OF(prefix)\n#endif\n");
    MAREA_CGEN_WRITE(generator->types, "__asm__(\n#if defined(__APPLE__)\n    \".section __TEXT,__const\\n\"\n#elif defined(_WIN32)\n    \".section .rdata,\\\"dr\\\"\\n\"\n#else\n    \".pushsection .rodata\\n\"\n    \".hidden ", symbol, "\\n\"\n#endif\n");
    MAREA_CGEN_WRITE(generator->types, "    \".globl \" ", label, "\\n\"\n    ", label, ":\\n\"\n");
    MAREA_CGEN_WRITE(generator->types, "    \".incbin \\\"", MAREA_G_EscapeForAssembler(generator, path), "\\\"\\n\"\n");
    MAREA_CGEN_WRITE(generator->types, "    \".if . - \" ", label, " != ", count, "\\n\"\n    \".error \\\"an included file has another size\\\"\\n\"\n    \".endif\\n\"\n");
    MAREA_CGEN_WRITE(generator->types, "#if defined(__APPLE__) || defined(_WIN32)\n    \".text\\n\"\n#else\n    \".popsection\\n\"\n#endif\n);\n");
    MAREA_CGEN_WRITE(generator->types, "extern const ", type, " ", symbol, ";\n#endif\n");
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteIncludedBytes(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_CgenValue *outValue)
{
    MAREA_Uinteger32 type = MAREA_CGEN_KIND_NONE;

    MAREA_CGEN_TRY(MAREA_CgenTypeOfNode(generator, node, &type));

    const MAREA_IrNode *path = MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(0));

    if ((MAREA_CgenCategoryOf(generator, type) != MAREA_CGEN_CATEGORY_ARRAY) || (path == MAREA_NULLPTR)) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "An included file makes an array of bytes.");
    }

    if (MAREA_IrNodeGetTextLength(path) == MAREA_INTEGER_SIZE(0)) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "The include resolver gave no path of the file.");
    }

    const MAREA_Character8 *symbol = MAREA_G_SymbolName(generator, node, path);

    MAREA_Boolean defined = MAREA_BOOLEAN_FALSE;

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < generator->includedCount; index = (index + MAREA_INTEGER_SIZE(1))) {
        defined = ((strcmp(generator->included[index], symbol) == 0) ? MAREA_BOOLEAN_TRUE : defined);
    }

    if (defined == MAREA_BOOLEAN_FALSE) {
        MAREA_Null items = (MAREA_Null)generator->included;

        MAREA_CGEN_TRY(MAREA_CgenGrow(generator, &items, &generator->includedCapacity, generator->includedCount, sizeof(const MAREA_Character8 *), MAREA_ALIGNOF(const MAREA_Character8 *)));

        generator->included = items;
        generator->included[generator->includedCount] = symbol;
        generator->includedCount = (generator->includedCount + MAREA_INTEGER_SIZE(1));

        MAREA_G_WriteDefinition(generator, symbol, MAREA_CgenTypeName(generator, type), MAREA_G_Escape(generator, MAREA_IrNodeGetText(path), MAREA_IrNodeGetTextLength(path)), MAREA_CgenNumber(generator, MAREA_CgenTypeAt(generator, type)->count));
    }

    // INFO: sema lets nothing write the value, so the const of the data is cast away for the place that reads it.
    outValue->text = MAREA_CGEN_JOIN(generator, "(*(", MAREA_CgenTypeName(generator, type), " *)&", symbol, ")");
    outValue->type = type;
    outValue->stable = MAREA_BOOLEAN_FALSE;
    outValue->unaligned = MAREA_BOOLEAN_FALSE;

    return MAREA_CGEN_EXCEPT_SUCCESS;
}
