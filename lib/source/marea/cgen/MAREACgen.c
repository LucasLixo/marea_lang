// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/cgen/MAREACgenInternal.h"

#include "marea/parse/MAREAAst.h"

#include <string.h>

MAREA_CgenExcept MAREA_CALL MAREA_CgenContextCreate(MAREA_Arena *arena, MAREA_DiagnosticSink *sink, MAREA_CgenContext **outContext)
{
    if (arena == MAREA_NULLPTR) {
        return MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER;
    }

    if (sink == MAREA_NULLPTR) {
        return MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outContext == MAREA_NULLPTR) {
        return MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER;
    }

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(arena, sizeof(MAREA_CgenContext), MAREA_ALIGNOF(MAREA_CgenContext), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_CGEN_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_CgenContext *context = memory;

    memset(context, 0, sizeof(*context));

    context->arena = arena;
    context->sink = sink;

    *outContext = context;

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

MAREA_Arena *MAREA_CALL MAREA_CgenContextGetArena(const MAREA_CgenContext *context)
{
    return (context == MAREA_NULLPTR) ? MAREA_NULLPTR : context->arena;
}

MAREA_DiagnosticSink *MAREA_CALL MAREA_CgenContextGetDiagnostics(const MAREA_CgenContext *context)
{
    return (context == MAREA_NULLPTR) ? MAREA_NULLPTR : context->sink;
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenContextSetTypeRegistry(MAREA_CgenContext *context, const MAREA_TypeRegistry *registry)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER;
    }

    context->registry = registry;

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

const MAREA_TypeRegistry *MAREA_CALL MAREA_CgenContextGetTypeRegistry(const MAREA_CgenContext *context)
{
    return (context == MAREA_NULLPTR) ? MAREA_NULLPTR : context->registry;
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenContextSetHostResolver(MAREA_CgenContext *context, MAREA_CgenHostResolveFunction resolve, MAREA_Null userData)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER;
    }

    context->resolve = resolve;
    context->userData = userData;

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

// INFO: a copy of a name of the caller, NUL terminated, in the arena of the context.
static MAREA_CgenExcept MAREA_CALL MAREA_G_CopyName(const MAREA_CgenContext *context, const MAREA_Character8 *text, MAREA_IntegerSize length, const MAREA_Character8 **outName)
{
    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(context->arena, (length + MAREA_INTEGER_SIZE(1)), MAREA_INTEGER_SIZE(1), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_CGEN_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_Character8 *name = memory;

    memcpy(name, text, length);

    name[length] = '\0';

    *outName = name;

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenContextAddInclude(MAREA_CgenContext *context, const MAREA_Character8 *header, MAREA_IntegerSize length)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER;
    }

    if (header == MAREA_NULLPTR) {
        return MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER;
    }

    if (context->includeCount == context->includeCapacity) {
        const MAREA_IntegerSize CAPACITY = (context->includeCapacity == MAREA_INTEGER_SIZE(0)) ? MAREA_INTEGER_SIZE(4) : (context->includeCapacity * MAREA_INTEGER_SIZE(2));

        MAREA_Null memory = MAREA_NULLPTR;

        if (MAREA_ArenaAllocate(context->arena, (CAPACITY * sizeof(const MAREA_Character8 *)), MAREA_ALIGNOF(const MAREA_Character8 *), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
            return MAREA_CGEN_EXCEPT_ERROR_OUT_OF_MEMORY;
        }

        if (context->includeCount > MAREA_INTEGER_SIZE(0)) {
            memcpy(memory, context->includes, (context->includeCount * sizeof(const MAREA_Character8 *)));
        }

        context->includes = memory;
        context->includeCapacity = CAPACITY;
    }

    MAREA_CGEN_TRY(MAREA_G_CopyName(context, header, length, &context->includes[context->includeCount]));

    context->includeCount = (context->includeCount + MAREA_INTEGER_SIZE(1));

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenContextSetAllocator(MAREA_CgenContext *context, const MAREA_Character8 *allocate, MAREA_IntegerSize allocateLength, const MAREA_Character8 *release, MAREA_IntegerSize releaseLength)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER;
    }

    if ((allocate == MAREA_NULLPTR) != (release == MAREA_NULLPTR)) {
        return MAREA_CGEN_EXCEPT_ERROR_UNPAIRED_ALLOCATOR;
    }

    const MAREA_Character8 *allocateName = MAREA_NULLPTR;
    const MAREA_Character8 *releaseName = MAREA_NULLPTR;

    if (allocate != MAREA_NULLPTR) {
        MAREA_CGEN_TRY(MAREA_G_CopyName(context, allocate, allocateLength, &allocateName));
        MAREA_CGEN_TRY(MAREA_G_CopyName(context, release, releaseLength, &releaseName));
    }

    context->allocate = allocateName;
    context->release = releaseName;

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

// INFO: the names of the destroy operators, which the ir gives only at their calls.
static MAREA_CgenExcept MAREA_CALL MAREA_G_CollectDestroys(MAREA_CgenGenerator *generator, const MAREA_IrNode *node)
{
    if (MAREA_IrNodeGetKind(node) == MAREA_IR_KIND_DESTROY_CALL) {
        MAREA_Null items = generator->destroys;

        MAREA_CGEN_TRY(MAREA_CgenGrow(generator, &items, &generator->destroyCapacity, generator->destroyCount, sizeof(const MAREA_IrNode *), MAREA_ALIGNOF(const MAREA_IrNode *)));

        generator->destroys = items;
        generator->destroys[generator->destroyCount] = node;
        generator->destroyCount = (generator->destroyCount + MAREA_INTEGER_SIZE(1));
    }

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < MAREA_IrNodeGetChildCount(node); index = (index + MAREA_INTEGER_SIZE(1))) {
        MAREA_CGEN_TRY(MAREA_G_CollectDestroys(generator, MAREA_CgenChildAt(node, index)));
    }

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_AddFunction(MAREA_CgenGenerator *generator, const MAREA_IrNode *node)
{
    MAREA_Null items = generator->functions;

    MAREA_CGEN_TRY(MAREA_CgenGrow(generator, &items, &generator->functionCapacity, generator->functionCount, sizeof(MAREA_CgenFunction), MAREA_ALIGNOF(MAREA_CgenFunction)));

    generator->functions = items;

    MAREA_CgenFunction *function = &generator->functions[generator->functionCount];

    memset(function, 0, sizeof(*function));

    function->node = node;
    function->name = MAREA_IrNodeGetText(node);
    function->nameLength = MAREA_IrNodeGetTextLength(node);

    MAREA_IntegerSize dot = MAREA_INTEGER_SIZE(0);

    while ((dot < function->nameLength) && (function->name[dot] != '.')) {
        dot = (dot + MAREA_INTEGER_SIZE(1));
    }

    function->ownerLength = (dot < function->nameLength) ? dot : MAREA_INTEGER_SIZE(0);

    // INFO: the length of the type name keeps "Point.sum" apart from a plain function named "Point_sum".
    if (function->ownerLength > MAREA_INTEGER_SIZE(0)) {
        function->cName = MAREA_CGEN_JOIN(generator, "f_", MAREA_CgenNumber(generator, function->ownerLength), MAREA_CgenMangled(generator, "_", function->name, function->ownerLength), MAREA_CgenMangled(generator, "_", (function->name + dot + MAREA_INTEGER_SIZE(1)), (function->nameLength - dot - MAREA_INTEGER_SIZE(1))));
    } else {
        function->cName = MAREA_CgenMangled(generator, "f_", function->name, function->nameLength);
    }

    generator->functionCount = (generator->functionCount + MAREA_INTEGER_SIZE(1));

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_Collect(MAREA_CgenGenerator *generator)
{
    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < MAREA_IrNodeGetChildCount(generator->module); index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_IrNode *child = MAREA_CgenChildAt(generator->module, index);

        if (MAREA_IrNodeGetKind(child) == MAREA_IR_KIND_FUNCTION_DECLARATION) {
            MAREA_CGEN_TRY(MAREA_G_AddFunction(generator, child));
        } else if (MAREA_IrNodeGetKind(child) == MAREA_IR_KIND_CONSTANT_DECLARATION) {
            MAREA_Null items = generator->constants;

            MAREA_CGEN_TRY(MAREA_CgenGrow(generator, &items, &generator->constantCapacity, generator->constantCount, sizeof(const MAREA_IrNode *), MAREA_ALIGNOF(const MAREA_IrNode *)));

            generator->constants = items;
            generator->constants[generator->constantCount] = child;
            generator->constantCount = (generator->constantCount + MAREA_INTEGER_SIZE(1));
        }
    }

    MAREA_CGEN_TRY(MAREA_G_CollectDestroys(generator, generator->module));

    return MAREA_CgenCollectAliases(generator, generator->module);
}

static void MAREA_CALL MAREA_G_Enter(MAREA_CgenGenerator *generator, MAREA_CgenFunction *function)
{
    generator->current = function;
    generator->selfName = (function->ownerLength > MAREA_INTEGER_SIZE(0)) ? function->name : MAREA_NULLPTR;
    generator->selfLength = function->ownerLength;
    generator->functionFailed = MAREA_BOOLEAN_FALSE;
    generator->symbolCount = MAREA_INTEGER_SIZE(0);
    generator->ownedCount = MAREA_INTEGER_SIZE(0);
    generator->loopMark = MAREA_INTEGER_SIZE(0);
    generator->temporaryCount = MAREA_UINTEGER_32(0);
    generator->constantDepth = MAREA_UINTEGER_32(0);
}

// INFO: a structure is a borrow, written with mutable or read only. An array or a vector is always passed by its address, since the ir sets no flag for a write borrow of one and a read-only borrow is never written.
static MAREA_Uinteger8 MAREA_CALL MAREA_G_PassingOf(const MAREA_CgenGenerator *generator, const MAREA_IrNode *parameter, MAREA_Uinteger32 type)
{
    const MAREA_Uinteger8 CATEGORY = MAREA_CgenCategoryOf(generator, type);

    // INFO: a value that holds storage passes by its address too, so a callee that takes it over leaves zeros in the value of the caller.
    if ((CATEGORY == MAREA_CGEN_CATEGORY_ARRAY) || (MAREA_CgenNeedsDrop(generator, type) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_CGEN_PASSING_MUTABLE_BORROW;
    }

    if (CATEGORY != MAREA_CGEN_CATEGORY_STRUCTURE) {
        return MAREA_CGEN_PASSING_VALUE;
    }

    return ((MAREA_IrNodeGetFlags(parameter) & (MAREA_Uinteger32)MAREA_AST_FLAG_MUTABLE) != MAREA_UINTEGER_32(0)) ? MAREA_CGEN_PASSING_MUTABLE_BORROW : MAREA_CGEN_PASSING_BORROW;
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_ReadSignature(MAREA_CgenGenerator *generator, MAREA_CgenFunction *function)
{
    MAREA_G_Enter(generator, function);

    MAREA_Boolean supported = MAREA_BOOLEAN_TRUE;

    function->result = MAREA_CGEN_KIND_VOID;

    if ((MAREA_IrNodeGetResolvedType(function->node) != MAREA_NULLPTR) || (MAREA_IrNodeGetTypeName(function->node) != MAREA_NULLPTR)) {
        MAREA_CGEN_TRY(MAREA_CgenTypeOfNode(generator, function->node, &function->result));

        supported = ((function->result == MAREA_CGEN_KIND_VOID) || (MAREA_CgenIsValueType(generator, function->result) == MAREA_BOOLEAN_TRUE));
    }

    const MAREA_IrNode *list = MAREA_CgenChildAt(function->node, MAREA_INTEGER_SIZE(0));

    const MAREA_IntegerSize COUNT = (list == MAREA_NULLPTR) ? MAREA_INTEGER_SIZE(0) : MAREA_IrNodeGetChildCount(list);

    MAREA_Null memory = MAREA_NULLPTR;

    MAREA_CGEN_TRY(MAREA_CgenAllocate(generator, (COUNT * sizeof(MAREA_CgenParameter)), MAREA_ALIGNOF(MAREA_CgenParameter), &memory));

    function->parameters = memory;
    function->parameterCount = COUNT;

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_IrNode *node = MAREA_CgenChildAt(list, index);

        MAREA_CgenParameter *parameter = &function->parameters[index];

        parameter->node = node;
        parameter->cName = MAREA_CgenLocalName(generator, MAREA_IrNodeGetText(node), MAREA_IrNodeGetTextLength(node));

        MAREA_CGEN_TRY(MAREA_CgenTypeOfNode(generator, node, &parameter->type));

        if (MAREA_CgenIsValueType(generator, parameter->type) == MAREA_BOOLEAN_FALSE) {
            supported = MAREA_BOOLEAN_FALSE;
        }

        parameter->passing = MAREA_G_PassingOf(generator, node, parameter->type);
    }

    function->supported = supported;

    if (supported == MAREA_BOOLEAN_FALSE) {
        return MAREA_CgenGeneratorFail(generator, function->node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A function has a type that cgen does not write.");
    }

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

static void MAREA_CALL MAREA_G_WriteSignature(const MAREA_CgenGenerator *generator, MAREA_CgenBuffer *buffer, const MAREA_CgenFunction *function)
{
    MAREA_CGEN_WRITE(buffer, MAREA_CgenTypeName(generator, function->result), " ", function->cName, "(");

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < function->parameterCount; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_CgenParameter *parameter = &function->parameters[index];

        const MAREA_Character8 *form = (parameter->passing == MAREA_CGEN_PASSING_VALUE) ? " " : " *";

        MAREA_CGEN_WRITE(buffer, ((parameter->passing == MAREA_CGEN_PASSING_BORROW) ? "const " : ""), MAREA_CgenTypeName(generator, parameter->type), form, parameter->cName, ", ");
    }

    MAREA_CgenWriteText(buffer, "marea_rt_trap *marea_trap)");
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_PushParameters(MAREA_CgenGenerator *generator, const MAREA_CgenFunction *function)
{
    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < function->parameterCount; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_CgenParameter *parameter = &function->parameters[index];

        MAREA_CGEN_WRITE(generator->buffer, "(void)", parameter->cName, ";\n");

        const MAREA_Boolean BORROWED = (parameter->passing != MAREA_CGEN_PASSING_VALUE);

        const MAREA_Character8 *text = (BORROWED == MAREA_BOOLEAN_TRUE) ? MAREA_CGEN_JOIN(generator, "(*", parameter->cName, ")") : parameter->cName;

        const MAREA_Boolean STABLE = ((BORROWED == MAREA_BOOLEAN_FALSE) && (MAREA_CgenIsAggregate(generator, parameter->type) == MAREA_BOOLEAN_FALSE));

        MAREA_CGEN_TRY(MAREA_CgenPushSymbol(generator, MAREA_IrNodeGetText(parameter->node), MAREA_IrNodeGetTextLength(parameter->node), text, parameter->type, STABLE, MAREA_BOOLEAN_FALSE));
    }

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

// INFO: a function is written into a buffer of its own, so one that cgen cannot write is left out whole.
static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteFunction(MAREA_CgenGenerator *generator, MAREA_CgenFunction *function)
{
    MAREA_G_Enter(generator, function);

    MAREA_CgenBuffer body = { .arena = generator->arena, .data = MAREA_NULLPTR, .length = MAREA_INTEGER_SIZE(0), .capacity = MAREA_INTEGER_SIZE(0), .failed = MAREA_BOOLEAN_FALSE };

    MAREA_CgenBuffer prelude = { .arena = generator->arena, .data = MAREA_NULLPTR, .length = MAREA_INTEGER_SIZE(0), .capacity = MAREA_INTEGER_SIZE(0), .failed = MAREA_BOOLEAN_FALSE };
    MAREA_CgenBuffer statements = { .arena = generator->arena, .data = MAREA_NULLPTR, .length = MAREA_INTEGER_SIZE(0), .capacity = MAREA_INTEGER_SIZE(0), .failed = MAREA_BOOLEAN_FALSE };

    generator->buffer = &body;
    generator->prelude = &prelude;

    MAREA_G_WriteSignature(generator, &body, function);

    MAREA_CgenWriteText(&body, " {\n(void)marea_trap;\n");

    MAREA_CGEN_TRY(MAREA_G_PushParameters(generator, function));

    const MAREA_IrNode *block = MAREA_CgenChildAt(function->node, MAREA_INTEGER_SIZE(1));

    // INFO: the statements go to a buffer of their own, since they add the declarations that stand before them.
    generator->buffer = &statements;

    if ((block == MAREA_NULLPTR) || (MAREA_IrNodeGetKind(block) != MAREA_IR_KIND_BLOCK)) {
        MAREA_CGEN_TRY(MAREA_CgenGeneratorFail(generator, function->node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A function has no body."));
    } else {
        MAREA_CGEN_TRY(MAREA_CgenWriteBlock(generator, block));
    }

    // INFO: sema asks every path of a function with a result to return, so the vm traps here too.
    if (function->result != MAREA_CGEN_KIND_VOID) {
        MAREA_CgenWriteTrap(generator, "MAREA_RT_TRAP_MISSING_RETURN");
    }

    MAREA_CgenWrite(&body, prelude.data, prelude.length);
    MAREA_CgenWrite(&body, statements.data, statements.length);
    MAREA_CgenWriteText(&body, "}\n");

    if ((prelude.failed == MAREA_BOOLEAN_TRUE) || (statements.failed == MAREA_BOOLEAN_TRUE)) {
        return MAREA_CGEN_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    if (body.failed == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    function->failed = generator->functionFailed;
    function->body = MAREA_CgenFinish(&body);

    generator->buffer = MAREA_NULLPTR;

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsWritten(const MAREA_CgenFunction *function)
{
    return ((function->supported == MAREA_BOOLEAN_TRUE) && (function->failed == MAREA_BOOLEAN_FALSE));
}

// INFO: a caller of a function that is left out is left out too, until no caller of one is left.
static MAREA_CgenExcept MAREA_CALL MAREA_G_LeaveOutCallers(MAREA_CgenGenerator *generator)
{
    MAREA_Boolean changed = MAREA_BOOLEAN_TRUE;

    while (changed == MAREA_BOOLEAN_TRUE) {
        changed = MAREA_BOOLEAN_FALSE;

        for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < generator->functionCount; index = (index + MAREA_INTEGER_SIZE(1))) {
            MAREA_CgenFunction *function = &generator->functions[index];

            for (MAREA_IntegerSize callee = MAREA_INTEGER_SIZE(0); (MAREA_G_IsWritten(function) == MAREA_BOOLEAN_TRUE) && (callee < function->calleeCount); callee = (callee + MAREA_INTEGER_SIZE(1))) {
                if (MAREA_G_IsWritten(&generator->functions[function->callees[callee]]) == MAREA_BOOLEAN_FALSE) {
                    MAREA_G_Enter(generator, function);

                    MAREA_CGEN_TRY(MAREA_CgenGeneratorFail(generator, function->node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A function calls a function that is not written."));

                    function->failed = MAREA_BOOLEAN_TRUE;
                    changed = MAREA_BOOLEAN_TRUE;
                }
            }
        }
    }

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

static void MAREA_CALL MAREA_G_Assemble(const MAREA_CgenGenerator *generator, MAREA_CgenBuffer *buffer)
{
    MAREA_CgenWritePreamble(buffer, generator->usesMath, generator->usesPacked);

    if (generator->usesContext == MAREA_BOOLEAN_TRUE) {
        MAREA_CgenWriteText(buffer, "#include \"marea/base/MAREAHostContext.h\"\n");
    }

    if (generator->usesVector == MAREA_BOOLEAN_TRUE) {
        MAREA_CgenWriteText(buffer, "#include \"marea/base/MAREAVectorRuntime.h\"\n");
    }

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < generator->context->includeCount; index = (index + MAREA_INTEGER_SIZE(1))) {
        MAREA_CGEN_WRITE(buffer, "#include \"", generator->context->includes[index], "\"\n");
    }

    MAREA_CgenWrite(buffer, generator->types->data, generator->types->length);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < generator->functionCount; index = (index + MAREA_INTEGER_SIZE(1))) {
        if (MAREA_G_IsWritten(&generator->functions[index]) == MAREA_BOOLEAN_TRUE) {
            MAREA_G_WriteSignature(generator, buffer, &generator->functions[index]);
            MAREA_CgenWriteText(buffer, ";\n");
        }
    }

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < generator->functionCount; index = (index + MAREA_INTEGER_SIZE(1))) {
        if (MAREA_G_IsWritten(&generator->functions[index]) == MAREA_BOOLEAN_TRUE) {
            MAREA_CgenWriteText(buffer, generator->functions[index].body);
        }
    }
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenGenerateModule(MAREA_CgenContext *context, const MAREA_IrNode *module, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER;
    }

    if (module == MAREA_NULLPTR) {
        return MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outText == MAREA_NULLPTR) {
        return MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outLength == MAREA_NULLPTR) {
        return MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER;
    }

    if (MAREA_IrNodeGetKind(module) != MAREA_IR_KIND_MODULE) {
        return MAREA_CGEN_EXCEPT_ERROR_BAD_ROOT;
    }

    MAREA_CgenBuffer buffer = { .arena = context->arena, .data = MAREA_NULLPTR, .length = MAREA_INTEGER_SIZE(0), .capacity = MAREA_INTEGER_SIZE(0), .failed = MAREA_BOOLEAN_FALSE };
    MAREA_CgenBuffer types = { .arena = context->arena, .data = MAREA_NULLPTR, .length = MAREA_INTEGER_SIZE(0), .capacity = MAREA_INTEGER_SIZE(0), .failed = MAREA_BOOLEAN_FALSE };

    // INFO: the generator is too large for a call stack, so it lives in the arena of the context.
    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(context->arena, sizeof(MAREA_CgenGenerator), MAREA_ALIGNOF(MAREA_CgenGenerator), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_CGEN_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_CgenGenerator *generator = memory;

    memset(generator, 0, sizeof(*generator));

    generator->context = context;
    generator->arena = context->arena;
    generator->sink = context->sink;
    generator->module = module;
    generator->namespaceText = MAREA_IrNodeGetText(module);
    generator->namespaceLength = MAREA_IrNodeGetTextLength(module);
    generator->types = &types;

    MAREA_CGEN_TRY(MAREA_G_Collect(generator));
    MAREA_CGEN_TRY(MAREA_CgenDeclareTypes(generator));

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < generator->functionCount; index = (index + MAREA_INTEGER_SIZE(1))) {
        MAREA_CGEN_TRY(MAREA_G_ReadSignature(generator, &generator->functions[index]));
    }

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < generator->functionCount; index = (index + MAREA_INTEGER_SIZE(1))) {
        if (generator->functions[index].supported == MAREA_BOOLEAN_TRUE) {
            MAREA_CGEN_TRY(MAREA_G_WriteFunction(generator, &generator->functions[index]));
        }
    }

    MAREA_CGEN_TRY(MAREA_G_LeaveOutCallers(generator));

    MAREA_G_Assemble(generator, &buffer);

    const MAREA_Character8 *text = MAREA_CgenFinish(&buffer);

    if ((buffer.failed == MAREA_BOOLEAN_TRUE) || (types.failed == MAREA_BOOLEAN_TRUE) || (generator->outOfMemory == MAREA_BOOLEAN_TRUE)) {
        return MAREA_CGEN_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    *outText = text;
    *outLength = buffer.length;

    return MAREA_CGEN_EXCEPT_SUCCESS;
}
