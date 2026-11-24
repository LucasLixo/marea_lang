// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <string.h>

MAREA_Boolean MAREA_CALL MAREA_SemaTextEquals(const MAREA_Character8 *left, MAREA_IntegerSize leftLength, const MAREA_Character8 *right, MAREA_IntegerSize rightLength)
{
    if (leftLength != rightLength) {
        return MAREA_BOOLEAN_FALSE;
    }

    return (memcmp(left, right, leftLength) == 0) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsDeclarationKind(MAREA_AstKind kind)
{
    switch (kind) {
    case MAREA_AST_KIND_CONSTANT_DECLARATION:
    case MAREA_AST_KIND_TYPE_DECLARATION:
    case MAREA_AST_KIND_STRUCTURE_DECLARATION:
    case MAREA_AST_KIND_OPAQUE_DECLARATION:
    case MAREA_AST_KIND_ENUM_DECLARATION:
    case MAREA_AST_KIND_CONTRACT_DECLARATION:
    case MAREA_AST_KIND_FUNCTION_DECLARATION:
        return MAREA_BOOLEAN_TRUE;
    default:
        return MAREA_BOOLEAN_FALSE;
    }
}

const MAREA_AstNode *MAREA_CALL MAREA_SemaChildAt(const MAREA_AstNode *node, MAREA_IntegerSize index)
{
    const MAREA_AstNode *child = MAREA_NULLPTR;

    if (MAREA_AstNodeGetChildAt(node, index, &child) != MAREA_AST_EXCEPT_SUCCESS) {
        return MAREA_NULLPTR;
    }

    return child;
}

MAREA_Boolean MAREA_CALL MAREA_SemaParameterHasDefault(const MAREA_AstNode *parameter)
{
    // INFO: name and type are the first two children; a trailing "semantics" attribute is not a default.
    const MAREA_AstNode *candidate = MAREA_SemaChildAt(parameter, 2);

    return ((candidate != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(candidate) != MAREA_AST_KIND_ATTRIBUTE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

MAREA_Boolean MAREA_CALL MAREA_SemaDeclarationName(const MAREA_AstNode *node, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_G_IsDeclarationKind(MAREA_AstNodeGetKind(node)) == MAREA_BOOLEAN_FALSE) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(node);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *child = MAREA_SemaChildAt(node, index);

        if ((child != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(child) == MAREA_AST_KIND_IDENTIFIER)) {
            *outText = MAREA_AstNodeGetText(child);
            *outLength = MAREA_AstNodeGetTextLength(child);

            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaQualifiedNameJoin(MAREA_SemaContext *context, const MAREA_AstNode *name, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength)
{
    if ((context == MAREA_NULLPTR) || (name == MAREA_NULLPTR) || (outText == MAREA_NULLPTR) || (outLength == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(name);

    MAREA_IntegerSize total = 0;

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        total += (MAREA_AstNodeGetTextLength(MAREA_SemaChildAt(name, index)) + MAREA_INTEGER_SIZE(1));
    }

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(context->arena, (total + MAREA_INTEGER_SIZE(1)), MAREA_ALIGNOF(MAREA_Character8), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_Character8 *joined = memory;

    MAREA_IntegerSize length = 0;

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *part = MAREA_SemaChildAt(name, index);

        if (index > 0) {
            joined[length] = '.';
            length += 1;
        }

        const MAREA_IntegerSize PART_LENGTH = MAREA_AstNodeGetTextLength(part);

        if (PART_LENGTH > 0) {
            memcpy((joined + length), MAREA_AstNodeGetText(part), PART_LENGTH);
        }

        length += PART_LENGTH;
    }

    joined[length] = '\0';

    *outText = joined;
    *outLength = length;

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

const MAREA_SemaName *MAREA_CALL MAREA_SemaNameTableFind(const MAREA_SemaContext *context, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    for (MAREA_IntegerSize index = 0; index < context->names.count; ++index) {
        const MAREA_SemaName *entry = &context->names.entries[index];

        if (MAREA_SemaTextEquals(entry->text, entry->length, text, length) == MAREA_BOOLEAN_TRUE) {
            return entry;
        }
    }

    return MAREA_NULLPTR;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaTableAppend(MAREA_SemaContext *context, MAREA_SemaNameTable *table, MAREA_SemaName entry)
{
    if ((context == MAREA_NULLPTR) || (table == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    if (table->count == table->capacity) {
        // ERROR: a capacity or byte size that does not fit in MAREA_IntegerSize would allocate a small block and then copy past it.
        if (table->capacity > (MAREA_INTEGER_SIZE_MAXIMUM / MAREA_INTEGER_SIZE(2))) {
            return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
        }

        const MAREA_IntegerSize NEW_CAPACITY = (table->capacity == 0) ? MAREA_INTEGER_SIZE(16) : (table->capacity * MAREA_INTEGER_SIZE(2));

        if (NEW_CAPACITY > (MAREA_INTEGER_SIZE_MAXIMUM / sizeof(MAREA_SemaName))) {
            return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
        }

        MAREA_Null memory = MAREA_NULLPTR;

        if (MAREA_ArenaAllocate(context->arena, (NEW_CAPACITY * sizeof(MAREA_SemaName)), MAREA_ALIGNOF(MAREA_SemaName), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
            return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
        }

        MAREA_SemaName *entries = memory;

        if (table->count > 0) {
            memcpy(entries, table->entries, (table->count * sizeof(MAREA_SemaName)));
        }

        table->entries = entries;
        table->capacity = NEW_CAPACITY;
    }

    table->entries[table->count] = entry;
    table->count += 1;

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaNameTableAdd(MAREA_SemaContext *context, const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_SemaNameKind kind, const MAREA_AstNode *declaration)
{
    return MAREA_SemaTableAppend(context, &context->names, (MAREA_SemaName){ .text = text, .declaration = declaration, .length = length, .kind = kind });
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_BindName(MAREA_SemaContext *context, MAREA_SourcePosition position, const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_SemaNameKind kind, const MAREA_AstNode *declaration, const MAREA_Character8 *original, MAREA_IntegerSize originalLength)
{
    if (MAREA_SemaNameTableFind(context, text, length) != MAREA_NULLPTR) {
        const MAREA_SemaExcept REPORT_RESULT = MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_NAME_ALREADY_DECLARED, position, "name is already declared:", text, length);

        if (REPORT_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return REPORT_RESULT;
        }
    }

    return MAREA_SemaTableAppend(context, &context->names, (MAREA_SemaName){ .text = text, .declaration = declaration, .original = original, .originalLength = originalLength, .length = length, .kind = kind });
}

// INFO: the last name bound remembers the namespace it came from, so two same-named members of two namespaces stay apart.
static MAREA_SemaExcept MAREA_CALL MAREA_G_BindQualified(MAREA_SemaContext *context, MAREA_SourcePosition position, const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_SemaNameKind kind, const MAREA_AstNode *declaration, const MAREA_Character8 *original, MAREA_IntegerSize originalLength, const MAREA_Character8 *qualifier, MAREA_IntegerSize qualifierLength)
{
    const MAREA_SemaExcept RESULT = MAREA_G_BindName(context, position, text, length, kind, declaration, original, originalLength);

    if (RESULT == MAREA_SEMA_EXCEPT_SUCCESS) {
        MAREA_SemaName *entry = &context->names.entries[context->names.count - MAREA_INTEGER_SIZE(1)];

        entry->qualifier = qualifier;
        entry->qualifierLength = qualifierLength;
    }

    return RESULT;
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_AddName(MAREA_SemaContext *context, MAREA_SourcePosition position, const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_SemaNameKind kind, const MAREA_AstNode *declaration)
{
    return MAREA_G_BindName(context, position, text, length, kind, declaration, MAREA_NULLPTR, MAREA_INTEGER_SIZE(0));
}

// INFO: c is the reserved namespace of C, so an alias or a type name must not hide it.
static MAREA_SemaExcept MAREA_CALL MAREA_G_ReportReserved(MAREA_SemaContext *context, MAREA_SourcePosition position, const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_Boolean *outReserved)
{
    *outReserved = MAREA_SemaTextEquals(text, length, "c", MAREA_INTEGER_SIZE(1));

    if (*outReserved == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_NAME_ALREADY_DECLARED, position, "name is reserved:", text, length);
}

// INFO: an import alias must not hide c or a primitive type, and must not be named like a built-in macro. A refused alias is not bound.
static MAREA_SemaExcept MAREA_CALL MAREA_G_ReportAliasName(MAREA_SemaContext *context, MAREA_SourcePosition position, const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_Boolean *outRefused)
{
    const MAREA_SemaExcept RESERVED_RESULT = MAREA_G_ReportReserved(context, position, text, length, outRefused);

    if ((RESERVED_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) || (*outRefused == MAREA_BOOLEAN_TRUE)) {
        return RESERVED_RESULT;
    }

    if (MAREA_SemaIsPrimitiveTypeName(text, length) == MAREA_BOOLEAN_TRUE) {
        *outRefused = MAREA_BOOLEAN_TRUE;

        return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_NAME_ALREADY_DECLARED, position, "name is a primitive type:", text, length);
    }

    if (MAREA_SemaIsBuiltInMacroName(text, length) == MAREA_BOOLEAN_TRUE) {
        *outRefused = MAREA_BOOLEAN_TRUE;

        return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_RESERVED_MACRO_NAME, position, "name is a built-in macro:", text, length);
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

static const MAREA_AstNode *MAREA_CALL MAREA_G_FindVisibleSymbol(const MAREA_AstNode *root, const MAREA_Character8 *name, MAREA_IntegerSize nameLength)
{
    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(root);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *declaration = MAREA_SemaChildAt(root, index);

        const MAREA_Character8 *declarationName = MAREA_NULLPTR;

        MAREA_IntegerSize declarationLength = 0;

        if (MAREA_SemaDeclarationName(declaration, &declarationName, &declarationLength) == MAREA_BOOLEAN_FALSE) {
            continue;
        }

        if ((MAREA_SemaTextEquals(declarationName, declarationLength, name, nameLength) == MAREA_BOOLEAN_TRUE) && (MAREA_SemaIsPrivate(root, declaration) == MAREA_BOOLEAN_FALSE)) {
            return declaration;
        }
    }

    return MAREA_NULLPTR;
}

// INFO: only an enum is kept from an import, so a switch over it is checked for coverage against its real variants.
static const MAREA_AstNode *MAREA_CALL MAREA_G_ImportedEnum(const MAREA_AstNode *namespaceRoot, const MAREA_Character8 *name, MAREA_IntegerSize nameLength)
{
    if (namespaceRoot == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    const MAREA_AstNode *symbol = MAREA_G_FindVisibleSymbol(namespaceRoot, name, nameLength);

    return ((symbol != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(symbol) == MAREA_AST_KIND_ENUM_DECLARATION)) ? symbol : MAREA_NULLPTR;
}

static MAREA_Boolean MAREA_CALL MAREA_G_EnumHasVariant(const MAREA_AstNode *enumeration, const MAREA_Character8 *name, MAREA_IntegerSize nameLength)
{
    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(enumeration);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *variant = MAREA_SemaChildAt(enumeration, index);

        if ((variant == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(variant) != MAREA_AST_KIND_VARIANT_DECLARATION)) {
            continue;
        }

        const MAREA_AstNode *variantName = MAREA_SemaChildAt(variant, 0);

        if ((variantName != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(variantName) == MAREA_AST_KIND_IDENTIFIER) && (MAREA_SemaTextEquals(MAREA_AstNodeGetText(variantName), MAREA_AstNodeGetTextLength(variantName), name, nameLength) == MAREA_BOOLEAN_TRUE)) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

static MAREA_SemaImportResult MAREA_CALL MAREA_G_Resolve(const MAREA_SemaContext *context, const MAREA_Character8 *text, MAREA_IntegerSize length, const MAREA_AstNode **outRoot)
{
    MAREA_SemaImportResult result = MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;

    *outRoot = MAREA_NULLPTR;

    context->resolver->resolve(context->resolver->userData, text, length, &result, outRoot);

    return result;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsPlainName(const MAREA_AstNode *qualifiedName)
{
    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(qualifiedName);

    if (COUNT == 0) {
        return MAREA_BOOLEAN_FALSE;
    }

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *segment = MAREA_SemaChildAt(qualifiedName, index);

        if ((segment == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(segment) != MAREA_AST_KIND_IDENTIFIER)) {
            return MAREA_BOOLEAN_FALSE;
        }
    }

    return MAREA_BOOLEAN_TRUE;
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckNamespacePath(MAREA_SemaContext *context, const MAREA_AstNode *root, const MAREA_AstNode *qualifiedName)
{
    if (MAREA_G_IsPlainName(qualifiedName) == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *file = MAREA_AstNodeGetPosition(root).file;

    if ((file == MAREA_NULLPTR) || (file[0] == '\0')) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    MAREA_IntegerSize fileLength = strlen(file);

    if ((fileLength >= MAREA_INTEGER_SIZE(6)) && (memcmp((file + fileLength - MAREA_INTEGER_SIZE(6)), ".marea", MAREA_INTEGER_SIZE(6)) == 0)) {
        fileLength -= MAREA_INTEGER_SIZE(6);
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(qualifiedName);

    MAREA_IntegerSize position = 0;

    MAREA_Boolean matches = MAREA_BOOLEAN_TRUE;

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *segment = MAREA_SemaChildAt(qualifiedName, index);

        const MAREA_IntegerSize SEGMENT_LENGTH = MAREA_AstNodeGetTextLength(segment);

        if (index > 0) {
            if ((position >= fileLength) || (file[position] != '/')) {
                matches = MAREA_BOOLEAN_FALSE;
                break;
            }

            position += 1;
        }

        if (((position + SEGMENT_LENGTH) > fileLength) || (memcmp((file + position), MAREA_AstNodeGetText(segment), SEGMENT_LENGTH) != 0)) {
            matches = MAREA_BOOLEAN_FALSE;
            break;
        }

        position += SEGMENT_LENGTH;
    }

    if ((matches == MAREA_BOOLEAN_TRUE) && (position != fileLength)) {
        matches = MAREA_BOOLEAN_FALSE;
    }

    if (matches == MAREA_BOOLEAN_TRUE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *joined = MAREA_NULLPTR;

    MAREA_IntegerSize joinedLength = 0;

    const MAREA_SemaExcept JOIN_RESULT = MAREA_SemaQualifiedNameJoin(context, qualifiedName, &joined, &joinedLength);

    if (JOIN_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
        return JOIN_RESULT;
    }

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_NAMESPACE_PATH_MISMATCH, MAREA_AstNodeGetPosition(qualifiedName), "namespace does not match the file path:", joined, joinedLength);
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_JoinDotted(MAREA_SemaContext *context, const MAREA_Character8 *head, MAREA_IntegerSize headLength, const MAREA_Character8 *tail, MAREA_IntegerSize tailLength, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength)
{
    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(context->arena, (headLength + tailLength + MAREA_INTEGER_SIZE(2)), MAREA_ALIGNOF(MAREA_Character8), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_Character8 *joined = memory;

    memcpy(joined, head, headLength);

    joined[headLength] = '.';

    memcpy((joined + headLength + MAREA_INTEGER_SIZE(1)), tail, tailLength);

    joined[(headLength + tailLength + MAREA_INTEGER_SIZE(1))] = '\0';

    *outText = joined;
    *outLength = (headLength + tailLength + MAREA_INTEGER_SIZE(1));

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

// INFO: a member is one name of the namespace, or the variant of an enum of it (two parts), never a longer path.
static MAREA_Boolean MAREA_CALL MAREA_G_MemberResolves(const MAREA_AstNode *path, MAREA_SemaImportResult namespaceResult, const MAREA_AstNode *namespaceRoot)
{
    const MAREA_IntegerSize SEGMENT_COUNT = MAREA_AstNodeGetChildCount(path);

    if ((SEGMENT_COUNT > MAREA_INTEGER_SIZE(2)) || (namespaceResult == MAREA_SEMA_IMPORT_RESULT_NOT_FOUND)) {
        return MAREA_BOOLEAN_FALSE;
    }

    if (namespaceResult == MAREA_SEMA_IMPORT_RESULT_EXTERNAL) {
        return MAREA_BOOLEAN_TRUE;
    }

    if (namespaceRoot == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *first = MAREA_SemaChildAt(path, 0);

    const MAREA_AstNode *symbol = MAREA_G_FindVisibleSymbol(namespaceRoot, MAREA_AstNodeGetText(first), MAREA_AstNodeGetTextLength(first));

    if (symbol == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    if (SEGMENT_COUNT == MAREA_INTEGER_SIZE(1)) {
        return MAREA_BOOLEAN_TRUE;
    }

    const MAREA_AstNode *variant = MAREA_SemaChildAt(path, 1);

    return ((MAREA_AstNodeGetKind(symbol) == MAREA_AST_KIND_ENUM_DECLARATION) && (MAREA_G_EnumHasVariant(symbol, MAREA_AstNodeGetText(variant), MAREA_AstNodeGetTextLength(variant)) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckImportMember(MAREA_SemaContext *context, const MAREA_AstNode *member, const MAREA_Character8 *namespaceText, MAREA_IntegerSize namespaceLength, MAREA_SemaImportResult namespaceResult, const MAREA_AstNode *namespaceRoot)
{
    const MAREA_AstNode *path = MAREA_SemaChildAt(member, 0);

    if ((path == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(path) != MAREA_AST_KIND_QUALIFIED_NAME) || (MAREA_G_IsPlainName(path) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *last = MAREA_SemaChildAt(path, (MAREA_AstNodeGetChildCount(path) - MAREA_INTEGER_SIZE(1)));

    const MAREA_AstNode *alias = MAREA_NULLPTR;

    if ((MAREA_AstNodeGetFlags(member) & MAREA_AST_FLAG_HAS_ALIAS) != 0) {
        alias = MAREA_SemaChildAt(member, 1);
    }

    const MAREA_AstNode *localName = ((alias != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(alias) == MAREA_AST_KIND_IDENTIFIER)) ? alias : last;

    const MAREA_Character8 *original = MAREA_NULLPTR;

    MAREA_IntegerSize originalLength = 0;

    MAREA_SemaExcept result = MAREA_SemaQualifiedNameJoin(context, path, &original, &originalLength);

    if (result != MAREA_SEMA_EXCEPT_SUCCESS) {
        return result;
    }

    if (MAREA_G_MemberResolves(path, namespaceResult, namespaceRoot) == MAREA_BOOLEAN_FALSE) {
        const MAREA_Character8 *full = MAREA_NULLPTR;

        MAREA_IntegerSize fullLength = 0;

        result = MAREA_G_JoinDotted(context, namespaceText, namespaceLength, original, originalLength, &full, &fullLength);

        if (result == MAREA_SEMA_EXCEPT_SUCCESS) {
            result = MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_UNRESOLVED_IMPORT, MAREA_AstNodeGetPosition(member), "unresolved import:", full, fullLength);
        }

        if (result != MAREA_SEMA_EXCEPT_SUCCESS) {
            return result;
        }
    }

    if (localName == alias) {
        MAREA_Boolean reserved = MAREA_BOOLEAN_FALSE;

        result = MAREA_G_ReportAliasName(context, MAREA_AstNodeGetPosition(alias), MAREA_AstNodeGetText(alias), MAREA_AstNodeGetTextLength(alias), &reserved);

        if ((result != MAREA_SEMA_EXCEPT_SUCCESS) || (reserved == MAREA_BOOLEAN_TRUE)) {
            return result;
        }
    }

    const MAREA_AstNode *first = MAREA_SemaChildAt(path, 0);

    const MAREA_AstNode *enumeration = ((namespaceResult == MAREA_SEMA_IMPORT_RESULT_FOUND) && (MAREA_AstNodeGetChildCount(path) == MAREA_INTEGER_SIZE(1))) ? MAREA_G_ImportedEnum(namespaceRoot, MAREA_AstNodeGetText(first), MAREA_AstNodeGetTextLength(first)) : MAREA_NULLPTR;

    return MAREA_G_BindQualified(context, MAREA_AstNodeGetPosition(localName), MAREA_AstNodeGetText(localName), MAREA_AstNodeGetTextLength(localName), MAREA_SEMA_NAME_KIND_IMPORTED_SYMBOL, enumeration, original, originalLength, namespaceText, namespaceLength);
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckImportMembers(MAREA_SemaContext *context, const MAREA_AstNode *import, const MAREA_AstNode *namespaceName)
{
    const MAREA_Character8 *joined = MAREA_NULLPTR;

    MAREA_IntegerSize joinedLength = 0;

    MAREA_SemaExcept result = MAREA_SemaQualifiedNameJoin(context, namespaceName, &joined, &joinedLength);

    if (result != MAREA_SEMA_EXCEPT_SUCCESS) {
        return result;
    }

    const MAREA_AstNode *foundRoot = MAREA_NULLPTR;

    const MAREA_SemaImportResult NAMESPACE_RESULT = MAREA_G_Resolve(context, joined, joinedLength, &foundRoot);

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(import);

    for (MAREA_IntegerSize index = 1; index < COUNT; ++index) {
        const MAREA_AstNode *member = MAREA_SemaChildAt(import, index);

        if ((member == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(member) != MAREA_AST_KIND_IMPORT_MEMBER)) {
            continue;
        }

        result = MAREA_G_CheckImportMember(context, member, joined, joinedLength, NAMESPACE_RESULT, foundRoot);

        if (result != MAREA_SEMA_EXCEPT_SUCCESS) {
            return result;
        }
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckImport(MAREA_SemaContext *context, const MAREA_AstNode *import)
{
    const MAREA_AstNode *qualifiedName = MAREA_SemaChildAt(import, 0);

    if ((qualifiedName == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(qualifiedName) != MAREA_AST_KIND_QUALIFIED_NAME)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_IntegerSize SEGMENT_COUNT = MAREA_AstNodeGetChildCount(qualifiedName);

    if (MAREA_G_IsPlainName(qualifiedName) == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *last = MAREA_SemaChildAt(qualifiedName, (SEGMENT_COUNT - MAREA_INTEGER_SIZE(1)));

    if ((last == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(last) != MAREA_AST_KIND_IDENTIFIER)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    if ((MAREA_AstNodeGetFlags(import) & MAREA_AST_FLAG_HAS_MEMBERS) != 0) {
        return MAREA_G_CheckImportMembers(context, import, qualifiedName);
    }

    const MAREA_AstNode *alias = MAREA_NULLPTR;

    if ((MAREA_AstNodeGetFlags(import) & MAREA_AST_FLAG_HAS_ALIAS) != 0) {
        alias = MAREA_SemaChildAt(import, 1);
    }

    const MAREA_Boolean HAS_NAMED_ALIAS = ((alias != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(alias) == MAREA_AST_KIND_IDENTIFIER)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    const MAREA_AstNode *localName = (HAS_NAMED_ALIAS == MAREA_BOOLEAN_TRUE) ? alias : last;

    const MAREA_Character8 *joined = MAREA_NULLPTR;

    MAREA_IntegerSize joinedLength = 0;

    const MAREA_SemaExcept JOIN_RESULT = MAREA_SemaQualifiedNameJoin(context, qualifiedName, &joined, &joinedLength);

    if (JOIN_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
        return JOIN_RESULT;
    }

    const MAREA_AstNode *foundRoot = MAREA_NULLPTR;

    MAREA_SemaNameKind kind = MAREA_SEMA_NAME_KIND_NAMESPACE;

    MAREA_Boolean resolved = MAREA_BOOLEAN_TRUE;

    const MAREA_AstNode *enumeration = MAREA_NULLPTR;

    if (MAREA_G_Resolve(context, joined, joinedLength, &foundRoot) == MAREA_SEMA_IMPORT_RESULT_NOT_FOUND) {
        resolved = MAREA_BOOLEAN_FALSE;
        kind = MAREA_SEMA_NAME_KIND_IMPORTED_SYMBOL;

        // INFO: the last segment may name a public symbol of the namespace one level up.
        if (SEGMENT_COUNT > 1) {
            const MAREA_IntegerSize PARENT_LENGTH = (joinedLength - MAREA_AstNodeGetTextLength(last) - MAREA_INTEGER_SIZE(1));

            const MAREA_SemaImportResult PARENT_RESULT = MAREA_G_Resolve(context, joined, PARENT_LENGTH, &foundRoot);

            if (PARENT_RESULT == MAREA_SEMA_IMPORT_RESULT_EXTERNAL) {
                resolved = MAREA_BOOLEAN_TRUE;
            }

            if ((PARENT_RESULT == MAREA_SEMA_IMPORT_RESULT_FOUND) && (foundRoot != MAREA_NULLPTR) && (MAREA_G_FindVisibleSymbol(foundRoot, MAREA_AstNodeGetText(last), MAREA_AstNodeGetTextLength(last)) != MAREA_NULLPTR)) {
                resolved = MAREA_BOOLEAN_TRUE;

                enumeration = MAREA_G_ImportedEnum(foundRoot, MAREA_AstNodeGetText(last), MAREA_AstNodeGetTextLength(last));
            }
        }
    }

    if (resolved == MAREA_BOOLEAN_FALSE) {
        const MAREA_SemaExcept REPORT_RESULT = MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_UNRESOLVED_IMPORT, MAREA_AstNodeGetPosition(import), "unresolved import:", joined, joinedLength);

        if (REPORT_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return REPORT_RESULT;
        }
    }

    // INFO: the namespace of the last segment, empty for a one-segment path.
    const MAREA_IntegerSize QUALIFIER_LENGTH = (SEGMENT_COUNT > 1) ? (joinedLength - MAREA_AstNodeGetTextLength(last) - MAREA_INTEGER_SIZE(1)) : MAREA_INTEGER_SIZE(0);

    const MAREA_Character8 *qualifier = (QUALIFIER_LENGTH > 0) ? joined : MAREA_NULLPTR;

    if (HAS_NAMED_ALIAS == MAREA_BOOLEAN_FALSE) {
        return MAREA_G_BindQualified(context, MAREA_AstNodeGetPosition(localName), MAREA_AstNodeGetText(localName), MAREA_AstNodeGetTextLength(localName), kind, enumeration, MAREA_NULLPTR, MAREA_INTEGER_SIZE(0), qualifier, QUALIFIER_LENGTH);
    }

    MAREA_Boolean reserved = MAREA_BOOLEAN_FALSE;

    const MAREA_SemaExcept RESERVED_RESULT = MAREA_G_ReportAliasName(context, MAREA_AstNodeGetPosition(alias), MAREA_AstNodeGetText(alias), MAREA_AstNodeGetTextLength(alias), &reserved);

    if ((RESERVED_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) || (reserved == MAREA_BOOLEAN_TRUE)) {
        return RESERVED_RESULT;
    }

    // INFO: the alias resolves to the last segment of the path, what the same import binds without "as".
    return MAREA_G_BindQualified(context, MAREA_AstNodeGetPosition(alias), MAREA_AstNodeGetText(alias), MAREA_AstNodeGetTextLength(alias), kind, enumeration, MAREA_AstNodeGetText(last), MAREA_AstNodeGetTextLength(last), qualifier, QUALIFIER_LENGTH);
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckNamespaceAndNames(MAREA_SemaContext *context, const MAREA_AstNode *root)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    if (root == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(root);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *child = MAREA_SemaChildAt(root, index);

        if (child == MAREA_NULLPTR) {
            continue;
        }

        const MAREA_AstKind KIND = MAREA_AstNodeGetKind(child);

        MAREA_SemaExcept result = MAREA_SEMA_EXCEPT_SUCCESS;

        if (KIND == MAREA_AST_KIND_NAMESPACE_DECLARATION) {
            const MAREA_AstNode *qualifiedName = MAREA_SemaChildAt(child, 0);

            if ((qualifiedName != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(qualifiedName) == MAREA_AST_KIND_QUALIFIED_NAME)) {
                result = MAREA_G_CheckNamespacePath(context, root, qualifiedName);
            }
        } else if (KIND == MAREA_AST_KIND_IMPORT_DECLARATION) {
            result = MAREA_G_CheckImport(context, child);
        } else {
            const MAREA_Character8 *name = MAREA_NULLPTR;

            MAREA_IntegerSize nameLength = 0;

            if (MAREA_SemaDeclarationName(child, &name, &nameLength) == MAREA_BOOLEAN_TRUE) {
                // INFO: a type name is an alias too, so it must not hide the reserved namespace c. It is still bound, so its uses resolve.
                if (KIND == MAREA_AST_KIND_TYPE_DECLARATION) {
                    MAREA_Boolean reserved = MAREA_BOOLEAN_FALSE;

                    result = MAREA_G_ReportReserved(context, MAREA_AstNodeGetPosition(child), name, nameLength, &reserved);

                    if (result != MAREA_SEMA_EXCEPT_SUCCESS) {
                        return result;
                    }

                    if (reserved == MAREA_BOOLEAN_TRUE) {
                        result = MAREA_SemaNameTableAdd(context, name, nameLength, MAREA_SEMA_NAME_KIND_DECLARATION, child);

                        if (result != MAREA_SEMA_EXCEPT_SUCCESS) {
                            return result;
                        }

                        continue;
                    }
                }

                // INFO: a declaration named like a built-in macro is reported and still bound, so its uses raise nothing more.
                if (MAREA_SemaIsBuiltInMacroName(name, nameLength) == MAREA_BOOLEAN_TRUE) {
                    result = MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_RESERVED_MACRO_NAME, MAREA_AstNodeGetPosition(child), "name is a built-in macro:", name, nameLength);

                    if (result != MAREA_SEMA_EXCEPT_SUCCESS) {
                        return result;
                    }
                }

                result = MAREA_G_AddName(context, MAREA_AstNodeGetPosition(child), name, nameLength, MAREA_SEMA_NAME_KIND_DECLARATION, child);
            }
        }

        if (result != MAREA_SEMA_EXCEPT_SUCCESS) {
            return result;
        }
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}
