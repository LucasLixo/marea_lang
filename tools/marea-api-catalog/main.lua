-- Copyright (c) 2026 Paulo Lucas Alves Franca
-- Licensed under the MIT License. See LICENSE.txt for full terms.

--- @script main
-- Entry point/CLI for marea-api-catalog. Run via the pinned `lua`, the same host-tool tier as
-- marea-ascii-check. No compile step: this is a tree of .lua files run directly.
--
-- Subcommands:
--   catalog - scan lib/include and lib/internal headers plus lib/source into a deterministic
--      JSON token/dependency map: every file's includes, MAREA_API_* gates, declared symbols
--      (the five C declaration forms scanner.lua recognizes) and used identifiers.
--
-- CLI shape (`-h`/`--help`, flags) comes from `tools/marea-shared/cli.lua`, the same spec-driven
-- parser marea-ascii-check uses.

local script_dir = (arg[0]:gsub("\\", "/")):match("(.*/)") or "./"
package.path = script_dir .. "?.lua;" .. script_dir .. "../marea-shared/?.lua;" .. package.path

local cli = require("cli")
local catalog = require("catalog")

--- Reads major.minor.patch.build from the repo-root metadata.yaml, for this tool's own `-V`.
-- @return string: "major.minor.patch.build", or "0.0.0.0" if metadata.yaml does not exist yet.
local function project_version()
   local file = io.open(script_dir .. "../../metadata.yaml", "r")
   if not file then
      return "0.0.0.0"
   end
   local content = file:read("*a")
   file:close()
   local major = content:match("major:%s*(%d+)") or "0"
   local minor = content:match("minor:%s*(%d+)") or "0"
   local patch = content:match("patch:%s*(%d+)") or "0"
   local build = content:match("build:%s*(%d+)") or "0"
   return major .. "." .. minor .. "." .. patch .. "." .. build
end

--- @return string: The repository root that contains this tool (two levels up from main.lua),
--   with "." and ".." segments resolved textually, used as the default for `--root`.
local function default_root()
   local path = script_dir .. "../.."
   local absolute = path:sub(1, 1) == "/"
   local parts = {}
   for part in path:gmatch("[^/]+") do
      if part == ".." then
         if #parts > 0 and parts[#parts] ~= ".." then
            parts[#parts] = nil
         elseif not absolute then
            parts[#parts + 1] = part
         end
      elseif part ~= "." then
         parts[#parts + 1] = part
      end
   end
   local joined = table.concat(parts, "/")
   if joined == "" then
      return absolute and "/" or "."
   end
   return absolute and ("/" .. joined) or joined
end

local SPEC = {
   name = "marea-api-catalog",
   summary = "scan C headers/sources for includes, MAREA_API_* gates and declared/used symbols",
   version = project_version(),
   commands = {
      {
         name = "catalog",
         summary = "Scan lib/include and lib/internal headers plus lib/source into a "
            .. "deterministic JSON token/dependency map",
         flags = {
            {
               name = "root",
               long = "root",
               arg = "PATH",
               default = default_root(),
               help = "Repository root to scan (the one containing lib/)",
            },
            {
               name = "out",
               short = "o",
               long = "out",
               arg = "PATH",
               required = true,
               help = "JSON output path",
            },
            {
               name = "sources",
               long = "sources",
               arg = "DIRS",
               default = "",
               help = "Comma-separated extra source directories relative to --root, scanned in "
                  .. "addition to lib/source",
            },
         },
      },
   },
}

local result = cli.parse(SPEC, arg)
cli.finish(result)

--- Splits a comma-separated flag value (`--sources`) into a list.
-- @param value string: Raw flag value ("" when not given).
-- @return table: Array of the items.
local function split_list(value)
   local dirs = {}
   for dir in value:gmatch("[^,]+") do
      dirs[#dirs + 1] = dir
   end
   return dirs
end

--- Runs the `catalog` subcommand: builds the token/dependency map and writes it as JSON.
-- @param options table: Parsed flag values for `catalog`.
local function run_catalog(options)
   local cat = catalog.build(options.root, { sources = split_list(options.sources) })
   catalog.write_json(cat, options.root, options.out)
   io.stderr:write(
      string.format(
         "marea-api-catalog: catalog: %d header(s), %d source(s), %d package(s)\n",
         cat.summary.headers,
         cat.summary.sources,
         #cat.packages
      )
   )
   os.exit(0)
end

local COMMANDS = {
   catalog = function(options)
      run_catalog(options)
   end,
}

-- An error the tool raises on purpose (a root with no header) carries the tool prefix: show it as
-- a one-line message and exit 1. Any other error is a bug and keeps its traceback.
local ok, err = pcall(COMMANDS[result.command], result.options, result.positional)
if not ok then
   if type(err) == "string" and err:find("^marea%-api%-catalog:") then
      io.stderr:write(err .. "\n")
      os.exit(1)
   end
   error(err, 0)
end
