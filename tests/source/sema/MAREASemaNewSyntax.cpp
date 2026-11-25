// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

TEST(MAREASemaNewSyntax, NodesOfTheNewSyntaxDoNotStopSema)
{
    GTEST_COUT << "Validating sema runs to the end over a require, a wildcard, a do-while and a when item, which the parser builds and sema does not check yet." << std::endl;

    const std::vector<std::string> SOURCES{
        "require c.stdio; function f() end",
        "require c.*; function f() end",
        "require marea.math as math; function f() end",
        "import game.*; function f() end",
        "function f() do local x: sinteger32 = 1; while true end end",
        "function f(a: boolean) do while a do end while a end end",
        "function f() do local x: sinteger32 = 1; while x == 1 end end",
        "#[when(architecture = \"x86_64\", inline = force)] function f() end",
        "#![when(not(feature = \"sse2\"), visibility = public)] function f() end",
    };

    for (const std::string &source : SOURCES) {
        MAREA_SemaFixture fixture([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; });

        EXPECT_EQ(fixture.MAREA_sema("namespace main; " + source), MAREA_SEMA_EXCEPT_SUCCESS) << "Sema must finish without an except for: " << source;
    }
}

TEST(MAREASemaNewSyntax, NodesOfTheNewSyntaxWithAResolverThatKnowsNothing)
{
    GTEST_COUT << "Validating sema runs to the end over the new syntax when the resolver answers not found for every namespace." << std::endl;

    const std::vector<std::string> SOURCES{
        "require c.stdio; function f() end",
        "import game.*; function f() end",
        "function f() do while true end end",
    };

    for (const std::string &source : SOURCES) {
        MAREA_SemaFixture fixture;

        EXPECT_EQ(fixture.MAREA_sema("namespace main; " + source), MAREA_SEMA_EXCEPT_SUCCESS) << "Sema must finish without an except for: " << source;
    }
}
