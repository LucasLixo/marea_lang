-- Copyright (c) 2026 Paulo Lucas Alves Franca
-- Licensed under the MIT License. See LICENSE.txt for full terms.

--- @script test_ship
-- Runs marea-ship against a throwaway git repository with a bare "origin" beside it. git is real.
-- gh is replaced by a fake that records each call, because the tests must not reach GitHub.

local script_dir = (arg[0]:gsub("\\", "/")):match("(.*/)") or "./"
package.path = script_dir .. "../?.lua;" .. script_dir .. "../../marea-shared/?.lua;" .. package.path

local ship = require("ship")

local function sh(command)
   local handle = io.popen("{ " .. command .. "; } 2>&1")
   local output = handle:read("*a")
   local ok = handle:close()
   assert(ok, "command failed: " .. command .. "\n" .. output)
   return output
end

local function write(path, content)
   local file = assert(io.open(path, "w"))
   file:write(content)
   file:close()
end

local function read(path)
   local file = assert(io.open(path, "r"))
   local content = file:read("*a")
   file:close()
   return content
end

local METADATA = [[
# Header comment.
title: MAREA
version:
  major: 4 # Breaks the public API
  minor: 1 # Adds to the public API
  patch: 2 # No public API impact
  build: 370 # Bumped once per commit
]]

--- @return string, string: A fresh working repository on branch `refactor/42`, and its parent.
local function new_repository()
   local parent = os.tmpname()
   os.remove(parent)
   local root = parent .. "/work"
   sh("mkdir -p " .. root)
   sh("git init -q --bare -b develop " .. parent .. "/origin.git")
   sh("git init -q -b develop " .. root)
   local git = "git -C " .. root .. " "
   sh(git .. "config user.name Test")
   sh(git .. "config user.email test@example.invalid")
   sh(git .. "config commit.gpgsign false")
   write(root .. "/metadata.yaml", METADATA)
   write(root .. "/old.txt", "old\n")
   sh(git .. "add . && " .. git .. "commit -q -m Start")
   sh(git .. "remote add origin " .. parent .. "/origin.git")
   sh(git .. "push -q -u origin develop")
   sh(git .. "switch -q -c refactor/42")
   return root, parent
end

