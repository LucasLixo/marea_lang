// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

    constexpr MAREA_Uinteger32 NEEDS_WORD{ MAREA_SEMA_DIAGNOSTIC_CODE_ASSIGNMENT_NEEDS_COPY_OR_MOVE };
    constexpr MAREA_Uinteger32 ESCAPES{ MAREA_SEMA_DIAGNOSTIC_CODE_REFERENCE_ESCAPES_SCOPE };
    constexpr MAREA_Uinteger32 INVALID_BORROW{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_BORROW };
    constexpr MAREA_Uinteger32 INVALID_JUMP{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_PARALLEL_JUMP };
    constexpr MAREA_Uinteger32 EXCLUSIVE{ MAREA_SEMA_DIAGNOSTIC_CODE_EXCLUSIVE_BORROW_CONFLICT };

    const std::string SHAPE{ "structure Shape x: float32; end structure Outer inner: Shape; end function make(): Shape return Shape{ x = 1.0 }; end " };

    Codes MAREA_codesOf(const std::string &body)
    {
        MAREA_SemaFixture fixture([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; });

        if (fixture.MAREA_sema("namespace main; " + body) != MAREA_SEMA_EXCEPT_SUCCESS) {
            return { 0 };
        }

        return fixture.MAREA_semaCodes();
    }
} // namespace

TEST(MAREASemaOwnership, ParameterModesAreAccepted)
{
    GTEST_COUT << "Validating a read-only borrow, a write borrow, a forced borrow and a move-in raise nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(s: Shape) end").empty()) << "A read-only borrow must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(s: mutable Shape) end").empty()) << "A write borrow must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("#[value] structure P x: float32; end function f(p: reference P) end").empty()) << "A forced borrow of a value type must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(o: owned<Shape>) end").empty()) << "A move-in must raise nothing.";
}

TEST(MAREASemaOwnership, ReferenceOnABorrowedTypeIsRejected)
{
    GTEST_COUT << "Validating the reference word on a type that is already borrowed is rejected." << std::endl;

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(s: reference Shape) end"), Codes{ INVALID_BORROW }) << "A reference on a plain structure must raise INVALID_BORROW.";

    EXPECT_TRUE(MAREA_codesOf("function f(n: reference sinteger32) end").empty()) << "A reference on a primitive must raise nothing.";
}

TEST(MAREASemaOwnership, AssignmentOfAnExistingValueNeedsAWord)
{
    GTEST_COUT << "Validating assigning a name, a field or an element of a plain type needs copy or move." << std::endl;

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(a: mutable Shape, b: Shape) a = b; end"), Codes{ NEEDS_WORD }) << "A name must raise ASSIGNMENT_NEEDS_COPY_OR_MOVE.";

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(a: mutable Shape, o: Outer) a = o.inner; end"), Codes{ NEEDS_WORD }) << "A field must raise ASSIGNMENT_NEEDS_COPY_OR_MOVE.";

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(a: mutable Shape, items: array<Shape>) a = items[0]; end"), Codes{ NEEDS_WORD }) << "An element must raise ASSIGNMENT_NEEDS_COPY_OR_MOVE.";

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(b: Shape) local a: Shape = b; end"), Codes{ NEEDS_WORD }) << "A local initializer must raise ASSIGNMENT_NEEDS_COPY_OR_MOVE.";
}

TEST(MAREASemaOwnership, CopyAndMoveSatisfyTheRule)
{
    GTEST_COUT << "Validating copy and move satisfy the assignment rule." << std::endl;

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(a: mutable Shape, b: Shape) a = copy b; end").empty()) << "copy must satisfy the rule.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(a: mutable Shape, b: owned<Shape>) a = move b; end").empty()) << "move must satisfy the rule.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(b: Shape) local a: Shape = copy b; end").empty()) << "copy in a local initializer must satisfy the rule.";
}

TEST(MAREASemaOwnership, PrimitiveAndValueAssignmentNeedNoWord)
{
    GTEST_COUT << "Validating a primitive or a value type is assigned with a plain equal sign." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f(a: mutable sinteger32, b: sinteger32) a = b; end").empty()) << "A primitive must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("#[value] structure P x: float32; end function f(a: mutable P, b: P) a = b; end").empty()) << "A value type must raise nothing.";
}

TEST(MAREASemaOwnership, NewValuesNeedNoWord)
{
    GTEST_COUT << "Validating a call or a literal on the right side needs no word." << std::endl;

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(a: mutable Shape) a = make(); end").empty()) << "A call must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(a: mutable Shape) a = Shape{ x = 2.0 }; end").empty()) << "A literal must raise nothing.";
}

TEST(MAREASemaOwnership, CompoundAssignmentIsNotChecked)
{
    GTEST_COUT << "Validating a compound assignment raises nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f(a: mutable sinteger32, b: sinteger32) a += b; end").empty()) << "A compound assignment must raise nothing.";
}

TEST(MAREASemaOwnership, UnknownTypesAreNotGuessed)
{
    GTEST_COUT << "Validating an assignment whose type is unknown raises nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f(a: mutable array<sinteger32>, b: array<sinteger32>) a = b; end").empty()) << "A library type must raise nothing.";
}

