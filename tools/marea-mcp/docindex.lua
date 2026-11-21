-- Copyright (c) 2026 Paulo Lucas Alves Franca
-- Licensed under the MIT License. See LICENSE.txt for full terms.

--- @module docindex
-- Position-aware symbol lookup over C11 headers and sources, independent of
-- tools/marea-api-catalog/scanner.lua: `scanner.declared` only answers "does this file declare
-- X" for the catalog, with no position tracked at all, so threading position-tracking through
-- its brace-consuming, text-rebuilding algorithm would complicate a module the catalog's own
-- byte-identical-rebuild behavior depends on. This module has a narrower job -- given a symbol
-- NAME already known to be declared somewhere, find exactly WHERE and return just that slice --
-- so it re-implements the same five declaration shapes independently (object/function-like
-- macros, typedefs, tagged enum/struct/union bodies with their enumerators, and MAREA_CALL
-- functions), scoped to position tracking from the start. Reuses only `scanner.strip` (comments
-- and string/char literals blanked to spaces, same length and line count as the raw file), never
-- `scanner.declared` or anything in catalog.lua.

local scanner = require("scanner")

local docindex = {}

local IDENT_PATTERN = "[%a_][%w_]*"

-- ---------------------------------------------------------------------------
-- Small text helpers
-- ---------------------------------------------------------------------------

--- Reads a file whole, in binary mode so byte values are exact.
-- @param root string: Repository (or fixture) root.
-- @param file string: Root-relative file path.
-- @return string: The raw file content. Raises when the file cannot be opened.
local function read_raw(root, file)
   local path = root .. "/" .. file
   local handle, open_err = io.open(path, "rb")
   if not handle then
      error("docindex: cannot open " .. path .. ": " .. tostring(open_err), 0)
   end
   local content = handle:read("*a")
   handle:close()
   return content
end

