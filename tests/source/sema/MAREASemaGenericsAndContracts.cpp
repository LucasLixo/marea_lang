// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

    constexpr MAREA_Uinteger32 NOT_SATISFIED{ MAREA_SEMA_DIAGNOSTIC_CODE_CONTRACT_NOT_SATISFIED };
    constexpr MAREA_Uinteger32 ARGUMENT_MISMATCH{ MAREA_SEMA_DIAGNOSTIC_CODE_GENERIC_ARGUMENT_MISMATCH };
    constexpr MAREA_Uinteger32 TOO_DEEP{ MAREA_SEMA_DIAGNOSTIC_CODE_INSTANTIATION_TOO_DEEP };
    constexpr MAREA_Uinteger32 UNDECLARED{ MAREA_SEMA_DIAGNOSTIC_CODE_UNDECLARED_NAME };
    constexpr MAREA_Uinteger32 COUNT_MISMATCH{ MAREA_SEMA_DIAGNOSTIC_CODE_ARGUMENT_COUNT_MISMATCH };
    constexpr MAREA_Uinteger32 UNKNOWN_TYPE_FUNCTION{ MAREA_SEMA_DIAGNOSTIC_CODE_UNKNOWN_TYPE_FUNCTION };

    Codes MAREA_codesOf(const std::string &body)
    {
        MAREA_SemaFixture fixture;

        if (fixture.MAREA_sema("namespace main; " + body) != MAREA_SEMA_EXCEPT_SUCCESS) {
            return { 0 };
        }

        return fixture.MAREA_semaCodes();
    }

    const std::string COMPARABLE{ "contract Comparable function less(self: Self, other: Self): boolean; end " };

    const std::string MAXIMUM{ "function maximum<Item>(left: Item, right: Item): Item where Item: Comparable return copy left; end " };

    std::string MAREA_nest(int levels)
    {
        std::string text{ "sinteger32" };

        for (int level{ 0 }; level < levels; ++level) {
            text = "Box<" + text + ">";
        }

        return text;
    }
} // namespace

TEST(MAREASemaGenericsAndContracts, TypeParametersResolve)
{
    GTEST_COUT << "Validating generic structures and functions with type parameters raise nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("structure Pair<First, Second> first: First; second: Second; end function same<Item>(a: Item): Item return copy a; end").empty()) << "Type parameters must raise nothing.";
}

TEST(MAREASemaGenericsAndContracts, ValueGenericSizesAVector)
{
    GTEST_COUT << "Validating a value generic is usable as a array size." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("structure Buffer<Item, constant Count: sinteger32> items: array<Item, Count>; end").empty()) << "A value generic must raise nothing.";
}

TEST(MAREASemaGenericsAndContracts, ContractMetByImplementation)
{
    GTEST_COUT << "Validating a call whose concrete type implements the contract raises nothing." << std::endl;

    const std::string BODY{ COMPARABLE + "structure Score points: sinteger32; end implementation Score function less(self: Score, other: Score): boolean return true; end end " + MAXIMUM + "function run(a: Score, b: Score): Score return maximum(a, b); end" };

    EXPECT_TRUE(MAREA_codesOf(BODY).empty()) << "A met contract must raise nothing.";
}

TEST(MAREASemaGenericsAndContracts, ContractMetThroughExplicitTypeArguments)
{
    GTEST_COUT << "Validating explicit type arguments are checked like inferred ones." << std::endl;

    const std::string MET{ COMPARABLE + "structure Score points: sinteger32; end implementation Score function less(self: Score, other: Score): boolean return true; end end " + MAXIMUM + "function run(a: Score, b: Score): Score return maximum<Score>(a, b); end" };

    EXPECT_TRUE(MAREA_codesOf(MET).empty()) << "Explicit type arguments that meet the contract must raise nothing.";

    const std::string UNMET{ COMPARABLE + "structure Score points: sinteger32; end " + MAXIMUM + "function run(a: Score, b: Score): Score return maximum<Score>(a, b); end" };

    EXPECT_EQ(MAREA_codesOf(UNMET), Codes{ NOT_SATISFIED }) << "Explicit type arguments that miss the contract must raise CONTRACT_NOT_SATISFIED.";
}

TEST(MAREASemaGenericsAndContracts, ContractWithMissingFunctionIsRejected)
{
    GTEST_COUT << "Validating a type with no implementation of the contract function is rejected." << std::endl;

    const std::string BODY{ COMPARABLE + "structure Score points: sinteger32; end " + MAXIMUM + "function run(a: Score, b: Score): Score return maximum(a, b); end" };

    EXPECT_EQ(MAREA_codesOf(BODY), Codes{ NOT_SATISFIED }) << "A missing function must raise CONTRACT_NOT_SATISFIED.";
}

