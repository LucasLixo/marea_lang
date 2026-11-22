// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

    constexpr MAREA_Uinteger32 MISMATCH{ MAREA_SEMA_DIAGNOSTIC_CODE_TYPE_MISMATCH };
    constexpr MAREA_Uinteger32 UNDECLARED{ MAREA_SEMA_DIAGNOSTIC_CODE_UNDECLARED_NAME };

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
} // namespace

TEST(MAREASemaExpressions, NameMustHaveTheDeclaredType)
{
    GTEST_COUT << "Validating a name with a declared type is compared with the primitive type that is expected." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f(a: integer32) end function g(s: string) f(s); end"), Codes{ MISMATCH }) << "A string name for an integer parameter must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("function g(s: string) local n: integer32 = s; end"), Codes{ MISMATCH }) << "A string name for an integer local must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("function f(a: integer64) end function g(n: integer32) f(n); end"), Codes{ MISMATCH }) << "A conversion is always written, so integer32 for integer64 must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("function g(a: float64) local n: integer32 = a; end"), Codes{ MISMATCH }) << "A float64 name for an integer32 local must raise TYPE_MISMATCH.";

    EXPECT_TRUE(MAREA_codesOf("function f(a: integer32) end function g(n: integer32) f(n); local m: integer32 = n; end").empty()) << "A name of the same type must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function f(a: pointer<integer32>) end function g(s: string) f(s); end").empty()) << "A non-primitive target must not be guessed.";
}

TEST(MAREASemaExpressions, FieldAndStructureAreTyped)
{
    GTEST_COUT << "Validating a field is typed and a structure is not a primitive." << std::endl;

    const std::string POINT{ "structure Point x: float32; label: string; end " };

    EXPECT_EQ(MAREA_codesOf(POINT + "function g(p: Point) local n: integer32 = p.label; end"), Codes{ MISMATCH }) << "A string field for an integer local must raise TYPE_MISMATCH.";

    EXPECT_TRUE(MAREA_codesOf(POINT + "function g(p: Point) local n: float32 = p.x; local s: string = p.label; end").empty()) << "A field of the right type must raise nothing.";

    EXPECT_EQ(MAREA_codesOf(POINT + "function f(a: integer32) end function g(p: Point) f(p); end"), Codes{ MISMATCH }) << "A structure for a primitive parameter must raise TYPE_MISMATCH.";

    EXPECT_TRUE(MAREA_codesOf("enum Color red; end function f(a: integer32) end function g(c: Color) f(c); end").empty()) << "An enum is not guessed against a primitive.";
}

TEST(MAREASemaExpressions, OperatorsGiveATypedResult)
{
    GTEST_COUT << "Validating unary and binary operators give a result type that is compared with the declared type." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function g(a: integer32, b: integer32) local f: boolean = a + b; end"), Codes{ MISMATCH }) << "An integer sum for a boolean local must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("function g(a: integer32, b: integer32) local n: integer32 = a < b; end"), Codes{ MISMATCH }) << "A comparison for an integer local must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("function g(a: boolean) local s: string = not a; end"), Codes{ MISMATCH }) << "A negation for a string local must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("function g(a: boolean, b: boolean) local n: integer32 = a and b; end"), Codes{ MISMATCH }) << "A logical and for an integer local must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("function g() local f: boolean = 1 + 2; end"), Codes{ MISMATCH }) << "A sum of two number literals for a boolean local must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("function g() local n: integer32 = 1.5 * 2.0; end"), Codes{ MISMATCH }) << "A product of two fractions for an integer local must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("function g(a: unsigned8) local n: boolean = -a; end"), Codes{ MISMATCH }) << "A negated name for a boolean local must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("function g(a: integer32) local n: boolean = (a + 1); end"), Codes{ MISMATCH }) << "A parenthesized sum must keep its type.";

    EXPECT_TRUE(MAREA_codesOf("function g(a: integer32, b: integer32): boolean local sum: integer32 = a + b; local power: integer32 = a ** 2; local less: boolean = a < b; local both: boolean = (a == b) and not less; local half: float64 = 2 * 3.5; return a > b or both; end").empty()) << "Operators with a fitting type must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function g(v: unsigned32) local shifted: unsigned32 = v << 4; local masked: unsigned32 = v & 255; local wrapped: unsigned32 = v +% 200; local inverted: unsigned32 = ~v; end").empty()) << "Shift, bit and wrap operators with a fitting type must raise nothing.";

    EXPECT_EQ(MAREA_codesOf("function g(a: integer32, b: float64) local n: float64 = a + b; end"), Codes{ MISMATCH }) << "A sum of two different primitives must raise one TYPE_MISMATCH and no second one for the local.";
}

