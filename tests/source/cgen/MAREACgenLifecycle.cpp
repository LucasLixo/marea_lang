// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAParityHelpers.hpp"

#include <string>
#include <vector>

namespace
{
    const std::string MAREA_HANDLE{ "namespace main; require demo.host as host; structure Handle id: sinteger32; end implementation Handle #[operator = \"construct\"] function open(id: sinteger32 = 7) host.record(id); end #[operator = \"destroy\"] function close() host.record(-1); end end " };

    MAREA_ParityResult MAREA_run(const std::string &source, const std::vector<MAREA_ParityCall> &calls = {})
    {
        const MAREA_ParityHost HOST{ MAREA_parityHost() };

        return MAREA_parityGenerated(source, calls, &HOST);
    }

    std::vector<std::int64_t> MAREA_log(const std::vector<std::int64_t> &entries)
    {
        return entries;
    }
} // namespace

TEST(MAREACgenLifecycle, LocalsAreDestroyedInTheReverseOrderOfTheirCreation)
{
    GTEST_COUT << "Validating the last local made is the first one destroyed, after the last statement." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_run("namespace main; require demo.host as host; structure First id: sinteger32; end structure Second id: sinteger32; end implementation First #[operator = \"destroy\"] function close() host.record(1); end end implementation Second #[operator = \"destroy\"] function close() host.record(2); end end function f(): sinteger32 local a: First = First{ id = 1 }; local b: Second = Second{ id = 2 }; host.record(9); return 5; end") };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(RESULT.generated[0].log, MAREA_log({ 9, 2, 1 })) << "Second is destroyed first.";

    EXPECT_EQ(MAREA_parityValue(RESULT.generated[0]), 5) << "The value is kept.";
}

TEST(MAREACgenLifecycle, ConstructRunsAtEveryCreationForm)
{
    GTEST_COUT << "Validating Type{...} and Type(args) run the construct operator once, with its defaults for a literal." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_run(MAREA_HANDLE + "function f(): sinteger32 local a: Handle = Handle{ id = 3 }; local b: Handle = Handle(5); local c: Handle = Handle(); return (a.id * 100) + (b.id * 10) + c.id; end") };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(RESULT.generated[0].log, MAREA_log({ 7, 5, 7, -1, -1, -1 })) << "Three constructs, then three destroys.";

    EXPECT_EQ(MAREA_parityValue(RESULT.generated[0]), 300) << "Only the literal sets a field: the others are zero.";
}

TEST(MAREACgenLifecycle, AMovedValueIsDestroyedOnce)
{
    GTEST_COUT << "Validating a moved local is not destroyed and the local that took it is." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_run(MAREA_HANDLE + "function f(): sinteger32 local first: Handle = Handle{ id = 1 }; local second: Handle = move first; host.record(9); return second.id; end") };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(RESULT.generated[0].log, MAREA_log({ 7, 9, -1 })) << "One destroy.";

    EXPECT_EQ(MAREA_parityValue(RESULT.generated[0]), 1) << "The value moved with its fields.";
}

TEST(MAREACgenLifecycle, EveryExitOfABlockRunsItsDestroys)
{
    GTEST_COUT << "Validating a loop turn, a continue, a break, a scope block and a return run the destroys, and a return value is read before them." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_run(MAREA_HANDLE + "function value(): sinteger32 host.record(50); return 4; end function f(n: sinteger32): sinteger32 local outer: Handle = Handle(100); for i in 0 .. n do local h: Handle = Handle(i); if i == 1 then continue; end if i == 2 then break; end end do local inner: Handle = Handle(200); end return value(); end", { MAREA_parityCall("f", "sinteger32", { MAREA_parityInt(3) }) }) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(RESULT.generated[0].log, MAREA_log({ 100, 0, -1, 1, -1, 2, -1, 200, -1, 50, -1 })) << "Each exit destroys what the block made.";

    EXPECT_EQ(MAREA_parityValue(RESULT.generated[0]), 4) << "The value of the return.";
}

TEST(MAREACgenLifecycle, ATrapInAConstructStopsTheRunWithNoDestroy)
{
    GTEST_COUT << "Validating a fault inside a construct stops the run: nothing after it runs, and nothing is destroyed." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_run("namespace main; require demo.host as host; structure Handle id: sinteger32; end implementation Handle #[operator = \"construct\"] function open(id: sinteger32 = 0) host.record(10 / id); end #[operator = \"destroy\"] function close() host.record(-1); end end function f(): sinteger32 local a: Handle = Handle(5); local b: Handle = Handle(0); host.record(9); return 1; end") };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(RESULT.generated[0].trap, 1) << "The division by zero.";

    EXPECT_EQ(RESULT.generated[0].log, MAREA_log({ 2 })) << "Only the first construct ran.";
}

TEST(MAREACgenLifecycle, ATypeWithNoConstructIsZeroFilled)
{
    GTEST_COUT << "Validating Type() of a type with no construct operator is the zero value and its destroy still runs." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_run("namespace main; require demo.host as host; structure Plain id: sinteger32; end structure Guard id: sinteger32; end implementation Guard #[operator = \"destroy\"] function close() host.record(-1); end end function f(): sinteger32 local a: Plain = Plain(); local b: Guard = Guard{ id = 6 }; return a.id + b.id; end") };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.generated[0]), 6) << "0 + 6.";

    EXPECT_EQ(RESULT.generated[0].log, MAREA_log({ -1 })) << "The guard is destroyed.";
}

TEST(MAREACgenLifecycle, AHiddenLocalNeverCollidesWithAUserName)
{
    GTEST_COUT << "Validating the hidden local of a returned value and a user local of a similar name stay apart." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_run(MAREA_HANDLE + "function f(): sinteger32 local a: Handle = Handle{ id = 1 }; local _return0: sinteger32 = 10; return _return0 + a.id; end") };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.generated[0]), 11) << "10 + 1.";
}

TEST(MAREACgenLifecycle, AConstructThatCannotBeToldApartIsReported)
{
    GTEST_COUT << "Validating cgen never guesses the construct operator: two functions of its shape are a diagnostic." << std::endl;

    EXPECT_EQ(MAREA_parityRejected("namespace main; structure Handle id: sinteger32; end implementation Handle #[operator = \"construct\"] function open(id: sinteger32 = 0) end function reset(id: sinteger32 = 0) end end function f(): sinteger32 local a: Handle = Handle(1); return a.id; end").count(MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT), 1U) << "The construct of Handle cannot be told from reset.";
}
