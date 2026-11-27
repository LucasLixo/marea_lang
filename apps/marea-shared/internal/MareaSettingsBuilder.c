// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "MareaSettingsBuilder.h"

#include <string.h>

static MAREA_Character8 *MareaSettingsBuilderCopyText(MAREA_Arena *arena, const char *text, MAREA_IntegerSize length)
{
    MAREA_Null memory;

    if (MAREA_ArenaAllocate(arena, length + 1, 1, &memory) != MAREA_ARENA_EXCEPT_SUCCESS)
    {
        return NULL;
    }

    memcpy(memory, text, length);
    ((char *)memory)[length] = '\0';

    return (MAREA_Character8 *)memory;
}

void MareaSettingsBuilderInit(MareaSettingsBuilder *builder, MAREA_Arena *arena)
{
    if (builder == NULL)
    {
        return;
    }

    builder->arena = arena;
    builder->pairs = NULL;
    builder->count = 0;
    builder->capacity = 0;
}

MAREA_Boolean MareaSettingsBuilderAddPair(MareaSettingsBuilder *builder, const MAREA_Character8 *argument)
{
    const char *equals;
    MAREA_IntegerSize nameLength;
    MAREA_IntegerSize valueLength;
    MAREA_Character8 *name;
    MAREA_Character8 *value;

    if (builder == NULL || argument == NULL)
    {
        return MAREA_BOOLEAN_FALSE;
    }

    equals = strchr((const char *)argument, '=');

    if (equals == NULL)
    {
        return MAREA_BOOLEAN_FALSE;
    }

    nameLength = (MAREA_IntegerSize)(equals - (const char *)argument);
    valueLength = (MAREA_IntegerSize)strlen(equals + 1);

    name = MareaSettingsBuilderCopyText(builder->arena, (const char *)argument, nameLength);
    value = MareaSettingsBuilderCopyText(builder->arena, equals + 1, valueLength);

    if (name == NULL || value == NULL)
    {
        return MAREA_BOOLEAN_FALSE;
    }

    if (builder->count == builder->capacity)
    {
        MAREA_IntegerSize newCapacity;
        MAREA_Null memory;

        newCapacity = builder->capacity == 0 ? 4 : builder->capacity * 2;

        if (MAREA_ArenaAllocate(builder->arena, newCapacity * sizeof(MAREA_SemaSetting), MAREA_ALIGNOF(MAREA_SemaSetting), &memory) != MAREA_ARENA_EXCEPT_SUCCESS)
        {
            return MAREA_BOOLEAN_FALSE;
        }

        if (builder->count > 0)
        {
            memcpy(memory, builder->pairs, builder->count * sizeof(MAREA_SemaSetting));
        }

        builder->pairs = (MAREA_SemaSetting *)memory;
        builder->capacity = newCapacity;
    }

    builder->pairs[builder->count].name = name;
    builder->pairs[builder->count].value = value;
    builder->count = builder->count + 1;

    return MAREA_BOOLEAN_TRUE;
}

MAREA_SemaSettings MareaSettingsBuilderBuild(const MareaSettingsBuilder *builder)
{
    MAREA_SemaSettings settings;

    settings.pairs = builder != NULL ? builder->pairs : NULL;
    settings.count = builder != NULL ? builder->count : 0;

    return settings;
}
