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

    EXPECT_TRUE(MAREA_codesOf("function g(v: unsigned32, n: integer32, q: pointer<integer32>) v << n; v >> n; q << n; n << q; end").empty()) << "A shift with an unknown operand must not be guessed.";
}

TEST(MAREASemaExpressions, ShiftCountIsAnyIntegerAndTheResultIsTheLeftType)
{
    GTEST_COUT << "Validating a shift takes any integer count and gives the type of its left operand." << std::endl;

    const std::string HEAD{ "function g(v: unsigned32, a: integer8, b: unsigned64, f: float64, p: boolean, s: string) " };

    EXPECT_TRUE(MAREA_codesOf(HEAD + "v << a; v >> b; v << 3; v >> 0x4; 1 << a; local r: unsigned32 = v << b; local t: integer8 = a >> v; end").empty()) << "A count of any integer width, or a literal, must raise nothing.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "local r: unsigned64 = v << b; end"), Codes{ MISMATCH }) << "The result must have the type of the left operand, not of the count.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "v << f; v >> 1.5; v << p; v >> s; end"), (Codes{ MISMATCH, MISMATCH, MISMATCH, MISMATCH })) << "A count that is not an integer must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "f << v; p >> a; s << 1; 1.5 << a; end"), (Codes{ MISMATCH, MISMATCH, MISMATCH, MISMATCH })) << "A left operand that is not an integer must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "local r: float64 = f << v; end"), Codes{ MISMATCH }) << "A broken shift must raise once and give no type for the local.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "local r: unsigned32 = v << f; end"), Codes{ MISMATCH }) << "A shift with a float count must raise once whatever the local.";
}

TEST(MAREASemaExpressions, PowerOperandsMustHaveTheSameType)
{
    GTEST_COUT << "Validating the operands of a power follow the rule of every arithmetic operator." << std::endl;

    const std::string HEAD{ "function g(a: integer32, c: integer64, f: float64) " };

    EXPECT_EQ(MAREA_codesOf(HEAD + "a ** f; f ** a; a ** c; end"), (Codes{ MISMATCH, MISMATCH, MISMATCH })) << "A base and an exponent of different primitives must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "local n: float64 = a ** f; end"), Codes{ MISMATCH }) << "An integer base with a float exponent must raise one TYPE_MISMATCH and no second one for the local.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "local n: integer32 = a ** f; end"), Codes{ MISMATCH }) << "An integer base with a float exponent must raise TYPE_MISMATCH whatever the declared type.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "local n: float64 = a ** 2; end"), Codes{ MISMATCH }) << "The result of a power must keep the type of its operands.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "a ** 2.5; end"), Codes{ MISMATCH }) << "A fraction literal exponent does not fit an integer base.";

    EXPECT_TRUE(MAREA_codesOf(HEAD + "local p: integer32 = a ** 2; local q: float64 = f ** 0.5; local r: float64 = f ** 2; local s: integer32 = 2 ** a; local t: integer32 = a ** a; end").empty()) << "A power of the same primitive, or with a fitting literal, must raise nothing.";
}

TEST(MAREASemaExpressions, CompoundAssignmentNeedsTheSameTypeOnBothSides)
{
    GTEST_COUT << "Validating a compound assignment is compared like a = a + b and a compound shift follows the shift rule." << std::endl;

    const std::string HEAD{ "structure Point x: float32; end function g(a: integer32, c: integer64, f: float64, v: unsigned32, p: Point, s: string) " };

    EXPECT_EQ(MAREA_codesOf(HEAD + "a += f; a -= c; f *= a; a /= f; a %= c; a **= f; end"), (Codes{ MISMATCH, MISMATCH, MISMATCH, MISMATCH, MISMATCH, MISMATCH })) << "Every compound operator must compare both sides.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "a &= c; a |= c; a ^= c; s += a; end"), (Codes{ MISMATCH, MISMATCH, MISMATCH, MISMATCH })) << "A bitwise compound operator and a string must compare both sides.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "a += 1.5; f += \"x\"; end"), (Codes{ MISMATCH, MISMATCH })) << "A literal that does not fit the target must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "p.x += a; end"), Codes{ MISMATCH }) << "A field target must be typed.";

    EXPECT_TRUE(MAREA_codesOf(HEAD + "a += a; a -= 1; f *= 2; f += 0.5; a **= 2; f **= 0.5; a &= 0xFF; s += s; p.x += 1.0; end").empty()) << "Both sides of the same type, or a fitting literal, must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(HEAD + "p += a; a += p; end").empty()) << "A structure side must not be compared.";

    EXPECT_TRUE(MAREA_codesOf(HEAD + "v <<= a; v >>= c; v <<= 2; a >>= v; end").empty()) << "A compound shift takes any integer count.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "v <<= f; v >>= 1.5; f <<= v; s >>= 1; end"), (Codes{ MISMATCH, MISMATCH, MISMATCH, MISMATCH })) << "A compound shift needs an integer on both sides.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "a += missing; end"), Codes{ UNDECLARED }) << "A missing name must raise UNDECLARED_NAME and no mismatch.";
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

