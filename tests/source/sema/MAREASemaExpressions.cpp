// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

    constexpr MAREA_Uinteger32 MISMATCH{ MAREA_SEMA_DIAGNOSTIC_CODE_TYPE_MISMATCH };
    constexpr MAREA_Uinteger32 UNDECLARED{ MAREA_SEMA_DIAGNOSTIC_CODE_UNDECLARED_NAME };
    constexpr MAREA_Uinteger32 COUNT_MISMATCH{ MAREA_SEMA_DIAGNOSTIC_CODE_ARGUMENT_COUNT_MISMATCH };
    constexpr MAREA_Uinteger32 OPAQUE_MISUSE{ MAREA_SEMA_DIAGNOSTIC_CODE_OPAQUE_TYPE_MISUSE };
    constexpr MAREA_Uinteger32 FUNCTION_TYPE_MISMATCH{ MAREA_SEMA_DIAGNOSTIC_CODE_FUNCTION_TYPE_MISMATCH };
    constexpr MAREA_Uinteger32 DEFAULT_BEFORE_REQUIRED{ MAREA_SEMA_DIAGNOSTIC_CODE_DEFAULT_PARAMETER_BEFORE_REQUIRED };

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

    EXPECT_EQ(MAREA_codesOf("function f(a: sinteger32) end function g(s: string) f(s); end"), Codes{ MISMATCH }) << "A string name for an integer parameter must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("function g(s: string) local n: sinteger32 = s; end"), Codes{ MISMATCH }) << "A string name for an integer local must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("function f(a: sinteger64) end function g(n: sinteger32) f(n); end"), Codes{ MISMATCH }) << "A conversion is always written, so sinteger32 for sinteger64 must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("function g(a: float64) local n: sinteger32 = a; end"), Codes{ MISMATCH }) << "A float64 name for an sinteger32 local must raise TYPE_MISMATCH.";

    EXPECT_TRUE(MAREA_codesOf("function f(a: sinteger32) end function g(n: sinteger32) f(n); local m: sinteger32 = n; end").empty()) << "A name of the same type must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function f(a: pointer<sinteger32>) end function g(s: string) f(s); end").empty()) << "A non-primitive target must not be guessed.";
}

TEST(MAREASemaExpressions, FieldAndStructureAreTyped)
{
    GTEST_COUT << "Validating a field is typed and a structure is not a primitive." << std::endl;

    const std::string POINT{ "structure Point x: float32; label: string; end " };

    EXPECT_EQ(MAREA_codesOf(POINT + "function g(p: Point) local n: sinteger32 = p.label; end"), Codes{ MISMATCH }) << "A string field for an integer local must raise TYPE_MISMATCH.";

    EXPECT_TRUE(MAREA_codesOf(POINT + "function g(p: Point) local n: float32 = p.x; local s: string = p.label; end").empty()) << "A field of the right type must raise nothing.";

    EXPECT_EQ(MAREA_codesOf(POINT + "function f(a: sinteger32) end function g(p: Point) f(p); end"), Codes{ MISMATCH }) << "A structure for a primitive parameter must raise TYPE_MISMATCH.";

    EXPECT_TRUE(MAREA_codesOf("enum Color red; end function f(a: sinteger32) end function g(c: Color) f(c); end").empty()) << "An enum is not guessed against a primitive.";
}

TEST(MAREASemaExpressions, OperatorsGiveATypedResult)
{
    GTEST_COUT << "Validating unary and binary operators give a result type that is compared with the declared type." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function g(a: sinteger32, b: sinteger32) local f: boolean = a + b; end"), Codes{ MISMATCH }) << "An integer sum for a boolean local must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("function g(a: sinteger32, b: sinteger32) local n: sinteger32 = a < b; end"), Codes{ MISMATCH }) << "A comparison for an integer local must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("function g(a: boolean) local s: string = not a; end"), Codes{ MISMATCH }) << "A negation for a string local must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("function g(a: boolean, b: boolean) local n: sinteger32 = a and b; end"), Codes{ MISMATCH }) << "A logical and for an integer local must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("function g() local f: boolean = 1 + 2; end"), Codes{ MISMATCH }) << "A sum of two number literals for a boolean local must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("function g() local n: sinteger32 = 1.5 * 2.0; end"), Codes{ MISMATCH }) << "A product of two fractions for an integer local must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("function g(a: sinteger8) local n: boolean = -a; end"), Codes{ MISMATCH }) << "A negated name for a boolean local must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("function g(a: sinteger32) local n: boolean = (a + 1); end"), Codes{ MISMATCH }) << "A parenthesized sum must keep its type.";

    EXPECT_TRUE(MAREA_codesOf("function g(a: sinteger32, b: sinteger32): boolean local sum: sinteger32 = a + b; local power: sinteger32 = a ** 2; local less: boolean = a < b; local both: boolean = (a == b) and not less; local half: float64 = 2 * 3.5; return a > b or both; end").empty()) << "Operators with a fitting type must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function g(v: uinteger32) local shifted: uinteger32 = v << 4; local masked: uinteger32 = v & 255; local wrapped: uinteger32 = v +% 200; local inverted: uinteger32 = ~v; end").empty()) << "Shift, bit and wrap operators with a fitting type must raise nothing.";

    EXPECT_EQ(MAREA_codesOf("function g(a: sinteger32, b: float64) local n: float64 = a + b; end"), Codes{ MISMATCH }) << "A sum of two different primitives must raise one TYPE_MISMATCH and no second one for the local.";
}

TEST(MAREASemaExpressions, BinaryOperandsMustHaveTheSameType)
{
    GTEST_COUT << "Validating the two operands of a binary operator are compared by exact primitive name." << std::endl;

    const std::string HEAD{ "function g(a: sinteger32, b: float64, c: sinteger64, p: boolean, s: string) " };

    EXPECT_EQ(MAREA_codesOf(HEAD + "a + b; end"), Codes{ MISMATCH }) << "An sinteger32 plus a float64 must raise TYPE_MISMATCH.";

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

    const std::string HEAD{ "function g(a: sinteger32, b: float64, p: boolean, s: string) " };

    EXPECT_EQ(MAREA_codesOf(HEAD + "a + 1.5; end"), Codes{ MISMATCH }) << "A fraction literal does not fit an sinteger32 operand.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "s + 1; end"), Codes{ MISMATCH }) << "A number literal does not fit a string operand.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "p == 1; 2 < s; end"), (Codes{ MISMATCH, MISMATCH })) << "A number literal does not fit a boolean or a string operand, on either side.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "a == true; \"x\" == b; end"), (Codes{ MISMATCH, MISMATCH })) << "A boolean or a string literal does not fit a number operand.";

    EXPECT_TRUE(MAREA_codesOf(HEAD + "a + 1; 1 + a; b * 2.5; b + 2; a < 0x10; p == true; s == \"x\"; 1 + 2.5; end").empty()) << "A literal that fits the other operand must raise nothing.";
}

