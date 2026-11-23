// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

    constexpr MAREA_Uinteger32 CALLED_DIRECTLY{ MAREA_SEMA_DIAGNOSTIC_CODE_LIFECYCLE_OPERATOR_CALLED_DIRECTLY };
    constexpr MAREA_Uinteger32 NEEDS_CONCRETE_TYPE{ MAREA_SEMA_DIAGNOSTIC_CODE_LIFECYCLE_OPERATOR_NEEDS_CONCRETE_TYPE };
    constexpr MAREA_Uinteger32 BAD_SHAPE{ MAREA_SEMA_DIAGNOSTIC_CODE_LIFECYCLE_OPERATOR_BAD_SHAPE };
    constexpr MAREA_Uinteger32 UNDECLARED{ MAREA_SEMA_DIAGNOSTIC_CODE_UNDECLARED_NAME };
    constexpr MAREA_Uinteger32 DEFAULT_BEFORE_REQUIRED{ MAREA_SEMA_DIAGNOSTIC_CODE_DEFAULT_PARAMETER_BEFORE_REQUIRED };

    const std::string POINT{ "structure Point x: sinteger32; end " };

    Codes MAREA_codesOf(const std::string &body)
    {
        MAREA_SemaFixture fixture([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; });

        if (fixture.MAREA_sema("namespace main; " + body) != MAREA_SEMA_EXCEPT_SUCCESS) {
            return { 0 };
        }

        return fixture.MAREA_semaCodes();
    }
} // namespace

TEST(MAREASemaLifecycle, WellFormedPairRaisesNothing)
{
    GTEST_COUT << "Validating a well formed construct and destroy pair raises nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf(POINT + "implementation Point #[operator = \"construct\"] function make(x: sinteger32 = 0) end #[operator = \"destroy\"] function drop() end end").empty()) << "A defaulted construct and a bare destroy must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(POINT + "implementation Point #[operator = \"construct\"] function make() end end").empty()) << "A construct with no parameter must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("enum Color red; green; end implementation Color #[operator = \"destroy\"] function drop() end end").empty()) << "A destroy in the implementation of an enum must raise nothing.";
}

TEST(MAREASemaLifecycle, OtherOperatorsAreNotChecked)
{
    GTEST_COUT << "Validating other operators and plain functions keep any shape." << std::endl;

    EXPECT_TRUE(MAREA_codesOf(POINT + "implementation Point #[operator = \"+\"] function add(a: Point, b: Point): Point return copy a; end function area(a: Point): sinteger32 return 1; end end").empty()) << "A non lifecycle function must raise nothing.";
}

TEST(MAREASemaLifecycle, DestroyWithParameterIsBadShape)
{
    GTEST_COUT << "Validating a destroy with a parameter is a bad shape." << std::endl;

    EXPECT_EQ(MAREA_codesOf(POINT + "implementation Point #[operator = \"destroy\"] function drop(a: sinteger32) end end"), Codes{ BAD_SHAPE }) << "A destroy with a parameter must raise BAD_SHAPE.";
}

TEST(MAREASemaLifecycle, DestroyWithReturnIsBadShape)
{
    GTEST_COUT << "Validating a destroy with a return type is a bad shape." << std::endl;

    EXPECT_EQ(MAREA_codesOf(POINT + "implementation Point #[operator = \"destroy\"] function drop(): sinteger32 return 1; end end"), Codes{ BAD_SHAPE }) << "A destroy with a return type must raise BAD_SHAPE.";
}

TEST(MAREASemaLifecycle, ConstructWithReturnIsBadShape)
{
    GTEST_COUT << "Validating a construct with a return type is a bad shape." << std::endl;

    EXPECT_EQ(MAREA_codesOf(POINT + "implementation Point #[operator = \"construct\"] function make(): sinteger32 return 1; end end"), Codes{ BAD_SHAPE }) << "A construct with a return type must raise BAD_SHAPE.";
}

TEST(MAREASemaLifecycle, ConstructWithRequiredParameterIsBadShape)
{
    GTEST_COUT << "Validating a construct with a parameter that has no default is a bad shape." << std::endl;

    EXPECT_EQ(MAREA_codesOf(POINT + "implementation Point #[operator = \"construct\"] function make(x: sinteger32) end end"), Codes{ BAD_SHAPE }) << "A required parameter must raise BAD_SHAPE.";

    EXPECT_EQ(MAREA_codesOf(POINT + "implementation Point #[operator = \"construct\"] function make(x: sinteger32 = 1, y: sinteger32) end end"), (Codes{ BAD_SHAPE, DEFAULT_BEFORE_REQUIRED })) << "A required parameter after a defaulted one must raise BAD_SHAPE once.";
}