TEST(MAREASemaExpressions, ConstantOfAnImplementationShadowsATopLevelConstant)
{
    GTEST_COUT << "Validating inside an implementation block the constant of the block wins over a top-level constant of the same name." << std::endl;

    const std::string HEAD{ "structure Planet mass: float64; end structure Moon mass: float64; end constant LIMIT: integer32 = 1; " };

    EXPECT_TRUE(MAREA_codesOf(HEAD + "implementation Planet constant LIMIT: float64 = 6.0; function pull(self: Planet): float64 return LIMIT; end end").empty()) << "The type of the constant of the block must be used inside the block.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "implementation Planet constant LIMIT: float64 = 6.0; function pull(self: Planet): integer32 return LIMIT; end end"), Codes{ MISMATCH }) << "The type of the top-level constant must not be used inside the block.";

    EXPECT_TRUE(MAREA_codesOf(HEAD + "implementation Planet constant LIMIT: float64 = 6.0; end function top(): integer32 return LIMIT; end").empty()) << "The top-level constant must be used outside the block.";

    EXPECT_TRUE(MAREA_codesOf(HEAD + "implementation Planet constant LIMIT: float64 = 6.0; end implementation Moon function push(self: Moon): integer32 return LIMIT; end end").empty()) << "The constant of another block must not shadow the top-level one.";

    EXPECT_TRUE(MAREA_codesOf(HEAD + "implementation Planet constant LIMIT: float64 = 6.0; function pull(self: Planet, LIMIT: integer32): integer32 return LIMIT; end end").empty()) << "A parameter must still hide the constant of the block.";

    EXPECT_EQ(MAREA_codesOf("structure Planet mass: float64; end structure Moon mass: float64; end implementation Planet constant GRAVITY: float64 = 6.0; end implementation Moon function push(self: Moon): float64 return GRAVITY; end end"), Codes{ UNDECLARED }) << "The constant of one block is not a name in another block.";
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

TEST(MAREASemaExpressions, ConvertBetweenNumberPrimitivesIsAccepted)
{
    GTEST_COUT << "Validating convert takes any number primitive to any other number primitive." << std::endl;

    const std::string HEAD{ "function g(a: integer32, b: unsigned8, c: integer64, f: float32, d: float64) " };

    EXPECT_TRUE(MAREA_codesOf(HEAD + "local w: integer64 = convert<integer64>(a); local s: unsigned8 = convert<unsigned8>(c); local x: unsigned64 = convert<unsigned64>(b); local y: integer8 = convert<integer8>(b); end").empty()) << "An integer to an integer of any width and sign must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(HEAD + "local x: float64 = convert<float64>(a); local y: float32 = convert<float32>(b); local z: float32 = convert<float32>(c); end").empty()) << "An integer to a float must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(HEAD + "local x: integer32 = convert<integer32>(f); local y: unsigned64 = convert<unsigned64>(d); end").empty()) << "A float to an integer must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(HEAD + "local x: float64 = convert<float64>(f); local y: float32 = convert<float32>(d); end").empty()) << "A float to a float must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(HEAD + "local x: float64 = convert<float64>(1); local y: integer32 = convert<integer32>(2.5); local z: integer64 = convert<integer64>(a + 1); end").empty()) << "A number literal and a calculation are number operands.";

    EXPECT_TRUE(MAREA_codesOf(HEAD + "local x: integer32 = convert<integer32>(a); local s: string = convert<string>(\"t\"); local b2: boolean = convert<boolean>(true); end").empty()) << "The same type to itself must raise nothing.";
}

