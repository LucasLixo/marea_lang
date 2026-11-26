// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

    constexpr MAREA_Uinteger32 MISMATCH{ MAREA_SEMA_DIAGNOSTIC_CODE_TYPE_MISMATCH };
    constexpr MAREA_Uinteger32 ARGUMENT_MISMATCH{ MAREA_SEMA_DIAGNOSTIC_CODE_GENERIC_ARGUMENT_MISMATCH };
    constexpr MAREA_Uinteger32 FUNCTION_MISMATCH{ MAREA_SEMA_DIAGNOSTIC_CODE_FUNCTION_TYPE_MISMATCH };

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

TEST(MAREASemaKnownTypes, ViewAndRawPointerAreKnownShapes)
{
    GTEST_COUT << "Validating a slice and a raw pointer are known shapes, as a value and as a target." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function g(s: demo.slice<float32>) local n: sinteger32 = s; end"), Codes{ MISMATCH }) << "A slice value for a primitive must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("require vulkan; function g() unsafe local p: demo.pointer<uinteger8> = vulkan.take(); local n: uinteger64 = p; end end"), Codes{ MISMATCH }) << "A raw pointer value for a primitive must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("function f(a: demo.slice<sinteger32>) end function g(s: string) f(s); end"), Codes{ MISMATCH }) << "A string for a slice parameter must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("function g() unsafe local p: demo.pointer<uinteger8> = 5; end end"), Codes{ MISMATCH }) << "A number for a raw pointer must raise TYPE_MISMATCH.";

    EXPECT_TRUE(MAREA_codesOf("function f(a: demo.slice<sinteger32>) end function g(s: demo.slice<sinteger32>) f(s); local v: demo.slice<sinteger32> = s; end").empty()) << "A slice of the same element must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("require vulkan; function g() unsafe local p: demo.pointer<uinteger8> = vulkan.take(); local q: demo.pointer<uinteger8> = p; local w: uinteger64 = convert<uinteger64>(p); end end").empty()) << "A raw pointer of the same element and a conversion of it must raise nothing.";
}

TEST(MAREASemaKnownTypes, FieldOfAnyTypeIsTypedFromItsDeclaration)
{
    GTEST_COUT << "Validating a field that is not a primitive has the type of its declaration." << std::endl;

    const std::string TYPES{ "structure Point x: sinteger32; end structure Holder inner: Point; run: function(): sinteger32; end function take(n: sinteger32) end function keep(p: Point) end " };

    EXPECT_EQ(MAREA_codesOf(TYPES + "function g(h: Holder) take(h.inner); end"), Codes{ MISMATCH }) << "A structure field for a primitive must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(TYPES + "function g(h: Holder) take(h.run); end"), Codes{ MISMATCH }) << "A function field for a primitive must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(TYPES + "function g(hs: demo.vector<Point>) take(hs[0]); end"), Codes{ MISMATCH }) << "A structure element for a primitive must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("structure Box<Item> value: Item; end function g(b: Box<float32>) local s: string = b.value; end"), Codes{ MISMATCH }) << "The field of a generic structure has the type argument of the value.";

    EXPECT_TRUE(MAREA_codesOf("structure Box<Item> value: Item; end function g(b: Box<float32>) local s: float32 = b.value; end").empty()) << "A field that fits its type argument must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(TYPES + "function g(h: Holder) keep(h.inner); take(h.inner.x); take(h.run()); end").empty()) << "Fitting fields must raise nothing.";

    EXPECT_EQ(MAREA_codesOf("structure Point x: sinteger32; end implementation Point function name(self: Self) local s: string = self.x; end end"), Codes{ MISMATCH }) << "A field of Self is typed from the implemented structure.";
}

TEST(MAREASemaKnownTypes, CallHasTheResultTypeOfItsCallee)
{
    GTEST_COUT << "Validating a method call, a static call, a construction and a call of a function value are typed." << std::endl;

    const std::string TYPES{ "structure Point x: sinteger32; end implementation Point function label(self: Self): string return \"p\"; end function origin(): Self return Point{ x = 0 }; end function twin(self: Self): Point return Point{ x = self.x }; end end function take(n: sinteger32) end function keep(p: Point) end " };

    EXPECT_EQ(MAREA_codesOf(TYPES + "function g(p: Point) take(p.label()); end"), Codes{ MISMATCH }) << "A method result of another type must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(TYPES + "function g() take(Point.origin()); end"), Codes{ MISMATCH }) << "A static result of Self must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(TYPES + "function g(p: Point) local s: string = p.twin().x; end"), Codes{ MISMATCH }) << "A field of a call result is typed.";

    EXPECT_EQ(MAREA_codesOf(TYPES + "function g() take(Point{ x = 1 }); take(Point()); end"), (Codes{ MISMATCH, MISMATCH })) << "A struct literal and a construction call have the structure type.";

    EXPECT_EQ(MAREA_codesOf("function g(run: function(): string) local n: sinteger32 = run(); end"), Codes{ MISMATCH }) << "A call of a function value has the result of its type.";

    EXPECT_TRUE(MAREA_codesOf(TYPES + "function g(p: Point) local s: string = p.label(); keep(Point.origin()); take(p.twin().x); keep(Point()); end").empty()) << "Fitting results must raise nothing.";
}