TEST(MAREASemaGenericsAndContracts, ContractWithMismatchedSignatureIsRejected)
{
    GTEST_COUT << "Validating an implementation with a wrong parameter or return type is rejected." << std::endl;

    const std::string WRONG_PARAMETER{ COMPARABLE + "structure Score points: sinteger32; end implementation Score function less(self: Score, other: sinteger32): boolean return true; end end " + MAXIMUM + "function run(a: Score, b: Score): Score return maximum(a, b); end" };

    EXPECT_EQ(MAREA_codesOf(WRONG_PARAMETER), Codes{ NOT_SATISFIED }) << "A mismatched parameter must raise CONTRACT_NOT_SATISFIED.";

    const std::string WRONG_RETURN{ COMPARABLE + "structure Score points: sinteger32; end implementation Score function less(self: Score, other: Score): sinteger32 return 1; end end " + MAXIMUM + "function run(a: Score, b: Score): Score return maximum(a, b); end" };

    EXPECT_EQ(MAREA_codesOf(WRONG_RETURN), Codes{ NOT_SATISFIED }) << "A mismatched return type must raise CONTRACT_NOT_SATISFIED.";

    const std::string WRONG_COUNT{ COMPARABLE + "structure Score points: sinteger32; end implementation Score function less(self: Score): boolean return true; end end " + MAXIMUM + "function run(a: Score, b: Score): Score return maximum(a, b); end" };

    EXPECT_EQ(MAREA_codesOf(WRONG_COUNT), Codes{ NOT_SATISFIED }) << "A mismatched parameter count must raise CONTRACT_NOT_SATISFIED.";
}

TEST(MAREASemaGenericsAndContracts, ImplementationInSecondBlockCounts)
{
    GTEST_COUT << "Validating a contract function found in any implementation block of the type is accepted." << std::endl;

    const std::string BODY{ COMPARABLE + "structure Score points: sinteger32; end implementation Score function other() end end implementation Score function less(self: Score, other: Score): boolean return true; end end " + MAXIMUM + "function run(a: Score, b: Score): Score return maximum(a, b); end" };

    EXPECT_TRUE(MAREA_codesOf(BODY).empty()) << "A second block must be searched.";
}

TEST(MAREASemaGenericsAndContracts, CopyableNeedsAPrimitiveAnArrayOrAnEnum)
{
    GTEST_COUT << "Validating Copyable is met by primitives and an enum (its documented intentional exception) and missed by a plain or move_only structure." << std::endl;

    const std::string COPY{ "function copy_of<Item>(a: Item): Item where Item: Copyable return copy a; end " };

    EXPECT_TRUE(MAREA_codesOf(COPY + "enum Color red; end function run(c: Color): Color return copy_of(c); end").empty()) << "An enum, the by-value exception, must be Copyable.";

    EXPECT_TRUE(MAREA_codesOf(COPY + "function run(): sinteger32 return copy_of<sinteger32>(1); end").empty()) << "A primitive must be Copyable.";

    EXPECT_EQ(MAREA_codesOf(COPY + "structure Plain x: float32; end function run(p: Plain): Plain return copy_of(p); end"), Codes{ NOT_SATISFIED }) << "A plain structure must miss Copyable; #[value] no longer exempts it.";

    EXPECT_EQ(MAREA_codesOf(COPY + "#[move_only] structure M x: float32; end function run(m: M): M return copy_of(m); end"), Codes{ NOT_SATISFIED }) << "A move_only structure must miss Copyable.";
}

TEST(MAREASemaGenericsAndContracts, MovableNeedsAnOwnedOrMoveOnlyType)
{
    GTEST_COUT << "Validating Movable is met by owned<T> and a move_only type." << std::endl;

    const std::string TAKE{ "function take<Item>(a: owned<Item>): owned<Item> where Item: Movable return a; end " };

    EXPECT_TRUE(MAREA_codesOf(TAKE + "function run(a: owned<sinteger32>): owned<sinteger32> return take<owned<sinteger32>>(a); end").empty()) << "owned<T> must be Movable.";

    EXPECT_TRUE(MAREA_codesOf(TAKE + "#[move_only] structure Token x: sinteger32; end function run(a: owned<Token>): owned<Token> return take<Token>(a); end").empty()) << "A move_only type must be Movable.";

    EXPECT_EQ(MAREA_codesOf(TAKE + "function run(a: owned<sinteger32>): owned<sinteger32> return take<sinteger32>(a); end"), Codes{ NOT_SATISFIED }) << "A primitive must miss Movable.";
}