TEST(MAREASemaExpressions, ConvertOfAnythingElseIsATypeMismatch)
{
    GTEST_COUT << "Validating convert refuses a string, a boolean and a structure against a number, with TYPE_MISMATCH." << std::endl;

    const std::string HEAD{ "structure Point x: float32; end function g(a: integer32, s: string, t: boolean, p: Point) " };

    EXPECT_EQ(MAREA_codesOf(HEAD + "convert<integer32>(s); end"), Codes{ MISMATCH }) << "A string to a number must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "convert<string>(a); convert<float64>(t); convert<boolean>(a); end"), (Codes{ MISMATCH, MISMATCH, MISMATCH })) << "A number to a string, and a boolean against a number, must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "convert<integer32>(\"1\"); convert<float32>(true); convert<string>(1); convert<string>(1.5); end"), (Codes{ MISMATCH, MISMATCH, MISMATCH, MISMATCH })) << "A literal is typed like a name.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "convert<integer32>(p); end"), Codes{ MISMATCH }) << "A structure to a primitive must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "convert<Point>(a); convert<Point>(s); end"), (Codes{ MISMATCH, MISMATCH })) << "A primitive to a structure must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "local n: integer32 = convert<integer32>(s); end"), Codes{ MISMATCH }) << "A bad conversion must raise once and the result type must still fit the local.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "local n: integer32 = convert<integer32>(convert<string>(a)); end"), (Codes{ MISMATCH, MISMATCH })) << "The inner conversion is checked, and its result type string is still the operand of the outer one.";
}

TEST(MAREASemaExpressions, ConvertResultHasTheTypeOfTheTypeArgument)
{
    GTEST_COUT << "Validating the result of convert is T, so it feeds the other expression checks." << std::endl;

    const std::string HEAD{ "function take(a: integer64) end function g(a: integer32, c: integer64, d: float64) " };

    EXPECT_EQ(MAREA_codesOf(HEAD + "local n: integer32 = convert<integer64>(a); end"), Codes{ MISMATCH }) << "The result integer64 does not fit an integer32 local.";

    EXPECT_TRUE(MAREA_codesOf(HEAD + "local n: integer64 = convert<integer64>(a); take(convert<integer64>(a)); end").empty()) << "The result integer64 fits an integer64 local and parameter.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "take(convert<float64>(a)); end"), Codes{ MISMATCH }) << "The result float64 does not fit an integer64 parameter.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "local n: integer64 = convert<integer32>(c) + c; end"), Codes{ MISMATCH }) << "The result is an operand with its own type.";

    EXPECT_TRUE(MAREA_codesOf(HEAD + "local n: integer64 = convert<integer64>(a) + c; local m: float64 = d * convert<float64>(a); end").empty()) << "An operand of the same type as the result must raise nothing.";

    EXPECT_EQ(MAREA_codesOf("function g(a: integer32): integer64 return convert<float64>(a); end"), Codes{ MISMATCH }) << "A return value is compared with the result type.";

    EXPECT_EQ(MAREA_codesOf("constant LIMIT: integer32 = convert<integer64>(5);"), Codes{ MISMATCH }) << "The initializer of a constant is compared with the result type.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "local n: integer64 = 0; n += convert<integer32>(a); end"), Codes{ MISMATCH }) << "A compound assignment is compared with the result type.";
}

TEST(MAREASemaExpressions, ConvertOfAnUnknownTypeIsAccepted)
{
    GTEST_COUT << "Validating sema never guesses: an unknown operand, or a target it cannot sort, raises nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("enum Color red; end function g(c: Color, a: integer32) local n: integer32 = convert<integer32>(c); local d: Color = convert<Color>(a); end").empty()) << "An enum is not decided.";

    EXPECT_TRUE(MAREA_codesOf("function g<Item>(x: Item, a: integer32) local n: integer32 = convert<integer32>(x); local y: Item = convert<Item>(a); end").empty()) << "A generic parameter is not decided.";

    EXPECT_TRUE(MAREA_codesOf("function g(a: integer32, v: pointer<integer32>) local n: integer32 = convert<integer32>(v); convert<pointer<integer32>>(a); end").empty()) << "A pointer is a type that sema does not sort.";

    EXPECT_TRUE(MAREA_codesOf("function g(a: integer32) local n: integer32 = convert<integer32>(a.missing); local m: integer32 = convert<integer32>(lib.get(a)); end").empty()) << "An operand of unknown type must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function g(a: integer32) local n: integer32 = convert<integer32, integer64>(a); local m: integer32 = convert<integer32>(a, a); local o: integer32 = convert<integer32>(); end").empty()) << "A call with not exactly one type argument and one argument is not checked here.";
}