TEST(MAREASemaExpressions, BinaryOperandMismatchIsReportedOnceAndWhereverItStands)
{
    GTEST_COUT << "Validating a mismatch is reported once per binary expression in every checked slot." << std::endl;

    const std::string HEAD{ "function g(a: sinteger32, b: float64, c: sinteger32): boolean " };

    EXPECT_EQ(MAREA_codesOf(HEAD + "a + b + c; return true; end"), Codes{ MISMATCH }) << "A mismatch inside a longer chain must be reported once, with no cascade.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "if a < b then end end return true; end"), Codes{ MISMATCH }) << "A condition of an if statement must be checked.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "while a < b do end return true; end"), Codes{ MISMATCH }) << "A condition of a while statement must be checked.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "return a < b; end"), Codes{ MISMATCH }) << "A returned value must be checked once, with no second report for the result type.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "local n: boolean = (a == b); return n; end"), Codes{ MISMATCH }) << "A parenthesized operand pair must be checked.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "local n: float64 = 1.0; n = n + a; return true; end"), Codes{ MISMATCH }) << "The value of an assignment must be checked.";

    EXPECT_EQ(MAREA_codesOf("function f(x: float64) end function g(a: sinteger32, b: float64) f(a * b); end"), Codes{ MISMATCH }) << "An argument of a call must be checked.";

    EXPECT_EQ(MAREA_codesOf("constant A: sinteger32 = 1; constant B: float64 = 1.5; constant C: float64 = A * B;"), Codes{ MISMATCH }) << "The initializer of a constant must be checked.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "(a + b) * (a + b); return true; end"), (Codes{ MISMATCH, MISMATCH })) << "Two separate mismatches must each be reported.";
}

TEST(MAREASemaExpressions, BinaryOperandOfAnUnknownTypeIsAccepted)
{
    GTEST_COUT << "Validating an operand whose type sema does not know is never guessed and the shift and power operators are left as they are." << std::endl;

    const std::string POINT{ "structure Point x: float32; end enum Color red; end " };

    EXPECT_TRUE(MAREA_codesOf(POINT + "function g(p: Point, c: Color, a: sinteger32, q: pointer<sinteger32>) p == a; c == a; c + 1; q + a; a + q; end").empty()) << "A structure, an enum and a pointer must not be compared with a primitive.";

    EXPECT_TRUE(MAREA_codesOf("function same<Item>(a: Item): Item return copy a; end function g(a: sinteger32, b: float64) same(a) + b; same(b) + a; end").empty()) << "The result of a generic call must not be guessed.";

    EXPECT_TRUE(MAREA_codesOf("function g(a: float64) for i in 0 .. 3 do i + a; end end").empty()) << "A binder with no known type must not be guessed.";

    EXPECT_TRUE(MAREA_codesOf("function g(v: uinteger32, n: sinteger32, q: pointer<sinteger32>) v << n; v >> n; q << n; n << q; end").empty()) << "A shift with an unknown operand must not be guessed.";
}

TEST(MAREASemaExpressions, ShiftCountIsAnyIntegerAndTheResultIsTheLeftType)
{
    GTEST_COUT << "Validating a shift takes any integer count and gives the type of its left operand." << std::endl;

    const std::string HEAD{ "function g(v: uinteger32, a: sinteger8, b: uinteger64, f: float64, p: boolean, s: string) " };

    EXPECT_TRUE(MAREA_codesOf(HEAD + "v << a; v >> b; v << 3; v >> 0x4; 1 << a; local r: uinteger32 = v << b; local t: sinteger8 = a >> v; end").empty()) << "A count of any integer width, or a literal, must raise nothing.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "local r: uinteger64 = v << b; end"), Codes{ MISMATCH }) << "The result must have the type of the left operand, not of the count.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "v << f; v >> 1.5; v << p; v >> s; end"), (Codes{ MISMATCH, MISMATCH, MISMATCH, MISMATCH })) << "A count that is not an integer must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "f << v; p >> a; s << 1; 1.5 << a; end"), (Codes{ MISMATCH, MISMATCH, MISMATCH, MISMATCH })) << "A left operand that is not an integer must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "local r: float64 = f << v; end"), Codes{ MISMATCH }) << "A broken shift must raise once and give no type for the local.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "local r: uinteger32 = v << f; end"), Codes{ MISMATCH }) << "A shift with a float count must raise once whatever the local.";
}

TEST(MAREASemaExpressions, PowerOperandsMustHaveTheSameType)
{
    GTEST_COUT << "Validating the operands of a power follow the rule of every arithmetic operator, as Rust pow and powf do." << std::endl;

    const std::string HEAD{ "function g(a: sinteger32, c: sinteger64, f: float64) " };

    EXPECT_EQ(MAREA_codesOf(HEAD + "a ** f; f ** a; a ** c; end"), (Codes{ MISMATCH, MISMATCH, MISMATCH })) << "A base and an exponent of different primitives must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "local n: float64 = a ** f; end"), Codes{ MISMATCH }) << "An integer base with a float exponent must raise one TYPE_MISMATCH and no second one for the local.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "local n: sinteger32 = a ** f; end"), Codes{ MISMATCH }) << "An integer base with a float exponent must raise TYPE_MISMATCH whatever the declared type.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "local n: float64 = a ** 2; end"), Codes{ MISMATCH }) << "The result of a power must keep the type of its operands.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "a ** 2.5; end"), Codes{ MISMATCH }) << "A fraction literal exponent does not fit an integer base.";

    EXPECT_TRUE(MAREA_codesOf(HEAD + "local p: sinteger32 = a ** 2; local q: float64 = f ** 0.5; local r: float64 = f ** 2; local s: sinteger32 = 2 ** a; local t: sinteger32 = a ** a; end").empty()) << "A power of the same primitive, or with a fitting literal, must raise nothing.";
}