--- Splits text into lines without a trailing empty line for a final "\n".
-- @param text string: Raw or stripped text.
-- @return table: Array of lines (1-based), no line terminators.
local function split_lines(text)
   local lines = {}
   local start = 1
   while true do
      local newline = text:find("\n", start, true)
      if not newline then
         if start <= #text then
            lines[#lines + 1] = text:sub(start)
         end
         break
      end
      lines[#lines + 1] = text:sub(start, newline - 1)
      start = newline + 1
   end
   return lines
end

--- Converts a 1-based character offset into its 1-based line number, by counting the newlines
-- before it. Valid against the RAW file too, since `scanner.strip` keeps the same length and
-- line count as the input.
-- @param text string: Stripped text the offset was found in.
-- @param offset number: 1-based character offset.
-- @return number: 1-based line number.
local function offset_to_line(text, offset)
   local count = 0
   for _ in text:sub(1, offset - 1):gmatch("\n") do
      count = count + 1
   end
   return count + 1
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
   error("docindex: unbalanced braces", 0)
end

--- Finds the paren-balanced `( ... )` span opening at `open_pos` (1-based index of the `(`).
-- @param text string: Text to scan.
-- @param open_pos number: Index of the opening paren.
-- @return number: Index of the matching closing paren.
local function match_paren(text, open_pos)
   local depth = 0
   for i = open_pos, #text do
      local char = text:sub(i, i)
      if char == "(" then
         depth = depth + 1
      elseif char == ")" then
         depth = depth - 1
         if depth == 0 then
            return i
         end
      end
   end
   error("docindex: unbalanced parens", 0)
end

--- Replaces the span `first..last` with spaces, keeping newlines, so a later pass sees the same
-- line structure but never matches inside an already-consumed declaration.
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

-- ---------------------------------------------------------------------------
-- The five declaration shapes, with position tracking
-- ---------------------------------------------------------------------------

--- Scans stripped text for every declared symbol, mirroring the five forms
-- `tools/marea-api-catalog/scanner.lua`'s `scanner.declared` recognizes, but keeping each
-- match's character span instead of only its name.
-- @param text string: Stripped file text (`scanner.strip` output).
-- @return table: Map `name -> { kind, start, finish }` (1-based, inclusive character offsets).
local function scan_declarations(text)
   local declared = {}
   local function record(name, kind, start, finish)
      if name and name ~= "" and not declared[name] then
         declared[name] = { kind = kind, start = start, finish = finish }
      end
   end

   local chars = {}
   for i = 1, #text do
      chars[i] = text:sub(i, i)
   end

   local attribute_tags = { MAREA_NODISCARD = true }

   --- Consumes every tagged body of one keyword ("enum", "struct" or "union"), recording its
   -- tag (plus enumerators for "enum") and blanking the consumed span.
   -- @param keyword string: "enum", "struct" or "union".
   local function consume_tagged_bodies(keyword)
      local working = rebuild(chars)
      while true do
         local s, _, after_keyword = working:find("%f[%w_]" .. keyword .. "%f[^%w_]%s*()")
         if not s then
            return
         end
         local keyword_end = s + #keyword - 1
         local tag = working:match("(" .. IDENT_PATTERN .. ")", after_keyword)
         local brace = working:find("{", after_keyword, true)
         local semicolon = working:find(";", after_keyword, true)
         local is_body = false
         if brace and (not semicolon or brace < semicolon) then
            local between = working:sub(after_keyword, brace - 1)
            is_body = not between:find("=", 1, true) and not between:find("%)%s*$")
         end
         if is_body then
            local close = match_brace(working, brace)
            local semi = working:find(";", close, true)
            local finish = semi or close
            if keyword == "enum" then
               local depth = 0
               local seg_start = brace + 1
               local idx = seg_start
               local function record_enumerator(from, to, end_pos)
                  local segment = working:sub(from, to)
                  local ident_start, ident_end, ident = segment:find("(" .. IDENT_PATTERN .. ")")
                  if ident then
                     record(ident, "enumerator", from + ident_start - 1, end_pos)
                  end
               end
               while idx <= close - 1 do
                  local char = working:sub(idx, idx)
                  if char == "{" or char == "(" then
                     depth = depth + 1
                  elseif char == "}" or char == ")" then
                     depth = depth - 1
                  elseif char == "," and depth == 0 then
                     record_enumerator(seg_start, idx - 1, idx)
                     seg_start = idx + 1
                  end
                  idx = idx + 1
               end
               record_enumerator(seg_start, close - 1, close)
               record(tag, "type", s, finish)
            else
               local region = working:sub(after_keyword, brace - 1)
               local struct_tag
               for candidate in region:gmatch(IDENT_PATTERN) do
                  struct_tag = candidate
               end
               if struct_tag and attribute_tags[struct_tag] then
                  struct_tag = nil
               end
               record(struct_tag, "type", s, finish)
            end
            blank_span(chars, s, close)
         else
            blank_span(chars, s, keyword_end)
         end
         working = rebuild(chars)
      end
   end

   consume_tagged_bodies("enum")
   consume_tagged_bodies("struct")
   consume_tagged_bodies("union")

   local working = rebuild(chars)

   -- Function-pointer and function typedefs, consumed before the generic typedef pass can
   -- mistake a parameter for the declared name -- same three forms and order as
   -- scanner.declared, reimplemented here with position tracking.
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
         local semi = working:find(";", e, true) or e
         record(name, "type", s, semi)
         blank_span(chars, s, e - 1)
         working = rebuild(chars)
      end
   end

   -- Generic typedefs: the declared name is the last identifier before the ";".
   while true do
      local _, _, s, declarator, e = working:find("()typedef%s+([^;{}]-)%s*;()")
      if not s then
         break
      end
      local name = declarator:match("(" .. IDENT_PATTERN .. ")%s*$")
      record(name, "type", s, e - 1)
      blank_span(chars, s, e - 1)
      working = rebuild(chars)
   end

   -- Macros: "#define NAME", continuing over any line ending in a bare "\". A name later
   -- "#undef"'d is a redefinition, not a declaration -- same rule as scanner.declared.
   local undefined = {}
   local directives = {}
   for init, name in working:gmatch("()#%s*define%s+(" .. IDENT_PATTERN .. ")") do
      directives[#directives + 1] = { init = init, kind = "define", name = name }
   end
   for init, name in working:gmatch("()#%s*undef%s+(" .. IDENT_PATTERN .. ")") do
      directives[#directives + 1] = { init = init, kind = "undef", name = name }
   end
   table.sort(directives, function(a, b)
      return a.init < b.init
   end)
   for _, item in ipairs(directives) do
      if item.kind == "undef" then
         undefined[item.name] = true
      elseif not undefined[item.name] then
         local line_end = working:find("\n", item.init, true) or (#working + 1)
         local finish = line_end - 1
         while finish >= item.init and working:sub(finish, finish):match("[ \t]") do
            finish = finish - 1
         end
         while finish >= 1 and working:sub(finish, finish) == "\\" do
            local next_start = (working:find("\n", finish, true) or #working) + 1
            local next_end = working:find("\n", next_start, true)
            finish = next_end and (next_end - 1) or #working
            while finish >= next_start and working:sub(finish, finish):match("[ \t]") do
               finish = finish - 1
            end
         end
         record(item.name, "macro", item.init, finish)
      end
   end

   -- Functions: the identifier directly after MAREA_CALL, anchored the same way
   -- scanner.declared finds it, ending at the parameter list's own closing paren, or at the
   -- following ";" (prototype) or "{" (definition head) when one directly follows.
   local search_from = 1
   while true do
      local s, open_paren, name =
         working:find("MAREA_CALL%s*%*?%s*(" .. IDENT_PATTERN .. ")%s*%(", search_from)
      if not s then
         break
      end
      local close_paren = match_paren(working, open_paren)
      local after = close_paren + 1
      local ws_pos = working:find("%S", after) or after
      local terminator = working:sub(ws_pos, ws_pos)
      local finish = close_paren
      if terminator == ";" or terminator == "{" then
         finish = ws_pos
      end
      record(name, "function", s, finish)
      search_from = close_paren + 1
   end

   return declared
end

-- ---------------------------------------------------------------------------
-- Backward doc-comment walk (get_symbol / get_function_body only)
-- ---------------------------------------------------------------------------

--- Trims leading and trailing whitespace.
-- @param text string: Text to trim.
-- @return string: The trimmed text.
local function trim(text)
   return text:match("^%s*(.-)%s*$")
end

--- Walks a declaration's own start line backward over a directly adjacent `/** ... */` block
-- (up to the matching opening `/**`/`/*`) or a contiguous run of `//` lines, stopping at the
-- first line that is neither -- a blank line stops the walk immediately, never skipped.
-- @param raw_lines table: The file's raw lines (1-based).
-- @param start_line number: The declaration's own first line (before this walk).
-- @return number: The (possibly earlier) first line to show, comment included.
local function backward_doc_walk(raw_lines, start_line)
   local prev_index = start_line - 1
   local prev_line = raw_lines[prev_index]
   if not prev_line then
      return start_line
   end
   local trimmed = trim(prev_line)
   if trimmed:sub(1, 2) == "//" then
      local top = prev_index
      while top - 1 >= 1 do
         local above = raw_lines[top - 1]
         if above and trim(above):sub(1, 2) == "//" then
            top = top - 1
         else
            break
         end
      end
      return top
   end
   if trimmed:sub(-2) == "*/" then
      local top = prev_index
      while top >= 1 do
         if trim(raw_lines[top]):sub(1, 2) == "/*" then
            return top
         end
         top = top - 1
      end
   end
   return start_line
end

-- ---------------------------------------------------------------------------
-- Public API
-- ---------------------------------------------------------------------------

--- Locates every requested name declared in one file.
-- @param root string: Repository (or fixture) root.
-- @param file string: Root-relative file path.
-- @param names table: Array of symbol names to locate.
-- @return table: Map `name -> { kind, start_line, end_line }` for every requested name that
--   `file` actually declares (a name not found is simply absent from the map). Raises when
--   `file` cannot be opened.
function docindex.locate_all(root, file, names)
   local raw = read_raw(root, file)
   local stripped = scanner.strip(raw)
   local declared = scan_declarations(stripped)

   local result = {}
   for _, name in ipairs(names) do
      local entry = declared[name]
      if entry then
         result[name] = {
            kind = entry.kind,
            start_line = offset_to_line(stripped, entry.start),
            end_line = offset_to_line(stripped, entry.finish),
         }
      end
   end
   return result
end

--- Returns one symbol's doc comment plus its declaration (never a whole file).
-- @param root string: Repository (or fixture) root.
-- @param file string: Root-relative file path.
-- @param name string: Symbol name.
-- @return table|nil: `{ file, name, kind, start_line, end_line, text }` on success.
-- @return string|nil: A non-empty error message when `name` is not declared in `file`; nil on
--   success. Never raises for an unknown name (a missing FILE still raises).
function docindex.get_symbol(root, file, name)
   local located = docindex.locate_all(root, file, { name })
   local entry = located[name]
   if not entry then
      return nil, "docindex: symbol '" .. name .. "' not found in " .. file
   end

   local raw_lines = split_lines(read_raw(root, file))
   local start_line = backward_doc_walk(raw_lines, entry.start_line)
   local text = table.concat(raw_lines, "\n", start_line, entry.end_line)
   return {
      file = file,
      name = name,
      kind = entry.kind,
      start_line = start_line,
      end_line = entry.end_line,
      text = text,
   }
end

--- Returns one `.c` function's full definition (doc/TAG comment plus body).
-- @param root string: Repository (or fixture) root.
-- @param file string: Root-relative file path.
-- @param name string: Function name.
-- @return table|nil: `{ file, name, start_line, end_line, text }` on success.
-- @return string|nil: A non-empty error message when `name` is not found, or is only a
--   prototype (a ";" where a "{" was expected); nil on success. Never raises for either case
--   (a missing FILE still raises).
function docindex.get_function_body(root, file, name)
   local raw = read_raw(root, file)
   local stripped = scanner.strip(raw)

   local pattern = "MAREA_CALL%s*%*?%s*" .. name .. "%f[^%w_]%s*%("
   local start, open_paren = stripped:find(pattern)
   if not start then
      return nil, "docindex: function '" .. name .. "' not found in " .. file
   end

   local close_paren = match_paren(stripped, open_paren)
   local ws_pos = stripped:find("%S", close_paren + 1) or (close_paren + 1)
   local terminator = stripped:sub(ws_pos, ws_pos)
   if terminator ~= "{" then
      return nil, "docindex: '" .. name .. "' in " .. file .. " is only a declaration, it has no body"
   end

   local close_brace = match_brace(stripped, ws_pos)
   local raw_lines = split_lines(raw)
   local start_line = backward_doc_walk(raw_lines, offset_to_line(stripped, start))
   local end_line = offset_to_line(stripped, close_brace)
   local text = table.concat(raw_lines, "\n", start_line, end_line)
   return { file = file, name = name, start_line = start_line, end_line = end_line, text = text }
end

--- Returns a bounded, literal line range (no doc-comment walk -- an explicit range is taken
-- exactly as given).
-- @param root string: Repository (or fixture) root.
-- @param file string: Root-relative file path.
-- @param start_line number: First 1-based line to return.
-- @param end_line number: Last 1-based line to return (inclusive).
-- @param max_lines number|nil: Cap on `end_line - start_line + 1`; defaults to 200.
-- @return table|nil: `{ file, start_line, end_line, text }` on success.
-- @return string|nil: A non-empty error message naming the cap when the requested range
--   exceeds it, or naming that `start_line` is past the end of the file; nil on success. Never
--   raises for either case (a missing FILE still raises).
function docindex.get_snippet(root, file, start_line, end_line, max_lines)
   max_lines = max_lines or 200
   local raw_lines = split_lines(read_raw(root, file))

   if start_line > #raw_lines then
      return nil,
         "docindex: start_line "
            .. start_line
            .. " is past the end of "
            .. file
            .. " ("
            .. #raw_lines
            .. " line(s))"
   end

   local span = end_line - start_line + 1
   if span > max_lines then
      return nil,
         "docindex: requested range ("
            .. span
            .. " line(s)) exceeds the cap of "
            .. max_lines
            .. "; narrow it"
   end

   end_line = math.min(end_line, #raw_lines)
   local text = table.concat(raw_lines, "\n", start_line, end_line)
   return { file = file, start_line = start_line, end_line = end_line, text = text }
end

return docindex
