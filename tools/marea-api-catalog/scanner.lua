-- Copyright (c) 2026 Paulo Lucas Alves Franca
-- Licensed under the MIT License. See LICENSE.txt for full terms.

--- @module scanner
-- Regex scanner over C11 headers and sources - no libclang, text only. Everything the catalog
-- needs is extracted from text after `strip`: quote/angle includes, the
-- first `MAREA_API_*` gate after the include guard, the symbols a file DECLARES (five forms:
-- object/function-like macros, typedefs, struct/enum/union tags, enum enumerators and function
-- names anchored by `MAREA_CALL`), and the identifiers a file USES. Comments and string/char
-- literals never contribute, so a commented-out `#include` or a name inside a doc string cannot
-- pollute the catalog.

local scanner = {}

local IDENT_PATTERN = "[%a_][%w_]*"

--- Scans a body for comma-separated segments at brace/paren depth 0 and returns the leading
-- identifier of each segment (the enumerator names of an `enum { ... }` body).
-- @param body string: Text between the enum's braces, already stripped.
-- @return table: Array of enumerator names.
local function enum_enumerators(body)
   local segments = {}
   local depth = 0
   local segment_start = 1
   for i = 1, #body do
      local char = body:sub(i, i)
      if char == "{" or char == "(" then
         depth = depth + 1
      elseif char == "}" or char == ")" then
         depth = depth - 1
      elseif char == "," and depth == 0 then
         segments[#segments + 1] = body:sub(segment_start, i - 1)
         segment_start = i + 1
      end
   end
   segments[#segments + 1] = body:sub(segment_start)

   local names = {}
   for _, segment in ipairs(segments) do
      local name = segment:match("^%s*(" .. IDENT_PATTERN .. ")")
      if name then
         names[#names + 1] = name
      end
   end
   return names
end

--- Finds the brace-balanced `{ ... }` span opening at `open_pos` (1-based index of the `{`).
-- @param text string: Text to scan.
-- @param open_pos number: Index of the opening brace.
-- @return number: Index of the matching closing brace.
local function match_brace(text, open_pos)
   local depth = 0
   for i = open_pos, #text do
      local char = text:sub(i, i)
      if char == "{" then
         depth = depth + 1
      elseif char == "}" then
         depth = depth - 1
         if depth == 0 then
            return i
         end
      end
   end
   error("scanner: unbalanced braces")
end

--- Replaces the span `first..last` with spaces, keeping newlines, so later passes see the same
-- line structure but no longer match inside the consumed declaration.
-- @param chars table: Mutable array of single characters.
-- @param first number: First index to blank.
-- @param last number: Last index to blank.
local function blank_span(chars, first, last)
   for i = first, last do
      if chars[i] ~= "\n" then
         chars[i] = " "
      end
   end
end

--- Rebuilds a string from the mutable char array.
-- @param chars table: Mutable array of single characters.
-- @return string: The joined text.
local function rebuild(chars)
   return table.concat(chars)
end

--- Blanks `//` and `/* */` comments inside one preprocessor-directive line while keeping string
-- literals intact (an `#include "..."` target must survive).
-- @param line string: One line of text (no trailing newline).
-- @return string, boolean: The same line with comment spans replaced by spaces, and whether a
--   `/*` comment is still open at the end of the line.
local function strip_directive_line(line)
   local out = {}
   local open_comment = false
   local i = 1
   local length = #line
   local quote = nil
   while i <= length do
      local char = line:sub(i, i)
      if quote then
         out[#out + 1] = char
         if char == "\\" then
            out[#out + 1] = line:sub(i + 1, i + 1)
            i = i + 1
         elseif char == quote then
            quote = nil
         end
         i = i + 1
      elseif char == '"' or char == "'" then
         quote = char
         out[#out + 1] = char
         i = i + 1
      elseif char == "/" and line:sub(i + 1, i + 1) == "/" then
         out[#out + 1] = string.rep(" ", length - i + 1)
         break
      elseif char == "/" and line:sub(i + 1, i + 1) == "*" then
         local close = line:find("*/", i + 2, true)
         local last = close and (close + 1) or length
         open_comment = close == nil
         out[#out + 1] = (line:sub(i, last):gsub("[^\n]", " "))
         i = last + 1
      else
         out[#out + 1] = char
         i = i + 1
      end
   end
   return table.concat(out), open_comment
end

--- Removes `/* */` and `//` comments and string/char literal contents while keeping line
-- structure (newlines survive at their positions). `#include` lines are kept intact - the
-- scanner's include extraction runs on the result, and a stripped include target would be
-- useless. A trailing comment on an include line still goes.
-- @param text string: Raw file text.
-- @return string: The stripped text, same length and line count.
function scanner.strip(text)
   local out = {}
   local i = 1
   local length = #text
   local only_leading_space = true
   while i <= length do
      local char = text:sub(i, i)
      if char == "\n" then
         out[#out + 1] = "\n"
         i = i + 1
         only_leading_space = true
      elseif only_leading_space and char:match("%s") then
         out[#out + 1] = char
         i = i + 1
      elseif only_leading_space and char == "#" then
         local line_end = text:find("\n", i, true)
         local last = line_end and (line_end - 1) or length
         local line, open_comment = strip_directive_line(text:sub(i, last))
         out[#out + 1] = line
         i = last + 1
         if open_comment then
            -- A `/*` opened on a directive line runs on until its `*/`, however many lines.
            local close = text:find("*/", i, true)
            local stop = close and (close + 1) or length
            out[#out + 1] = (text:sub(i, stop):gsub("[^\n]", " "))
            i = stop + 1
            only_leading_space = false
         end
      elseif char == "/" and text:sub(i + 1, i + 1) == "/" then
         local line_end = text:find("\n", i, true)
         local last = line_end and (line_end - 1) or length
         out[#out + 1] = string.rep(" ", last - i + 1)
         i = last + 1
      elseif char == "/" and text:sub(i + 1, i + 1) == "*" then
         local close = text:find("*/", i + 2, true)
         local last = close and (close + 1) or length
         out[#out + 1] = (text:sub(i, last):gsub("[^\n]", " "))
         i = last + 1
         only_leading_space = false
      elseif char == '"' or char == "'" then
         local j = i + 1
         while j <= length do
            local inner = text:sub(j, j)
            if inner == "\\" then
               j = j + 2
            elseif inner == char then
               break
            else
               j = j + 1
            end
         end
         j = math.min(j, length)
         out[#out + 1] = (text:sub(i, j):gsub("[^\n]", " "))
         i = j + 1
         only_leading_space = false
      else
         out[#out + 1] = char
         if not char:match("%s") then
            only_leading_space = false
         end
         i = i + 1
      end
   end
   return table.concat(out)
end

--- Extracts `#include` directives, one entry per line, in document order. Runs on stripped text
-- so commented-out includes cannot appear.
-- @param text string: Stripped file text.
-- @return table: Array of `{ kind = "quote" | "angle", target = "..." }`.
function scanner.includes(text)
   local includes = {}
   for line in text:gmatch("[^\n]+") do
      local target = line:match('^%s*#%s*include%s+"(.-)"')
      if target then
         includes[#includes + 1] = { kind = "quote", target = target }
      else
         target = line:match("^%s*#%s*include%s+<(.-)>")
         if target then
            includes[#includes + 1] = { kind = "angle", target = target }
         end
      end
   end
   return includes
end

--- Finds the sorted `MAREA_API_*` gate names of a header: the names inside the first
-- `#if defined(...)` (a conjunction counts) that follows the include guard's `#define`. A
-- header whose first such condition only names `SETTING_*`/compiler macros is ungated and
-- returns `{}`, as does a header with no gate at all.
-- @param text string: Stripped file text (`scanner.strip` output). The guard `#define` is
--   searched as plain text, so a commented-out guard in raw text would move the start of the
--   gate search; `catalog.lua` always strips first.
-- @return table: Sorted array of `MAREA_API_*` names, empty when ungated.
function scanner.gates(text)
   local guard_pos
   for _, pattern in ipairs({ "#%s*define%s+MAREA_INCLUDE_", "#%s*define%s+MAREA_INTERNAL_" }) do
      local pos = text:find(pattern)
      if pos and (not guard_pos or pos < guard_pos) then
         guard_pos = pos
      end
   end
   local start = 1
   if guard_pos then
      local line_end = text:find("\n", guard_pos, true)
      start = line_end and (line_end + 1) or (#text + 1)
   end

   local names = {}
   for line in text:sub(start):gmatch("[^\n]+") do
      local condition = line:match("^%s*#%s*if%s+(.-)%s*$")
      if condition and condition:match("^defined%s*%(") then
         for name in condition:gmatch("defined%s*%(%s*([%u_][%w_]*)%s*%)") do
            if name:match("^MAREA_API_") then
               names[name] = true
            end
         end
         break
      end
   end

   local gates = {}
   for name in pairs(names) do
      gates[#gates + 1] = name
   end
   table.sort(gates)
   return gates
end

--- Records the names of every `enum { ... }` / `struct { ... }` / `union { ... }` body whose
-- keyword is found in `text`, blanking each consumed brace span so later passes never match
-- inside it. Enum tags come from the first identifier after `enum` (a fixed-underlying-type
-- colon clause may follow); struct/union tags come from the LAST identifier before the `{`,
-- because attribute macros such as `MAREA_NODISCARD` sit between the keyword and the tag.
-- @param chars table: Mutable char array of the working text.
-- @param keyword string: "enum", "struct" or "union".
-- @param record function: Called with each declared name.
local function consume_tagged_bodies(chars, keyword, record)
   local attribute_tags = { ["MAREA_NODISCARD"] = true }
   local text = rebuild(chars)
   while true do
      -- The frontier patterns keep an identifier that merely contains the keyword (`is_enum`,
      -- `enumerators`, `structSize`) from reading as the keyword.
      local s, _, after_keyword = text:find("%f[%w_]" .. keyword .. "%f[^%w_]%s*()")
      if not s then
         return
      end
      local keyword_end = s + #keyword - 1
      local tag = text:match("(" .. IDENT_PATTERN .. ")", after_keyword)
      -- Only a `{` before the next `;` starts a body - `typedef struct X X;` has none.
      local brace = text:find("{", after_keyword, true)
      local semicolon = text:find(";", after_keyword, true)
      -- A `=` between the keyword and the brace, or a `)` right before it, means the brace
      -- opens an initializer or a function body (`struct Foo * MAREA_CALL get(void) { ... }`), not a
      -- type body. A paren group earlier in the span is an attribute macro (`MAREA_ALIGNED(4)`) and
      -- is fine.
      local is_body = false
      if brace and (not semicolon or brace < semicolon) then
         local between = text:sub(after_keyword, brace - 1)
         is_body = not between:find("=", 1, true) and not between:find("%)%s*$")
      end
      if is_body then
         local close = match_brace(text, brace)
         local body = text:sub(brace + 1, close - 1)
         if keyword == "enum" then
            for _, name in ipairs(enum_enumerators(body)) do
               record(name)
            end
            record(tag)
         else
            local region = text:sub(after_keyword, brace - 1)
            local struct_tag = region:match("(" .. IDENT_PATTERN .. ")%s*$")
            if struct_tag and attribute_tags[struct_tag] then
               struct_tag = nil
            end
            -- The first identifier after `struct` may be an attribute macro (MAREA_NODISCARD),
            -- so the tag is the LAST identifier before the brace, never that first one.
            record(struct_tag)
         end
         blank_span(chars, s, close)
      else
         blank_span(chars, s, keyword_end)
      end
      text = rebuild(chars)
   end
end

--- Collects the declared symbols of a file by the five declaration forms: `#define` (a name the
-- file also `#undef`s is a redefinition, not a declaration), tagged and anonymous enum/struct/
-- union typedefs (enumerators included), opaque and simple typedefs, and function names
-- anchored by `MAREA_CALL`. Struct/union fields are not symbols; member accesses are excluded
-- from `used` instead.
-- @param text string: Stripped file text.
-- @return table: Set (name -> true) of declared symbol names.
function scanner.declared(text)
   local declared = {}
   local function record(name)
      if name and name ~= "" then
         declared[name] = true
      end
   end

   local chars = {}
   for i = 1, #text do
      chars[i] = text:sub(i, i)
   end

   consume_tagged_bodies(chars, "enum", record)
   consume_tagged_bodies(chars, "struct", record)
   consume_tagged_bodies(chars, "union", record)

   local working = rebuild(chars)

   -- Function-pointer and function typedefs: `typedef R(MAREA_CALL * MAREA_Name)(args);` has the name
   -- between the star and the group's closing paren; `typedef R (MAREA_CALL MAREA_Name)(args);` and
   -- `typedef R MAREA_CALL MAREA_Name(args);` have no star and put the name right after `MAREA_CALL`.
   -- They must be consumed before the generic typedef pass can mistake a parameter for the
   -- declared name. The star-less forms go first, or the star pattern would take a `*x)`
   -- parameter for the name.
   local typedef_forms = {
      "()typedef[^;{}]-%(%s*MAREA_CALL%s+(" .. IDENT_PATTERN .. ")%s*%)()",
      "()typedef[^;{}]-MAREA_CALL%s+(" .. IDENT_PATTERN .. ")%s*%(()",
      "()typedef[^;{}]-%*%s*(" .. IDENT_PATTERN .. ")%s*%)()",
   }
   for _, form in ipairs(typedef_forms) do
      while true do
         local _, _, s, name, e = working:find(form)
         if not s then
            break
         end
         record(name)
         blank_span(chars, s, e - 1)
         working = rebuild(chars)
      end
   end

   -- Generic typedefs: the declared name is the last identifier before the `;` (struct/enum
   -- bodies were already consumed, so only simple and opaque typedefs remain).
   while true do
      local _, _, s, declarator, e = working:find("()typedef%s+([^;{}]-)%s*;()")
      if not s then
         break
      end
      record(declarator:match("(" .. IDENT_PATTERN .. ")%s*$"))
      blank_span(chars, s, e - 1)
      working = rebuild(chars)
   end

   -- Macros: `#define NAME` (the parameter list of a function-like macro stops the capture).
   -- A define preceded in file order by an `#undef` of the same name is a redefinition (the
   -- internal umbrella does this to MAREA_EXTERN/MAREA_EXTERN_DECL), not a declaration.
   local undefined = {}
   local directives = {}
   for init, directive, name in working:gmatch("()#%s*(define)%s+(" .. IDENT_PATTERN .. ")") do
      directives[#directives + 1] = { init = init, kind = "define", name = name }
   end
   for init, directive, name in working:gmatch("()#%s*(undef)%s+(" .. IDENT_PATTERN .. ")") do
      directives[#directives + 1] = { init = init, kind = "undef", name = name }
   end
   table.sort(directives, function(a, b)
      return a.init < b.init
   end)
   for _, item in ipairs(directives) do
      if item.kind == "undef" then
         undefined[item.name] = true
      elseif not undefined[item.name] then
         record(item.name)
      end
   end

   -- Functions: the identifier directly after `MAREA_CALL`, declarations and definitions alike.
   for name in working:gmatch("MAREA_CALL%s*%*?%s*(" .. IDENT_PATTERN .. ")%s*%(") do
      record(name)
   end

   return declared
end

--- Collects the identifiers a file uses: every identifier outside comments and string literals,
-- except struct/union member accesses (`a.b`, `a->b`) and `#include` lines, whose words name a
-- file rather than a symbol (`SDL_image` in `<SDL3_image/SDL_image.h>`). The catalog intersects
-- this superset with the declared-symbol index and the provider prefixes, so plain C
-- identifiers that match nothing simply drop out.
-- @param text string: Stripped file text.
-- @return table: Set (name -> true) of used identifiers.
function scanner.used(text)
   text = text:gsub("[^\n]+", function(line)
      if line:match("^%s*#%s*include") then
         return ""
      end
   end)
   local used = {}
   local i = 1
   local length = #text
   while i <= length do
      local char = text:sub(i, i)
      if char:match("[%a_]") then
         local j = i
         while j + 1 <= length and text:sub(j + 1, j + 1):match("[%w_]") do
            j = j + 1
         end
         local name = text:sub(i, j)
         local previous = i > 1 and text:sub(i - 1, i - 1) or ""
         local before = i > 2 and text:sub(i - 2, i - 1) or ""
         local member_access = previous == "." or before == "->"
         if not member_access then
            used[name] = true
         end
         i = j + 1
      else
         i = i + 1
      end
   end
   return used
end

return scanner