TEST(MAREASemaExpressions, CompoundAssignmentNeedsTheSameTypeOnBothSides)
{
    GTEST_COUT << "Validating a compound assignment is compared like a = a + b and a compound shift follows the shift rule." << std::endl;

    const std::string HEAD{ "structure Point x: float32; end function g(a: sinteger32, c: sinteger64, f: float64, v: uinteger32, #[semantics = reference_mutable] p: Point, #[semantics = reference_mutable] s: string) " };

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

    EXPECT_EQ(MAREA_codesOf(NAME + "function g() local n: sinteger32 = name(); end"), Codes{ MISMATCH }) << "A string call for an integer local must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(NAME + "function f(a: boolean) end function g() f(name()); end"), Codes{ MISMATCH }) << "A string call for a boolean parameter must raise TYPE_MISMATCH.";

    EXPECT_TRUE(MAREA_codesOf(NAME + "function g() local s: string = name(); end").empty()) << "A call of the right type must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function same<Item>(a: Item): Item return copy a; end function g() local n: sinteger32 = same(4); end").empty()) << "A generic call must not be guessed.";

    EXPECT_TRUE(MAREA_codesOf("function none() end function g() local n: sinteger32 = none(); end").empty()) << "A call with no result type must not be guessed.";
}

TEST(MAREASemaExpressions, AssignmentAndReturnAreChecked)
{
    GTEST_COUT << "Validating a plain assignment and a return value are compared with the declared type." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function g(n: sinteger32, s: string) n = s; end"), Codes{ MISMATCH }) << "A string for an integer target must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("function g(n: sinteger32) n = true; end"), Codes{ MISMATCH }) << "A boolean literal for an integer target must raise TYPE_MISMATCH.";

    EXPECT_TRUE(MAREA_codesOf("function g(n: sinteger32, m: sinteger32) n = m + 1; n += 2; n <<= 1; end").empty()) << "A fitting assignment must raise nothing.";

    EXPECT_EQ(MAREA_codesOf("function g(a: sinteger32): boolean return a + 1; end"), Codes{ MISMATCH }) << "An integer for a boolean result must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("function g(): sinteger32 return \"text\"; end"), Codes{ MISMATCH }) << "A string literal for an integer result must raise TYPE_MISMATCH.";

    EXPECT_TRUE(MAREA_codesOf("function g(a: sinteger32): sinteger32 return a + 1; end function h(): float64 return 1; end").empty()) << "A fitting result must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function g(a: sinteger32) return; end").empty()) << "A return with no value must raise nothing.";
}

TEST(MAREASemaExpressions, ConstantInitializerIsChecked)
{
    GTEST_COUT << "Validating the initializer of a constant is compared with the declared type." << std::endl;

    EXPECT_EQ(MAREA_codesOf("constant LIMIT: sinteger32 = \"text\";"), Codes{ MISMATCH }) << "A string for an integer constant must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("constant A: float64 = 1.5; constant B: string = A;"), Codes{ MISMATCH }) << "A constant of another type must raise TYPE_MISMATCH.";

    EXPECT_TRUE(MAREA_codesOf("constant A: float64 = 1; constant B: float64 = A * 2.0; constant C: uinteger64 = size_of<sinteger32>() * 2; constant D: boolean = B > 1.0;").empty()) << "A fitting constant must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("constant B: sinteger32 = LATER + 1; constant LATER: sinteger32 = 4;").empty()) << "A name of a later declaration in a constant must raise nothing.";
}

TEST(MAREASemaExpressions, BinderShadowsAnOuterLocal)
{
    GTEST_COUT << "Validating a loop, with or partition name hides an outer local of the same name." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function g() local i: string = \"a\"; for i in 0 .. 3 do local n: sinteger32 = i; end end").empty()) << "A for name must hide the outer local.";

    EXPECT_TRUE(MAREA_codesOf("function g(items: c.array<sinteger32>) local item: string = \"a\"; for each item in items do local n: sinteger32 = item; end end").empty()) << "A for each name must hide the outer local.";

    EXPECT_EQ(MAREA_codesOf("function g() for i in 0 .. 3 do end local n: sinteger32 = i; end"), Codes{ UNDECLARED }) << "A for name must end with its loop.";
}

TEST(MAREASemaExpressions, UndeclaredNameInAnExpression)
{
    GTEST_COUT << "Validating a name that nothing declares is UNDECLARED_NAME." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function g() local x: sinteger32 = missing; end"), Codes{ UNDECLARED }) << "A missing name in a local initializer must raise UNDECLARED_NAME.";

    EXPECT_EQ(MAREA_codesOf("function g() missing(1); end"), Codes{ UNDECLARED }) << "A missing callee must raise UNDECLARED_NAME.";

    EXPECT_EQ(MAREA_codesOf("function g(a: sinteger32): sinteger32 return a + missing; end"), Codes{ UNDECLARED }) << "A missing name in an operator must raise UNDECLARED_NAME once.";

    EXPECT_EQ(MAREA_codesOf("function g(a: sinteger32) if missing then end end while a < missing do end end"), (Codes{ UNDECLARED, UNDECLARED })) << "A missing name in a condition must raise UNDECLARED_NAME.";

    EXPECT_EQ(MAREA_codesOf("function f(a: sinteger32) end function g() f(missing); end"), Codes{ UNDECLARED }) << "A missing name in an argument must raise UNDECLARED_NAME.";

    EXPECT_EQ(MAREA_codesOf("function g(a: sinteger32) a = missing; end"), Codes{ UNDECLARED }) << "A missing name in an assignment must raise UNDECLARED_NAME.";

    EXPECT_EQ(MAREA_codesOf("function g(a: sinteger32) switch missing case 1 then end end end"), Codes{ UNDECLARED }) << "A missing name as the switch value must raise UNDECLARED_NAME.";

    EXPECT_EQ(MAREA_codesOf("function g() local x: sinteger32 = x; end"), Codes{ UNDECLARED }) << "A local is not in scope in its own initializer.";

    EXPECT_EQ(MAREA_codesOf("function g() local x: sinteger32 = (missing).field; end"), Codes{ UNDECLARED }) << "A value that starts a member access and is not a bare name must be checked.";

    EXPECT_EQ(MAREA_codesOf("function f(a: sinteger32): sinteger32 return a; end function g() local x: sinteger32 = f(missing).field; end"), Codes{ UNDECLARED }) << "An argument below a member access must be checked.";
}