TEST(MAREASemaExpressions, ConvertNamedByTheUnitIsNotTheBuiltIn)
{
    GTEST_COUT << "Validating a function or a local named convert is a plain call, as the IR reads it." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function convert<Item>(a: string): Item return copy a; end function g(s: string) local n: integer32 = convert<integer32>(s); end").empty()) << "A function named convert in the unit is a user function.";

    EXPECT_TRUE(MAREA_codesOf("function g(s: string, convert: function(text: string): integer32) local n: integer32 = convert<integer32>(s); end").empty()) << "A parameter named convert hides the built-in.";
}

TEST(MAREASemaExpressions, ConvertBetweenStructuresNeedsTheSameLayout)
{
    GTEST_COUT << "Validating a structure converts to a structure only when both have the same fields, size and alignment." << std::endl;

    const std::string STRUCTURES{ "#[layout = c] structure A x: integer32; y: float32; end "
                                  "#[layout = c] structure B left: integer32; right: float32; end "
                                  "#[layout = c] structure Wide x: integer64; y: float32; end "
                                  "#[layout = c] structure Short x: integer32; end "
                                  "#[layout = c] structure Swapped x: float32; y: integer32; end " };

    const std::string HEAD{ STRUCTURES + "function g(a: A, b: B, w: Wide, s: Short, p: Swapped) " };

    EXPECT_TRUE(MAREA_codesOf(HEAD + "local x: B = convert<B>(a); local y: A = convert<A>(b); local z: A = convert<A>(a); end").empty()) << "The same field types in the same order, and the same type to itself, must raise nothing.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "convert<A>(w); end"), Codes{ MISMATCH }) << "A structure of another size must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "convert<A>(s); convert<Short>(a); end"), (Codes{ MISMATCH, MISMATCH })) << "A structure with another field count must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "convert<A>(p); end"), Codes{ MISMATCH }) << "Fields of another type at the same place must raise TYPE_MISMATCH, although the size is the same.";

    EXPECT_EQ(MAREA_codesOf(STRUCTURES + "function take(n: integer32) end function g(a: A) take(convert<B>(a)); end"), Codes{ MISMATCH }) << "The result type is the target structure, which is not a primitive.";
}

TEST(MAREASemaExpressions, ConvertBetweenStructuresComparesTheComputedSizeAndAlignment)
{
    GTEST_COUT << "Validating pack and align change the size and the alignment, and the auto layout is compared by the fields." << std::endl;

    const std::string FIELDS{ "x: unsigned8; y: unsigned32; end " };

    const std::string STRUCTURES{ "#[layout = c] structure Plain " + FIELDS + "#[layout = c, pack = 1] structure Packed " + FIELDS + "#[layout = c, align = 16] structure Aligned " + FIELDS + "#[layout = c] structure Other " + FIELDS + "structure AutoOne " + FIELDS + "structure AutoTwo " + FIELDS + "#[layout = c, pack = 1] structure PackedOther " + FIELDS };

    const std::string HEAD{ STRUCTURES + "function g(a: Plain, b: Packed, c: Aligned, d: Other, e: AutoOne, f: AutoTwo, h: PackedOther) " };

    EXPECT_EQ(MAREA_codesOf(HEAD + "convert<Packed>(a); end"), Codes{ MISMATCH }) << "Pack 1 gives 5 bytes against 8, so it must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "convert<Aligned>(a); convert<Plain>(c); end"), (Codes{ MISMATCH, MISMATCH })) << "Align 16 gives another alignment and size.";

    EXPECT_TRUE(MAREA_codesOf(HEAD + "local x: Other = convert<Other>(a); local y: PackedOther = convert<PackedOther>(b); end").empty()) << "Two structures with the same fields and the same pack and align must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(HEAD + "local x: AutoTwo = convert<AutoTwo>(e); end").empty()) << "Two structures of the default auto layout with the same fields get the same layout, so it is accepted.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "convert<Plain>(e); convert<AutoOne>(a); end"), (Codes{ MISMATCH, MISMATCH })) << "An auto layout against a c layout has no common size, so it must raise TYPE_MISMATCH.";
}