TEST(MAREASemaLifecycle, TwoShapeFaultsRaiseOnce)
{
    GTEST_COUT << "Validating a function with two shape faults raises one diagnostic." << std::endl;

    EXPECT_EQ(MAREA_codesOf(POINT + "implementation Point #[operator = \"construct\"] function make(x: sinteger32): sinteger32 return 1; end end"), Codes{ BAD_SHAPE }) << "One function must raise BAD_SHAPE once.";
}

TEST(MAREASemaLifecycle, TopLevelFunctionNeedsAnImplementation)
{
    GTEST_COUT << "Validating a lifecycle function outside an implementation is rejected." << std::endl;

    EXPECT_EQ(MAREA_codesOf("#[operator = \"construct\"] function make() end"), Codes{ NEEDS_CONCRETE_TYPE }) << "A top level construct must raise NEEDS_CONCRETE_TYPE.";

    EXPECT_EQ(MAREA_codesOf("#[operator = \"destroy\"] function drop() end"), Codes{ NEEDS_CONCRETE_TYPE }) << "A top level destroy must raise NEEDS_CONCRETE_TYPE.";
}

TEST(MAREASemaLifecycle, GenericImplementationIsRejected)
{
    GTEST_COUT << "Validating a lifecycle function of a generic implementation is rejected." << std::endl;

    EXPECT_EQ(MAREA_codesOf("structure Box<Item> inner: Item; end implementation<Item> Box<Item> #[operator = \"destroy\"] function drop() end end"), Codes{ NEEDS_CONCRETE_TYPE }) << "A generic implementation must raise NEEDS_CONCRETE_TYPE.";
}

TEST(MAREASemaLifecycle, GenericFunctionIsRejected)
{
    GTEST_COUT << "Validating a generic lifecycle function is rejected." << std::endl;

    EXPECT_EQ(MAREA_codesOf(POINT + "implementation Point #[operator = \"construct\"] function make<Item>() end end"), Codes{ NEEDS_CONCRETE_TYPE }) << "A generic function must raise NEEDS_CONCRETE_TYPE.";
}

TEST(MAREASemaLifecycle, WrongContainerIsRejected)
{
    GTEST_COUT << "Validating a lifecycle function of a type that is not a structure or an enum is rejected." << std::endl;

    EXPECT_EQ(MAREA_codesOf("type Id = sinteger32; implementation Id #[operator = \"destroy\"] function drop() end end"), Codes{ NEEDS_CONCRETE_TYPE }) << "An implementation of an alias must raise NEEDS_CONCRETE_TYPE.";
}

TEST(MAREASemaLifecycle, DirectCallByNameIsRejected)
{
    GTEST_COUT << "Validating a direct call of a lifecycle function by name is rejected." << std::endl;

    const std::string TYPE{ POINT + "implementation Point #[operator = \"construct\"] function make() end #[operator = \"destroy\"] function drop() end end " };

    EXPECT_EQ(MAREA_codesOf(TYPE + "function run() make(); end"), (Codes{ UNDECLARED, CALLED_DIRECTLY })) << "A call of construct by name must raise CALLED_DIRECTLY.";

    EXPECT_EQ(MAREA_codesOf(TYPE + "function run() drop(); end"), (Codes{ UNDECLARED, CALLED_DIRECTLY })) << "A call of destroy by name must raise CALLED_DIRECTLY.";

    EXPECT_EQ(MAREA_codesOf("#[operator = \"destroy\"] function drop() end function run() drop(); end"), (Codes{ NEEDS_CONCRETE_TYPE, CALLED_DIRECTLY })) << "A call of a top level lifecycle function must raise CALLED_DIRECTLY.";
}

TEST(MAREASemaLifecycle, DirectCallByStaticFormIsRejected)
{
    GTEST_COUT << "Validating a direct call of a lifecycle function by the static form is rejected." << std::endl;

    const std::string TYPE{ POINT + "implementation Point #[operator = \"construct\"] function make() end #[operator = \"destroy\"] function drop() end end " };

    EXPECT_EQ(MAREA_codesOf(TYPE + "function run() Point.make(); end"), Codes{ CALLED_DIRECTLY }) << "Type.construct() must raise CALLED_DIRECTLY.";

    EXPECT_EQ(MAREA_codesOf(TYPE + "function run() Point.drop(); end"), Codes{ CALLED_DIRECTLY }) << "Type.destroy() must raise CALLED_DIRECTLY.";
}

TEST(MAREASemaLifecycle, OrdinaryCallsAreNotRejected)
{
    GTEST_COUT << "Validating a call of an ordinary function or a hidden name is not rejected." << std::endl;

    const std::string TYPE{ POINT + "implementation Point #[operator = \"destroy\"] function drop() end function area() end end " };

    EXPECT_TRUE(MAREA_codesOf(TYPE + "function run() Point.area(); end").empty()) << "A call of a plain member must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(TYPE + "function drop() end function run() drop(); end").empty()) << "A top level function of the same name hides the member.";
}