TEST(MAREASemaExpressions, UndeclaredNameInAHeaderOrAConstant)
{
    GTEST_COUT << "Validating a missing name in a loop or with header and in a constant initializer is UNDECLARED_NAME." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function g() for i in 0 .. missing do end end"), Codes{ UNDECLARED }) << "A missing name in a for range bound must raise UNDECLARED_NAME.";

    EXPECT_EQ(MAREA_codesOf("function g() for i in missing .. 3 do end end"), Codes{ UNDECLARED }) << "A missing name in a for range start must raise UNDECLARED_NAME.";

    EXPECT_EQ(MAREA_codesOf("function g() for each item in missing do end end"), Codes{ UNDECLARED }) << "A missing for each collection must raise UNDECLARED_NAME.";

    EXPECT_EQ(MAREA_codesOf("function g() for each key, item in missing do end end"), Codes{ UNDECLARED }) << "A missing for each collection with two names must raise UNDECLARED_NAME.";

    EXPECT_EQ(MAREA_codesOf("function g() with missing as handle do end end"), Codes{ UNDECLARED }) << "A missing with value must raise UNDECLARED_NAME.";

    EXPECT_EQ(MAREA_codesOf("function g() partition missing into 4 as part do end end"), Codes{ UNDECLARED }) << "A missing partition value must raise UNDECLARED_NAME.";

    EXPECT_EQ(MAREA_codesOf("function g() partition mutable missing into 4 as part do end end"), Codes{ UNDECLARED }) << "A missing mutable partition value must raise UNDECLARED_NAME.";

    EXPECT_EQ(MAREA_codesOf("function g(a: sinteger32) partition a into missing as part do end end"), Codes{ UNDECLARED }) << "A missing partition count must raise UNDECLARED_NAME.";

    EXPECT_EQ(MAREA_codesOf("constant A: sinteger32 = missing;"), Codes{ UNDECLARED }) << "A missing name in a constant initializer must raise UNDECLARED_NAME.";

    EXPECT_EQ(MAREA_codesOf("constant A: sinteger32 = 1 + missing;"), Codes{ UNDECLARED }) << "A missing name in a constant operator must raise UNDECLARED_NAME once.";
}

TEST(MAREASemaExpressions, KnownNamesInAHeaderOrAConstantAreNotReported)
{
    GTEST_COUT << "Validating a declared name in a header or a constant raises nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function g(n: sinteger32, items: c.array<sinteger32>) for i in 0 .. n do end for each item in items do end end").empty()) << "A parameter in a header must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("constant N: sinteger32 = 4; function g() for i in 0 .. N do end end").empty()) << "A constant in a header must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("constant A: sinteger32 = B + 1; constant B: sinteger32 = 4;").empty()) << "A constant that uses a later constant must raise nothing.";
}

TEST(MAREASemaExpressions, KnownNamesAreNotReported)
{
    GTEST_COUT << "Validating every name that something declares raises nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("constant LIMIT: sinteger32 = 9; function twice(a: sinteger32): sinteger32 return a * 2; end function g(p: sinteger32): sinteger32 local q: sinteger32 = twice(p) + LIMIT; return q; end").empty()) << "A parameter, a local, a constant and a function must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("enum Color red; green; end function g(c: Color): sinteger32 switch c case red then return 1; end case Color.green then return 2; end end end").empty()) << "A bare variant in a case must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function g(values: c.array<float32>): float32 local total: float32 = 0.0; for each value in values do total = total + value; end for i in 0 .. 3 do total = total + 1.0; end return total; end").empty()) << "A for and a for each name must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function work(a: c.array<float32>, k: float32) end function g(values: c.array<float32>, k: float32) fork_join partition values into 4 as part do spawn work(part, k); end end end").empty()) << "A partition part must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function open(): string return \"a\"; end function g() with open() as file do local n: string = file; end end").empty()) << "A with name must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function same<Item>(a: Item): Item return copy a; end function g<Item>(a: Item) local b: sinteger32 = Item.zero(); end").empty()) << "A generic parameter must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("structure Point x: float32; end function g(): Point return Point{ x = 1.0 }; end").empty()) << "A field key of a literal is not an expression name.";

    EXPECT_TRUE(MAREA_codesOf("function g(a: sinteger32) c.stdio.puts(\"x\"); local b: sinteger32 = convert<sinteger32>(a); local s: uinteger64 = size_of<sinteger32>() + align_of<sinteger32>(); end").empty()) << "The reserved namespace c and the built-in calls must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("import c.stdio; function g() stdio.puts(\"x\"); end").empty()) << "The bare name that starts a member chain is declared when an import binds it.";

    EXPECT_EQ(MAREA_codesOf("function g() local x: sinteger32 = missing.field; end"), Codes{ UNDECLARED }) << "The bare name that starts a member chain is checked like any other name.";

    EXPECT_TRUE(MAREA_codesOf("structure Point x: float32; end function g(p: Point): float32 return p.missing_field; end").empty()) << "A member name is not a bare name.";
}

TEST(MAREASemaExpressions, ImportedAndNamespaceNamesAreNotReported)
{
    GTEST_COUT << "Validating an import alias and the first segment of the own namespace raise nothing." << std::endl;

    MAREA_SemaFixture fixture{ [](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; } };

    EXPECT_EQ(fixture.MAREA_sema("namespace main; import library.text as text; function g() local x: sinteger32 = text.size(); end"), MAREA_SEMA_EXCEPT_SUCCESS) << "A program with an import alias must pass the sema run.";

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

    const std::string HEAD{ "structure Planet mass: float64; end structure Moon mass: float64; end constant LIMIT: sinteger32 = 1; " };

    EXPECT_TRUE(MAREA_codesOf(HEAD + "implementation Planet constant LIMIT: float64 = 6.0; function pull(self: Planet): float64 return LIMIT; end end").empty()) << "The type of the constant of the block must be used inside the block.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "implementation Planet constant LIMIT: float64 = 6.0; function pull(self: Planet): sinteger32 return LIMIT; end end"), Codes{ MISMATCH }) << "The type of the top-level constant must not be used inside the block.";

    EXPECT_TRUE(MAREA_codesOf(HEAD + "implementation Planet constant LIMIT: float64 = 6.0; end function top(): sinteger32 return LIMIT; end").empty()) << "The top-level constant must be used outside the block.";

    EXPECT_TRUE(MAREA_codesOf(HEAD + "implementation Planet constant LIMIT: float64 = 6.0; end implementation Moon function push(self: Moon): sinteger32 return LIMIT; end end").empty()) << "The constant of another block must not shadow the top-level one.";

    EXPECT_TRUE(MAREA_codesOf(HEAD + "implementation Planet constant LIMIT: float64 = 6.0; function pull(self: Planet, LIMIT: sinteger32): sinteger32 return LIMIT; end end").empty()) << "A parameter must still hide the constant of the block.";

    EXPECT_EQ(MAREA_codesOf("structure Planet mass: float64; end structure Moon mass: float64; end implementation Planet constant GRAVITY: float64 = 6.0; end implementation Moon function push(self: Moon): float64 return GRAVITY; end end"), Codes{ UNDECLARED }) << "The constant of one block is not a name in another block.";
}

