// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_APPS_SHARED_MAREASETTINGSBUILDER_H)
    #define MAREA_APPS_SHARED_MAREASETTINGSBUILDER_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAArena.h"
    #include "marea/sema/MAREASema.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Growing collection of MAREA_SemaSetting pairs built from repeatable --settings NAME=value
     * flags. Every name and value text is copied into the builder's own arena, so the caller's argv
     * strings need not outlive the build.
     */
    typedef struct MareaSettingsBuilder
    {
        MAREA_Arena *arena;
        MAREA_SemaSetting *pairs;
        MAREA_IntegerSize count;
        MAREA_IntegerSize capacity;
    } MareaSettingsBuilder;

    /**
     * @brief Initializes an empty builder.
     * @param builder Builder to initialize. Must not be null.
     * @param arena Arena that owns every copied name/value text and the pair array. Must not be null.
     */
    void MareaSettingsBuilderInit(MareaSettingsBuilder * builder, MAREA_Arena * arena);

    /**
     * @brief Parses one "NAME=value" argument and appends it to the builder.
     * @param builder Builder to append to. Must not be null.
     * @param argument NUL-terminated "NAME=value" text. Must contain '='.
     * @return MAREA_BOOLEAN_TRUE on success, MAREA_BOOLEAN_FALSE when argument has no '=' or growing the
     * arena fails.
     */
    MAREA_Boolean MareaSettingsBuilderAddPair(MareaSettingsBuilder * builder, const MAREA_Character8 *argument);

    /**
     * @brief Returns the settings built so far, as the plain struct MAREA_SemaContextSetSettings takes.
     * @param builder Builder to read. Must not be null.
     * @return The settings, valid for as long as the builder's arena is.
     */
    MAREA_SemaSettings MareaSettingsBuilderBuild(const MareaSettingsBuilder *builder);

MAREA_END_EXTERN_C

#endif /* MAREA_APPS_SHARED_MAREASETTINGSBUILDER_H */