TEST(MAREASemaKnownTypes, FileConstantHasItsDeclaredType)
{
    GTEST_COUT << "Validating a constant that is not a primitive has its declared type." << std::endl;

    const std::string TYPES{ "structure Point x: sinteger32; end constant ORIGIN: Point = Point{ x = 0 }; implementation Point constant UNIT: Point = Point{ x = 1 }; end function take(n: sinteger32) end function keep(p: Point) end " };

    EXPECT_EQ(MAREA_codesOf(TYPES + "function g() take(ORIGIN); end"), Codes{ MISMATCH }) << "A structure constant for a primitive must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(TYPES + "function g() take(Point.UNIT); end"), Codes{ MISMATCH }) << "An implementation constant for a primitive must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("constant TABLE: demo.array<sinteger32, 2> = demo.array<sinteger32, 2>{ 1, 2 }; function keep(t: demo.array<sinteger8, 2>) end function g() keep(TABLE); end"), Codes{ ARGUMENT_MISMATCH }) << "The type arguments of a constant are compared.";

    EXPECT_EQ(MAREA_codesOf("function make(): demo.vector<sinteger32> return demo.vector<sinteger32>{}; end function keep(t: demo.vector<string>) end function g() keep(make()); end"), Codes{ ARGUMENT_MISMATCH }) << "The type arguments of a call result are compared.";

    EXPECT_TRUE(MAREA_codesOf(TYPES + "function g() keep(ORIGIN); take(ORIGIN.x); keep(Point.UNIT); end").empty()) << "A fitting constant must raise nothing.";
}

TEST(MAREASemaKnownTypes, FunctionTypeIsComparedForAnyExpressionThatHasOne)
{
    GTEST_COUT << "Validating a function type is compared by parameters and result for a field, a call result and a local." << std::endl;

    const std::string TYPES{ "structure Holder run: function(a: sinteger32): sinteger32; text: string; end function use(f: function(a: string): sinteger32) end " };

    EXPECT_EQ(MAREA_codesOf(TYPES + "function g(h: Holder) use(h.run); end"), Codes{ FUNCTION_MISMATCH }) << "A field of another signature must raise FUNCTION_TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(TYPES + "function bad(a: string): string return a; end function make(): function(a: string): string return bad; end function g() use(make()); end"), Codes{ FUNCTION_MISMATCH }) << "A call result of another result type must raise FUNCTION_TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(TYPES + "function g(run: function(a: string, b: string): sinteger32) use(run); end"), Codes{ FUNCTION_MISMATCH }) << "A local of another parameter count must raise FUNCTION_TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(TYPES + "function g(h: Holder) use(h.text); end"), Codes{ MISMATCH }) << "A field that is not a function must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(TYPES + "function g(n: sinteger32) use(n + 1); end"), Codes{ MISMATCH }) << "A number that is not a function must raise TYPE_MISMATCH.";

    EXPECT_TRUE(MAREA_codesOf(TYPES + "type Text = string; function same(a: Text): sinteger32 return 0; end function make(): function(a: Text): sinteger32 return same; end function g(run: function(a: string): sinteger32) use(run); use(make()); end").empty()) << "The same signature, through an alias too, must raise nothing.";
}

