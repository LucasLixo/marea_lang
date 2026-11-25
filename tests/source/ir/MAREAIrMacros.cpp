// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAIrHelpers.hpp"

namespace
{
    const MAREA_ResolveCallback RESOLVE_EXTERNAL{ [](const std::string &text, const MAREA_AstNode **) {
        return ((text.rfind("marea.", 0) == 0) || (text.rfind("c.", 0) == 0) || (text.rfind("demo.", 0) == 0)) ? MAREA_SEMA_IMPORT_RESULT_EXTERNAL : MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
    } };

    // INFO: the include resolver that the registered macro needs, so the size of logo.bin is 12 bytes.
    MAREA_Boolean MAREA_CALL MAREA_includeSize(MAREA_Null, const MAREA_Character8 *path, MAREA_IntegerSize pathLength, MAREA_IntegerSize *outSize)
    {
        if (std::string(path, pathLength) != "logo.bin") {
            return MAREA_BOOLEAN_FALSE;
        }

        *outSize = 12;

        return MAREA_BOOLEAN_TRUE;
    }

    struct MAREA_MacroFixture
    {
        MAREA_IrFixture fixture{ RESOLVE_EXTERNAL };
        MAREA_SemaIncludeResolver includes{ &MAREA_includeSize, nullptr };

        MAREA_MacroFixture()
        {
            MAREA_IGNORE(MAREA_SemaContextSetIncludeResolver(fixture.sema.context, &includes));
        }

        // INFO: the render of the first statement of the function named check, or a marker.
        std::string MAREA_firstStatement(const std::string &imports, const std::string &body)
        {
            if (fixture.MAREA_lower("namespace main;\n" + imports + "function check()\n" + body + "\nend\n") != MAREA_IR_EXCEPT_SUCCESS) {
                return "<lower failed>";
            }

            const MAREA_IrNode *function{ MAREA_irChild(fixture.module, MAREA_IrNodeGetChildCount(fixture.module) - 1) };

            if ((MAREA_IrNodeGetChildCount(fixture.module) == 0) || (MAREA_irText(function) != "check")) {
                return "<no function>";
            }

            const MAREA_IrNode *block{ MAREA_irChild(function, MAREA_IrNodeGetChildCount(function) - 1) };

            return (MAREA_IrNodeGetChildCount(block) == 0) ? "<empty>" : MAREA_irRender(MAREA_irChild(block, 0));
        }
    };
} // namespace

TEST(MAREAIrMacros, KindHasAStableName)
{
    GTEST_COUT << "Validating the new kind has its stable upper-case name." << std::endl;

    EXPECT_STREQ(MAREA_IrKindGetName(MAREA_IR_KIND_MACRO_CALL), "MACRO_CALL") << "The name must be stable.";

    EXPECT_EQ(static_cast<int>(MAREA_IR_KIND_MACRO_CALL), static_cast<int>(MAREA_IR_KIND_BACKEND_ATTRIBUTE) + 1) << "The kind must come after BACKEND_ATTRIBUTE, so no value moves.";
}

TEST(MAREAIrMacros, BuildValueInALocalLowersToAMacroCall)
{
    GTEST_COUT << "Validating build_value! lowers to a MACRO_CALL that keeps its three arguments." << std::endl;

    MAREA_MacroFixture macro;

    EXPECT_EQ(macro.MAREA_firstStatement("", "local level: sinteger32 = build_value!(\"LEVEL\", sinteger32, 2);"), "(LOCAL_DECLARATION level :sinteger32 (MACRO_CALL build_value (ARGUMENT_LIST (STRING_LITERAL \"LEVEL\") (IDENTIFIER sinteger32) (INTEGER_LITERAL 2))))") << "The call must keep the name, the type and the default.";

    EXPECT_TRUE(macro.fixture.MAREA_irCodes().empty()) << "No IR diagnostic may be raised.";
}

