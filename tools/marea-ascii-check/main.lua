-- Copyright (c) 2026 Paulo Lucas Alves Franca
-- Licensed under the MIT License. See LICENSE.txt for full terms.

--- @script main
-- Entry point/CLI for marea-ascii-check. Run via the pinned `lua` (mise `lua`) -- no Cargo
-- package, no compile step.
--
-- Usage: lua main.lua [-h|--help] <file> [file...]
--
-- Checks every given file against AGENTS.md's "No Unicode characters ... ASCII only" rule and
-- exits non-zero if any file has a violation or could not be read, so this can be wired into a
-- mise task or a pre-commit/CI check.
--
-- No subcommands here, so this uses `tools/marea-shared/cli.lua`'s flat (positional-only) shape
-- rather than its multi-command one.

local script_dir = (arg[0]:gsub("\\", "/")):match("(.*/)") or "./"
package.path = script_dir .. "?.lua;" .. script_dir .. "../marea-shared/?.lua;" .. package.path

local cli = require("cli")
local ascii_check = require("ascii_check")

local SPEC = {
   name = "marea-ascii-check",
   summary = "check files for non-ASCII bytes",
   flags = {},
   positional = { label = "<file> [file...]", min = 1 },
}

local result = cli.parse(SPEC, arg)
cli.finish(result)

local violation_count = 0
local error_count = 0

for _, path in ipairs(result.positional) do
   local violations, err = ascii_check.check_file(path)
   if not violations then
      -- io.open's own error message already carries the path ("path: No such file or
      -- directory"), so prefixing it again here would just repeat it.
      io.stderr:write("marea-ascii-check: " .. err .. "\n")
      error_count = error_count + 1
   else
      for _, violation in ipairs(violations) do
         print(ascii_check.format_violation(path, violation))
         violation_count = violation_count + 1
      end
   end
end

io.stderr:write(
   string.format("marea-ascii-check: %d non-ASCII byte(s), %d file error(s)\n", violation_count, error_count)
)

if violation_count > 0 or error_count > 0 then
   os.exit(1)
end

os.exit(0)