TEST(MAREASemaGenericsAndContracts, GenericParameterSatisfiesThroughItsOwnWhereClause)
{
    GTEST_COUT << "Validating a generic parameter meets a contract only when its own where clause lists it." << std::endl;

    const std::string COPY{ "function copy_of<Item>(a: Item): Item where Item: Copyable return copy a; end " };

    EXPECT_TRUE(MAREA_codesOf(COPY + "function twice<Other>(a: Other): Other where Other: Copyable return copy_of(a); end").empty()) << "A listed contract must be met.";

    EXPECT_EQ(MAREA_codesOf(COPY + "function twice<Other>(a: Other): Other return copy_of(a); end"), Codes{ NOT_SATISFIED }) << "An unlisted contract must be missed.";
}

TEST(MAREASemaGenericsAndContracts, InstantiationDepthIsLimitedTo64)
{
    GTEST_COUT << "Validating 64 nested instantiations are accepted and 65 are rejected." << std::endl;

    const std::string BOX{ "structure Box<Item> v: Item; end " };

    EXPECT_TRUE(MAREA_codesOf(BOX + "structure Holder a: " + MAREA_nest(64) + "; end").empty()) << "64 levels must raise nothing.";

    EXPECT_EQ(MAREA_codesOf(BOX + "structure Holder a: " + MAREA_nest(65) + "; end"), Codes{ TOO_DEEP }) << "65 levels must raise INSTANTIATION_TOO_DEEP once.";
}

TEST(MAREASemaGenericsAndContracts, TypeArgumentCountMustMatch)
{
    GTEST_COUT << "Validating the type argument count must match the generic parameter list." << std::endl;

    const std::string PAIR{ "structure Pair<First, Second> first: First; second: Second; end " };

    EXPECT_TRUE(MAREA_codesOf(PAIR + "structure H a: Pair<sinteger32, float32>; end").empty()) << "A matching count must raise nothing.";

    EXPECT_EQ(MAREA_codesOf(PAIR + "structure H a: Pair<sinteger32>; end"), Codes{ ARGUMENT_MISMATCH }) << "Too few arguments must raise GENERIC_ARGUMENT_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(PAIR + "structure H a: Pair<sinteger32, float32, string>; end"), Codes{ ARGUMENT_MISMATCH }) << "Too many arguments must raise GENERIC_ARGUMENT_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("structure Plain x: float32; end structure H a: Plain<sinteger32>; end"), Codes{ ARGUMENT_MISMATCH }) << "Arguments on a non-generic type must raise GENERIC_ARGUMENT_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("structure H a: pointer<sinteger32, sinteger32>; end"), Codes{ ARGUMENT_MISMATCH }) << "A core generic type has a fixed count.";

    EXPECT_EQ(MAREA_codesOf("structure H a: array<float32>; end"), Codes{ ARGUMENT_MISMATCH }) << "An array takes a type and a size.";

    EXPECT_EQ(MAREA_codesOf("structure H a: array<float32, 4, 4>; end"), Codes{ ARGUMENT_MISMATCH }) << "An array takes exactly two arguments.";

    EXPECT_EQ(MAREA_codesOf("structure H a: vector<float32, 4>; end"), Codes{ ARGUMENT_MISMATCH }) << "A vector takes one type and no size.";

    EXPECT_EQ(MAREA_codesOf("structure H a: vector; end"), Codes{ ARGUMENT_MISMATCH }) << "A vector takes one argument.";

    EXPECT_TRUE(MAREA_codesOf("structure H a: array<float32, 4>; b: vector<float32>; end").empty()) << "The two valid shapes must raise nothing.";

    EXPECT_EQ(MAREA_codesOf("structure H a: sinteger32<float32>; end"), Codes{ ARGUMENT_MISMATCH }) << "A primitive takes no arguments.";
}

TEST(MAREASemaGenericsAndContracts, TypeArgumentKindMustMatch)
{
    GTEST_COUT << "Validating a number cannot fill a type parameter." << std::endl;

    EXPECT_EQ(MAREA_codesOf("structure Box<Item> v: Item; end structure H a: Box<4>; end"), Codes{ ARGUMENT_MISMATCH }) << "A number for a type parameter must raise GENERIC_ARGUMENT_MISMATCH.";

    EXPECT_TRUE(MAREA_codesOf("structure Buffer<Item, constant Count: sinteger32> v: array<Item, Count>; end structure H a: Buffer<float32, 4>; end").empty()) << "A number for a value parameter must raise nothing.";
}

