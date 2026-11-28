// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

    constexpr MAREA_Uinteger32 MISMATCH{ MAREA_SEMA_DIAGNOSTIC_CODE_TYPE_MISMATCH };
    constexpr MAREA_Uinteger32 BITWISE{ MAREA_SEMA_DIAGNOSTIC_CODE_BITWISE_OPERAND_NOT_INTEGER };
    constexpr MAREA_Uinteger32 LOGICAL{ MAREA_SEMA_DIAGNOSTIC_CODE_LOGICAL_OPERAND_NOT_BOOLEAN };
    constexpr MAREA_Uinteger32 UNDECLARED{ MAREA_SEMA_DIAGNOSTIC_CODE_UNDECLARED_OPERATOR };

    Codes MAREA_codesOf(const std::string &body)
    {
        MAREA_SemaFixture fixture([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; });

        if (fixture.MAREA_sema("namespace main; " + body) != MAREA_SEMA_EXCEPT_SUCCESS) {
            return { 0 };
        }

        for (MAREA_Uinteger32 code : fixture.parser.MAREA_codes()) {
            EXPECT_GE(code, 3000U) << "The program must lex and parse without a diagnostic: " << body;
        }

        return fixture.MAREA_semaCodes();
    }

    std::string MAREA_function(const std::string &statement)
    {
        return "function f(a: float64, b: float64, i: sinteger32, j: sinteger32, p: boolean, q: boolean, s: string) " + statement + " end";
    }
} // namespace

TEST(MAREASemaOperatorOperand, BitwiseOperatorNeedsIntegers)
{
    GTEST_COUT << "Validating a bitwise operator on a float raises BITWISE_OPERAND_NOT_INTEGER." << std::endl;

    EXPECT_EQ(MAREA_codesOf(MAREA_function("local x: float64 = a & b;")), Codes{ BITWISE }) << "A float64 and must raise 3038.";

    EXPECT_EQ(MAREA_codesOf(MAREA_function("local x: float64 = a | b;")), Codes{ BITWISE }) << "A float64 or must raise 3038.";

    EXPECT_EQ(MAREA_codesOf(MAREA_function("local x: float64 = a ^ b;")), Codes{ BITWISE }) << "A float64 xor must raise 3038.";

    EXPECT_EQ(MAREA_codesOf(MAREA_function("local x: float64 = ~a;")), Codes{ BITWISE }) << "A float64 complement must raise 3038.";

    EXPECT_EQ(MAREA_codesOf(MAREA_function("a &= b;")), Codes{ BITWISE }) << "A compound and on floats must raise 3038.";

    EXPECT_EQ(MAREA_codesOf(MAREA_function("a |= b;")), Codes{ BITWISE }) << "A compound or on floats must raise 3038.";

    EXPECT_EQ(MAREA_codesOf(MAREA_function("a ^= b;")), Codes{ BITWISE }) << "A compound xor on floats must raise 3038.";

    EXPECT_EQ(MAREA_codesOf(MAREA_function("local x: float64 = a & 1.5;")), Codes{ BITWISE }) << "A float literal operand must raise 3038.";

    EXPECT_EQ(MAREA_codesOf(MAREA_function("local x: boolean = p & q;")), Codes{ BITWISE }) << "A boolean operand must raise 3038.";

    EXPECT_EQ(MAREA_codesOf(MAREA_function("local x: string = s & s;")), Codes{ BITWISE }) << "A string operand must raise 3038.";
}

TEST(MAREASemaOperatorOperand, BitwiseOperatorOnIntegersIsSilent)
{
    GTEST_COUT << "Validating a bitwise operator on integers raises nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf(MAREA_function("local x: sinteger32 = i & j; local y: sinteger32 = i | 1; local z: sinteger32 = ~i; i ^= j; i &= 3; i |= j;")).empty()) << "Integer operands must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function f(x: pointer<sinteger32>) local y: sinteger32 = x & 1; end").empty()) << "An operand of an unknown type must raise nothing.";
}

TEST(MAREASemaOperatorOperand, LogicalOperatorNeedsBooleans)
{
    GTEST_COUT << "Validating a logical operator on a non-boolean raises LOGICAL_OPERAND_NOT_BOOLEAN." << std::endl;

    EXPECT_EQ(MAREA_codesOf(MAREA_function("local x: boolean = i and j;")), Codes{ LOGICAL }) << "An integer and must raise 3039.";

    EXPECT_EQ(MAREA_codesOf(MAREA_function("local x: boolean = i or j;")), Codes{ LOGICAL }) << "An integer or must raise 3039.";

    EXPECT_EQ(MAREA_codesOf(MAREA_function("local x: boolean = not i;")), Codes{ LOGICAL }) << "An integer not must raise 3039.";

    EXPECT_EQ(MAREA_codesOf(MAREA_function("local x: boolean = not 1;")), Codes{ LOGICAL }) << "An integer literal must raise 3039.";

    EXPECT_EQ(MAREA_codesOf(MAREA_function("local x: boolean = not \"text\";")), Codes{ LOGICAL }) << "A string literal must raise 3039.";

    EXPECT_EQ(MAREA_codesOf(MAREA_function("local x: boolean = not 1.5;")), Codes{ LOGICAL }) << "A float literal must raise 3039.";
}