--- @return function, table: A runner that fakes gh and passes git through, and the gh calls.
local function fake_gh(on_merge)
   local calls = {}
   local function runner(argv, root)
      if argv[1] ~= "gh" then
         return ship.run(argv, root)
      end
      calls[#calls + 1] = table.concat(argv, " ")
      if argv[2] == "pr" and argv[3] == "create" then
         return true, "https://example.invalid/pull/7\n"
      end
      if argv[2] == "pr" and argv[3] == "merge" and on_merge then
         on_merge(root)
      end
      if argv[2] == "issue" and argv[3] == "view" then
         return true, "OPEN\n"
      end
      return true, ""
   end
   return runner, calls
end

-- The branch name carries the issue number; a branch with no digit has none.
assert(ship.issue_number("refactor/94") == "94")
assert(ship.issue_number("feature/7-foo") == "7")
assert(ship.issue_number("develop") == nil)

-- The number closes the subject line, once.
assert(ship.with_issue("Add a thing.\n\nBody.\n", "42") == "Add a thing #42\n\nBody.\n")
assert(ship.with_issue("Add a thing #42\n", "42") == "Add a thing #42\n")
assert(ship.with_issue("Add a thing #421\n", "42") == "Add a thing #421 #42\n")
assert(ship.with_issue("Add a thing\n", nil) == "Add a thing\n")

-- A message must be present, ASCII, and not in the "type: description" format.
assert(ship.check_message("Add a thing"))
assert(not ship.check_message("  \n"))
assert(not ship.check_message("Add a thing \195\169"))
assert(not ship.check_message("feat: add a thing"))
assert(not ship.check_message("fix(vm): add a thing"))

-- A bump moves build by one and resets the lower SemVer fields; comments stay.
local function versions(level)
   local content, version = ship.bump(METADATA, level)
   assert(content:find("# Header comment.", 1, true))
   assert(content:find("# Bumped once per commit", 1, true))
   return version
end
assert(versions("build") == "4.1.2.371")
assert(versions("patch") == "4.1.3.371")
assert(versions("minor") == "4.2.0.371")
assert(versions("major") == "5.0.0.371")
assert(not ship.bump(METADATA, "huge"))

-- commit: stages only the named paths, bumps the version, and appends the issue number.
do
   local root, parent = new_repository()
   local git = "git -C " .. root .. " "
   write(root .. "/new.txt", "new\n")
   write(root .. "/unrelated.txt", "unrelated\n")
   write(root .. "/staged.txt", "staged\n")
   sh(git .. "add staged.txt")

   local result =
      assert(ship.commit({ root = root, paths = { "new.txt" }, bump = "minor", message = "Add new" }))
   assert(result.subject == "Add new #42", result.subject)
   assert(result.version == "4.2.0.371", result.version)
   local files = sh(git .. "show --name-only --format= HEAD")
   assert(files == "metadata.yaml\nnew.txt\n", files)
   local status = sh(git .. "status --porcelain")
   assert(status:find("A  staged.txt", 1, true), status)
   assert(status:find("?? unrelated.txt", 1, true), status)

   -- A removal is committed, whether git rm or rm made it.
   sh(git .. "rm -q old.txt")
   assert(ship.commit({ root = root, paths = { "old.txt" }, bump = "build", message = "Remove old" }))
   os.remove(root .. "/new.txt")
   assert(ship.commit({ root = root, paths = { "new.txt" }, bump = "build", message = "Remove new" }))
   assert(read(root .. "/metadata.yaml"):find("build: 373", 1, true))

   -- A refused commit leaves metadata.yaml as it was.
   local before = read(root .. "/metadata.yaml")
   assert(not ship.commit({ root = root, paths = { "absent.txt" }, bump = "build", message = "Nothing" }))
   assert(not ship.commit({ root = root, paths = {}, bump = "build", message = "Nothing" }))
   assert(read(root .. "/metadata.yaml") == before)

   -- A version that somebody already edited is never bumped a second time.
   write(root .. "/metadata.yaml", (before:gsub("build: 373", "build: 999")))
   local ok, err =
      ship.commit({ root = root, paths = { "unrelated.txt" }, bump = "build", message = "Add it" })
   assert(not ok and err:find("metadata.yaml", 1, true), tostring(err))
   sh("rm -rf " .. parent)
end

-- open_pr: pushes the branch and opens the pull request against the base.
do
   local root, parent = new_repository()
   local git = "git -C " .. root .. " "
   write(root .. "/new.txt", "new\n")
   assert(ship.commit({
      root = root,
      paths = { "new.txt" },
      bump = "build",
      message = "Add new\n\nWhy it is new.",
   }))

   local runner, calls = fake_gh(function()
      sh(git .. "push -q origin refactor/42:develop")
   end)
   local result = assert(ship.open_pr({ root = root, base = "develop", title = "Add new", runner = runner }))
   assert(result.url == "https://example.invalid/pull/7", result.url)
   assert(sh(git .. "ls-remote --heads origin refactor/42") ~= "")
   local created = "gh pr create --base develop --head refactor/42 --title Add new --body "
   assert(calls[1] == created .. "## Changes\n\n- Add new #42\n  Why it is new.\n\nResolves #42", calls[1])

   -- A body from the caller replaces the list of commits.
   assert(ship.open_pr({ root = root, base = "develop", title = "t", body = "By hand.", runner = runner }))
   assert(calls[2]:find("--body By hand.\n\nResolves #42", 1, true), calls[2])
   table.remove(calls)

   -- merge: refuses a branch with a commit that was never pushed.
   write(root .. "/late.txt", "late\n")
   assert(ship.commit({ root = root, paths = { "late.txt" }, bump = "build", message = "Add late" }))
   assert(not ship.merge({ root = root, base = "develop", runner = runner }))
   assert(#calls == 1)
   sh(git .. "push -q origin refactor/42")

   -- merge: merges, closes the issue, returns to the base and removes the branch.
   local merged = assert(ship.merge({ root = root, base = "develop", runner = runner }))
   assert(#merged.warnings == 0, table.concat(merged.warnings, "\n"))
   assert(calls[2] == "gh pr merge refactor/42 --merge")
   assert(calls[4] == "gh issue close 42")
   assert(sh(git .. "branch --show-current") == "develop\n")
   assert(sh(git .. "branch --list refactor/42") == "")
   assert(sh(git .. "ls-remote --heads origin refactor/42") == "")
   sh("rm -rf " .. parent)
end

-- merge: removes the branch also from a worktree, where another checkout holds the base.
do
   local root, parent = new_repository()
   local git = "git -C " .. root .. " "
   local tree = parent .. "/tree"
   sh(git .. "switch -q develop")
   sh(git .. "worktree add -q " .. tree .. " refactor/42")
   write(tree .. "/new.txt", "new\n")
   assert(ship.commit({ root = tree, paths = { "new.txt" }, bump = "build", message = "Add new" }))
   local runner = fake_gh(function()
      sh(git .. "push -q origin refactor/42:develop")
   end)
   assert(ship.open_pr({ root = tree, base = "develop", title = "Add new", runner = runner }))

   local merged = assert(ship.merge({ root = tree, base = "develop", runner = runner }))
   assert(#merged.warnings == 0, table.concat(merged.warnings, "\n"))
   assert(sh(git .. "branch --list refactor/42") == "")
   assert(sh(git .. "ls-remote --heads origin refactor/42") == "")
   sh("rm -rf " .. parent)
end

-- Neither command works from the base branch or towards main.
do
   local root, parent = new_repository()
   local runner, calls = fake_gh()
   assert(not ship.open_pr({ root = root, base = "main", title = "x", runner = runner }))
   sh("git -C " .. root .. " switch -q develop")
   assert(not ship.open_pr({ root = root, base = "develop", title = "x", runner = runner }))
   assert(not ship.merge({ root = root, base = "develop", runner = runner }))
   assert(#calls == 0)
   sh("rm -rf " .. parent)
end

print("test_ship: ok")
