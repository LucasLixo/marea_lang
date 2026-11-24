// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

    const std::string SHAPES{ "structure Point x: sinteger32; y: sinteger32; end structure Box inner: Point; count: sinteger32; end function make_point(): Point return Point{ x = 1, y = 2 }; end " };

    Codes MAREA_codesOf(const std::string &body)
    {
        MAREA_SemaFixture fixture([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; });

        if (fixture.MAREA_sema("namespace main; import c.array; " + SHAPES + body) != MAREA_SEMA_EXCEPT_SUCCESS) {
            return { 0 };
        }

        return fixture.MAREA_semaCodes();
    }

    const Codes ONE{ MAREA_SEMA_DIAGNOSTIC_CODE_WRITE_THROUGH_READ_ONLY_BORROW };
    const Codes NONE{};
} // namespace

TEST(MAREASemaReadOnlyBorrow, AssignmentThroughAReadOnlyParameterIsRejected)
{
    GTEST_COUT << "Validating that a field, a nested field, a whole name, an element and a compound assignment of a read-only parameter each report once." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f(#[semantics = reference_read_only] p: Point) p.x = 1; end"), ONE) << "A field write must be rejected.";

    EXPECT_EQ(MAREA_codesOf("function f(#[semantics = reference_read_only] b: Box) b.inner.x = 1; end"), ONE) << "A nested field write must be rejected.";

    EXPECT_EQ(MAREA_codesOf("function f(#[semantics = reference_read_only] p: Point) p = Point{ x = 1, y = 2 }; end"), ONE) << "A whole-name write must be rejected.";

    EXPECT_EQ(MAREA_codesOf("function f(#[semantics = reference_read_only] p: Point) p.x += 1; end"), ONE) << "A compound write must be rejected.";

    EXPECT_EQ(MAREA_codesOf("function f(#[semantics = reference_read_only] p: c.array<sinteger32>) p[0] = 1; end"), ONE) << "An element write must be rejected.";
}

TEST(MAREASemaReadOnlyBorrow, AssignmentThroughAWritableRootIsAccepted)
{
    GTEST_COUT << "Validating that a mutable parameter, a local, a primitive parameter and a shadowing local raise nothing." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f(#[semantics = reference_mutable] p: Point) p.x = 1; end"), NONE) << "A mutable parameter is writable.";

    EXPECT_EQ(MAREA_codesOf("function f() local p: Point = make_point(); p.x = 1; end"), NONE) << "A local is writable.";

    EXPECT_EQ(MAREA_codesOf("function f(#[semantics = value_read_only] n: sinteger32) n = 1; end"), NONE) << "A primitive parameter is by value.";

    EXPECT_EQ(MAREA_codesOf("function f(#[semantics = reference_read_only] p: Point) do local p: Point = make_point(); p.x = 1; end end"), NONE) << "A local that shadows a read-only parameter is writable.";
}

TEST(MAREASemaReadOnlyBorrow, ParameterWithNoAttributeFollowsItsDefault)
{
    GTEST_COUT << "Validating that a structure parameter with no attribute is read-only and a move-in parameter is not." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f(p: Point) p.x = 1; end"), ONE) << "A default structure parameter is a read-only borrow.";

    EXPECT_EQ(MAREA_codesOf("function f(o: owned<Point>) o.x = 1; end"), NONE) << "An owned parameter is a move-in type.";

    EXPECT_EQ(MAREA_codesOf("function f(v: vector<Point>) v[0] = make_point(); end"), NONE) << "A vector parameter is a move-in type.";
}

TEST(MAREASemaReadOnlyBorrow, ForEachVariableOverAReadOnlySourceIsReadOnly)
{
    GTEST_COUT << "Validating that the variable of a for each over a read-only source cannot be written." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f(xs: c.array<Point>) for each v in xs do v.x = 1; end end"), ONE) << "A for each variable is read-only without mutable.";
}

TEST(MAREASemaReadOnlyBorrow, WithNameOverAnExistingValueIsReadOnly)
{
    GTEST_COUT << "Validating that a with name over a name is read-only and over a call is writable." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f() local p: Point = make_point(); with p as v do v.x = 1; end end"), ONE) << "A view of an existing value is read-only.";

    EXPECT_EQ(MAREA_codesOf("function f() with make_point() as v do v.x = 1; end end"), NONE) << "A with over a call is writable.";
}