TEST(MAREASemaExpressions, ErrorNodeInAnExpressionIsSkipped)
{
    GTEST_COUT << "Validating a broken expression gives no sema diagnostic." << std::endl;

    for (const char *body : { "function g() local x: sinteger32 = ; end", "function g() local x: sinteger32 = 1 + ; end", "function g() local x: sinteger32 = (; end" }) {
        MAREA_SemaFixture fixture;

        EXPECT_EQ(fixture.MAREA_sema(std::string{ "namespace main; " } + body), MAREA_SEMA_EXCEPT_SUCCESS) << "A broken expression must not stop the sema run: " << body;

        for (MAREA_Uinteger32 code : fixture.MAREA_semaCodes()) {
            EXPECT_NE(code, UNDECLARED) << body;
        }
    }
}

TEST(MAREASemaExpressions, ConvertBetweenNumberPrimitivesIsAccepted)
{
    GTEST_COUT << "Validating convert takes any number primitive to any other number primitive, as Rust as does." << std::endl;

    const std::string HEAD{ "function g(a: sinteger32, b: uinteger8, c: sinteger64, f: float32, d: float64) " };

    EXPECT_TRUE(MAREA_codesOf(HEAD + "local w: sinteger64 = convert<sinteger64>(a); local s: uinteger8 = convert<uinteger8>(c); local x: uinteger64 = convert<uinteger64>(b); local y: sinteger8 = convert<sinteger8>(b); end").empty()) << "An integer to an integer of any width and sign must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(HEAD + "local x: float64 = convert<float64>(a); local y: float32 = convert<float32>(b); local z: float32 = convert<float32>(c); end").empty()) << "An integer to a float must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(HEAD + "local x: sinteger32 = convert<sinteger32>(f); local y: uinteger64 = convert<uinteger64>(d); end").empty()) << "A float to an integer must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(HEAD + "local x: float64 = convert<float64>(f); local y: float32 = convert<float32>(d); end").empty()) << "A float to a float must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(HEAD + "local x: float64 = convert<float64>(1); local y: sinteger32 = convert<sinteger32>(2.5); local z: sinteger64 = convert<sinteger64>(a + 1); end").empty()) << "A number literal and a calculation are number operands.";

    EXPECT_TRUE(MAREA_codesOf(HEAD + "local x: sinteger32 = convert<sinteger32>(a); local s: string = convert<string>(\"t\"); local b2: boolean = convert<boolean>(true); end").empty()) << "The same type to itself must raise nothing.";
}

TEST(MAREASemaExpressions, ConvertOfAnythingElseIsATypeMismatch)
{
    GTEST_COUT << "Validating convert refuses a string, a boolean and a structure against a number, with TYPE_MISMATCH." << std::endl;

    const std::string HEAD{ "structure Point x: float32; end function g(a: sinteger32, s: string, t: boolean, p: Point) " };

    EXPECT_EQ(MAREA_codesOf(HEAD + "convert<sinteger32>(s); end"), Codes{ MISMATCH }) << "A string to a number must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "convert<string>(a); convert<float64>(t); convert<boolean>(a); end"), (Codes{ MISMATCH, MISMATCH, MISMATCH })) << "A number to a string, and a boolean against a number, must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "convert<sinteger32>(\"1\"); convert<float32>(true); convert<string>(1); convert<string>(1.5); end"), (Codes{ MISMATCH, MISMATCH, MISMATCH, MISMATCH })) << "A literal is typed like a name.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "convert<sinteger32>(p); end"), Codes{ MISMATCH }) << "A structure to a primitive must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "convert<Point>(a); convert<Point>(s); end"), (Codes{ MISMATCH, MISMATCH })) << "A primitive to a structure must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "local n: sinteger32 = convert<sinteger32>(s); end"), Codes{ MISMATCH }) << "A bad conversion must raise once and the result type must still fit the local.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "local n: sinteger32 = convert<sinteger32>(convert<string>(a)); end"), (Codes{ MISMATCH, MISMATCH })) << "The inner conversion is checked, and its result type string is still the operand of the outer one.";
}

TEST(MAREASemaExpressions, ConvertResultHasTheTypeOfTheTypeArgument)
{
    GTEST_COUT << "Validating the result of convert is T, so it feeds the other expression checks." << std::endl;

    const std::string HEAD{ "function take(a: sinteger64) end function g(a: sinteger32, c: sinteger64, d: float64) " };

    EXPECT_EQ(MAREA_codesOf(HEAD + "local n: sinteger32 = convert<sinteger64>(a); end"), Codes{ MISMATCH }) << "The result sinteger64 does not fit an sinteger32 local.";

    EXPECT_TRUE(MAREA_codesOf(HEAD + "local n: sinteger64 = convert<sinteger64>(a); take(convert<sinteger64>(a)); end").empty()) << "The result sinteger64 fits an sinteger64 local and parameter.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "take(convert<float64>(a)); end"), Codes{ MISMATCH }) << "The result float64 does not fit an sinteger64 parameter.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "local n: sinteger64 = convert<sinteger32>(c) + c; end"), Codes{ MISMATCH }) << "The result is an operand with its own type.";

    EXPECT_TRUE(MAREA_codesOf(HEAD + "local n: sinteger64 = convert<sinteger64>(a) + c; local m: float64 = d * convert<float64>(a); end").empty()) << "An operand of the same type as the result must raise nothing.";

    EXPECT_EQ(MAREA_codesOf("function g(a: sinteger32): sinteger64 return convert<float64>(a); end"), Codes{ MISMATCH }) << "A return value is compared with the result type.";

    EXPECT_EQ(MAREA_codesOf("constant LIMIT: sinteger32 = convert<sinteger64>(5);"), Codes{ MISMATCH }) << "The initializer of a constant is compared with the result type.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "local n: sinteger64 = 0; n += convert<sinteger32>(a); end"), Codes{ MISMATCH }) << "A compound assignment is compared with the result type.";
}

TEST(MAREASemaExpressions, ConvertOfAnUnknownTypeIsAccepted)
{
    GTEST_COUT << "Validating sema never guesses: an unknown operand, or a target it cannot sort, raises nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("enum Color red; end function g(c: Color, a: sinteger32) local n: sinteger32 = convert<sinteger32>(c); local d: Color = convert<Color>(a); end").empty()) << "An enum is not decided.";

    EXPECT_TRUE(MAREA_codesOf("function g<Item>(x: Item, a: sinteger32) local n: sinteger32 = convert<sinteger32>(x); local y: Item = convert<Item>(a); end").empty()) << "A generic parameter is not decided.";

    EXPECT_TRUE(MAREA_codesOf("function g(a: sinteger32, v: pointer<sinteger32>) local n: sinteger32 = convert<sinteger32>(v); convert<pointer<sinteger32>>(a); end").empty()) << "A pointer is a type that sema does not sort.";

    EXPECT_TRUE(MAREA_codesOf("import lib; function g(a: sinteger32) local n: sinteger32 = convert<sinteger32>(a.missing); local m: sinteger32 = convert<sinteger32>(lib.get(a)); end").empty()) << "An operand of unknown type must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function g(a: sinteger32) local n: sinteger32 = convert<sinteger32, sinteger64>(a); local m: sinteger32 = convert<sinteger32>(a, a); local o: sinteger32 = convert<sinteger32>(); end").empty()) << "A call with not exactly one type argument and one argument is not checked here.";
}

