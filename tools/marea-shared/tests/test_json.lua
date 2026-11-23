-- Copyright (c) 2026 Paulo Lucas Alves Franca
-- Licensed under the MIT License. See LICENSE.txt for full terms.

--- @script test_json
-- A generic JSON encoder/decoder, independent of marea-api-catalog's own purpose-built one
-- (catalog.lua's encoder is pretty-only with embedded newlines, which would break MCP's
-- one-JSON-object-per-line stdio framing). Covers both the compact and pretty encode forms,
-- the ASCII-only rule the repository holds everywhere, and a decode that never raises.

local script_dir = (arg[0]:gsub("\\", "/")):match("(.*/)") or "./"
package.path = script_dir .. "../?.lua;" .. package.path

local json = require("json")

--- Deep-equality for plain JSON-shaped values (string/number/boolean/table), ignoring any
-- metatable (the array marker) so a decoded array compares equal to a hand-written Lua array.
-- @param a any: First value.
-- @param b any: Second value.
-- @return boolean: Whether the two values are structurally equal.
local function deep_equal(a, b)
   if type(a) ~= type(b) then
      return false
   end
   if type(a) ~= "table" then
      return a == b
   end
   for key, value in pairs(a) do
      if not deep_equal(value, b[key]) then
         return false
      end
   end
   for key in pairs(b) do
      if a[key] == nil then
         return false
      end
   end
   return true
end

-- encode/decode round-trip: string, number, boolean, object, empty array.
do
   local cases = {
      "hello",
      42,
      true,
      false,
      { name = "marea", count = 3 },
      json.array({}),
   }
   for _, value in ipairs(cases) do
      local encoded = json.encode(value)
      local decoded, err = json.decode(encoded)
      assert(decoded ~= nil or value == nil, err)
      assert(deep_equal(value, decoded), "round-trip mismatch for " .. tostring(encoded))
   end
   print("test_json: round-trip OK")
end

-- encode with no opts.indent: compact, single line, no embedded newline.
do
   local encoded = json.encode({ a = 1, b = json.array({ 1, 2, 3 }) })
   assert(not encoded:find("\n", 1, true), "compact encode must not contain a newline: " .. encoded)
   print("test_json: compact encode OK")
end

-- encode with opts.indent = 2: pretty, multiple lines.
do
   local encoded = json.encode({ a = 1, b = 2 }, { indent = 2 })
   local newline_count = 0
   for _ in encoded:gmatch("\n") do
      newline_count = newline_count + 1
   end
   assert(newline_count > 0, "pretty encode must contain at least one newline")
   print("test_json: pretty encode OK")
end

-- A control byte and a literal newline inside a string both encode as \u00XX escapes.
do
   local encoded = json.encode("a\1b\nc")
   assert(encoded:find("\\u0001", 1, true), "control byte must be escaped: " .. encoded)
   assert(encoded:find("\\u000a", 1, true), "newline must be escaped: " .. encoded)
   assert(not encoded:find("\n", 1, true), "a raw newline must never survive encode: " .. encoded)
   print("test_json: control byte and newline escaping OK")
end

-- A non-ASCII byte in a string makes encode raise (mirrors catalog.lua's own rule).
do
   local ok, err = pcall(json.encode, "cafe\195\169")
   assert(not ok, "a non-ASCII string must raise")
   assert(type(err) == "string" and #err > 0)
   print("test_json: non-ASCII rejection OK")
end

-- decode of a malformed document returns nil, err (a non-empty string), never raises.
do
   local ok, value, err = pcall(json.decode, "{")
   assert(ok, "a malformed document must not raise: " .. tostring(value))
   assert(value == nil)
   assert(type(err) == "string" and #err > 0)
   print("test_json: malformed decode OK")
end

-- decode of encode's own output returns an equal value for a nested table (array of objects).
do
   local value = json.array({
      { name = "one", tags = json.array({ "x", "y" }) },
      { name = "two", tags = json.array({}) },
   })
   local encoded = json.encode(value, { indent = 2 })
   local decoded, err = json.decode(encoded)
   assert(decoded ~= nil, err)
   assert(deep_equal(value, decoded), "nested round-trip mismatch")
   print("test_json: nested round-trip OK")
end

print("test_json: ALL OK")