TEST(MAREASemaExpressions, BinaryOperandsMustHaveTheSameType)
{
    GTEST_COUT << "Validating the two operands of a binary operator are compared by exact primitive name." << std::endl;

    const std::string HEAD{ "function g(a: integer32, b: float64, c: integer64, p: boolean, s: string) " };

    EXPECT_EQ(MAREA_codesOf(HEAD + "a + b; end"), Codes{ MISMATCH }) << "An integer32 plus a float64 must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "a - c; a * c; a / c; a % c; end"), (Codes{ MISMATCH, MISMATCH, MISMATCH, MISMATCH })) << "Every arithmetic operator must compare its operands.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "a +% c; a -% c; a *% c; a +| c; a -| c; a *| c; end"), (Codes{ MISMATCH, MISMATCH, MISMATCH, MISMATCH, MISMATCH, MISMATCH })) << "Every wrapping and saturating operator must compare its operands.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "a == b; a != b; a < b; a <= b; a > b; a >= b; end"), (Codes{ MISMATCH, MISMATCH, MISMATCH, MISMATCH, MISMATCH, MISMATCH })) << "Every comparison operator must compare its operands.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "a & c; a | c; a ^ c; end"), (Codes{ MISMATCH, MISMATCH, MISMATCH })) << "Every bitwise operator must compare its operands.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "p and a; a or p; end"), (Codes{ MISMATCH, MISMATCH })) << "A logical operator with a boolean and an integer must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "s + a; end"), Codes{ MISMATCH }) << "A string plus an integer must raise TYPE_MISMATCH.";

    EXPECT_TRUE(MAREA_codesOf(HEAD + "a + a; b * b; c - c; p and p; s == s; a < a; a & a; end").empty()) << "Two operands of the same primitive must raise nothing.";
}

TEST(MAREASemaExpressions, BinaryOperandWithALiteralFitsLikeADeclaredType)
{
    GTEST_COUT << "Validating a number literal is compared with the other operand as it is compared with a declared type." << std::endl;

    const std::string HEAD{ "function g(a: integer32, b: float64, p: boolean, s: string) " };

    EXPECT_EQ(MAREA_codesOf(HEAD + "a + 1.5; end"), Codes{ MISMATCH }) << "A fraction literal does not fit an integer32 operand.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "s + 1; end"), Codes{ MISMATCH }) << "A number literal does not fit a string operand.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "p == 1; 2 < s; end"), (Codes{ MISMATCH, MISMATCH })) << "A number literal does not fit a boolean or a string operand, on either side.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "a == true; \"x\" == b; end"), (Codes{ MISMATCH, MISMATCH })) << "A boolean or a string literal does not fit a number operand.";

    EXPECT_TRUE(MAREA_codesOf(HEAD + "a + 1; 1 + a; b * 2.5; b + 2; a < 0x10; p == true; s == \"x\"; 1 + 2.5; end").empty()) << "A literal that fits the other operand must raise nothing.";
}