TEST(MAREASemaExpressions, ConvertNamedByTheUnitIsNotTheBuiltIn)
{
    GTEST_COUT << "Validating a function or a local named convert is a plain call, as the IR reads it." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function convert<Item>(a: string): Item return copy a; end function g(s: string) local n: sinteger32 = convert<sinteger32>(s); end").empty()) << "A function named convert in the unit is a user function.";

    EXPECT_TRUE(MAREA_codesOf("function g(s: string, convert: function(text: string): sinteger32) local n: sinteger32 = convert<sinteger32>(s); end").empty()) << "A parameter named convert hides the built-in.";
}

TEST(MAREASemaExpressions, ConvertBetweenStructuresNeedsTheSameLayout)
{
    GTEST_COUT << "Validating a structure converts to a structure only when both have the same fields, size and alignment." << std::endl;

    const std::string STRUCTURES{ "#[layout = c] structure A x: sinteger32; y: float32; end "
                                  "#[layout = c] structure B left: sinteger32; right: float32; end "
                                  "#[layout = c] structure Wide x: sinteger64; y: float32; end "
                                  "#[layout = c] structure Short x: sinteger32; end "
                                  "#[layout = c] structure Swapped x: float32; y: sinteger32; end " };

    const std::string HEAD{ STRUCTURES + "function g(a: A, b: B, w: Wide, s: Short, p: Swapped) " };

    EXPECT_TRUE(MAREA_codesOf(HEAD + "local x: B = convert<B>(a); local y: A = convert<A>(b); local z: A = convert<A>(a); end").empty()) << "The same field types in the same order, and the same type to itself, must raise nothing.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "convert<A>(w); end"), Codes{ MISMATCH }) << "A structure of another size must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "convert<A>(s); convert<Short>(a); end"), (Codes{ MISMATCH, MISMATCH })) << "A structure with another field count must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "convert<A>(p); end"), Codes{ MISMATCH }) << "Fields of another type at the same place must raise TYPE_MISMATCH, although the size is the same.";

    EXPECT_EQ(MAREA_codesOf(STRUCTURES + "function take(n: sinteger32) end function g(a: A) take(convert<B>(a)); end"), Codes{ MISMATCH }) << "The result type is the target structure, which is not a primitive.";
}

TEST(MAREASemaExpressions, ConvertBetweenStructuresComparesTheComputedSizeAndAlignment)
{
    GTEST_COUT << "Validating pack and align change the size and the alignment, and the auto layout is compared by the fields." << std::endl;

    const std::string FIELDS{ "x: uinteger8; y: uinteger32; end " };

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
                                  "#[layout = c] structure Number x: sinteger32; end "
                                  "#[layout = c] structure Handle raw: pointer<sinteger32>; end "
                                  "#[layout = c] structure Holder<Item> value: Item; end "
                                  "#[layout = c] structure Again<Item> value: Item; end " };

    const std::string HEAD{ STRUCTURES + "function g(t: Text, n: Number, h: Handle, v: Holder<sinteger32>) " };

    EXPECT_EQ(MAREA_codesOf(HEAD + "convert<Other>(t); convert<Number>(t); end"), (Codes{ MISMATCH, MISMATCH })) << "A string field has no size in sema, so the pair is rejected, whatever the other fields are.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "convert<Number>(h); end"), Codes{ MISMATCH }) << "A pointer field is rejected.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "convert<Number>(v); convert<Again<sinteger32>>(v); end"), (Codes{ MISMATCH, MISMATCH })) << "A generic structure has no layout in sema, so it is rejected against another type and against another instance.";

    EXPECT_TRUE(MAREA_codesOf(HEAD + "local x: Text = convert<Text>(t); local y: Holder<sinteger32> = convert<Holder<sinteger32>>(v); end").empty()) << "A structure to itself needs no size, so it stays accepted.";
}

TEST(MAREASemaExpressions, ConvertBetweenStructuresMeasuresAFieldAlignAndANestedStructure)
{
    GTEST_COUT << "Validating a field align and a nested structure of the c layout are measured and compared." << std::endl;

    const std::string STRUCTURES{ "#[layout = c] structure Number x: sinteger32; end "
                                  "#[layout = c] structure Count y: sinteger32; end "
                                  "#[layout = c] structure Wide z: sinteger64; end "
                                  "#[layout = c] structure Nested inner: Number; end "
                                  "#[layout = c] structure NestedCount other: Count; end "
                                  "#[layout = c] structure NestedWide other: Wide; end "
                                  "structure AutoNumber x: sinteger32; end "
                                  "#[layout = c] structure OfAuto inner: AutoNumber; end "
                                  "#[layout = c] structure AlignedOne #[align = 16] x: sinteger32; end "
                                  "#[layout = c] structure AlignedTwo #[align = 16] y: sinteger32; end "
                                  "#[layout = c] structure AlignedEight #[align = 8] x: sinteger32; end "
                                  "#[layout = c] structure Plain x: sinteger32; end " };

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
                                  "#[layout = c] structure Count value: sinteger32; end " };

    const std::string HEAD{ STRUCTURES + "function g(l: Length, r: Raw, c: Count) " };

    EXPECT_TRUE(MAREA_codesOf(HEAD + "local x: Raw = convert<Raw>(l); local y: Length = convert<Length>(r); end").empty()) << "An alias of float32 is float32.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "convert<Count>(l); convert<Length>(c); end"), (Codes{ MISMATCH, MISMATCH })) << "An alias of float32 is not sinteger32.";
}

