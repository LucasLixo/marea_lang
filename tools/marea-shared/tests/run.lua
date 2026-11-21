-- Copyright (c) 2026 Paulo Lucas Alves Franca
-- Licensed under the MIT License. See LICENSE.txt for full terms.

--- @script run
-- Runs every test_*.lua in this directory as its own lua process (each uses plain `assert`,
-- no framework) and fails loudly on the first one that errors or exits non-zero.

local script_dir = (arg[0]:gsub("\\", "/")):match("(.*/)") or "./"

local handle = io.popen("ls " .. ("%q"):format(script_dir) .. "test_*.lua 2>/dev/null")
local test_files = {}
for line in handle:lines() do
   test_files[#test_files + 1] = line
end
handle:close()
table.sort(test_files)

assert(#test_files > 0, "no test_*.lua files found under " .. script_dir)

local failures = 0
for _, test_file in ipairs(test_files) do
   print("=== " .. test_file .. " ===")
   local ok = os.execute("lua " .. ("%q"):format(test_file))
   -- Lua 5.1 os.execute returns the raw OS exit status (0 == success); Lua 5.2+ returns
   -- true/nil -- checking both keeps this correct under either os.execute contract.
   if not (ok == 0 or ok == true) then
      failures = failures + 1
      io.stderr:write("FAILED: " .. test_file .. "\n")
   end
end

if failures > 0 then
   io.stderr:write(failures .. " of " .. #test_files .. " test files failed\n")
   os.exit(1)
end

print("all " .. #test_files .. " test files passed")