TEST(MAREASemaGenericsAndContracts, UnknownContractIsRejected)
{
    GTEST_COUT << "Validating a where clause that names no contract is rejected." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f<Item>(a: Item) where Item: Missing end"), Codes{ UNDECLARED }) << "An unknown contract must raise UNDECLARED_NAME.";

    EXPECT_EQ(MAREA_codesOf("function f<Item>(a: Item) where Other: Copyable end"), Codes{ UNDECLARED }) << "A constraint on a non-parameter must raise UNDECLARED_NAME.";

    EXPECT_TRUE(MAREA_codesOf(COMPARABLE + "function f<Item>(a: Item) where Item: Comparable, Copyable end").empty()) << "Known contracts must raise nothing.";
}

TEST(MAREASemaGenericsAndContracts, DirectCallArgumentCountMustMatch)
{
    GTEST_COUT << "Validating a direct call of a file function needs the right argument count." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function add(a: sinteger32, b: sinteger32): sinteger32 return a; end function run(): sinteger32 return add(1, 2); end").empty()) << "A matching count must raise nothing.";

    EXPECT_EQ(MAREA_codesOf("function add(a: sinteger32, b: sinteger32): sinteger32 return a; end function run(): sinteger32 return add(1); end"), Codes{ COUNT_MISMATCH }) << "A wrong count must raise ARGUMENT_COUNT_MISMATCH.";
}

TEST(MAREASemaGenericsAndContracts, CallOfALocalThatHidesAFunctionIsNotChecked)
{
    GTEST_COUT << "Validating a call through a local that hides a file function is not checked against that function." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f(count: sinteger32) end function g(f: function(text: string)) f(\"x\"); f(\"a\"); end").empty()) << "A parameter that hides a function must raise nothing for its own call.";

    EXPECT_TRUE(MAREA_codesOf("function f(count: sinteger32) end function g(f: function(text: string, other: string), name: string) f(name, name); end").empty()) << "A name argument and a wrong count of the hidden function must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function f(count: sinteger32) end function g<Item>(names: slice<Item>) for each f in names do f(\"x\"); end end").empty()) << "A binder that hides a function must raise nothing for its own call.";

    EXPECT_EQ(MAREA_codesOf("function f(count: sinteger32) end function g() f(\"x\"); end"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_TYPE_MISMATCH }) << "A call of the function itself must still raise TYPE_MISMATCH.";
}

TEST(MAREASemaGenericsAndContracts, ErrorNodeInWhereClauseIsSkipped)
{
    GTEST_COUT << "Validating a broken where clause raises no sema diagnostic." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f<Item>(a: Item) where end").empty()) << "A where clause without a constraint must raise no sema diagnostic.";

    EXPECT_TRUE(MAREA_codesOf("function f<Item>(a: Item) where Item: end").empty()) << "A constraint without a contract must raise no sema diagnostic.";

    EXPECT_TRUE(MAREA_codesOf("function f<Item>(a: Item) where : Copyable end").empty()) << "A constraint without a parameter must raise no sema diagnostic.";
}

TEST(MAREASemaGenericsAndContracts, ArrayIsCopyableAndVectorIsNot)
{
    GTEST_COUT << "Validating Copyable is met by an array of copyable elements and missed by a vector." << std::endl;

    const std::string COPY{ "function copy_of<Item>(a: Item): Item where Item: Copyable return copy a; end " };

    EXPECT_TRUE(MAREA_codesOf(COPY + "function run(a: array<float32, 4>): array<float32, 4> return copy_of(a); end").empty()) << "An array of primitives must be Copyable.";

    EXPECT_EQ(MAREA_codesOf(COPY + "function run(a: vector<float32>): vector<float32> return copy_of(a); end"), Codes{ NOT_SATISFIED }) << "A vector must miss Copyable.";

    EXPECT_EQ(MAREA_codesOf(COPY + "structure Plain x: float32; end function run(a: array<Plain, 4>): array<Plain, 4> return copy_of(a); end"), Codes{ NOT_SATISFIED }) << "An array of a plain structure must miss Copyable.";
}