TEST(MAREAIrMacros, BuildValueInAConstantIsTypedByItsDeclaration)
{
    GTEST_COUT << "Validating the MACRO_CALL of a constant initializer is the one child of the constant, and the constant carries the type." << std::endl;

    MAREA_MacroFixture macro;

    ASSERT_EQ(macro.fixture.MAREA_lower("namespace main; constant LEVEL: sinteger32 = build_value!(\"LEVEL\", sinteger32, 2);"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    const MAREA_IrNode *constant{ MAREA_irChild(macro.fixture.module, 0) };

    ASSERT_EQ(MAREA_IrNodeGetChildCount(constant), 1U) << "The constant must keep its initializer.";

    EXPECT_EQ(MAREA_IrNodeGetKind(MAREA_irChild(constant, 0)), MAREA_IR_KIND_MACRO_CALL) << "The initializer must be a MACRO_CALL.";

    ASSERT_NE(MAREA_IrNodeGetResolvedType(constant), nullptr) << "The constant must carry its type, since sema gives none to the call.";

    EXPECT_EQ(std::string(MAREA_IrNodeGetResolvedType(constant), MAREA_IrNodeGetResolvedTypeLength(constant)), "sinteger32") << "The type of the call is the type of its declaration.";
}

TEST(MAREAIrMacros, LibraryMacroThroughAnAliasLowersToTheFullPath)
{
    GTEST_COUT << "Validating a macro of a shipped library called through an alias lowers to a MACRO_CALL named by the full path." << std::endl;

    MAREA_MacroFixture macro;

    EXPECT_EQ(macro.MAREA_firstStatement("require marea.array as array;\n", "local data: marea.array.Array<uinteger8, 12> = array.include_bytes!(\"logo.bin\");"), "(LOCAL_DECLARATION data :marea.array.Array (MACRO_CALL marea.array.include_bytes (ARGUMENT_LIST (STRING_LITERAL \"logo.bin\"))))") << "The alias must be replaced by the namespace it stands for.";
}

TEST(MAREAIrMacros, LibraryMacroThroughTheFullPathLowers)
{
    GTEST_COUT << "Validating a macro of a shipped library called through its full path lowers as it is written." << std::endl;

    MAREA_MacroFixture macro;

    EXPECT_NE(macro.MAREA_firstStatement("require marea.array;\n", "local data: marea.array.Array<uinteger8, 12> = marea.array.include_bytes!(\"logo.bin\");").find("(MACRO_CALL marea.array.include_bytes (ARGUMENT_LIST (STRING_LITERAL \"logo.bin\")))"), std::string::npos) << "The full path must be the name of the call.";
}

TEST(MAREAIrMacros, MacroStatementWithAQualifiedNameIsKept)
{
    GTEST_COUT << "Validating a macro statement with a qualified name is no longer dropped: the function stays and the statement is an expression statement of a MACRO_CALL." << std::endl;

    MAREA_MacroFixture macro;

    EXPECT_EQ(macro.MAREA_firstStatement("require marea.array;\n", "marea.array.include_bytes!(\"logo.bin\");"), "(EXPRESSION_STATEMENT (MACRO_CALL marea.array.include_bytes (ARGUMENT_LIST (STRING_LITERAL \"logo.bin\"))))") << "The statement must be kept.";
}

TEST(MAREAIrMacros, BuildValueAsAStatementIsKept)
{
    GTEST_COUT << "Validating build_value! as a statement is kept as an expression statement." << std::endl;

    MAREA_MacroFixture macro;

    EXPECT_NE(macro.MAREA_firstStatement("", "build_value!(\"LEVEL\", sinteger32, 2);").find("(EXPRESSION_STATEMENT (MACRO_CALL build_value"), std::string::npos) << "The statement must be kept.";
}

TEST(MAREAIrMacros, OtherBuiltinMacrosStillRaise)
{
    GTEST_COUT << "Validating print! is still not lowered and raises 4005, in a statement and in an expression." << std::endl;

    MAREA_MacroFixture macro;

    EXPECT_EQ(macro.MAREA_firstStatement("", "print!(\"hi\");"), "<no function>") << "The function must be dropped.";

    EXPECT_EQ(macro.fixture.MAREA_irCodes(), std::vector<MAREA_Uinteger32>{ MAREA_IR_DIAGNOSTIC_CODE_UNSUPPORTED_MACRO }) << "The statement must raise 4005 once.";

    MAREA_MacroFixture second;

    EXPECT_EQ(second.MAREA_firstStatement("", "local text: string = format!(\"hi\");"), "<no function>") << "The function must be dropped.";

    EXPECT_EQ(second.fixture.MAREA_irCodes(), std::vector<MAREA_Uinteger32>{ MAREA_IR_DIAGNOSTIC_CODE_UNSUPPORTED_MACRO }) << "The expression must raise 4005 once.";
}