TEST(MAREASemaOperatorOperand, LogicalOperatorOnBooleansIsSilent)
{
    GTEST_COUT << "Validating a logical operator on booleans raises nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf(MAREA_function("local x: boolean = p and q; local y: boolean = p or q; local z: boolean = not p; local w: boolean = (i < j) and not (a == b);")).empty()) << "Boolean operands must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(MAREA_function("local x: boolean = p and true;")).empty()) << "A boolean literal must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function f(x: pointer<boolean>, y: pointer<boolean>) local z: boolean = x and y; end").empty()) << "An operand of an unknown type must raise nothing.";
}

TEST(MAREASemaOperatorOperand, TypeMismatchSuppressesTheOperandCode)
{
    GTEST_COUT << "Validating that one node never carries two diagnostics." << std::endl;

    EXPECT_EQ(MAREA_codesOf(MAREA_function("local x: float64 = a & i;")), Codes{ MISMATCH }) << "A float and an integer raise only 3011.";

    EXPECT_EQ(MAREA_codesOf(MAREA_function("local x: boolean = i and p;")), Codes{ MISMATCH }) << "An integer and a boolean raise only 3011.";
}

TEST(MAREASemaOperatorOperand, BitwiseOperatorRejectsACharacter)
{
    GTEST_COUT << "Validating a bitwise operator on a character raises BITWISE_OPERAND_NOT_INTEGER." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f(c: character, d: character) local x: character = c & d; end"), Codes{ BITWISE }) << "A character and must raise 3038.";

    EXPECT_EQ(MAREA_codesOf("function f(c: character) local x: character = ~c; end"), Codes{ BITWISE }) << "A character complement must raise 3038.";
}

TEST(MAREASemaOperatorOperand, ArithmeticOnABooleanOrAStringIsAMismatch)
{
    GTEST_COUT << "Validating arithmetic on a boolean or a string raises TYPE_MISMATCH." << std::endl;

    EXPECT_EQ(MAREA_codesOf(MAREA_function("local x: boolean = p + q;")), Codes{ MISMATCH }) << "A boolean sum must raise 3011.";

    EXPECT_EQ(MAREA_codesOf(MAREA_function("local x: string = s + s;")), Codes{ MISMATCH }) << "A string sum must raise 3011.";

    EXPECT_EQ(MAREA_codesOf(MAREA_function("local x: boolean = p - q;")), Codes{ MISMATCH }) << "A boolean difference must raise 3011.";

    EXPECT_EQ(MAREA_codesOf(MAREA_function("local x: string = s * s;")), Codes{ MISMATCH }) << "A string product must raise 3011.";

    EXPECT_EQ(MAREA_codesOf(MAREA_function("local x: boolean = p / q;")), Codes{ MISMATCH }) << "A boolean quotient must raise 3011.";

    EXPECT_EQ(MAREA_codesOf(MAREA_function("local x: string = s % s;")), Codes{ MISMATCH }) << "A string remainder must raise 3011.";

    EXPECT_EQ(MAREA_codesOf(MAREA_function("local x: boolean = p ** q;")), Codes{ MISMATCH }) << "A boolean power must raise 3011.";

    EXPECT_EQ(MAREA_codesOf(MAREA_function("local x: boolean = p +% q;")), Codes{ MISMATCH }) << "A boolean wrapping sum must raise 3011.";

    EXPECT_EQ(MAREA_codesOf(MAREA_function("local x: boolean = -p;")), Codes{ MISMATCH }) << "A negated boolean must raise 3011.";

    EXPECT_EQ(MAREA_codesOf("function f(s: string) s += \"a\"; end"), Codes{ MISMATCH }) << "A compound sum of strings must raise 3011.";
}

