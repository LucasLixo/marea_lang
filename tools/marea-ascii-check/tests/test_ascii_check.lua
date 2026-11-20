-- Copyright (c) 2026 Paulo Lucas Alves Franca
-- Licensed under the MIT License. See LICENSE.txt for full terms.

--- @script test_ascii_check
-- Byte-level ASCII scanning that CLAUDE.md's "No Unicode characters ... ASCII only" rule rests
-- on. Fixtures are synthetic (tests/fixtures/with_violations.txt) since the repo's own ASCII-only
-- rule means no real corpus file can carry the violations this checks for.

local script_dir = (arg[0]:gsub("\\", "/")):match("(.*/)") or "./"
package.path = script_dir .. "../?.lua;" .. package.path

local ascii_check = require("ascii_check")

local fixtures_dir = script_dir .. "fixtures/"

-- check_file: an all-ASCII file reports no violations.
do
   local violations, err = ascii_check.check_file(fixtures_dir .. "clean.txt")
   assert(violations ~= nil, err)
   assert(#violations == 0, "expected no violations, got " .. #violations)
   print("test_ascii_check: check_file clean OK")
end

-- check_file: every non-ASCII byte is reported, with the 1-based line it occurs on --
-- with_violations.txt has "cafe" + 0xC3 0xA9 on line 2 and an em-dash (0xE2 0x80 0x94) on line 3.
do
   local violations, err = ascii_check.check_file(fixtures_dir .. "with_violations.txt")
   assert(violations ~= nil, err)
   assert(#violations == 5, "expected 5 violations, got " .. #violations)

   assert(violations[1].line == 2 and violations[1].byte == 0xC3)
   assert(violations[2].line == 2 and violations[2].byte == 0xA9)
   assert(violations[3].line == 3 and violations[3].byte == 0xE2)
   assert(violations[4].line == 3 and violations[4].byte == 0x80)
   assert(violations[5].line == 3 and violations[5].byte == 0x94)
   print("test_ascii_check: check_file with_violations OK")
end

-- check_file: a missing file returns nil plus an error, not a false-clean empty list.
do
   local violations, err = ascii_check.check_file(fixtures_dir .. "does_not_exist.txt")
   assert(violations == nil)
   assert(type(err) == "string" and #err > 0)
   print("test_ascii_check: check_file missing file OK")
end

-- check_file: a directory returns nil plus a path-prefixed message and does not raise.
do
   local ok, violations, err = pcall(ascii_check.check_file, fixtures_dir)
   assert(ok, "a directory raised: " .. tostring(violations))
   assert(violations == nil, "a directory must not give a violation list")
   assert(
      type(err) == "string" and err:find(fixtures_dir, 1, true),
      "message must name the path: " .. tostring(err)
   )
   print("test_ascii_check: check_file directory OK")
end

-- format_violation: path, 1-based line, and the byte value as uppercase hex.
do
   local message = ascii_check.format_violation("some/file.txt", { line = 2, byte = 0xC3 })
   assert(message == "some/file.txt:2: non-ASCII byte 0xC3", message)
   print("test_ascii_check: format_violation OK")
end

print("test_ascii_check: ALL OK")
