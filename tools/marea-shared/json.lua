-- Copyright (c) 2026 Paulo Lucas Alves Franca
-- Licensed under the MIT License. See LICENSE.txt for full terms.

--- @module json
-- A generic JSON encoder/decoder for the tooling scripts under tools/. Separate from
-- tools/marea-api-catalog/catalog.lua's own encoder, which is pretty-only, with embedded real
-- newlines, purpose-built for the catalog's byte-identical-rebuild acceptance test; that shape
-- cannot be reused as-is for a one-JSON-object-per-line wire format (MCP's stdio framing), so
-- this module stands on its own and never touches catalog.lua. ASCII-only, matching the
-- repository's own rule: encode raises on a non-ASCII byte in a string instead of passing it
-- through.

local json = {}

-- Marks a table as a JSON array, so an empty one encodes as "[]" and not as "{}" (an empty Lua
-- table carries no kind of its own) -- the same idea catalog.lua's own ARRAY_MARK solves, as a
-- local marker table set via setmetatable, reimplemented here rather than shared, since the two
-- encoders are otherwise unrelated.
local ARRAY_MARK = {}

--- Marks `value` as an array for the encoder. Only array fields need it; an unmarked empty table
-- encodes as "{}".
-- @param value table: The array (or empty table).
-- @return table: The same table, marked.
function json.array(value)
   return setmetatable(value, ARRAY_MARK)
end

-- ---------------------------------------------------------------------------
-- Encode
-- ---------------------------------------------------------------------------

--- Escapes and quotes one JSON string, rejecting non-ASCII bytes so a caller can never produce
-- wire text that drifts between locales (project rule: source is ASCII-only anyway).
-- @param value string: Raw string.
-- @return string: Quoted, escaped JSON string.
local function encode_string(value)
   local escaped = value:gsub('["\\%c]', function(char)
      if char == '"' then
         return '\\"'
      elseif char == "\\" then
         return "\\\\"
      end
      return string.format("\\u%04x", string.byte(char))
   end)
   if escaped:find("[\128-\255]") then
      error("json: refusing to encode non-ASCII string: " .. value, 0)
   end
   return '"' .. escaped .. '"'
end

--- Encodes one value as JSON text, compact or pretty-printed depending on `indent`.
-- @param value table|string|number|boolean: Value to encode.
-- @param indent number|nil: nil for compact (no indent, no newline); a number of spaces per
--   nesting level otherwise.
-- @param depth number: Current nesting depth (0 at the top).
-- @return string: The encoded fragment.
local function encode_value(value, indent, depth)
   local kind = type(value)
   if kind == "string" then
      return encode_string(value)
   elseif kind == "number" then
      if value ~= value or value == math.huge or value == -math.huge then
         error("json: refusing to encode a non-finite number: " .. tostring(value), 0)
      end
      if value == math.floor(value) and math.abs(value) < 2 ^ 53 then
         return string.format("%d", value)
      end
      return string.format("%.17g", value)
   elseif kind == "boolean" then
      return tostring(value)
   elseif kind == "table" then
      local newline = indent and "\n" or ""
      local padding = indent and string.rep(" ", indent * depth) or ""
      local inner_padding = indent and string.rep(" ", indent * (depth + 1)) or ""
      local colon = indent and ": " or ":"
      if #value > 0 or next(value) == nil then
         local items = {}
         for i = 1, #value do
            items[#items + 1] = inner_padding .. encode_value(value[i], indent, depth + 1)
         end
         if #items == 0 then
            return getmetatable(value) == ARRAY_MARK and "[]" or "{}"
         end
         return "[" .. newline .. table.concat(items, "," .. newline) .. newline .. padding .. "]"
      end
      local keys = {}
      for key in pairs(value) do
         keys[#keys + 1] = key
      end
      table.sort(keys)
      local items = {}
      for _, key in ipairs(keys) do
         items[#items + 1] = inner_padding
            .. encode_string(key)
            .. colon
            .. encode_value(value[key], indent, depth + 1)
      end
      return "{" .. newline .. table.concat(items, "," .. newline) .. newline .. padding .. "}"
   end
   error("json: cannot encode value of type " .. kind, 0)
end

--- Encodes `value` as JSON text.
-- @param value table|string|number|boolean: Value to encode.
-- @param opts table|nil: `opts.indent` absent/nil for a compact single line (the wire form);
--   a number for a pretty-printed document at that many spaces per nesting level.
-- @return string: The encoded JSON document.
function json.encode(value, opts)
   local indent = opts and opts.indent or nil
   return encode_value(value, indent, 0)
end

-- ---------------------------------------------------------------------------
-- Decode
-- ---------------------------------------------------------------------------

--- Skips JSON whitespace (space, tab, CR, LF) starting at `pos`.
-- @param text string: Document text.
-- @param pos number: 1-based position to start from.
-- @return number: The first non-whitespace position at or after `pos`.
local function skip_space(text, pos)
   local _, last = text:find("^[ \t\r\n]*", pos)
   return last + 1
end

-- Forward declaration: decode_value and decode_string call each other indirectly through
-- decode_array/decode_object, so the whole family is declared local up front.
local decode_value

--- Decodes a JSON string literal starting at the opening quote.
-- @param text string: Document text.
-- @param pos number: Position of the opening '"'.
-- @return string|nil, number|string: The decoded string and the position after the closing
--   quote, or nil plus an error message.
local function decode_string(text, pos)
   local out = {}
   local i = pos + 1
   local length = #text
   while true do
      if i > length then
         return nil, "json: unterminated string at " .. pos
      end
      local char = text:sub(i, i)
      if char == '"' then
         return table.concat(out), i + 1
      elseif char == "\\" then
         local escape = text:sub(i + 1, i + 1)
         if escape == '"' or escape == "\\" or escape == "/" then
            out[#out + 1] = escape
            i = i + 2
         elseif escape == "b" then
            out[#out + 1] = "\b"
            i = i + 2
         elseif escape == "f" then
            out[#out + 1] = "\f"
            i = i + 2
         elseif escape == "n" then
            out[#out + 1] = "\n"
            i = i + 2
         elseif escape == "r" then
            out[#out + 1] = "\r"
            i = i + 2
         elseif escape == "t" then
            out[#out + 1] = "\t"
            i = i + 2
         elseif escape == "u" then
            local hex = text:sub(i + 2, i + 5)
            local code = tonumber(hex, 16)
            if not code or #hex ~= 4 then
               return nil, "json: bad \\u escape at " .. i
            end
            if code > 255 then
               return nil, "json: non-ASCII \\u escape at " .. i
            end
            out[#out + 1] = string.char(code)
            i = i + 6
         else
            return nil, "json: bad escape at " .. i
         end
      else
         out[#out + 1] = char
         i = i + 1
      end
   end
end

--- Decodes a JSON array starting at the opening '['.
-- @param text string: Document text.
-- @param pos number: Position of the opening '['.
-- @return table|nil, number|string: The decoded array (array-marked) and the position after
--   the closing ']', or nil plus an error message.
local function decode_array(text, pos)
   local items = json.array({})
   local i = skip_space(text, pos + 1)
   if text:sub(i, i) == "]" then
      return items, i + 1
   end
   while true do
      local value, next_pos = decode_value(text, i)
      if value == nil then
         return nil, next_pos
      end
      items[#items + 1] = value
      i = skip_space(text, next_pos)
      local char = text:sub(i, i)
      if char == "," then
         i = skip_space(text, i + 1)
      elseif char == "]" then
         return items, i + 1
      else
         return nil, "json: expected ',' or ']' at " .. i
      end
   end
end

--- Decodes a JSON object starting at the opening '{'.
-- @param text string: Document text.
-- @param pos number: Position of the opening '{'.
-- @return table|nil, number|string: The decoded object and the position after the closing
--   '}', or nil plus an error message.
local function decode_object(text, pos)
   local result = {}
   local i = skip_space(text, pos + 1)
   if text:sub(i, i) == "}" then
      return result, i + 1
   end
   while true do
      if text:sub(i, i) ~= '"' then
         return nil, "json: expected a string key at " .. i
      end
      local key, after_key = decode_string(text, i)
      if not key then
         return nil, after_key
      end
      i = skip_space(text, after_key)
      if text:sub(i, i) ~= ":" then
         return nil, "json: expected ':' at " .. i
      end
      i = skip_space(text, i + 1)
      local value, next_pos = decode_value(text, i)
      if value == nil then
         return nil, next_pos
      end
      result[key] = value
      i = skip_space(text, next_pos)
      local char = text:sub(i, i)
      if char == "," then
         i = skip_space(text, i + 1)
      elseif char == "}" then
         return result, i + 1
      else
         return nil, "json: expected ',' or '}' at " .. i
      end
   end
end

--- Decodes one JSON value (any kind) starting at `pos`. A decoded JSON "null" becomes
-- `json.null`, never Lua `nil`, so the contract below stays exact: the first return value is
-- `nil` if, and only if, decoding failed.
-- @param text string: Document text.
-- @param pos number: 1-based position of the value's first byte.
-- @return any, number|string: On success, the decoded value and the position right after it;
--   on failure, nil and a non-empty error message.
decode_value = function(text, pos)
   local char = text:sub(pos, pos)
   if char == '"' then
      return decode_string(text, pos)
   elseif char == "{" then
      return decode_object(text, pos)
   elseif char == "[" then
      return decode_array(text, pos)
   elseif char == "t" and text:sub(pos, pos + 3) == "true" then
      return true, pos + 4
   elseif char == "f" and text:sub(pos, pos + 4) == "false" then
      return false, pos + 5
   elseif char == "n" and text:sub(pos, pos + 3) == "null" then
      return json.null, pos + 4
   elseif char:match("[%d%-]") then
      local number_text = text:match("^-?%d+%.?%d*[eE]?[+-]?%d*", pos)
      local value = number_text and tonumber(number_text)
      if not value then
         return nil, "json: bad number at " .. pos
      end
      return value, pos + #number_text
   end
   return nil, "json: unexpected character at " .. pos
end

-- A distinct sentinel for a decoded JSON "null" -- Lua has no value that can live inside a
-- table and still mean "present but null", so a plain nil would silently drop the key instead.
json.null = setmetatable({}, {
   __tostring = function()
      return "null"
   end,
})

--- Decodes a complete JSON document.
-- @param text string: The document text.
-- @return any|nil, string|nil: The decoded value and nil on success; nil and a non-empty error
--   message ("json: <reason> at <position>") on failure. Never raises.
function json.decode(text)
   local ok, decoded, err = pcall(function()
      local start = skip_space(text, 1)
      local value, after = decode_value(text, start)
      if value == nil then
         return nil, after
      end
      local trailing = skip_space(text, after)
      if trailing <= #text then
         return nil, "json: trailing data at " .. trailing
      end
      return value, nil
   end)
   if not ok then
      return nil, "json: " .. tostring(decoded)
   end
   if decoded == nil then
      return nil, err
   end
   return decoded
end

return json