TEST(MAREASemaExpressions, BinaryOperandMismatchIsReportedOnceAndWhereverItStands)
{
    GTEST_COUT << "Validating a mismatch is reported once per binary expression in every checked slot." << std::endl;

    const std::string HEAD{ "function g(a: integer32, b: float64, c: integer32): boolean " };

    EXPECT_EQ(MAREA_codesOf(HEAD + "a + b + c; return true; end"), Codes{ MISMATCH }) << "A mismatch inside a longer chain must be reported once, with no cascade.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "if a < b then end return true; end"), Codes{ MISMATCH }) << "A condition of an if statement must be checked.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "while a < b do end return true; end"), Codes{ MISMATCH }) << "A condition of a while statement must be checked.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "return a < b; end"), Codes{ MISMATCH }) << "A returned value must be checked once, with no second report for the result type.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "local n: boolean = (a == b); return n; end"), Codes{ MISMATCH }) << "A parenthesized operand pair must be checked.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "local n: float64 = 1.0; n = n + a; return true; end"), Codes{ MISMATCH }) << "The value of an assignment must be checked.";

    EXPECT_EQ(MAREA_codesOf("function f(x: float64) end function g(a: integer32, b: float64) f(a * b); end"), Codes{ MISMATCH }) << "An argument of a call must be checked.";

    EXPECT_EQ(MAREA_codesOf("constant A: integer32 = 1; constant B: float64 = 1.5; constant C: float64 = A * B;"), Codes{ MISMATCH }) << "The initializer of a constant must be checked.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "(a + b) * (a + b); return true; end"), (Codes{ MISMATCH, MISMATCH })) << "Two separate mismatches must each be reported.";
}

TEST(MAREASemaExpressions, BinaryOperandOfAnUnknownTypeIsAccepted)
{
    GTEST_COUT << "Validating an operand whose type sema does not know is never guessed and the shift and power operators are left as they are." << std::endl;

    const std::string POINT{ "structure Point x: float32; end enum Color red; end " };

    EXPECT_TRUE(MAREA_codesOf(POINT + "function g(p: Point, c: Color, a: integer32, q: pointer<integer32>) p == a; c == a; c + 1; q + a; a + q; end").empty()) << "A structure, an enum and a pointer must not be compared with a primitive.";

    EXPECT_TRUE(MAREA_codesOf("function same<Item>(a: Item): Item return copy a; end function g(a: integer32, b: float64) same(a) + b; same(b) + a; end").empty()) << "The result of a generic call must not be guessed.";

    EXPECT_TRUE(MAREA_codesOf("function g(a: float64) for i in 0 .. 3 do i + a; end end").empty()) << "A binder with no known type must not be guessed.";

    EXPECT_TRUE(MAREA_codesOf("function g(v: unsigned32, n: integer32, f: float64) v << n; v >> n; n ** f; f ** n; n ** 2.0; end").empty()) << "The shift and the power operators must keep the type of their right side free.";
}

TEST(MAREASemaExpressions, CallResultIsTypedForAKnownFunction)
{
    GTEST_COUT << "Validating a call of a non-generic function of the file has its declared result type." << std::endl;

    const std::string NAME{ "function name(): string return \"a\"; end " };

    EXPECT_EQ(MAREA_codesOf(NAME + "function g() local n: integer32 = name(); end"), Codes{ MISMATCH }) << "A string call for an integer local must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(NAME + "function f(a: boolean) end function g() f(name()); end"), Codes{ MISMATCH }) << "A string call for a boolean parameter must raise TYPE_MISMATCH.";

    EXPECT_TRUE(MAREA_codesOf(NAME + "function g() local s: string = name(); end").empty()) << "A call of the right type must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function same<Item>(a: Item): Item return copy a; end function g() local n: integer32 = same(4); end").empty()) << "A generic call must not be guessed.";

    EXPECT_TRUE(MAREA_codesOf("function none() end function g() local n: integer32 = none(); end").empty()) << "A call with no result type must not be guessed.";
}

TEST(MAREASemaExpressions, AssignmentAndReturnAreChecked)
{
    GTEST_COUT << "Validating a plain assignment and a return value are compared with the declared type." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function g(n: integer32, s: string) n = s; end"), Codes{ MISMATCH }) << "A string for an integer target must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("function g(n: integer32) n = true; end"), Codes{ MISMATCH }) << "A boolean literal for an integer target must raise TYPE_MISMATCH.";

    EXPECT_TRUE(MAREA_codesOf("function g(n: integer32, m: integer32) n = m + 1; n += 2; n <<= 1; end").empty()) << "A fitting assignment must raise nothing.";

    EXPECT_EQ(MAREA_codesOf("function g(a: integer32): boolean return a + 1; end"), Codes{ MISMATCH }) << "An integer for a boolean result must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("function g(): integer32 return \"text\"; end"), Codes{ MISMATCH }) << "A string literal for an integer result must raise TYPE_MISMATCH.";

    EXPECT_TRUE(MAREA_codesOf("function g(a: integer32): integer32 return a + 1; end function h(): float64 return 1; end").empty()) << "A fitting result must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function g(a: integer32) return; end").empty()) << "A return with no value must raise nothing.";
}

