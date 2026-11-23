-- Copyright (c) 2026 Paulo Lucas Alves Franca
-- Licensed under the MIT License. See LICENSE.txt for full terms.

--- @module catalog
-- File discovery, include resolution, the symbol -> home-header index, per-file reachability
-- and the deterministic JSON encoder that the `catalog` subcommand writes. Everything is sorted
-- (object keys, arrays) so two consecutive runs over an unchanged tree produce byte-identical
-- output - the acceptance test for every later consumer of the catalog.

local scanner = require("scanner")
local path_util = require("path_util")

local catalog = {}

local HEADER_DIRS = { "lib/include", "lib/internal" }
local SOURCE_EXTENSIONS = { ".c", ".cpp", ".hpp", ".h" }

--- Converts a Windows path ("C:/foo" or "C:\foo") to an MSYS/POSIX path ("/c/foo") for use
-- inside a bash -c "..." invocation.
-- @param path string: A Windows-style path.
-- @return string: The path in MSYS form.
local function to_posix_path(path)
   local converted = path:gsub("^([A-Za-z]):[/\\]?", function(d)
      return "/" .. d:lower() .. "/"
   end)
   return converted:gsub("\\", "/")
end

--- Lists files under `dirs` (repo-relative) matching the shell `find` name expression, via one
-- `find` call per directory, returning repo-relative paths normalized for the host.
-- @param root string: Repository root.
-- @param dirs table: Array of repo-relative directories (skipped when absent on disk).
-- @param name_expr string: `-name` expression for find, e.g. "-name '*.h'".
-- @return table: Sorted array of repo-relative file paths.
local function discover(root, dirs, name_expr)
   local found = {}
   -- On Windows, io.popen routes through cmd.exe, which calls the Windows text-search FIND.EXE
   -- instead of POSIX find. Route through Git-for-Windows bash so POSIX find is used; bash also
   -- produces MSYS paths that path_util.normalize already handles.
   local on_windows = package.config:sub(1, 1) == "\\"
   for _, dir in ipairs(dirs) do
      local handle
      if on_windows then
         local posix_path = to_posix_path(root .. "/" .. dir)
         local bash_cmd = "find '" .. posix_path .. "' -type f " .. name_expr .. " 2>/dev/null"
         handle = io.popen('bash --norc -c "' .. bash_cmd .. '"')
      else
         handle = io.popen(
            "find " .. ("%q"):format(root .. "/" .. dir) .. " -type f " .. name_expr .. " 2>/dev/null"
         )
      end
      if handle then
         for line in handle:lines() do
            local normalized = path_util.normalize(line)
            local relative = normalized:sub(#root + 2)
            found[#found + 1] = relative
         end
         handle:close()
      end
   end
   table.sort(found)
   return found
end

--- Builds the find `-name` expression covering every source extension.
-- @return string: e.g. "( -name '*.c' -o -name '*.cpp' ... )".
local function source_name_expr()
   local parts = {}
   for _, extension in ipairs(SOURCE_EXTENSIONS) do
      parts[#parts + 1] = "-name '*" .. extension .. "'"
   end
   return "\\( " .. table.concat(parts, " -o ") .. " \\)"
end

--- Reads a file under root; a missing file yields nil rather than an error so discovery races
-- never crash the catalog.
-- @param root string: Repository root.
-- @param path string: Repo-relative path.
-- @return string|nil: The file content.
local function read_file(root, path)
   local file = io.open(root .. "/" .. path, "r")
   if not file then
      return nil
   end
   local content = file:read("*a")
   file:close()
   return content
end

--- Resolves a quote-include target ("marea/base/MAREAMacros.h") against lib/include then
-- lib/internal, exactly the compiler's search order for engine headers.
-- @param root string: Repository root.
-- @param target string: Include target as written.
-- @return string|nil: Repo-relative header path, or nil when the target is not an engine
--   header (external or unresolved).
local function resolve_include(root, target)
   for _, prefix in ipairs(HEADER_DIRS) do
      local candidate = prefix .. "/" .. target
      if read_file(root, candidate) then
         return candidate
      end
   end
   return nil
end

--- Sorts a set (name -> true) into a fresh sorted array.
-- @param set table: Set to flatten.
-- @return table: Sorted array of the set's keys.
local function sorted_keys(set)
   local keys = {}
   for key in pairs(set) do
      keys[#keys + 1] = key
   end
   table.sort(keys)
   return keys
end

--- Scans one file into a catalog entry: resolved includes, gates, declared symbols and uses
-- (identifiers minus the symbols the file declares itself).
-- @param root string: Repository root.
-- @param path string: Repo-relative file path.
-- @param layer string|nil: "public"/"internal" for headers, nil for sources.
-- @return table: `{ layer?, gates?, includes, declares, declared_set, uses, uses_set }`.
local function scan_file(root, path, layer)
   local content = read_file(root, path) or ""
   local stripped = scanner.strip(content)

   local includes = {}
   for _, include in ipairs(scanner.includes(stripped)) do
      if include.kind == "quote" then
         local resolved = resolve_include(root, include.target)
         if resolved then
            includes[resolved] = true
         end
      end
   end

   local declared_set = scanner.declared(stripped)
   local used_set = scanner.used(stripped)
   local uses_set = {}
   for name in pairs(used_set) do
      if not declared_set[name] then
         uses_set[name] = true
      end
   end

   local entry = {
      includes = sorted_keys(includes),
      declares = sorted_keys(declared_set),
      declared_set = declared_set,
      uses = sorted_keys(uses_set),
      uses_set = uses_set,
   }
   if layer then
      entry.layer = layer
      entry.gates = scanner.gates(stripped)
   end
   return entry
end

--- Computes the transitive include closure of every file (headers resolving other headers;
-- sources resolving headers), exposed as `catalog.reachable[path]` sets for later subcommands.
-- @param cat table: The catalog being built.
local function compute_reachability(cat)
   local function closure_for(path, visited)
      local entry = cat.files[path]
      if not entry then
         return
      end
      for _, include in ipairs(entry.includes) do
         if not visited[include] then
            visited[include] = true
            closure_for(include, visited)
         end
      end
   end

   for path in pairs(cat.files) do
      local visited = { [path] = false }
      closure_for(path, visited)
      visited[path] = nil
      cat.reachable[path] = visited
   end
end

--- Builds the full catalog of a repository tree.
-- @param root string: Repository root (the one containing lib/).
-- @param options table: `{ sources = {dir, ...} }` extra source directories, relative to root.
-- @return table: The catalog (`headers`, `sources`, `files`, `index`, `duplicates`,
--   `packages`, `reachable`, `summary`). Raises when the root holds no header at all, so a
--   wrong root never reads as an empty success.
function catalog.build(root, options)
   local cat = {
      root = root,
      headers = {},
      sources = {},
      files = {},
      index = {},
      duplicates = {},
      packages = {},
      reachable = {},
      summary = { headers = 0, sources = 0 },
   }

   local packages = {}
   local header_paths = discover(root, HEADER_DIRS, "-name '*.h'")
   if #header_paths == 0 then
      error("marea-api-catalog: no header found under root '" .. root .. "'", 0)
   end
   for _, path in ipairs(header_paths) do
      local layer = path:match("^lib/include/") and "public" or "internal"
      local entry = scan_file(root, path, layer)
      cat.headers[path] = entry
      cat.files[path] = entry
      cat.summary.headers = cat.summary.headers + 1
      for _, gate in ipairs(entry.gates) do
         packages[gate] = true
      end
      for _, name in ipairs(entry.declares) do
         local homes = cat.index[name] or {}
         homes[#homes + 1] = path
         cat.index[name] = homes
      end
   end

   for name, homes in pairs(cat.index) do
      table.sort(homes)
      if #homes > 1 then
         cat.duplicates[name] = homes
      end
   end

   cat.packages = sorted_keys(packages)

   local source_dirs = { "lib/source" }
   for _, dir in ipairs(options.sources or {}) do
      source_dirs[#source_dirs + 1] = dir
   end
   local source_paths = discover(root, source_dirs, source_name_expr())
   for _, path in ipairs(source_paths) do
      -- Headers under lib/include and lib/internal already entered with a layer; a source dir
      -- that overlaps them keeps the header entry (first scan wins).
      if not cat.files[path] then
         local entry = scan_file(root, path, nil)
         cat.sources[path] = entry
         cat.files[path] = entry
         cat.summary.sources = cat.summary.sources + 1
      end
   end

   compute_reachability(cat)
   return cat
end

-- ---------------------------------------------------------------------------
-- Deterministic JSON encoder
-- ---------------------------------------------------------------------------

-- Marks a table as a JSON array, so an empty one is written as `[]` and not as `{}` (an empty
-- Lua table has no kind of its own).
local ARRAY_MARK = {}

--- Marks `value` as an array for the encoder. Only array fields need it; an unmarked empty table
-- is encoded as `{}`.
-- @param value table: The array (or empty table).
-- @return table: The same table, marked.
function catalog.array(value)
   return setmetatable(value, ARRAY_MARK)
end

--- Escapes and quotes one JSON string, rejecting non-ASCII bytes so the catalog can never
-- drift between locales (project rule: source is ASCII-only anyway).
-- @param value string: Raw string.
-- @return string: Quoted, escaped JSON string.
local function encode_string(value)
   local escaped = value:gsub('["\\%z\1-\31]', function(char)
      if char == '"' then
         return '\\"'
      elseif char == "\\" then
         return "\\\\"
      end
      return string.format("\\u%04x", string.byte(char))
   end)
   if escaped:find("[\128-\255]") then
      error("catalog: refusing to encode non-ASCII string: " .. value)
   end
   return '"' .. escaped .. '"'
end

--- Encodes one value as deterministic JSON: object keys sorted, arrays in order, 2-space
-- indent per level, ASCII only.
-- @param value table|string|number|boolean: Value to encode.
-- @param indent number: Current nesting depth.
-- @return string: The encoded fragment.
local function encode_value(value, indent)
   local kind = type(value)
   if kind == "string" then
      return encode_string(value)
   elseif kind == "number" then
      if value ~= math.floor(value) then
         error("catalog: refusing to encode non-integer number: " .. tostring(value))
      end
      return string.format("%d", value)
   elseif kind == "boolean" then
      return tostring(value)
   elseif kind == "table" then
      local padding = string.rep("  ", indent)
      local inner = string.rep("  ", indent + 1)
      if #value > 0 or next(value) == nil then
         local items = {}
         for i = 1, #value do
            items[#items + 1] = inner .. encode_value(value[i], indent + 1)
         end
         if #items == 0 then
            return getmetatable(value) == ARRAY_MARK and "[]" or "{}"
         end
         return "[\n" .. table.concat(items, ",\n") .. "\n" .. padding .. "]"
      end
      local keys = sorted_keys(value)
      local items = {}
      for _, key in ipairs(keys) do
         items[#items + 1] = inner .. encode_string(key) .. ": " .. encode_value(value[key], indent + 1)
      end
      return "{\n" .. table.concat(items, ",\n") .. "\n" .. padding .. "}"
   end
   error("catalog: cannot encode value of type " .. kind)
end

--- Projects the catalog into the JSON document shape: headers carry layer/gates/includes/
-- declares, sources carry includes/uses, plus the sorted packages union and duplicates map.
-- @param cat table: A built catalog.
-- @return table: The JSON-ready document.
local function to_document(cat)
   local document = { headers = {}, sources = {}, packages = catalog.array(cat.packages), duplicates = {} }
   for path, entry in pairs(cat.headers) do
      document.headers[path] = {
         layer = entry.layer,
         gates = catalog.array(entry.gates),
         includes = catalog.array(entry.includes),
         declares = catalog.array(entry.declares),
      }
   end
   for path, entry in pairs(cat.sources) do
      document.sources[path] = {
         includes = catalog.array(entry.includes),
         uses = catalog.array(entry.uses),
      }
   end
   for name, homes in pairs(cat.duplicates) do
      document.duplicates[name] = homes
   end
   return document
end

--- Encodes the catalog as deterministic JSON text (sorted keys, 2-space indent, trailing
-- newline left to the writer).
-- @param cat table: A built catalog.
-- @return string: The JSON document.
function catalog.encode_json(cat)
   return encode_value(to_document(cat), 0)
end

--- Encodes any JSON-representable value (the same deterministic encoder the catalog document
-- uses) - the check subcommand reuses it for its `--json` report.
-- @param value table|string|number|boolean: Value to encode.
-- @return string: The encoded fragment.
function catalog.encode(value)
   return encode_value(value, 0)
end

--- Writes the catalog JSON to `out`, creating the parent directory when needed. `out` is
-- interpreted exactly as given (relative to the caller's working directory).
-- @param cat table: A built catalog.
-- @param root string: Repository root (unused today; keeps the call symmetric with build).
-- @param out string: Output path.
function catalog.write_json(cat, root, out)
   local dir = out:match("^(.*)/[^/]+$")
   if dir and dir ~= "" then
      os.execute("cmake -E make_directory " .. ("%q"):format(dir))
   end
   local file = assert(io.open(out, "w"), "catalog: cannot open " .. out .. " for writing")
   file:write(catalog.encode_json(cat))
   file:write("\n")
   file:close()
end

return catalog
