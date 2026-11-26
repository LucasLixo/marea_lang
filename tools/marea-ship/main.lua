-- Copyright (c) 2026 Paulo Lucas Alves Franca
-- Licensed under the MIT License. See LICENSE.txt for full terms.

--- @script main
-- Entry point/CLI for marea-ship. `commit` reads the message from stdin, so a quoted heredoc
-- carries it verbatim:
--
--   lua tools/marea-ship/main.lua commit --bump patch path/a path/b <<'EOF'
--   Subject line
--
--   Body.
--   EOF
--
-- Run from the root of the checkout (or worktree) that it acts on, or pass --root.

local script_dir = (arg[0]:gsub("\\", "/")):match("(.*/)") or "./"
package.path = script_dir .. "?.lua;" .. script_dir .. "../marea-shared/?.lua;" .. package.path

local cli = require("cli")
local ship = require("ship")

local ROOT = { name = "root", long = "root", arg = "DIR", help = "checkout to act on", default = "." }
local BASE = { name = "base", long = "base", arg = "BRANCH", help = "target branch", default = "develop" }
local ISSUE = { name = "issue", long = "issue", arg = "N", help = "issue number [default: from the branch]" }

local SPEC = {
   name = "marea-ship",
   summary = "commit, open a pull request and merge, one call each",
   commands = {
      {
         name = "commit",
         summary = "bump metadata.yaml and commit the named paths; message on stdin",
         flags = {
            {
               name = "bump",
               long = "bump",
               arg = "LEVEL",
               help = "build|patch|minor|major",
               default = "build",
            },
            ROOT,
         },
         positional = { label = "<path> [path...]", min = 1 },
      },
      {
         name = "open-pr",
         summary = "push the current branch and open its pull request",
         flags = {
            { name = "title", long = "title", arg = "TEXT", help = "pull request title", required = true },
            {
               name = "body",
               long = "body",
               arg = "TEXT",
               help = "description [default: the list of commits]",
            },
            { name = "merge", long = "merge", help = "merge and clean up at once" },
            BASE,
            ISSUE,
            ROOT,
         },
      },
      {
         name = "merge",
         summary = "merge a branch's pull request, close its issue and delete the branch",
         flags = {
            { name = "branch", long = "branch", arg = "NAME", help = "branch [default: current]" },
            BASE,
            ISSUE,
            ROOT,
         },
      },
   },
}

local parsed = cli.parse(SPEC, arg)
cli.finish(parsed)

local options = parsed.options
local result, err
if parsed.command == "commit" then
   result, err = ship.commit({
      root = options.root,
      paths = parsed.positional,
      bump = options.bump,
      message = io.read("a") or "",
   })
elseif parsed.command == "open-pr" then
   result, err = ship.open_pr(options)
else
   result, err = ship.merge(options)
end

if not result then
   io.stderr:write("marea-ship: " .. err .. "\n")
   os.exit(1)
end

if parsed.command == "commit" then
   local version = result.version and (" (" .. result.version .. ")") or ""
   print(result.hash .. " " .. result.subject .. version)
else
   print(parsed.command .. " ok: " .. result.branch .. (result.url and (" " .. result.url) or ""))
   for _, warning in ipairs(result.warnings) do
      print("WARNING: " .. warning)
   end
end