namespace
{
    // INFO: one value of each opaque kind is used in every slot that compares a shape, so a kind that is classified as comparable would raise TYPE_MISMATCH here. A local initializer uses copy, as the ownership rule asks.
    void MAREA_expectOpaqueValueAccepted(const std::string &declarations, const std::string &parameter, const std::string &kind)
    {
        const std::string HEAD{ declarations + "function f(a: sinteger32) end function g(v: " + parameter + ", w: " + parameter + ", s: string) " };

        EXPECT_TRUE(MAREA_codesOf(HEAD + "f(v); local n: sinteger32 = copy v; local b: boolean = copy v; local t: string = copy v; end").empty()) << kind << " value must not be compared with a primitive target.";

        EXPECT_TRUE(MAREA_codesOf(HEAD + "local x: boolean = v == w; local y: boolean = v < 1; local z: boolean = 1 == v; end").empty()) << kind << " value must not be compared in a binary operator.";

        EXPECT_TRUE(MAREA_codesOf(HEAD + "local x: sinteger32 = v << 1; local y: sinteger32 = 1 >> v; local z: sinteger32 = v + 1; end").empty()) << kind << " value must not be checked as an integer in a shift or in arithmetic.";

        EXPECT_TRUE(MAREA_codesOf(HEAD + "local n: sinteger32 = convert<sinteger32>(v); local m: boolean = convert<boolean>(v); end").empty()) << kind << " value must not be refused by a conversion.";

        EXPECT_EQ(MAREA_codesOf(HEAD + "f(v); f(s); end"), Codes{ MISMATCH }) << kind << " value must not hide a real mismatch next to it.";
    }
} // namespace

TEST(MAREASemaExpressions, EnumValueIsAKnownShapeNeverComparedWithAPrimitive)
{
    GTEST_COUT << "Validating an enum value is classified and never compared with a primitive." << std::endl;

    MAREA_expectOpaqueValueAccepted("enum Color red; green; end ", "Color", "An enum");
}

TEST(MAREASemaExpressions, LibraryValueIsAKnownShapeNeverComparedWithAPrimitive)
{
    GTEST_COUT << "Validating a library type value is classified and never compared with a primitive." << std::endl;

    MAREA_expectOpaqueValueAccepted("", "Handle", "A library type");
}

TEST(MAREASemaExpressions, GenericParameterValueIsAKnownShapeNeverComparedWithAPrimitive)
{
    GTEST_COUT << "Validating a generic parameter value is classified and never compared with a primitive." << std::endl;

    const std::string HEAD{ "function f(a: sinteger32) end function g<Item>(v: Item, w: Item, s: string) " };

    EXPECT_TRUE(MAREA_codesOf(HEAD + "f(v); local n: sinteger32 = copy v; local x: boolean = v == w; local y: sinteger32 = v << 1; local z: sinteger32 = v + 1; local m: sinteger32 = convert<sinteger32>(v); end").empty()) << "A generic parameter value must not be compared with a primitive.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "f(v); f(s); end"), Codes{ MISMATCH }) << "A generic parameter value must not hide a real mismatch next to it.";
}

TEST(MAREASemaExpressions, OwnedTaskVectorAndArrayValuesAreKnownShapesNeverComparedWithAPrimitive)
{
    GTEST_COUT << "Validating an owned, a task, a vector and an array value are classified and never compared with a primitive." << std::endl;

    MAREA_expectOpaqueValueAccepted("", "owned<sinteger32>", "An owned");

    MAREA_expectOpaqueValueAccepted("", "task<sinteger32>", "A task");

    MAREA_expectOpaqueValueAccepted("", "array<float32, 4>", "An array");

    MAREA_expectOpaqueValueAccepted("", "vector<float32>", "A vector");
}

TEST(MAREASemaExpressions, OpaqueKindsStayUnknownAtAFieldAccess)
{
    GTEST_COUT << "Validating a field of an opaque kind is accepted at every slot." << std::endl;

    const std::string HEAD{ "enum Color red; end structure Box c: Color; o: owned<sinteger32>; v: array<float32, 4>; end function f(a: sinteger32) end function g(b: Box) " };

    EXPECT_TRUE(MAREA_codesOf(HEAD + "f(b.c); f(b.o); f(b.v); local n: sinteger32 = b.c + 1; end").empty()) << "A field of an opaque kind must not be compared with a primitive.";
}

TEST(MAREASemaExpressions, CallMayOmitDefaultedTrailingArguments)
{
    GTEST_COUT << "Validating a call may omit a trailing argument that has a default value." << std::endl;

    const std::string HEAD{ "function f(a: sinteger32, b: sinteger32 = 0) end function g() " };

    EXPECT_TRUE(MAREA_codesOf(HEAD + "f(1); end").empty()) << "A call with only the required argument must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(HEAD + "f(1, 2); end").empty()) << "A call with every argument must raise nothing.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "f(); end"), Codes{ COUNT_MISMATCH }) << "A call with too few arguments must raise ARGUMENT_COUNT_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "f(1, 2, 3); end"), Codes{ COUNT_MISMATCH }) << "A call with too many arguments must raise ARGUMENT_COUNT_MISMATCH.";
}

TEST(MAREASemaExpressions, DefaultValueMustFitItsParameterType)
{
    GTEST_COUT << "Validating a default value is checked against the type of its parameter." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f(a: sinteger32 = 1) end").empty()) << "A default of the right type must raise nothing.";

    EXPECT_EQ(MAREA_codesOf("function f(a: sinteger32 = \"x\") end"), Codes{ MISMATCH }) << "A default of the wrong type must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("function f(a: sinteger32 = missing) end"), Codes{ UNDECLARED }) << "An undeclared name in a default must raise UNDECLARED_NAME.";
}

TEST(MAREASemaExpressions, DefaultedParameterMustNotPrecedeARequiredOne)
{
    GTEST_COUT << "Validating a defaulted parameter must not come before a required parameter." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f(a: sinteger32 = 0, b: sinteger32) end"), Codes{ DEFAULT_BEFORE_REQUIRED }) << "A default before a required parameter must raise DEFAULT_PARAMETER_BEFORE_REQUIRED.";

    EXPECT_EQ(MAREA_codesOf("function f(a: sinteger32 = 0, b: sinteger32, c: sinteger32 = 1, d: sinteger32) end"), (Codes{ DEFAULT_BEFORE_REQUIRED, DEFAULT_BEFORE_REQUIRED })) << "Each defaulted parameter that precedes a required one must raise once.";

    EXPECT_TRUE(MAREA_codesOf("function f(a: sinteger32, b: sinteger32 = 0, c: sinteger32 = 1) end").empty()) << "Defaults at the end must raise nothing.";
}

TEST(MAREASemaExpressions, CallOfATypeNameResolvesToItsConstructOperator)
{
    GTEST_COUT << "Validating a call whose callee is a type name is checked against its construct operator." << std::endl;

    const std::string POINT{ "structure Point x: float32; y: float32; end implementation Point #[operator = \"construct\"] function make(x: float32 = 0.0, y: float32 = 0.0) end end function g() " };

    EXPECT_TRUE(MAREA_codesOf(POINT + "Point(1.0, 2.0); end").empty()) << "A construction call with every argument must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(POINT + "Point(1.0); end").empty()) << "A construction call that omits a defaulted trailing argument must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(POINT + "Point(); end").empty()) << "A construction call with no argument, every parameter defaulted, must raise nothing.";

    EXPECT_EQ(MAREA_codesOf(POINT + "Point(1.0, 2.0, 3.0); end"), Codes{ COUNT_MISMATCH }) << "A construction call with too many arguments must raise ARGUMENT_COUNT_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(POINT + "Point(\"x\", 2.0); end"), Codes{ MISMATCH }) << "A construction call with a wrong argument type must raise TYPE_MISMATCH.";
}