TEST(MAREASemaKnownTypes, DifferentGenericHeadIsRejected)
{
    GTEST_COUT << "Validating two generic types of two heads are never taken for one type." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function keep(v: demo.vector<float32>) end function g(o: demo.owned<demo.vector<float32>>) keep(o); end"), Codes{ ARGUMENT_MISMATCH }) << "An owned vector for a vector must raise GENERIC_ARGUMENT_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("function keep(v: demo.slice<float32>) end function g() unsafe local p: demo.pointer<float32> = demo.slice<float32>{}; end end"), Codes{ ARGUMENT_MISMATCH }) << "A slice for a raw pointer must raise GENERIC_ARGUMENT_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("function keep(v: demo.vector<demo.owned<float32>>) end function g(w: demo.vector<demo.slice<float32>>) keep(w); end"), Codes{ ARGUMENT_MISMATCH }) << "A nested head that differs must raise GENERIC_ARGUMENT_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("structure A<Item> x: Item; end structure B<Item> x: Item; end function keep(v: A<float32>) end function g(b: B<float32>) keep(b); end"), Codes{ ARGUMENT_MISMATCH }) << "Two generic structures of the file are two types.";

    EXPECT_EQ(MAREA_codesOf("structure A x: float32; end function keep(v: demo.vector<float32>) end function g(w: demo.vector<A>) keep(w); end"), Codes{ ARGUMENT_MISMATCH }) << "A structure element for a primitive element must raise GENERIC_ARGUMENT_MISMATCH.";

    EXPECT_TRUE(MAREA_codesOf("type Floats = demo.vector<float32>; function keep(v: demo.vector<Floats>) end function g(w: demo.vector<demo.vector<float32>>) keep(w); end").empty()) << "An alias is not compared by its head.";

    EXPECT_TRUE(MAREA_codesOf("function keep<Item>(v: demo.vector<demo.vector<float32>>) end function g<Item>(w: demo.vector<Item>) keep<Item>(w); end").empty()) << "A generic parameter in scope is not compared.";
}

TEST(MAREASemaKnownTypes, ResultOfAGenericCallIsComparedAfterBinding)
{
    GTEST_COUT << "Validating the result of a generic call has the bound type arguments." << std::endl;

    const std::string TYPES{ "function same<Item>(a: Item): Item return copy a; end function wrap<Item>(a: Item): demo.vector<Item> return demo.vector<Item>{}; end function take(n: sinteger32) end function keep(v: demo.vector<string>) end " };

    EXPECT_EQ(MAREA_codesOf(TYPES + "function g(s: string) take(same(s)); end"), Codes{ MISMATCH }) << "A result bound from an argument must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(TYPES + "function g() take(same<string>(\"x\")); end"), Codes{ MISMATCH }) << "A result bound from a written type argument must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(TYPES + "function g(n: sinteger32) keep(wrap(n)); end"), Codes{ ARGUMENT_MISMATCH }) << "A compound result with a bound argument must raise GENERIC_ARGUMENT_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("structure Box<Item> value: Item; end implementation<Item> Box<Item> function get(self: Self): Item return self.value; end end function take(n: sinteger32) end function g(b: Box<string>) take(b.get()); end"), Codes{ MISMATCH }) << "A method of a generic structure has the type argument of the value.";

    EXPECT_TRUE(MAREA_codesOf(TYPES + "function g(n: sinteger32, s: string) take(same(n)); take(same<sinteger32>(1)); keep(wrap(s)); take(same(1)); end").empty()) << "A fitting or unbound result must raise nothing.";
}

TEST(MAREASemaKnownTypes, ParameterThatMentionsAGenericIsComparedAfterBinding)
{
    GTEST_COUT << "Validating the arguments of a generic call are compared with the bound parameter types." << std::endl;

    const std::string TYPES{ "function pair<Item>(a: Item, b: Item) end function add<Item>(v: demo.vector<Item>, x: Item) end function apply<Item>(f: function(a: Item): Item, x: Item) end function label(a: string): string return a; end function twice(a: sinteger32): sinteger32 return a; end " };

    EXPECT_EQ(MAREA_codesOf(TYPES + "function g(n: sinteger32, s: string) pair(n, s); end"), Codes{ MISMATCH }) << "A second argument of another type must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(TYPES + "function g() pair<sinteger32>(\"x\", 1); end"), Codes{ MISMATCH }) << "A literal against a written type argument must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(TYPES + "function g(v: demo.vector<sinteger32>) add<string>(v, \"x\"); end"), Codes{ ARGUMENT_MISMATCH }) << "A compound parameter must raise GENERIC_ARGUMENT_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf(TYPES + "function g() apply<sinteger32>(label, 1); end"), Codes{ FUNCTION_MISMATCH }) << "A function parameter is compared with its bound types.";

    EXPECT_EQ(MAREA_codesOf(TYPES + "function g(n: sinteger32) pair(n, 2); end"), Codes{}) << "A literal that fits the bound type must raise nothing.";

    EXPECT_EQ(MAREA_codesOf(TYPES + "function g(v: demo.vector<sinteger32>, n: sinteger32) add(v, n); end"), Codes{}) << "A fitting compound parameter must raise nothing.";

    EXPECT_EQ(MAREA_codesOf(TYPES + "function g(n: sinteger32) apply<sinteger32>(twice, n); end"), Codes{}) << "A fitting function with a written type argument must raise nothing.";

    EXPECT_EQ(MAREA_codesOf(TYPES + "function g(n: sinteger32) apply(twice, n); end"), Codes{}) << "A fitting function with an inferred type argument must raise nothing.";
}
