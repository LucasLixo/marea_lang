-- Copyright (c) 2026 Paulo Lucas Alves Franca
-- Licensed under the MIT License. See LICENSE.txt for full terms.

--- @script main
-- Entry point for marea-quiet. Run it through mise, which sets the compiler environment, from the
-- root of the checkout:
--
--   mise run dev:quiet-build [-- --config Release] [--target <name>]
--   mise run dev:quiet-test  [-- --config Release] [--filter <regex>]

local script_dir = (arg[0]:gsub("\\", "/")):match("(.*/)") or "./"
package.path = script_dir .. "?.lua;" .. package.path

local quiet = require("quiet")

local command = arg[1] or ""
local steps, err = quiet.steps(command, { table.unpack(arg, 2) })
if not steps then
   io.stderr:write("marea-quiet: " .. err .. "\n")
   io.stderr:write("usage: marea-quiet build|test [--config NAME] [--target NAME] [--filter REGEX]\n")
   os.exit(2)
end

os.execute("mkdir -p tmp")
local log_path = "tmp/quiet-" .. command .. ".log"
local report = quiet.run(steps, log_path)

local state = report.code == 0 and "ok" or ("FAILED, exit " .. report.code)
print(command .. ": " .. state .. " (full log: " .. log_path .. ")")
for _, line in ipairs(report.lines) do
   print(line)
end
if report.omitted > 0 then
   print("... " .. report.omitted .. " more lines in the log")
end
os.exit(report.code)