TEST(MAREASemaExpressions, TypeWithNoConstructOperatorAcceptsOnlyAnEmptyCall)
{
    GTEST_COUT << "Validating a type with no construct operator accepts only a call with no argument." << std::endl;

    const std::string LINE{ "structure Line length: float32; end function g() " };

    EXPECT_TRUE(MAREA_codesOf(LINE + "Line(); end").empty()) << "A call with no argument and no construct operator must raise nothing.";

    EXPECT_EQ(MAREA_codesOf(LINE + "Line(1.0); end"), Codes{ COUNT_MISMATCH }) << "A call with an argument, and no construct operator, must raise ARGUMENT_COUNT_MISMATCH.";
}

TEST(MAREASemaExpressions, OpaqueTypeCannotBeBuiltReadOrOperated)
{
    GTEST_COUT << "Validating a literal, a field read and an operator of an opaque type raise OPAQUE_TYPE_MISUSE." << std::endl;

    const std::string OPAQUE{ "opaque structure T; " };

    EXPECT_EQ(MAREA_codesOf(OPAQUE + "function f(a: T, b: T): T return a + b; end"), Codes{ OPAQUE_MISUSE }) << "An operator on two opaque names must raise OPAQUE_TYPE_MISUSE.";

    EXPECT_EQ(MAREA_codesOf(OPAQUE + "function f(a: T, b: T): boolean return a == b; end"), Codes{ OPAQUE_MISUSE }) << "A comparison of two opaque names must raise OPAQUE_TYPE_MISUSE.";

    EXPECT_EQ(MAREA_codesOf(OPAQUE + "function f(a: T): sinteger32 return a.width; end"), Codes{ OPAQUE_MISUSE }) << "A field read of an opaque name must raise OPAQUE_TYPE_MISUSE.";

    EXPECT_EQ(MAREA_codesOf(OPAQUE + "function f(a: T): sinteger32 return (a).width; end"), Codes{ OPAQUE_MISUSE }) << "A field read of a parenthesized opaque name must raise OPAQUE_TYPE_MISUSE.";

    EXPECT_EQ(MAREA_codesOf(OPAQUE + "function f() local t: T = T{ }; end"), Codes{ OPAQUE_MISUSE }) << "A struct literal of an opaque type must raise OPAQUE_TYPE_MISUSE.";
}

TEST(MAREASemaExpressions, OpaqueTypeUsedThroughABorrowOrAnOwnerIsAccepted)
{
    GTEST_COUT << "Validating an opaque type through a borrow, an owner or a C call raises nothing." << std::endl;

    const std::string OPAQUE{ "opaque structure T; " };

    EXPECT_TRUE(MAREA_codesOf(OPAQUE + "function f(a: T) local v: sinteger32 = 1; end").empty()) << "A borrow parameter of an opaque type must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(OPAQUE + "function g(a: T) end function f(a: T) g(a); end").empty()) << "An opaque name passed as a borrow must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(OPAQUE + "function g(a: T) end function f(a: owned<T>) g(a); end").empty()) << "An owner passed as a borrow must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(OPAQUE + "function f(a: T): sinteger32 return c.texture.width(a); end").empty()) << "A C call with an opaque argument must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("structure S width: sinteger32; end function f(a: S): sinteger32 return a.width; end").empty()) << "A field read of an ordinary structure must raise nothing.";
}

TEST(MAREASemaExpressions, FunctionTypeValueIsChecked)
{
    GTEST_COUT << "Validating a function type value: a literal, a function signature and a call through the value." << std::endl;

    const std::string OP{ "type Op = function(left: sinteger32, right: sinteger32): sinteger32; function add(left: sinteger32, right: sinteger32): sinteger32 return left + right; end function apply(operation: Op): sinteger32 return operation(1, 2); end " };

    EXPECT_EQ(MAREA_codesOf("type Callback = function(text: string); function run() local callback: Callback = \"x\"; end"), Codes{ MISMATCH }) << "A text literal for a function type local must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("type Callback = function(); function run() local callback: Callback = 1; end"), Codes{ MISMATCH }) << "An integer literal for a function type local must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(OP + "function negate(value: sinteger32): sinteger32 return -value; end function run(): sinteger32 return apply(negate); end"), Codes{ FUNCTION_TYPE_MISMATCH }) << "A function with fewer parameters must raise FUNCTION_TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(OP + "function wide(left: sinteger32, right: sinteger32): sinteger64 return 0; end function run(): sinteger32 return apply(wide); end"), Codes{ FUNCTION_TYPE_MISMATCH }) << "A function with another result type must raise FUNCTION_TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(OP + "function other(left: string, right: sinteger32): sinteger32 return 0; end function run(): sinteger32 return apply(other); end"), Codes{ FUNCTION_TYPE_MISMATCH }) << "A function with another parameter type must raise FUNCTION_TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(OP + "function other(left: string, right: sinteger32): sinteger32 return 0; end function run() local operation: Op = other; end"), Codes{ FUNCTION_TYPE_MISMATCH }) << "A local initializer is compared too.";

    EXPECT_EQ(MAREA_codesOf("type One = function(value: sinteger32): sinteger32; function apply(operation: One): sinteger32 return operation(1, 2); end"), Codes{ COUNT_MISMATCH }) << "A call through a function type value with too many arguments must raise ARGUMENT_COUNT_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("type One = function(value: sinteger32): sinteger32; function apply(operation: One): sinteger32 return operation(\"x\"); end"), Codes{ MISMATCH }) << "A call through a function type value with a wrong argument type must raise TYPE_MISMATCH.";

    EXPECT_TRUE(MAREA_codesOf(OP + "function run(): sinteger32 return apply(add); end").empty()) << "A function with the same signature must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(OP + "function pass(operation: Op): sinteger32 return apply(operation); end").empty()) << "A function type value passed on must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("type Callback = function(text: string); structure Holder callback: Callback; end function run(holder: Holder) end").empty()) << "A function type field must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(OP + "function pick<T>(left: sinteger32, right: sinteger64): sinteger32 return left; end function run() local operation: Op = pick; end").empty()) << "A generic function must not be compared.";
}
