-- Copyright (c) 2026 Paulo Lucas Alves Franca
-- Licensed under the MIT License. See LICENSE.txt for full terms.

--- @script main
-- Entry point for marea-mcp: a stdio MCP server that lets an agent (this project, Codex) look
-- up a C symbol's doc comment and declaration, or a '.c' function's body, or a bounded line
-- range -- never a whole file -- built on tools/marea-api-catalog's symbol index. No CLI flags:
-- an MCP server takes no command-line arguments, it is launched once per client session with a
-- fixed `args` list in the client config (.mcp.json, .codex/config.toml), so this does not reuse
-- tools/marea-shared/cli.lua's flag parser.

local script_dir = (arg[0]:gsub("\\", "/")):match("(.*/)") or "./"
package.path = script_dir
   .. "?.lua;"
   .. script_dir
   .. "../marea-api-catalog/?.lua;"
   .. script_dir
   .. "../marea-shared/?.lua;"
   .. package.path

local index = require("index")
local protocol = require("protocol")
local tools = require("tools")

--- Reads major.minor.patch.build from the repo-root metadata.yaml, for this server's own
-- `serverInfo.version`.
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
--   with "." and ".." segments resolved textually, the same way
--   tools/marea-api-catalog/main.lua's own default_root() does -- independent of the caller's
--   cwd.
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

local root = default_root()
local state = {
   cat = index.build(root),
   root = root,
   tools = tools,
   version = project_version(),
}

for line in io.stdin:lines() do
   local response = protocol.handle_line(line, state)
   if response then
      io.stdout:write(response .. "\n")
      io.stdout:flush()
   end
end
