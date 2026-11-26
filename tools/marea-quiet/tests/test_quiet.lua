-- Copyright (c) 2026 Paulo Lucas Alves Franca
-- Licensed under the MIT License. See LICENSE.txt for full terms.

--- @script test_quiet
-- Checks which lines of a build or test log marea-quiet keeps, that it passes the exit code on, and
-- that nothing outside its two commands can be run through it.

local script_dir = (arg[0]:gsub("\\", "/")):match("(.*/)") or "./"
package.path = script_dir .. "../?.lua;" .. package.path

local quiet = require("quiet")

local function kept(log, limit)
   local lines, omitted = quiet.filter(log, limit or 100)
   return table.concat(lines, "\n"), omitted
end

-- Progress noise goes, including a file name that merely contains "Error" or "Warning".
assert(kept("[1/9] Building C object MAREAErrorWarning.c.o\n-- Configuring done\n") == "")

-- Compiler, linker, ninja and CMake diagnostics stay; a repeated one stays once.
local warning = "a.c:3:5: warning: unused variable 'x' [-Wunused-variable]"
assert(kept(warning .. "\n" .. warning .. "\n") == warning)
assert(kept("a.c:9:1: error: expected ';'\n") == "a.c:9:1: error: expected ';'")
assert(kept("ld.lld: error: undefined symbol: foo\n") == "ld.lld: error: undefined symbol: foo")
assert(kept("FAILED: lib/a.o\n") == "FAILED: lib/a.o")
assert(
   kept("CMake Warning at a.cmake:4 (message):\n  Old policy.\n\n-- ok\n")
      == "CMake Warning at a.cmake:4 (message):\n  Old policy."
)

-- ctest and GoogleTest failures stay, with the assertion text.
local failure = table.concat({
   "      Start  1: Version.Major",
   " 1/2 Test  #1: Version.Major ......   Passed    0.01 sec",
   " 2/2 Test  #2: Version.Minor ......***Failed    0.01 sec",
   "[ RUN      ] Version.Minor",
   "tests/a.cpp:12: Failure",
   "Expected equality of these values:",
   "  1",
   "  2",
   "",
   "[  FAILED  ] Version.Minor (0 ms)",
   "50% tests passed, 1 tests failed out of 2",
   "The following tests FAILED:",
   "\t  2 - Version.Minor (Failed)",
}, "\n")
assert(kept(failure) == table.concat({
   " 2/2 Test  #2: Version.Minor ......***Failed    0.01 sec",
   "tests/a.cpp:12: Failure",
   "Expected equality of these values:",
   "  1",
   "  2",
   "[  FAILED  ] Version.Minor (0 ms)",
   "50% tests passed, 1 tests failed out of 2",
   "The following tests FAILED:",
   "\t  2 - Version.Minor (Failed)",
}, "\n"), kept(failure))

-- A test that is disabled on purpose is not a diagnostic.
assert(kept(" 3/4 Test  #3: A.B ......***Not Run (Disabled)   0.00 sec\n") == "")

-- A long result is cut, and says by how much.
local many = {}
for index = 1, 10 do
   many[index] = "a.c:" .. index .. ": error: x"
end
local text, omitted = kept(table.concat(many, "\n"), 3)
assert(omitted == 7 and not text:find("a.c:4:", 1, true))

-- The exit code of the first failed step comes back, and later steps do not run.
local log = os.tmpname()
local report = quiet.run({ { "sh", "-c", "echo 'x: error: boom'; exit 3" }, { "echo", "later" } }, log)
assert(report.code == 3 and report.lines[1] == "x: error: boom")
assert(not io.open(log):read("a"):find("later", 1, true))

-- A failure that matches nothing still shows the end of the log.
report = quiet.run({ { "sh", "-c", "echo 'something odd'; exit 1" } }, log)
assert(report.code == 1 and report.lines[#report.lines] == "something odd")

-- Success without diagnostics is silent.
report = quiet.run({ { "echo", "fine" } }, log)
assert(report.code == 0 and #report.lines == 0)
os.remove(log)

-- Only build and test exist, and their values are checked.
local steps = assert(quiet.steps("build", {}))
assert(table.concat(steps[2], " "):find("--build cmake-build-debug --target marea_tests", 1, true))
steps = assert(quiet.steps("test", { "--config", "Release", "--filter", "Version" }))
assert(
   table
      .concat(steps[3], " ")
      :find("ctest --test-dir cmake-build-release --output-on-failure -R Version", 1, true)
)
assert(not quiet.steps("rm", {}))
assert(not quiet.steps("build", { "--config", "x; rm -rf ." }))
assert(not quiet.steps("build", { "--target", "--clean-first" }))
assert(not quiet.steps("build", { "--filter", "x" }))
assert(not quiet.steps("build", { "stray" }))

print("test_quiet: ok")