TEST(MAREASemaExpressions, ConstantInitializerIsChecked)
{
    GTEST_COUT << "Validating the initializer of a constant is compared with the declared type." << std::endl;

    EXPECT_EQ(MAREA_codesOf("constant LIMIT: integer32 = \"text\";"), Codes{ MISMATCH }) << "A string for an integer constant must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("constant A: float64 = 1.5; constant B: string = A;"), Codes{ MISMATCH }) << "A constant of another type must raise TYPE_MISMATCH.";

    EXPECT_TRUE(MAREA_codesOf("constant A: float64 = 1; constant B: float64 = A * 2.0; constant C: unsigned64 = size_of<integer32>() * 2; constant D: boolean = B > 1.0;").empty()) << "A fitting constant must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("constant B: integer32 = LATER + 1; constant LATER: integer32 = 4;").empty()) << "A name of a later declaration in a constant must raise nothing.";
}

TEST(MAREASemaExpressions, BinderShadowsAnOuterLocal)
{
    GTEST_COUT << "Validating a loop, with or partition name hides an outer local of the same name." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function g() local i: string = \"a\"; for i in 0 .. 3 do local n: integer32 = i; end end").empty()) << "A for name must hide the outer local.";

    EXPECT_TRUE(MAREA_codesOf("function g(items: array<integer32>) local item: string = \"a\"; for each item in items do local n: integer32 = item; end end").empty()) << "A for each name must hide the outer local.";

    EXPECT_EQ(MAREA_codesOf("function g() for i in 0 .. 3 do end local n: integer32 = i; end"), Codes{ UNDECLARED }) << "A for name must end with its loop.";
}

TEST(MAREASemaExpressions, UndeclaredNameInAnExpression)
{
    GTEST_COUT << "Validating a name that nothing declares is UNDECLARED_NAME." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function g() local x: integer32 = missing; end"), Codes{ UNDECLARED }) << "A missing name in a local initializer must raise UNDECLARED_NAME.";

    EXPECT_EQ(MAREA_codesOf("function g() missing(1); end"), Codes{ UNDECLARED }) << "A missing callee must raise UNDECLARED_NAME.";

    EXPECT_EQ(MAREA_codesOf("function g(a: integer32): integer32 return a + missing; end"), Codes{ UNDECLARED }) << "A missing name in an operator must raise UNDECLARED_NAME once.";

    EXPECT_EQ(MAREA_codesOf("function g(a: integer32) if missing then end while a < missing do end end"), (Codes{ UNDECLARED, UNDECLARED })) << "A missing name in a condition must raise UNDECLARED_NAME.";

    EXPECT_EQ(MAREA_codesOf("function f(a: integer32) end function g() f(missing); end"), Codes{ UNDECLARED }) << "A missing name in an argument must raise UNDECLARED_NAME.";

    EXPECT_EQ(MAREA_codesOf("function g(a: integer32) a = missing; end"), Codes{ UNDECLARED }) << "A missing name in an assignment must raise UNDECLARED_NAME.";

    EXPECT_EQ(MAREA_codesOf("function g(a: integer32) switch missing case 1 then end end"), Codes{ UNDECLARED }) << "A missing name as the switch value must raise UNDECLARED_NAME.";

    EXPECT_EQ(MAREA_codesOf("function g() local x: integer32 = x; end"), Codes{ UNDECLARED }) << "A local is not in scope in its own initializer.";

    EXPECT_EQ(MAREA_codesOf("function g() local x: integer32 = (missing).field; end"), Codes{ UNDECLARED }) << "A value that starts a member access and is not a bare name must be checked.";

    EXPECT_EQ(MAREA_codesOf("function f(a: integer32): integer32 return a; end function g() local x: integer32 = f(missing).field; end"), Codes{ UNDECLARED }) << "An argument below a member access must be checked.";
}

