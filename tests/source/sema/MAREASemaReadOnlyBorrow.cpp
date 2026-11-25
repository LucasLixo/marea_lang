// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREALibraryRegistryHelpers.hpp"
#include "tests/MAREASemaHelpers.hpp"

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

    const std::string SHAPES{ "structure Point x: sinteger32; y: sinteger32; end structure Box inner: Point; count: sinteger32; end function make_point(): Point return Point{ x = 1, y = 2 }; end " };

    Codes MAREA_codesOf(const std::string &body)
    {
        MAREA_SemaFixture fixture([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; });

        MAREA_IGNORE(MAREA_SemaContextSetTypeRegistry(fixture.context, MAREA_buildLibraryRegistry(fixture.parser.lexer.arena)));

        if (fixture.MAREA_sema("namespace main; import c.array as array; require demo.owned; require demo.vector; " + SHAPES + body) != MAREA_SEMA_EXCEPT_SUCCESS) {
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

    EXPECT_EQ(MAREA_codesOf("function f(o: demo.owned<Point>) o.x = 1; end"), NONE) << "An owned parameter is a move-in type.";

    EXPECT_EQ(MAREA_codesOf("function f(v: demo.vector<Point>) v[0] = make_point(); end"), NONE) << "A vector parameter is a move-in type.";
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

TEST(MAREASemaReadOnlyBorrow, MutableForEachNeedsAWritableSource)
{
    GTEST_COUT << "Validating that for each in mutable over a read-only source reports once and over a writable one reports nothing." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f(#[semantics = reference_read_only] xs: c.array<Point>) for each v in mutable xs do end end"), ONE) << "A read-only source must be rejected.";

    EXPECT_EQ(MAREA_codesOf("function f(xs: c.array<Point>) for each v in mutable xs do end end"), ONE) << "A default read-only source must be rejected.";

    EXPECT_EQ(MAREA_codesOf("function f(#[semantics = reference_mutable] xs: c.array<Point>) for each v in mutable xs do v.x = 1; end end"), NONE) << "A mutable source must be accepted and its variable written.";

    EXPECT_EQ(MAREA_codesOf("function f(#[semantics = reference_read_only] b: Box) for each v in mutable b.inner do end end"), ONE) << "A field of a read-only name is read-only.";
}

TEST(MAREASemaReadOnlyBorrow, ReadOnlyBorrowGivenToAWriteBorrowIsRejected)
{
    GTEST_COUT << "Validating that a read-only borrow, or a part of one, passed to a write-borrow parameter reports once." << std::endl;

    const std::string HEAD{ "function write_point(#[semantics = reference_mutable] p: Point) p.x = 1; end function bump<Item>(#[semantics = reference_mutable] value: Item) end " };

    EXPECT_EQ(MAREA_codesOf(HEAD + "function f(#[semantics = reference_read_only] p: Point) write_point(p); end"), ONE) << "A read-only parameter must be rejected.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "function f(p: Point) write_point(p); end"), ONE) << "A parameter with no attribute is read-only.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "function f(#[semantics = reference_read_only] b: Box) write_point(b.inner); end"), ONE) << "A field of a read-only name must be rejected.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "function f(#[semantics = reference_read_only] ps: c.array<Point>) write_point(ps[0]); end"), ONE) << "An element of a read-only name must be rejected.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "function f(#[semantics = reference_read_only] p: Point) bump(p); end"), ONE) << "A generic write borrow must be checked too.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "function f() local p: Point = make_point(); write_point(p); end"), NONE) << "A local must be accepted.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "function f(#[semantics = reference_mutable] p: Point) write_point(p); end"), NONE) << "A mutable parameter must be accepted.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "import c.task as task; function f(#[semantics = reference_read_only] p: Point) fork_join spawn write_point(p); end end"), ONE) << "A spawn of a read-only parameter must be rejected.";
}

TEST(MAREASemaReadOnlyBorrow, TemporaryGivenToAWriteBorrowIsRejected)
{
    GTEST_COUT << "Validating that a call result or a literal passed to a write-borrow parameter reports once and an owned parameter accepts it." << std::endl;

    const std::string HEAD{ "function write_point(#[semantics = reference_mutable] p: Point) p.x = 1; end function take(o: owned<Point>) end function set(#[semantics = reference_mutable] n: sinteger32) end " };

    EXPECT_EQ(MAREA_codesOf(HEAD + "function f() write_point(make_point()); end"), ONE) << "A call result must be rejected.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "function f() write_point(Point{ x = 1, y = 2 }); end"), ONE) << "A literal value must be rejected.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "function f() set(1); end"), ONE) << "A scalar literal must be rejected.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "function f() take(make_point()); end"), NONE) << "An owned parameter must accept a call result.";
}

TEST(MAREASemaReadOnlyBorrow, PartitionMutableNeedsAWritableSource)
{
    GTEST_COUT << "Validating that partition mutable over a read-only source reports once and the part of a writable one may be written." << std::endl;

    const std::string HEAD{ "import c.slice as slice; import c.task as task; function scale(#[semantics = reference_mutable] part: c.slice<float32>) end " };

    EXPECT_EQ(MAREA_codesOf(HEAD + "function f(#[semantics = reference_read_only] values: c.array<float32>) fork_join partition mutable values into 2 as part do end end end"), ONE) << "A read-only source must be rejected.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "function f(#[semantics = reference_mutable] values: c.array<float32>) fork_join partition mutable values into 2 as part do for each v in mutable part do v = 1.0; end end end end"), NONE) << "A writable source must be accepted.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "function f(#[semantics = reference_mutable] values: c.array<float32>) fork_join partition mutable values into 2 as part do scale(part); end end end"), NONE) << "A writable part may be passed to a write borrow.";

    EXPECT_EQ(MAREA_codesOf(HEAD + "function f(#[semantics = reference_mutable] values: c.array<float32>) fork_join partition values into 2 as part do for each v in mutable part do end end end end"), ONE) << "The part of a partition with no mutable is read-only.";
}