TEST(MAREASemaGenericsAndContracts, EnumIsCopyableAndBuiltInTypesMeetUserContracts)
{
    GTEST_COUT << "Validating an enum is Copyable and a built-in type is not searched for contract functions." << std::endl;

    const std::string COPY{ "function copy_of<Item>(a: Item): Item where Item: Copyable return copy a; end " };

    EXPECT_TRUE(MAREA_codesOf(COPY + "enum Color red; end function run(c: Color): Color return copy_of(c); end").empty()) << "An enum must be Copyable.";

    EXPECT_TRUE(MAREA_codesOf(COMPARABLE + MAXIMUM + "function run(): sinteger32 return maximum<sinteger32>(4, 8); end").empty()) << "A primitive must not be searched for implementations.";
}

TEST(MAREASemaGenericsAndContracts, LiteralMustFitThePrimitiveType)
{
    GTEST_COUT << "Validating a literal that cannot have the primitive type is a type mismatch." << std::endl;

    constexpr MAREA_Uinteger32 MISMATCH{ MAREA_SEMA_DIAGNOSTIC_CODE_TYPE_MISMATCH };

    EXPECT_EQ(MAREA_codesOf("function f(a: sinteger32) end function g() f(\"text\"); end"), Codes{ MISMATCH }) << "A string for an integer must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("function f(a: string) end function g() f(true); end"), Codes{ MISMATCH }) << "A boolean for a string must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("function f(a: sinteger32) end function g() f(1.5); end"), Codes{ MISMATCH }) << "A fraction for an integer must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("function g() local x: boolean = 1; end"), Codes{ MISMATCH }) << "A number for a boolean must raise TYPE_MISMATCH.";

    EXPECT_TRUE(MAREA_codesOf("function f(a: float32, b: string, c: boolean, d: sinteger32) end function g() f(1, \"x\", true, 2); local x: float64 = 2.5; end").empty()) << "Fitting literals must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function f(a: character) end function g() f('a'); local x: character = '\\n'; end").empty()) << "A character literal for a character must raise nothing.";

    EXPECT_EQ(MAREA_codesOf("function f(a: sinteger32) end function g() f('a'); end"), Codes{ MISMATCH }) << "A character for an integer must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("function f(a: character) end function g() f(65); end"), Codes{ MISMATCH }) << "An integer for a character must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("function f(a: character) end function g() f(\"a\"); end"), Codes{ MISMATCH }) << "A string for a character must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("function f(a: string) end function g() f('a'); end"), Codes{ MISMATCH }) << "A character for a string must raise TYPE_MISMATCH.";

    EXPECT_TRUE(MAREA_codesOf("function f(a: pointer<sinteger32>) end function g() f(\"text\"); end").empty()) << "A non-primitive type must not be guessed.";
}

TEST(MAREASemaGenericsAndContracts, MissingTypeFunctionIsRejected)
{
    GTEST_COUT << "Validating a method or static call of a function that the type does not have is rejected." << std::endl;

    const std::string POINT{ "structure Point x: float64; end " };

    EXPECT_EQ(MAREA_codesOf(POINT + "function run(point: Point): float64 return point.length(); end"), Codes{ UNKNOWN_TYPE_FUNCTION }) << "A method no block declares must raise UNKNOWN_TYPE_FUNCTION.";

    EXPECT_EQ(MAREA_codesOf(POINT + "function run(): Point return Point.origin(); end"), Codes{ UNKNOWN_TYPE_FUNCTION }) << "A static call no block declares must raise UNKNOWN_TYPE_FUNCTION.";

    EXPECT_EQ(MAREA_codesOf(POINT + "implementation Point function area(self: Point): float64 return 1.0; end end function run(point: Point): float64 return point.length(); end"), Codes{ UNKNOWN_TYPE_FUNCTION }) << "A name the blocks do not declare must raise UNKNOWN_TYPE_FUNCTION.";

    EXPECT_EQ(MAREA_codesOf("enum Color red; green; end function run(): Color return Color.mix(); end"), Codes{ UNKNOWN_TYPE_FUNCTION }) << "A static call on an enum must raise UNKNOWN_TYPE_FUNCTION.";

    EXPECT_EQ(MAREA_codesOf(POINT + "function run() local point: Point = Point{ x = 1.0 }; point.length(); end"), Codes{ UNKNOWN_TYPE_FUNCTION }) << "A method on a local must raise UNKNOWN_TYPE_FUNCTION.";
}

