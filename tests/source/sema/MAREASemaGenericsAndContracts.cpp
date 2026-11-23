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
    GTEST_COUT << "Validating a value generic is usable as a vector size." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("structure Buffer<Item, constant Count: sinteger32> items: vector<Item, Count>; end").empty()) << "A value generic must raise nothing.";
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

TEST(MAREASemaGenericsAndContracts, CopyableNeedsAValueType)
{
    GTEST_COUT << "Validating Copyable is met by a value type and primitives and missed by a plain structure." << std::endl;

    const std::string COPY{ "function copy_of<Item>(a: Item): Item where Item: Copyable return copy a; end " };

    EXPECT_TRUE(MAREA_codesOf(COPY + "#[value] structure Point x: float32; end function run(p: Point): Point return copy_of(p); end").empty()) << "A value type must be Copyable.";

    EXPECT_TRUE(MAREA_codesOf(COPY + "function run(): sinteger32 return copy_of<sinteger32>(1); end").empty()) << "A primitive must be Copyable.";

    EXPECT_EQ(MAREA_codesOf(COPY + "structure Plain x: float32; end function run(p: Plain): Plain return copy_of(p); end"), Codes{ NOT_SATISFIED }) << "A plain structure must miss Copyable.";
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

    EXPECT_EQ(MAREA_codesOf("structure H a: vector<float32>; end"), Codes{ ARGUMENT_MISMATCH }) << "A vector takes a type and a size.";

    EXPECT_EQ(MAREA_codesOf("structure H a: sinteger32<float32>; end"), Codes{ ARGUMENT_MISMATCH }) << "A primitive takes no arguments.";
}

TEST(MAREASemaGenericsAndContracts, TypeArgumentKindMustMatch)
{
    GTEST_COUT << "Validating a number cannot fill a type parameter." << std::endl;

    EXPECT_EQ(MAREA_codesOf("structure Box<Item> v: Item; end structure H a: Box<4>; end"), Codes{ ARGUMENT_MISMATCH }) << "A number for a type parameter must raise GENERIC_ARGUMENT_MISMATCH.";

    EXPECT_TRUE(MAREA_codesOf("structure Buffer<Item, constant Count: sinteger32> v: vector<Item, Count>; end structure H a: Buffer<float32, 4>; end").empty()) << "A number for a value parameter must raise nothing.";
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

    EXPECT_TRUE(MAREA_codesOf("function f(count: sinteger32) end function g(names: slice<string>) for each f in names do f(\"x\"); end end").empty()) << "A binder that hides a function must raise nothing for its own call.";

    EXPECT_EQ(MAREA_codesOf("function f(count: sinteger32) end function g() f(\"x\"); end"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_TYPE_MISMATCH }) << "A call of the function itself must still raise TYPE_MISMATCH.";
}

TEST(MAREASemaGenericsAndContracts, ErrorNodeInWhereClauseIsSkipped)
{
    GTEST_COUT << "Validating a broken where clause raises no sema diagnostic." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f<Item>(a: Item) where end").empty()) << "A where clause without a constraint must raise no sema diagnostic.";

    EXPECT_TRUE(MAREA_codesOf("function f<Item>(a: Item) where Item: end").empty()) << "A constraint without a contract must raise no sema diagnostic.";

    EXPECT_TRUE(MAREA_codesOf("function f<Item>(a: Item) where : Copyable end").empty()) << "A constraint without a parameter must raise no sema diagnostic.";
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

    EXPECT_TRUE(MAREA_codesOf("function f(a: pointer<sinteger32>) end function g() f(\"text\"); end").empty()) << "A non-primitive type must not be guessed.";
}
