// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAParityHelpers.hpp"

namespace
{
    bool MAREA_generates(MAREA_TypeRegistry *(*registryOf)(MAREA_Arena *))
    {
        MAREA_IrFixture fixture(MAREA_parityLibraries());

        EXPECT_EQ(fixture.MAREA_lower("namespace main; require marea.array; function f(): sinteger32 local a: marea.array.Array<sinteger32, 2> = marea.array.Array<sinteger32, 2>{ 1, 2 }; return a[1]; end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

        MAREA_BytecodeContext *context{ nullptr };

        MAREA_IGNORE(MAREA_BytecodeContextCreate(fixture.sema.parser.lexer.arena, fixture.sema.parser.lexer.sink, &context));

        if (registryOf != nullptr) {
            EXPECT_EQ(MAREA_BytecodeContextSetTypeRegistry(context, registryOf(fixture.sema.parser.lexer.arena)), MAREA_BYTECODE_EXCEPT_SUCCESS) << "The registry is taken.";
        }

        MAREA_BytecodeModule *module{ nullptr };

        return (MAREA_BytecodeGenerate(context, fixture.module, &module) == MAREA_BYTECODE_EXCEPT_SUCCESS) && (module != nullptr);
    }
} // namespace

TEST(MAREABytecodeTypeRegistry, AnArrayIsLoweredOnlyWithTheRegistryThatDescribesIt)
{
    GTEST_COUT << "Validating the lowering reads the shape of an array from the registry of its context." << std::endl;

    EXPECT_FALSE(MAREA_generates(nullptr)) << "With no registry the array type is unknown.";

    EXPECT_TRUE(MAREA_generates([](MAREA_Arena *arena) { return MAREA_buildLibraryRegistry(arena); })) << "With the registry the array is lowered.";
}

TEST(MAREABytecodeTypeRegistry, SetTypeRegistryNeedsAContext)
{
    GTEST_COUT << "Validating a null context is reported." << std::endl;

    EXPECT_EQ(MAREA_BytecodeContextSetTypeRegistry(nullptr, nullptr), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A context is needed.";
}