TEST(MAREASemaExpressions, ConvertBetweenStructuresRejectsWhatSemaCannotMeasure)
{
    GTEST_COUT << "Validating a field of a type that sema cannot size makes the conversion a TYPE_MISMATCH, never a silent accept." << std::endl;

    const std::string STRUCTURES{ "#[layout = c] structure Text name: string; end "
                                  "#[layout = c] structure Other label: string; end "
                                  "#[layout = c] structure Number x: integer32; end "
                                  "#[layout = c] structure Handle raw: pointer<integer32>; end "
                                  "#[layout = c] structure Holder<Item> value: Item; end "
                                  "#[layout = c] structure Again<Item> value: Item; end " };

    const std::string HEAD{ STRUCTURES + "function g(t: Text, n: Number, h: Handle, v: Holder<integer32>) " };

    EXPECT_EQ(MAREA_codesOf(HEAD + "convert<Other>(t); convert<Number>(t); end"), (Codes{ MISMATCH, MISMATCH })) << "A string field has no size in sema, so the pair is rejected, whatever the other fields are.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "convert<Number>(h); end"), Codes{ MISMATCH }) << "A pointer field is rejected.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "convert<Number>(v); convert<Again<integer32>>(v); end"), (Codes{ MISMATCH, MISMATCH })) << "A generic structure has no layout in sema, so it is rejected against another type and against another instance.";

    EXPECT_TRUE(MAREA_codesOf(HEAD + "local x: Text = convert<Text>(t); local y: Holder<integer32> = convert<Holder<integer32>>(v); end").empty()) << "A structure to itself needs no size, so it stays accepted.";
}

TEST(MAREASemaExpressions, ConvertBetweenStructuresMeasuresAFieldAlignAndANestedStructure)
{
    GTEST_COUT << "Validating a field align and a nested structure of the c layout are measured and compared." << std::endl;

    const std::string STRUCTURES{ "#[layout = c] structure Number x: integer32; end "
                                  "#[layout = c] structure Count y: integer32; end "
                                  "#[layout = c] structure Wide z: integer64; end "
                                  "#[layout = c] structure Nested inner: Number; end "
                                  "#[layout = c] structure NestedCount other: Count; end "
                                  "#[layout = c] structure NestedWide other: Wide; end "
                                  "structure AutoNumber x: integer32; end "
                                  "#[layout = c] structure OfAuto inner: AutoNumber; end "
                                  "#[layout = c] structure AlignedOne #[align = 16] x: integer32; end "
                                  "#[layout = c] structure AlignedTwo #[align = 16] y: integer32; end "
                                  "#[layout = c] structure AlignedEight #[align = 8] x: integer32; end "
                                  "#[layout = c] structure Plain x: integer32; end " };

    const std::string HEAD{ STRUCTURES + "function g(n: Number, d: Nested, c: NestedCount, w: NestedWide, o: OfAuto, a: AlignedOne, b: AlignedTwo, e: AlignedEight, p: Plain) " };

    EXPECT_TRUE(MAREA_codesOf(HEAD + "local x: NestedCount = convert<NestedCount>(d); local y: Nested = convert<Nested>(c); end").empty()) << "Two nested structures with the same layout must raise nothing.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "convert<Nested>(w); convert<NestedWide>(d); end"), (Codes{ MISMATCH, MISMATCH })) << "A nested structure of another layout must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "convert<Number>(d); convert<Nested>(n); end"), (Codes{ MISMATCH, MISMATCH })) << "A nested structure is not the flat field, so it must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "convert<Nested>(o); convert<OfAuto>(d); end"), (Codes{ MISMATCH, MISMATCH })) << "A nested structure with the auto layout has no size in a c structure, so it is rejected.";

    EXPECT_TRUE(MAREA_codesOf(HEAD + "local x: AlignedTwo = convert<AlignedTwo>(a); end").empty()) << "The same field align must raise nothing.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "convert<Plain>(a); convert<AlignedOne>(p); convert<AlignedEight>(a); end"), (Codes{ MISMATCH, MISMATCH, MISMATCH })) << "Another field align must raise TYPE_MISMATCH.";
}

TEST(MAREASemaExpressions, ConvertBetweenStructuresFollowsAnAliasOfAPrimitive)
{
    GTEST_COUT << "Validating a field whose type is an alias of a primitive counts as that primitive." << std::endl;

    const std::string STRUCTURES{ "type Meters = float32; "
                                  "#[layout = c] structure Length value: Meters; end "
                                  "#[layout = c] structure Raw value: float32; end "
                                  "#[layout = c] structure Count value: integer32; end " };

    const std::string HEAD{ STRUCTURES + "function g(l: Length, r: Raw, c: Count) " };

    EXPECT_TRUE(MAREA_codesOf(HEAD + "local x: Raw = convert<Raw>(l); local y: Length = convert<Length>(r); end").empty()) << "An alias of float32 is float32.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "convert<Count>(l); convert<Length>(c); end"), (Codes{ MISMATCH, MISMATCH })) << "An alias of float32 is not integer32.";
}