TEST(MAREASemaOperatorOperand, ComparisonOfTwoBooleansIsAMismatch)
{
    GTEST_COUT << "Validating a comparison between two booleans raises TYPE_MISMATCH." << std::endl;

    for (const char *operatorText : { "==", "!=", "<", "<=", ">", ">=" }) {
        EXPECT_EQ(MAREA_codesOf(MAREA_function(std::string("local x: boolean = p ") + operatorText + " q;")), Codes{ MISMATCH }) << "A boolean comparison must raise 3011: " << operatorText;
    }

    EXPECT_TRUE(MAREA_codesOf(MAREA_function("local x: boolean = s == s; local y: boolean = i < j;")).empty()) << "A comparison of strings or numbers must raise nothing.";
}

namespace
{
    const std::string POINT{ "structure Point x: float32; end structure Size x: float32; end implementation Point #[operator = \"+\"] function add(self: Self, other: Point): Point return Point { x = self.x }; end #[operator = \"*\"] function scale(self: Self, factor: float32): Point return Point { x = self.x }; end end " };
} // namespace

TEST(MAREASemaOperatorOperand, DeclaredOperatorChecksItsOtherOperand)
{
    GTEST_COUT << "Validating the right operand of a declared operator against the parameter of the function." << std::endl;

    EXPECT_EQ(MAREA_codesOf(POINT + "function f(a: Point) local c: Point = a + 1; end"), Codes{ MISMATCH }) << "A literal for a structure operand must raise 3011.";

    EXPECT_EQ(MAREA_codesOf(POINT + "function f(a: Point, s: Size) local c: Point = a + s; end"), Codes{ MISMATCH }) << "Another structure for a structure operand must raise 3011.";

    EXPECT_EQ(MAREA_codesOf(POINT + "function f(a: Point, s: string) local c: Point = a * s; end"), Codes{ MISMATCH }) << "A string for a float32 operand must raise 3011.";

    EXPECT_EQ(MAREA_codesOf(POINT + "function f(a: Point, b: Point) local c: Point = a * b; end"), Codes{ MISMATCH }) << "A structure for a float32 operand must raise 3011.";

    EXPECT_TRUE(MAREA_codesOf(POINT + "function f(a: Point, b: Point, k: float32) local c: Point = a + b; local d: Point = a * k; local e: Point = a * 2.0; end").empty()) << "A fitting operand must raise nothing.";

    EXPECT_EQ(MAREA_codesOf(POINT + "function f(a: Point) local c: Point = a - 1; end"), Codes{ UNDECLARED }) << "An operator that no function declares must raise 3091.";
}

TEST(MAREASemaOperatorOperand, UndeclaredOperatorOnAStructureIsRejected)
{
    GTEST_COUT << "Validating that a binary operator with zero declared functions on a structure raises UNDECLARED_OPERATOR." << std::endl;

    EXPECT_EQ(MAREA_codesOf(POINT + "function f(a: Point, b: Point) local c: boolean = a == b; end"), Codes{ UNDECLARED }) << "An undeclared == on a structure must raise 3091.";

    EXPECT_EQ(MAREA_codesOf(POINT + "function f(a: Point, b: Point) local c: Point = a / b; end"), Codes{ UNDECLARED }) << "An undeclared / on a structure must raise 3091.";
}

TEST(MAREASemaOperatorOperand, UndeclaredIndexOnAStructureIsRejected)
{
    GTEST_COUT << "Validating that an index on a structure of the file with no declared [] raises UNDECLARED_OPERATOR." << std::endl;

    EXPECT_EQ(MAREA_codesOf(POINT + "function f(a: Point) local c: float32 = a[0]; end"), Codes{ UNDECLARED }) << "A read through an undeclared [] must raise 3091.";

    EXPECT_EQ(MAREA_codesOf(POINT + "function f(#[semantics = reference_mutable] a: Point) a[0] = 1.0; end"), Codes{ UNDECLARED }) << "A write through an undeclared [] must raise 3091.";

    EXPECT_EQ(MAREA_codesOf(POINT + "structure Holder point: Point; end function f(h: Holder) local c: float32 = h.point[0]; end"), Codes{ UNDECLARED }) << "An index on a field of a structure type must raise 3091.";

    EXPECT_EQ(MAREA_codesOf("structure Own x: float32; end implementation Own function first(self: Self): float32 return self[0]; end end"), Codes{ UNDECLARED }) << "An index on self must raise 3091.";

    EXPECT_TRUE(MAREA_codesOf("structure Row x: float32; end implementation Row #[operator = \"[]\"] function at(self: Self, index: uinteger64): float32 return self.x; end end function f(a: Row) local c: float32 = a[0]; end").empty()) << "A declared [] must raise nothing.";
}

