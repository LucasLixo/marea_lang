-- Copyright (c) 2026 Paulo Lucas Alves Franca
-- Licensed under the MIT License. See LICENSE.txt for full terms.

--- @module quiet
-- Builds and tests with the whole output in a log file, and returns only the diagnostics, so an
-- agent does not read thousands of progress lines. It runs only the commands it writes itself:
-- the project allows the scripts under tools/ without a prompt, so it must never run a command
-- that the caller supplies. POSIX shell only.

local quiet = {}

local CONFIGS = { Debug = true, Release = true, RelWithDebInfo = true, MinSizeRel = true }
local MARKS = {
   "error:",
   "warning:",
   "undefined reference",
   "^FAILED:",
   "%*%*%*Failed",
   "%*%*%*Exception",
   "%*%*%*Timeout",
   "%*%*%*Not Run",
   "%[  FAILED  %]",
   "tests passed",
   "No tests were found",
}
-- A head line whose detail is on the lines below it, and which of those lines belong to it.
local BLOCKS = {
   { head = "^CMake Warning", keep = "^%s", blank_ends = false },
   { head = "^CMake Error", keep = "^%s", blank_ends = false },
   { head = ":%d+: Failure$", keep = "^[^%[]", blank_ends = true },
   { head = "^The following tests FAILED:", keep = "^%s", blank_ends = true },
}
local BLOCK_LINES = 12

local function is_mark(line)
   if line:find("(Disabled)", 1, true) then
      return false
   end
   for _, mark in ipairs(MARKS) do
      if line:find(mark) then
         return true
      end
   end
   return false
end

--- @param log string: The full output of a build or test run.
-- @param limit number: The most lines to return.
-- @return table, number: The diagnostic lines in order, and how many more were left out.
function quiet.filter(log, limit)
   local found, seen = {}, {}
   local block, taken = nil, 0
   for line in (log .. "\n"):gmatch("([^\n]*)\n") do
      local head = nil
      for _, candidate in ipairs(BLOCKS) do
         if line:find(candidate.head) then
            head = candidate
         end
      end
      if head then
         block, taken = head, 0
         found[#found + 1] = line
      else
         local blank = line:match("^%s*$") ~= nil
         if block and not blank and line:find(block.keep) and taken < BLOCK_LINES then
            found[#found + 1] = line
            taken = taken + 1
         else
            if not blank or (block and block.blank_ends) then
               block = nil
            end
            if is_mark(line) and not seen[line] then
               seen[line] = true
               found[#found + 1] = line
            end
         end
      end
   end
   local omitted = math.max(0, #found - limit)
   for index = #found, limit + 1, -1 do
      found[index] = nil
   end
   return found, omitted
end

local function quote(text)
   return "'" .. text:gsub("'", "'\\''") .. "'"
end

--- @param command string: "build" or "test".
-- @param argv table: Any of `--config <name>`, `--target <name>` (build, repeatable) and
--   `--filter <regex>` (test).
-- @return table|nil, string|nil: The commands to run, each as an argument list, or nil and why not.
function quiet.steps(command, argv)
   if command ~= "build" and command ~= "test" then
      return nil, "the command is `build` or `test`"
   end
   local config, filter, targets = "Debug", nil, {}
   for index = 1, #argv, 2 do
      local flag, value = argv[index], argv[index + 1]
      if value == nil then
         return nil, "missing value for `" .. flag .. "`"
      elseif flag == "--config" and CONFIGS[value] then
         config = value
      elseif flag == "--target" and command == "build" and value:match("^%a[%w_%-]*$") then
         targets[#targets + 1] = value
      elseif flag == "--filter" and command == "test" then
         filter = value
      else
         return nil, "not accepted: `" .. flag .. " " .. value .. "`"
      end
   end
   if command == "test" or #targets == 0 then
      targets = { "marea_tests" }
   end

   local directory = "cmake-build-" .. config:lower()
   local build = { "cmake", "--build", directory, "--target" }
   for _, target in ipairs(targets) do
      build[#build + 1] = target
   end
   build[#build + 1] = "--parallel"
   local steps = {
      {
         "cmake",
         "-S",
         ".",
         "-B",
         directory,
         "-DCMAKE_BUILD_TYPE=" .. config,
         "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
      },
      build,
   }
   if command == "test" then
      local test = { "ctest", "--test-dir", directory, "--output-on-failure" }
      if filter then
         test[#test + 1] = "-R"
         test[#test + 1] = filter
      end
      steps[#steps + 1] = test
   end
   return steps
end

--- Runs the steps until one fails, with all their output in `log_path`.
-- @return table: `{ code, lines, omitted }`. A failure with no diagnostic returns the log's tail.
function quiet.run(steps, log_path)
   assert(io.open(log_path, "w")):close()
   local code = 0
   for _, step in ipairs(steps) do
      local words = {}
      for index, word in ipairs(step) do
         words[index] = quote(word)
      end
      local ok, _, status = os.execute(table.concat(words, " ") .. " >> " .. quote(log_path) .. " 2>&1")
      if not ok then
         code = status
         break
      end
   end

   local file = assert(io.open(log_path, "r"))
   local log = file:read("a")
   file:close()
   local lines, omitted = quiet.filter(log, 60)
   if code ~= 0 and #lines == 0 then
      for line in log:gmatch("[^\n]+") do
         lines[#lines + 1] = line
         if #lines > 20 then
            table.remove(lines, 1)
         end
      end
   end
   return { code = code, lines = lines, omitted = omitted }
end

return quiet