TEST(MAREASemaOwnership, ReturningABorrowIsRejected)
{
    GTEST_COUT << "Validating a borrow parameter cannot be returned directly." << std::endl;

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(s: Shape): Shape return s; end"), Codes{ ESCAPES }) << "A read-only borrow must raise REFERENCE_ESCAPES_SCOPE.";

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(s: mutable Shape): Shape return s; end"), Codes{ ESCAPES }) << "A write borrow must raise REFERENCE_ESCAPES_SCOPE.";
}

TEST(MAREASemaOwnership, ReturningOwnedValuesIsAccepted)
{
    GTEST_COUT << "Validating a move-in parameter, a value, a copy and a local can be returned." << std::endl;

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(o: owned<Shape>): owned<Shape> return o; end").empty()) << "A move-in must be returnable.";

    EXPECT_TRUE(MAREA_codesOf("function f(n: sinteger32): sinteger32 return n; end").empty()) << "A by-value parameter must be returnable.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(s: Shape): Shape return copy s; end").empty()) << "A copy must be returnable.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(): Shape local s: Shape = make(); return s; end").empty()) << "A local must be returnable.";
}

TEST(MAREASemaOwnership, LocalShadowingABorrowIsReturnable)
{
    GTEST_COUT << "Validating a local that shadows a borrow parameter is not confined." << std::endl;

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(s: Shape): Shape local s: Shape = make(); return s; end").empty()) << "A shadowing local must be returnable.";
}

TEST(MAREASemaOwnership, ForEachVariableCannotEscape)
{
    GTEST_COUT << "Validating a for each variable cannot be returned and can be used inside its loop." << std::endl;

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(items: array<Shape>): Shape for each item in items do return item; end return make(); end"), Codes{ ESCAPES }) << "A for each variable must raise REFERENCE_ESCAPES_SCOPE.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(items: array<Shape>): Shape for each item in items do local y: float32 = item.x; end return make(); end").empty()) << "Use inside the loop must raise nothing.";
}

TEST(MAREASemaOwnership, PartitionAndWithNamesCannotEscape)
{
    GTEST_COUT << "Validating a partition part and a with name cannot be returned." << std::endl;

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(items: array<Shape>): Shape fork_join partition items into 4 as part do return part; end end return make(); end"), (Codes{ ESCAPES, INVALID_JUMP })) << "A partition part returned from its body must raise REFERENCE_ESCAPES_SCOPE and INVALID_PARALLEL_JUMP.";

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(items: array<Shape>) fork_join partition items into 4 as part do local o: Outer = Outer{ inner = part }; end end end"), Codes{ ESCAPES }) << "A partition part stored in a field must raise REFERENCE_ESCAPES_SCOPE only.";

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(items: array<Shape>): Shape with items as view do return view; end return make(); end"), Codes{ ESCAPES }) << "A with name must raise REFERENCE_ESCAPES_SCOPE.";
}

TEST(MAREASemaOwnership, ReferenceCannotBeStoredInAField)
{
    GTEST_COUT << "Validating a reference cannot be stored in a literal field or a field assignment." << std::endl;

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(s: Shape): Outer return Outer{ inner = s }; end"), Codes{ ESCAPES }) << "A literal field must raise REFERENCE_ESCAPES_SCOPE.";

    EXPECT_EQ(MAREA_codesOf(SHAPE + "function f(s: Shape, o: mutable Outer) o.inner = s; end"), (Codes{ ESCAPES, NEEDS_WORD })) << "A field assignment must raise REFERENCE_ESCAPES_SCOPE.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(s: Shape): Outer return Outer{ inner = copy s }; end").empty()) << "A copy in a field must raise nothing.";
}

TEST(MAREASemaOwnership, SameNameInTwoMutableArgumentsConflicts)
{
    GTEST_COUT << "Validating one name passed twice with a write borrow among them conflicts." << std::endl;

    const std::string TAKE{ SHAPE + "function take(a: mutable Shape, b: Shape) end function other(a: Shape, b: Shape) end " };

    EXPECT_EQ(MAREA_codesOf(TAKE + "function f(s: mutable Shape) take(s, s); end"), Codes{ EXCLUSIVE }) << "A repeated name with a write borrow must raise EXCLUSIVE_BORROW_CONFLICT.";

    EXPECT_TRUE(MAREA_codesOf(TAKE + "function f(s: Shape) other(s, s); end").empty()) << "Two read-only borrows must raise nothing.";
}

TEST(MAREASemaOwnership, ErrorNodeInPlaceOfAssignmentOrReturnIsSkipped)
{
    GTEST_COUT << "Validating a broken assignment or return raises no sema diagnostic." << std::endl;

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(a: mutable Shape) a = ; end").empty()) << "A broken assignment must raise no sema diagnostic.";

    EXPECT_TRUE(MAREA_codesOf(SHAPE + "function f(a: Shape): Shape return + ; end").empty()) << "A broken return must raise no sema diagnostic.";
}

TEST(MAREASemaOwnership, EnumsAreByValue)
{
    GTEST_COUT << "Validating an enum is copied by a plain equal sign and can be returned from a parameter." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("enum Color red; green; end function f(a: mutable Color, b: Color) a = b; end").empty()) << "An enum assignment must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("enum Color red; green; end function f(b: Color): Color return b; end").empty()) << "An enum parameter must be returnable.";
}