TEST(MAREASemaOperatorOperand, UndeclaredIndexOnAnImportedStructureIsRejected)
{
    GTEST_COUT << "Validating that an index on a structure of a FOUND namespace follows the [] of its own tree." << std::endl;

    MAREA_ParseFixture library;

    ASSERT_EQ(library.MAREA_parse("namespace shapes; structure Bare x: float32; end structure Row x: float32; end implementation Row #[operator = \"[]\"] function at(self: Self, index: uinteger64): float32 return self.x; end end type Plain = Bare; structure Holder bare: Bare; row: Row; plain: Plain; cells: Cells; end type Cells = marea.array.Array<float32, 2>;"), MAREA_PARSE_EXCEPT_SUCCESS) << "The imported tree must parse.";

    const auto codesOf{ [&library](const std::string &body) {
        MAREA_SemaFixture fixture([&library](const std::string &text, const MAREA_AstNode **outRoot) {
            if (text != "shapes") {
                return MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
            }

            *outRoot = library.root;

            return MAREA_SEMA_IMPORT_RESULT_FOUND;
        });

        if (fixture.MAREA_sema("namespace main; require shapes as shapes; " + body) != MAREA_SEMA_EXCEPT_SUCCESS) {
            return Codes{ 0 };
        }

        return fixture.MAREA_semaCodes();
    } };

    EXPECT_EQ(codesOf("function f(a: shapes.Bare) local c: float32 = a[0]; end"), Codes{ UNDECLARED }) << "An imported structure with no [] must raise 3091.";

    EXPECT_EQ(codesOf("function f(h: shapes.Holder) local c: float32 = h.bare[0]; end"), Codes{ UNDECLARED }) << "A field of an imported structure with no [] must raise 3091.";

    EXPECT_TRUE(codesOf("function f(a: shapes.Row, h: shapes.Holder) local c: float32 = a[0]; local d: float32 = h.row[0]; end").empty()) << "An imported structure with a declared [] must raise nothing.";

    EXPECT_TRUE(codesOf("structure Bare x: float32; end implementation Bare #[operator = \"[]\"] function at(self: Self, index: uinteger64): float32 return self.x; end end function f(h: shapes.Holder, own: Bare) local c: float32 = own[0]; end").empty()) << "A structure of the file with a declared [] must raise nothing beside an imported one of the same name.";

    EXPECT_TRUE(codesOf("structure Plain x: float32; end implementation Plain #[operator = \"[]\"] function at(self: Self, index: uinteger64): float32 return self.x; end end function f(h: shapes.Holder) local c: float32 = h.row[0]; end").empty()) << "A name of the file must not stand for a name of the imported tree.";

    EXPECT_EQ(codesOf("structure Plain x: float32; end implementation Plain #[operator = \"[]\"] function at(self: Self, index: uinteger64): float32 return self.x; end end function f(h: shapes.Holder) local c: float32 = h.plain[0]; end"), Codes{ UNDECLARED }) << "An alias of the imported tree must be read in that tree.";

    EXPECT_TRUE(codesOf("structure Cells x: float32; end function f(h: shapes.Holder) local c: float32 = h.cells[0]; end").empty()) << "A library type behind an imported alias must not stand for a structure of the file.";
}

TEST(MAREASemaOperatorOperand, UndeclaredBinaryOperatorOnAnImportedStructureIsRejected)
{
    GTEST_COUT << "Validating that a binary operator on a structure of a FOUND namespace follows the operators of its own tree." << std::endl;

    MAREA_ParseFixture library;

    ASSERT_EQ(library.MAREA_parse("namespace shapes; structure Bare x: float32; end structure Point x: float32; end implementation Point #[operator = \"+\"] function add(self: Self, other: Point): Point return Point { x = self.x }; end end"), MAREA_PARSE_EXCEPT_SUCCESS) << "The imported tree must parse.";

    const auto codesOf{ [&library](const std::string &body) {
        MAREA_SemaFixture fixture([&library](const std::string &text, const MAREA_AstNode **outRoot) {
            if (text != "shapes") {
                return MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
            }

            *outRoot = library.root;

            return MAREA_SEMA_IMPORT_RESULT_FOUND;
        });

        if (fixture.MAREA_sema("namespace main; require shapes as shapes; " + body) != MAREA_SEMA_EXCEPT_SUCCESS) {
            return Codes{ 0 };
        }

        return fixture.MAREA_semaCodes();
    } };

    EXPECT_EQ(codesOf("function f(a: shapes.Bare, b: shapes.Bare) local c: boolean = a == b; end"), Codes{ UNDECLARED }) << "An imported structure with no implementation that declares == must raise 3091.";

    EXPECT_TRUE(codesOf("function f(a: shapes.Point, b: shapes.Point) local c: shapes.Point = a + b; end").empty()) << "An imported structure with a declared + must raise nothing.";
}
