// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

    constexpr MAREA_Uinteger32 EXCLUSIVE{ MAREA_SEMA_DIAGNOSTIC_CODE_EXCLUSIVE_BORROW_CONFLICT };

    constexpr MAREA_Uinteger32 CHANGED{ MAREA_SEMA_DIAGNOSTIC_CODE_VALUE_CHANGED_WHILE_BORROWED };

    const std::string HEAD{ "structure P x: sinteger32; end structure B inner: P; other: P; end function w(#[semantics = reference_mutable] a: P, #[semantics = reference_mutable] b: P) end function r(#[semantics = reference_read_only] a: P, #[semantics = reference_read_only] b: P) end function wr(#[semantics = reference_mutable] a: P, #[semantics = reference_read_only] b: B) end function wt(#[semantics = reference_mutable] a: P, b: owned<P>) end function pair(#[semantics = reference_mutable] item: P, #[semantics = reference_read_only] all: vector<P>) end function make(): P return P{ x = 1 }; end " };

    Codes MAREA_codesOf(const std::string &body)
    {
        MAREA_SemaFixture fixture([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; });

        if (fixture.MAREA_sema("namespace main; " + HEAD + body) != MAREA_SEMA_EXCEPT_SUCCESS) {
            return { 0 };
        }

        return fixture.MAREA_semaCodes();
    }
} // namespace

TEST(MAREASemaBorrowLive, OneCallThatSharesAPlaceWithAWriteBorrowIsRejected)
{
    GTEST_COUT << "Validating one call cannot pass overlapping places when one is a write borrow." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f() local b: B = B{ inner = make(), other = make() }; w(b.inner, b.inner); end"), Codes{ EXCLUSIVE }) << "One field passed twice to write borrows must raise 3019.";

    EXPECT_EQ(MAREA_codesOf("function f(#[semantics = reference_mutable] v: vector<P>) w(v[0], v[0]); end"), Codes{ EXCLUSIVE }) << "One element passed twice to write borrows must raise 3019.";

    EXPECT_EQ(MAREA_codesOf("function f() local b: B = B{ inner = make(), other = make() }; wr(b.inner, b); end"), Codes{ EXCLUSIVE }) << "A field and its owner passed to a write and a read borrow must raise 3019.";

    EXPECT_EQ(MAREA_codesOf("function f() local p: P = make(); wt(p, move p); end"), Codes{ EXCLUSIVE }) << "A value passed to a write borrow and moved in the same call must raise 3019.";

    EXPECT_EQ(MAREA_codesOf("function f(#[semantics = reference_mutable] v: vector<P>) for each item in mutable v do pair(item, v); end end"), Codes{ EXCLUSIVE }) << "A mutable loop element and its own vector in one call must raise 3019.";
}

TEST(MAREASemaBorrowLive, OneCallThatSharesNoPlaceOrOnlyReadsIsAccepted)
{
    GTEST_COUT << "Validating disjoint fields and two read borrows of one place raise nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f() local b: B = B{ inner = make(), other = make() }; w(b.inner, b.other); end").empty()) << "Two fields of one value must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function f() local b: B = B{ inner = make(), other = make() }; r(b.inner, b.inner); end").empty()) << "Two read borrows of one field must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function f(#[semantics = reference_mutable] v: vector<P>) w(v[0], v[1]); end").empty()) << "Two elements with two literal indices must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function f() local p: P = make(); wt(p, copy p); end").empty()) << "A write borrow and a copy of one value must raise nothing.";
}

TEST(MAREASemaBorrowLive, ValueChangedWhileAForEachReadsItIsRejected)
{
    GTEST_COUT << "Validating a vector cannot be moved, written or appended to while a for each walks it." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f(v: owned<vector<P>>): sinteger32 for each item in v do local other: vector<P> = move v; break; end return 1; end"), Codes{ CHANGED }) << "A move of the walked vector must raise 3073.";

    EXPECT_EQ(MAREA_codesOf("function f(#[semantics = reference_mutable] v: vector<P>): sinteger32 for each item in v do v[0] = make(); end return 1; end"), Codes{ CHANGED }) << "A write of an element of the walked vector must raise 3073.";

    EXPECT_EQ(MAREA_codesOf("function f(#[semantics = reference_mutable] v: vector<P>): sinteger32 for each item in mutable v do v.append(make()); end return 1; end"), Codes{ CHANGED }) << "An append to the walked vector must raise 3073.";
}

TEST(MAREASemaBorrowLive, SourceOfAWithBlockChangedInsideIsRejected)
{
    GTEST_COUT << "Validating the source of a with block cannot be written or moved inside the block." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f(): sinteger32 local p: P = make(); with p as view do p.x = 2; end return 1; end"), Codes{ CHANGED }) << "A write of the source must raise 3073.";

    EXPECT_EQ(MAREA_codesOf("function f(): sinteger32 local p: P = make(); with p as view do local o: P = move p; end return 1; end"), Codes{ CHANGED }) << "A move of the source must raise 3073.";
}

TEST(MAREASemaBorrowLive, BorrowThatHasEndedOrOtherValuesAreAccepted)
{
    GTEST_COUT << "Validating a change after the block, of another value, or through the loop name raises nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f(#[semantics = reference_mutable] v: vector<P>): sinteger32 for each item in v do end v[0] = make(); return 1; end").empty()) << "A write after the loop has ended must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function f(#[semantics = reference_mutable] v: vector<P>): sinteger32 for each item in mutable v do item.x = 2; end return 1; end").empty()) << "A write through the loop name must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function f(v: vector<P>): sinteger32 local q: P = make(); for each item in v do q.x = 2; end return 1; end").empty()) << "A write of another local must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function f(): sinteger32 local p: P = make(); with p as view do end p.x = 2; return 1; end").empty()) << "A write after the with block must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function f(): sinteger32 with make() as view do view.x = 2; end return 1; end").empty()) << "A with over a call result may write its own name.";
}
