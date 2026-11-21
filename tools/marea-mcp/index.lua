-- Copyright (c) 2026 Paulo Lucas Alves Franca
-- Licensed under the MIT License. See LICENSE.txt for full terms.

--- @module index
-- A thin layer over tools/marea-api-catalog/catalog.lua's symbol index: module listing, a
-- per-file symbol listing and a substring search, each resolved to a position through
-- docindex.lua. `index.build` runs a live filesystem scan every call (no persisted/cached JSON
-- anywhere), so any file already on disk when a caller builds the index is picked up with no
-- code change here.

local catalog = require("catalog")
local docindex = require("docindex")

local index = {}

local LAYER_ORDER = { public = 0, internal = 1 }

--- Splits a header path into its module name and layer, by the path segment right after
-- "lib/include/marea/" (layer "public") or "lib/internal/marea/internal/" (layer "internal").
-- @param path string: Repo-relative header path.
-- @return string|nil, string|nil: The module name and layer, or nil, nil when `path` matches
--   neither prefix.
local function module_of(path)
   local rest = path:match("^lib/include/marea/([^/]+)/")
   if rest then
      return rest, "public"
   end
   rest = path:match("^lib/internal/marea/internal/([^/]+)/")
   if rest then
      return rest, "internal"
   end
   return nil, nil
end

--- Builds the catalog for `root` (wraps `catalog.build`).
-- @param root string: Repository root (the one containing lib/).
-- @return table: The catalog, exactly as `catalog.build` returns it.
function index.build(root)
   return catalog.build(root, {})
end

--- Lists every header module, grouped by module name and layer.
-- @param cat table: A built catalog.
-- @param layer string|nil: "public" or "internal" to narrow the listing; nil for both.
-- @return table: Array of `{ module, layer, headers = {sorted paths}, symbol_count }`, sorted
--   by module name, then layer ("public" before "internal").
function index.list_modules(cat, layer)
   local groups = {}
   local order = {}
   for path, entry in pairs(cat.headers) do
      local module, entry_layer = module_of(path)
      if module and (not layer or entry_layer == layer) then
         local key = module .. "|" .. entry_layer
         local group = groups[key]
         if not group then
            group = { module = module, layer = entry_layer, headers = {}, symbol_count = 0 }
            groups[key] = group
            order[#order + 1] = group
         end
         group.headers[#group.headers + 1] = path
         group.symbol_count = group.symbol_count + #entry.declares
      end
   end
   for _, group in ipairs(order) do
      table.sort(group.headers)
   end
   table.sort(order, function(a, b)
      if a.module ~= b.module then
         return a.module < b.module
      end
      return LAYER_ORDER[a.layer] < LAYER_ORDER[b.layer]
   end)
   return order
end

--- Lists every symbol one file declares, resolved to its line and kind.
-- @param root string: Repository root.
-- @param cat table: A built catalog.
-- @param file string: Repo-relative file path (a key of `cat.files`).
-- @return table: Array of `{ name, kind, line }`, in ascending line order (empty when `file`
--   is not in the catalog or declares nothing).
function index.list_symbols_in_file(root, cat, file)
   local entry = cat.files[file]
   if not entry or #entry.declares == 0 then
      return {}
   end

   local located = docindex.locate_all(root, file, entry.declares)
   local results = {}
   for name, info in pairs(located) do
      results[#results + 1] = { name = name, kind = info.kind, line = info.start_line }
   end
   table.sort(results, function(a, b)
      if a.line ~= b.line then
         return a.line < b.line
      end
      return a.name < b.name
   end)
   return results
end

--- Searches declared symbol names by case-insensitive substring, optionally narrowed to one
-- module.
-- @param root string: Repository root.
-- @param cat table: A built catalog.
-- @param query string: Substring to match (case-insensitive) against a symbol name.
-- @param module string|nil: Module name (as `list_modules` reports it) to narrow the search to.
-- @param limit number|nil: Maximum number of hits to return; defaults to 20.
-- @return table: Array of `{ name, file, line, kind }` (never nil; empty when nothing
--   matches), one `docindex.locate_all` call per distinct home file, not per matching symbol.
function index.search_symbols(root, cat, query, module, limit)
   limit = limit or 20
   local lower_query = query:lower()

   local names_by_file = {}
   local files = {}
   for name, homes in pairs(cat.index) do
      if name:lower():find(lower_query, 1, true) then
         for _, home in ipairs(homes) do
            local home_module = module_of(home)
            if not module or home_module == module then
               if not names_by_file[home] then
                  names_by_file[home] = {}
                  files[#files + 1] = home
               end
               local names = names_by_file[home]
               names[#names + 1] = name
            end
         end
      end
   end
   table.sort(files)

   local results = {}
   for _, file in ipairs(files) do
      local names = names_by_file[file]
      table.sort(names)
      local located = docindex.locate_all(root, file, names)
      for _, name in ipairs(names) do
         local info = located[name]
         if info and #results < limit then
            results[#results + 1] = { name = name, file = file, line = info.start_line, kind = info.kind }
         end
      end
   end
   return results
end

return index
