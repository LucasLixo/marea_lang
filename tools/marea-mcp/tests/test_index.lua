-- Copyright (c) 2026 Paulo Lucas Alves Franca
-- Licensed under the MIT License. See LICENSE.txt for full terms.

--- @script test_index
-- index.lua wraps tools/marea-api-catalog/catalog.lua's symbol index with module listing, a
-- per-file symbol listing and a substring search, each backed by docindex.lua for position.
-- The fixture tree under fixtures/index-root mirrors catalog.build's own hardcoded layout
-- (lib/include, lib/internal, lib/source relative to root) in miniature, so this test never
-- breaks as the real lib/ tree grows.

local script_dir = (arg[0]:gsub("\\", "/")):match("(.*/)") or "./"
package.path = script_dir
   .. "../?.lua;"
   .. script_dir
   .. "../../marea-api-catalog/?.lua;"
   .. script_dir
   .. "../../marea-shared/?.lua;"
   .. package.path

local index = require("index")

local root = script_dir .. "fixtures/index-root"
local cat = index.build(root)

--- Finds the one entry of `list` whose fields match every key of `filter`.
-- @param list table: Array of plain tables.
-- @param filter table: Field -> expected value.
-- @return table|nil: The matching entry, or nil.
local function find(list, filter)
   for _, item in ipairs(list) do
      local matches = true
      for key, value in pairs(filter) do
         if item[key] ~= value then
            matches = false
            break
         end
      end
      if matches then
         return item
      end
   end
   return nil
end

-- list_modules: every header groups by module then layer, in that sorted order.
do
   local modules = index.list_modules(cat)
   assert(#modules == 3, "expected 3 module groups, got " .. #modules)
   assert(modules[1].module == "gizmo" and modules[1].layer == "public")
   assert(modules[2].module == "widget" and modules[2].layer == "public")
   assert(modules[3].module == "widget" and modules[3].layer == "internal")

   local gizmo = find(modules, { module = "gizmo", layer = "public" })
   assert(gizmo.symbol_count == 1, "gizmo symbol_count: " .. tostring(gizmo.symbol_count))
   assert(#gizmo.headers == 1 and gizmo.headers[1]:find("MAREAGizmo.h", 1, true))

   local widget_public = find(modules, { module = "widget", layer = "public" })
   assert(
      widget_public.symbol_count == 2,
      "widget public symbol_count: " .. tostring(widget_public.symbol_count)
   )

   local widget_internal = find(modules, { module = "widget", layer = "internal" })
   assert(
      widget_internal.symbol_count == 1,
      "widget internal symbol_count: " .. tostring(widget_internal.symbol_count)
   )
   print("test_index: list_modules all layers OK")
end

-- list_modules: a layer filter narrows the listing.
do
   local public_modules = index.list_modules(cat, "public")
   assert(#public_modules == 2, "expected 2 public module groups, got " .. #public_modules)
   local internal_modules = index.list_modules(cat, "internal")
   assert(#internal_modules == 1, "expected 1 internal module group, got " .. #internal_modules)
   print("test_index: list_modules layer filter OK")
end

-- list_symbols_in_file: resolved in line order, via one docindex call (not asserted directly,
-- just the observable result shape).
do
   local symbols = index.list_symbols_in_file(root, cat, "lib/include/marea/widget/MAREAWidget.h")
   assert(#symbols == 2, "expected 2 symbols, got " .. #symbols)
   assert(symbols[1].name == "WIDGET_MAGIC" and symbols[1].kind == "macro" and symbols[1].line == 4)
   assert(symbols[2].name == "WidgetGetMagic" and symbols[2].kind == "function" and symbols[2].line == 6)
   print("test_index: list_symbols_in_file OK")
end

-- search_symbols: case-insensitive substring match, grouped hits from both headers that
-- declare a "widget"-matching name (the public one and the internal one); the source file's own
-- WidgetGetMagic definition is not separately indexed by catalog.build, so still exactly 3 hits.
do
   local hits = index.search_symbols(root, cat, "widget")
   assert(#hits == 3, "expected 3 hits, got " .. #hits)
   local magic = find(hits, { name = "WIDGET_MAGIC" })
   assert(magic and magic.kind == "macro" and magic.line == 4 and magic.file:find("MAREAWidget.h", 1, true))
   local get_magic = find(hits, { name = "WidgetGetMagic" })
   assert(get_magic and get_magic.kind == "function" and get_magic.line == 6)
   local internal_flag = find(hits, { name = "WIDGET_INTERNAL_FLAG" })
   assert(
      internal_flag
         and internal_flag.kind == "macro"
         and internal_flag.file:find("MAREAWidgetInternal.h", 1, true)
   )
   print("test_index: search_symbols case-insensitive OK")
end

-- search_symbols: a module filter excludes every hit outside it.
do
   local hits = index.search_symbols(root, cat, "Widget", "gizmo")
   assert(#hits == 0, "expected no hits, got " .. #hits)
   print("test_index: search_symbols module filter OK")
end

-- search_symbols: one hit in the other module.
do
   local hits = index.search_symbols(root, cat, "Gizmo")
   assert(#hits == 1, "expected 1 hit, got " .. #hits)
   assert(hits[1].name == "GizmoPing" and hits[1].kind == "function" and hits[1].line == 4)
   print("test_index: search_symbols single hit OK")
end

-- search_symbols: no match returns an empty array, never nil.
do
   local hits = index.search_symbols(root, cat, "doesnotexist")
   assert(hits ~= nil, "search_symbols must never return nil")
   assert(#hits == 0, "expected no hits, got " .. #hits)
   print("test_index: search_symbols no match OK")
end

print("test_index: ALL OK")
