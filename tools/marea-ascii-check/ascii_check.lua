-- Copyright (c) 2026 Paulo Lucas Alves Franca
-- Licensed under the MIT License. See LICENSE.txt for full terms.

--- @module ascii_check
-- Verifies CLAUDE.md's "No Unicode characters in code, comments, docs, or commit messages --
-- ASCII only" rule: every byte of a text file must fall within the 0-127 ASCII range. Files are
-- read in binary mode so byte values are exact, independent of platform line-ending translation.

local ascii_check = {}

--- Scans one file for bytes outside the 0-127 ASCII range.
-- @param path string: Path to the file to check.
-- @return table|nil: A list of `{ line = number, byte = number }` violations, in file order
--   (empty if the file is fully ASCII); nil if the file could not be opened.
-- @return string|nil: An error message when the file could not be opened (mirrors io.open's own
--   `nil, err` convention), otherwise nil.
function ascii_check.check_file(path)
   local file, open_err = io.open(path, "rb")
   if not file then
      return nil, open_err
   end

   local contents, read_err = file:read("*a")
   file:close()
   if not contents then
      return nil, path .. ": " .. tostring(read_err)
   end

   local violations = {}
   local line = 1
   for index = 1, #contents do
      local byte = contents:byte(index)
      if byte > 127 then
         violations[#violations + 1] = { line = line, byte = byte }
      end
      if byte == 10 then
         line = line + 1
      end
   end

   return violations
end

--- Formats one violation for CLI output.
-- @param path string: The file the violation was found in.
-- @param violation table: One `{ line, byte }` entry from `check_file`.
-- @return string: "path:line: non-ASCII byte 0xHH".
function ascii_check.format_violation(path, violation)
   return string.format("%s:%d: non-ASCII byte 0x%02X", path, violation.line, violation.byte)
end

return ascii_check