TEST(MAREASemaGenericsAndContracts, DeclaredTypeFunctionIsAccepted)
{
    GTEST_COUT << "Validating a call of a declared function, a callback field or an enum variant raises nothing." << std::endl;

    const std::string POINT{ "structure Point x: float64; end " };

    EXPECT_TRUE(MAREA_codesOf(POINT + "implementation Point function area(self: Point): float64 return 1.0; end function origin(): Point return Point{ x = 0.0 }; end end function run(point: Point): float64 local other: Point = Point.origin(); return point.area(); end").empty()) << "A method and a static function of a block must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(POINT + "implementation Point function area(self: Point): float64 return 1.0; end end implementation Point function twice(self: Point): float64 return self.area(); end end").empty()) << "A function of another block must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("structure Box<Item> value: Item; end implementation<Item> Box<Item> function get(self: Box<Item>): Item return copy self.value; end end function run(box: Box<sinteger32>): sinteger32 return box.get(); end").empty()) << "A function of a generic block must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("structure Holder callback: function(sinteger32): sinteger32; end function run(holder: Holder): sinteger32 return holder.callback(1); end").empty()) << "A callback field must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("enum Shape Circle(radius: float64 = 1.0); Square; end function run(): Shape return Shape.Circle(2.0); end").empty()) << "An enum variant used as a constructor must raise nothing.";
}

TEST(MAREASemaGenericsAndContracts, UnseenReceiverIsNotChecked)
{
    GTEST_COUT << "Validating a call on a receiver whose functions sema cannot see raises nothing." << std::endl;

    MAREA_SemaFixture fixture([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; });

    ASSERT_EQ(fixture.MAREA_sema("namespace main; import c.lib { Thing }; function run(value: Thing) value.go(); Thing.make(); end"), MAREA_SEMA_EXCEPT_SUCCESS);

    EXPECT_EQ(fixture.MAREA_semaCodes(), Codes{}) << "A library type must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function run<Item>(value: Item) value.anything(); end").empty()) << "A generic parameter must raise nothing.";
}

TEST(MAREASemaGenericsAndContracts, NarrowedTypeArgumentRaisesMismatch)
{
    GTEST_COUT << "Validating a generic type whose type arguments differ from the declared type raises GENERIC_ARGUMENT_MISMATCH." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function check(values: array<sinteger32, 4>): sinteger32 local second: array<sinteger8, 4> = values; return 1; end"), Codes{ ARGUMENT_MISMATCH }) << "A narrowed array element must raise GENERIC_ARGUMENT_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("function check(values: array<sinteger32, 4>): sinteger32 local second: array<sinteger32, 8> = values; return 1; end"), Codes{ ARGUMENT_MISMATCH }) << "A different array size must raise GENERIC_ARGUMENT_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("function check(#[semantics = reference_read_only] values: vector<sinteger32>): sinteger32 local second: vector<sinteger8> = copy values; return 1; end"), Codes{ ARGUMENT_MISMATCH }) << "A narrowed vector element through copy must raise GENERIC_ARGUMENT_MISMATCH.";

    const std::string PAIR{ "structure Pair<First, Second> first: First; second: Second; end " };

    EXPECT_EQ(MAREA_codesOf(PAIR + "function make(): Pair<sinteger32, float64> return Pair<sinteger8, float64>{ first = 1, second = 2.0 }; end"), Codes{ ARGUMENT_MISMATCH }) << "A returned struct literal with a narrowed argument must raise GENERIC_ARGUMENT_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("function take(values: array<sinteger8, 4>): sinteger32 return 1; end function check(values: array<sinteger32, 4>): sinteger32 return take(values); end"), Codes{ ARGUMENT_MISMATCH }) << "A call argument with a narrowed type argument must raise GENERIC_ARGUMENT_MISMATCH.";
}

TEST(MAREASemaGenericsAndContracts, MatchingTypeArgumentRaisesNothing)
{
    GTEST_COUT << "Validating equal, generic, aliased and wrapped type arguments raise nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function check(values: array<sinteger32, 4>): sinteger32 local second: array<sinteger32, 4> = values; return 1; end").empty()) << "Equal type arguments must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("structure Point x: sinteger32; end function check(points: owned<vector<Point>>): sinteger32 local other: vector<Point> = move points; return 1; end").empty()) << "A move of an owned vector into a vector must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function check<Item>(values: array<Item, 4>): sinteger32 local second: array<sinteger32, 4> = values; return 1; end").empty()) << "A generic parameter argument must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("type Small = sinteger8; function check(values: array<Small, 4>): sinteger32 local second: array<sinteger8, 4> = values; return 1; end").empty()) << "An alias argument must raise nothing.";
}