TEST(MAREASemaExpressions, KnownNamesAreNotReported)
{
    GTEST_COUT << "Validating every name that something declares raises nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("constant LIMIT: integer32 = 9; function twice(a: integer32): integer32 return a * 2; end function g(p: integer32): integer32 local q: integer32 = twice(p) + LIMIT; return q; end").empty()) << "A parameter, a local, a constant and a function must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("enum Color red; green; end function g(c: Color): integer32 switch c case red then return 1; case Color.green then return 2; end end").empty()) << "A bare variant in a case must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function g(values: array<float32>): float32 local total: float32 = 0.0; for each value in values do total = total + value; end for i in 0 .. 3 do total = total + 1.0; end return total; end").empty()) << "A for and a for each name must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function work(a: array<float32>, k: float32) end function g(values: array<float32>, k: float32) fork_join partition values into 4 as part do spawn work(part, k); end end end").empty()) << "A partition part must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function open(): string return \"a\"; end function g() with open() as file do local n: string = file; end end").empty()) << "A with name must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function same<Item>(a: Item): Item return copy a; end function g<Item>(a: Item) local b: integer32 = Item.zero(); end").empty()) << "A generic parameter must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("structure Point x: float32; end function g(): Point return Point{ x = 1.0 }; end").empty()) << "A field key of a literal is not an expression name.";

    EXPECT_TRUE(MAREA_codesOf("function g(a: integer32) c.stdio.puts(\"x\"); local b: integer32 = convert<integer32>(a); local s: unsigned64 = size_of<integer32>() + align_of<integer32>(); end").empty()) << "The reserved namespace c and the built-in calls must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function g() stdio.puts(\"x\"); local x: integer32 = missing.field; end").empty()) << "The bare name that starts a member chain may be a namespace and is not checked.";

    EXPECT_TRUE(MAREA_codesOf("structure Point x: float32; end function g(p: Point): float32 return p.missing_field; end").empty()) << "A member name is not a bare name.";
}

TEST(MAREASemaExpressions, ImportedAndNamespaceNamesAreNotReported)
{
    GTEST_COUT << "Validating an import alias and the first segment of the own namespace raise nothing." << std::endl;

    MAREA_SemaFixture fixture{ [](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; } };

    EXPECT_EQ(fixture.MAREA_sema("namespace main; import library.text as text; function g() local x: integer32 = text.size(); end"), MAREA_SEMA_EXCEPT_SUCCESS) << "A program with an import alias must pass the sema run.";

    EXPECT_TRUE(fixture.MAREA_semaCodes().empty()) << "An import alias must raise nothing.";

    MAREA_SemaFixture own{ [](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; } };

    EXPECT_EQ(own.MAREA_sema("namespace main.sub; function helper() end function g() main.sub.helper(); end", "main/sub.marea"), MAREA_SEMA_EXCEPT_SUCCESS) << "A program that names its own namespace must pass the sema run.";

    EXPECT_TRUE(own.MAREA_semaCodes().empty()) << "The first segment of the own namespace must raise nothing.";
}

TEST(MAREASemaExpressions, ConstantOfAnImplementationIsNotReported)
{
    GTEST_COUT << "Validating a constant of an implementation block is a known name inside the block only." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("structure Planet mass: float64; end implementation Planet constant GRAVITY: float64 = 6.0; function pull(self: Planet): float64 return GRAVITY * self.mass; end end").empty()) << "A bare constant of the block must raise nothing.";

    EXPECT_EQ(MAREA_codesOf("structure Planet mass: float64; end implementation Planet constant GRAVITY: float64 = 6.0; end function g(): float64 return GRAVITY; end"), Codes{ UNDECLARED }) << "The constant is not a name outside the block.";
}

TEST(MAREASemaExpressions, ErrorNodeInAnExpressionIsSkipped)
{
    GTEST_COUT << "Validating a broken expression gives no sema diagnostic." << std::endl;

    for (const char *body : { "function g() local x: integer32 = ; end", "function g() local x: integer32 = 1 + ; end", "function g() local x: integer32 = (; end" }) {
        MAREA_SemaFixture fixture;

        EXPECT_EQ(fixture.MAREA_sema(std::string{ "namespace main; " } + body), MAREA_SEMA_EXCEPT_SUCCESS) << "A broken expression must not stop the sema run: " << body;

        for (MAREA_Uinteger32 code : fixture.MAREA_semaCodes()) {
            EXPECT_NE(code, UNDECLARED) << body;
        }
    }
}
