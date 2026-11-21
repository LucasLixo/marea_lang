-- Copyright (c) 2026 Paulo Lucas Alves Franca
-- Licensed under the MIT License. See LICENSE.txt for full terms.

--- @module tools
-- The static MCP tool catalog: seven `{ name, description, inputSchema, handler(state, args) }`
-- entries. The six read-only handlers read `state.cat`/`state.root` and call straight into
-- index.lua/docindex.lua, returning a plain Lua value (never writing JSON themselves --
-- protocol.lua decides how to shape a handler's result into MCP content). `refresh_index` is
-- the only handler allowed to replace `state.cat`, which it does by mutating `state` itself so
-- every other tool call in the same session sees the rebuilt index with no restart.

local json = require("json")
local index = require("index")
local docindex = require("docindex")

--- Raises when a docindex lookup signals failure (its own `nil, err` convention), so every
-- handler fails through the one path `protocol.lua`'s `pcall` already covers.
-- @param value any: The lookup's first return value.
-- @param err string|nil: The lookup's second return value (the error, when `value` is nil).
-- @return any: `value`, unchanged, when it is not nil.
local function unwrap(value, err)
   if value == nil then
      error(err or "tools: lookup failed", 0)
   end
   return value
end

--- Builds a tool's `inputSchema`: a JSON Schema object, `required` always array-marked (an
-- empty `required` list round-trips as `{}`, not `[]`, unless marked -- see json.lua).
-- @param properties table: Property name -> JSON Schema fragment.
-- @param required table: Array of required property names (may be empty).
-- @return table: `{ type = "object", properties, required }`.
local function schema(properties, required)
   return { type = "object", properties = properties, required = json.array(required) }
end

local tools = {
   {
      name = "list_modules",
      description = "List every header module under lib/include and lib/internal, grouped by "
         .. "module name and layer, with each group's header count and symbol count. Optionally "
         .. "narrow to one layer.",
      inputSchema = schema({ layer = { type = "string", description = "'public' or 'internal'" } }, {}),
      handler = function(state, args)
         return index.list_modules(state.cat, args.layer)
      end,
   },
   {
      name = "search_symbols",
      description = "Search declared symbol names by case-insensitive substring, optionally "
         .. "narrowed to one module. Returns each hit's name, home file, line and kind.",
      inputSchema = schema({
         query = { type = "string", description = "Substring to match against a symbol name" },
         module = { type = "string", description = "Module name to narrow the search to" },
         limit = { type = "number", description = "Maximum number of hits (default 20)" },
      }, { "query" }),
      handler = function(state, args)
         return index.search_symbols(state.root, state.cat, args.query, args.module, args.limit)
      end,
   },
   {
      name = "list_symbols_in_file",
      description = "List every symbol one file declares, resolved to its line and kind, in "
         .. "ascending line order.",
      inputSchema = schema(
         { file = { type = "string", description = "Repo-relative file path" } },
         { "file" }
      ),
      handler = function(state, args)
         return index.list_symbols_in_file(state.root, state.cat, args.file)
      end,
   },
   {
      name = "get_symbol",
      description = "Return one symbol's doc comment plus its declaration -- never a whole "
         .. "file. Pass 'file' when known; otherwise the symbol's single home header is used "
         .. "when it has exactly one.",
      inputSchema = schema({
         name = { type = "string", description = "Symbol name" },
         file = { type = "string", description = "Repo-relative file path (optional when unambiguous)" },
      }, { "name" }),
      handler = function(state, args)
         local file = args.file
         if not file then
            local homes = state.cat.index[args.name]
            if not homes or #homes == 0 then
               error(
                  "get_symbol: no file declares '" .. tostring(args.name) .. "'; pass 'file' explicitly",
                  0
               )
            end
            if #homes > 1 then
               error(
                  "get_symbol: '"
                     .. args.name
                     .. "' is declared in more than one file ("
                     .. table.concat(homes, ", ")
                     .. "); pass 'file' explicitly",
                  0
               )
            end
            file = homes[1]
         end
         return unwrap(docindex.get_symbol(state.root, file, args.name))
      end,
   },
   {
      name = "get_function_body",
      description = "Return one '.c' function's full definition (doc/TAG comment plus body) -- "
         .. "never a whole file.",
      inputSchema = schema({
         name = { type = "string", description = "Function name" },
         file = { type = "string", description = "Repo-relative '.c' file path" },
      }, { "name", "file" }),
      handler = function(state, args)
         return unwrap(docindex.get_function_body(state.root, args.file, args.name))
      end,
   },
   {
      name = "get_snippet",
      description = "Return a bounded, literal line range from one file (never a whole file); "
         .. "a range wider than max_lines (default 200) is rejected rather than truncated.",
      inputSchema = schema({
         file = { type = "string", description = "Repo-relative file path" },
         start_line = { type = "number", description = "First 1-based line to return" },
         end_line = { type = "number", description = "Last 1-based line to return (inclusive)" },
         max_lines = { type = "number", description = "Cap on the range width (default 200)" },
      }, { "file", "start_line", "end_line" }),
      handler = function(state, args)
         return unwrap(
            docindex.get_snippet(state.root, args.file, args.start_line, args.end_line, args.max_lines)
         )
      end,
   },
   {
      name = "refresh_index",
      description = "Rebuild the symbol index from disk now, so a file added or changed mid-"
         .. "session (after the server started) becomes visible to every other tool call in "
         .. "this session, with no restart.",
      inputSchema = schema({}, {}),
      handler = function(state, _)
         state.cat = index.build(state.root)
         return {
            rebuilt = true,
            headers = state.cat.summary.headers,
            sources = state.cat.summary.sources,
            packages = #state.cat.packages,
         }
      end,
   },
}

return tools
