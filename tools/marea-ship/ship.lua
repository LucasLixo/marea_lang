-- Copyright (c) 2026 Paulo Lucas Alves Franca
-- Licensed under the MIT License. See LICENSE.txt for full terms.

--- @module ship
-- The mechanical half of a delivery: version bump, commit, push, pull request, merge and branch
-- cleanup, each as one call, so an agent spends its turns on the message and not on the steps.
-- Grouping, the message and the SemVer level stay with the caller. See generate-commit for the
-- commit rules and generate-apply's apply-part-template for the delivery rules.
--
-- Every public function returns a result table, or `nil, err`. None of them prints or exits.
-- POSIX shell only.

local ship = {}

local FIELDS = { "major", "minor", "patch", "build" }
local LEVELS = { build = true, patch = true, minor = true, major = true }

local function quote(text)
   return "'" .. tostring(text):gsub("'", "'\\''") .. "'"
end

local function trim(text)
   return (text:gsub("^%s+", ""):gsub("%s+$", ""))
end

--- @param argv table: Program and arguments, each passed verbatim.
-- @param root string: Directory to run in.
-- @return boolean, string: Whether it exited with zero, and its stdout and stderr together.
function ship.run(argv, root)
   local words = {}
   for index, word in ipairs(argv) do
      words[index] = quote(word)
   end
   local handle = io.popen("cd " .. quote(root) .. " && " .. table.concat(words, " ") .. " 2>&1")
   local output = handle:read("*a")
   local ok = handle:close()
   return ok == true, output
end

--- Runs one command and turns a failure into an error message that names it.
-- @return string|nil, string|nil: Trimmed output, or nil and the message.
local function call(runner, root, ...)
   local argv = { ... }
   local ok, output = runner(argv, root)
   if not ok then
      return nil, "`" .. table.concat(argv, " ") .. "` failed: " .. trim(output)
   end
   return trim(output)
end

local function read_file(path)
   local file = io.open(path, "r")
   if not file then
      return nil
   end
   local content = file:read("*a")
   file:close()
   return content
end

local function write_file(path, content)
   local file = assert(io.open(path, "w"))
   file:write(content)
   file:close()
end

--- @return string|nil: The first run of digits in the branch name.
function ship.issue_number(branch)
   return branch:match("%d+")
end

--- @return string: The message with " #N" closing its subject line, added only when absent.
function ship.with_issue(message, issue)
   if not issue then
      return message
   end
   local subject, rest = message:match("^([^\n]*)(.*)$")
   subject = trim(subject)
   local tag = "#" .. issue
   if subject == tag or subject:sub(-#tag - 1) == " " .. tag then
      return subject .. rest
   end
   return (subject:gsub("%.$", "")) .. " " .. tag .. rest
end

--- @return boolean|nil, string|nil: True, or nil and why the message breaks a commit rule.
function ship.check_message(message)
   if trim(message) == "" then
      return nil, "the message is empty"
   end
   if message:find("[\128-\255]") then
      return nil, "the message is not ASCII"
   end
   local subject = trim(message):match("^[^\n]*")
   if subject:match("^%l+%b():") or subject:match("^%l+!?:") then
      return nil, "the message uses the `type: description` format"
   end
   return true
end

local function read_version(content)
   local version = {}
   for _, field in ipairs(FIELDS) do
      version[field] = tonumber(content:match("\n%s+" .. field .. ":%s*(%d+)"))
      if not version[field] then
         return nil
      end
   end
   return version
end

local function format_version(version)
   return string.format("%d.%d.%d.%d", version.major, version.minor, version.patch, version.build)
end

--- @param content string: The text of metadata.yaml.
-- @param level string: "build", "patch", "minor" or "major". build moves with every level.
-- @return string|nil, string: New text and "major.minor.patch.build", or nil and why not.
function ship.bump(content, level)
   if not LEVELS[level] then
      return nil, "unknown bump level `" .. tostring(level) .. "`"
   end
   local version = read_version(content)
   if not version then
      return nil, "metadata.yaml has no version block with " .. table.concat(FIELDS, ", ")
   end
   version.build = version.build + 1
   if level == "major" then
      version.major, version.minor, version.patch = version.major + 1, 0, 0
   elseif level == "minor" then
      version.minor, version.patch = version.minor + 1, 0
   elseif level == "patch" then
      version.patch = version.patch + 1
   end
   for _, field in ipairs(FIELDS) do
      content = content:gsub("(\n%s+" .. field .. ":%s*)%d+", function(prefix)
         return prefix .. version[field]
      end, 1)
   end
   return content, format_version(version)
end

--- @return string|nil, string|nil, string|nil: The new metadata text and version to write (both
--   nil when this commit carries no bump), or a third value that says why the commit must stop.
local function plan_bump(runner, root, paths, level)
   local content = read_file(root .. "/metadata.yaml")
   if not content then
      return nil, nil, nil
   end
   local committed_ok, committed = runner({ "git", "show", "HEAD:metadata.yaml" }, root)
   if not committed_ok then
      return nil, nil, nil -- This commit creates the file.
   end
   local status, err = call(runner, root, "git", "status", "--porcelain", "--", "metadata.yaml")
   if not status then
      return nil, nil, err
   end
   if status ~= "" then
      local listed = false
      for _, path in ipairs(paths) do
         listed = listed or path == "metadata.yaml"
      end
      local before, after = read_version("\n" .. committed), read_version("\n" .. content)
      if not listed or not before or not after or format_version(before) ~= format_version(after) then
         return nil, nil, "metadata.yaml already has uncommitted changes; restore it or list it unbumped"
      end
   end
   local bumped, version = ship.bump(content, level)
   if not bumped then
      return nil, nil, version
   end
   return bumped, version, nil
end

--- Commits exactly the named paths (plus metadata.yaml), whatever else is staged or dirty.
-- @param options table: `{ root, paths, bump, message, runner? }`.
-- @return table|nil, string|nil: `{ hash, subject, version }`, or nil and why not.
function ship.commit(options)
   local runner, root, paths = options.runner or ship.run, options.root, options.paths
   if #paths == 0 then
      return nil, "name at least one path"
   end
   local ok, why = ship.check_message(options.message)
   if not ok then
      return nil, why
   end
   if not LEVELS[options.bump] then
      return nil, "unknown bump level `" .. tostring(options.bump) .. "`"
   end
   local branch, err = call(runner, root, "git", "branch", "--show-current")
   if not branch then
      return nil, err
   end
   local message = ship.with_issue(trim(options.message), ship.issue_number(branch)) .. "\n"

   local metadata_path = root .. "/metadata.yaml"
   local original = read_file(metadata_path)
   local bumped, version, stop = plan_bump(runner, root, paths, options.bump)
   if stop then
      return nil, stop
   end

   for _, path in ipairs(paths) do
      local present = runner({ "test", "-e", path }, root)
         or runner({ "git", "ls-files", "--error-unmatch", "--", path }, root)
      if present then
         local added, add_error = call(runner, root, "git", "add", "-A", "--", path)
         if not added then
            return nil, add_error
         end
      elseif not runner({ "git", "cat-file", "-e", "HEAD:" .. path }, root) then
         return nil, "no such path: " .. path
      end
   end

   local message_path = os.tmpname()
   write_file(message_path, message)
   local argv = { "git", "commit", "-q", "-F", message_path, "--" }
   for _, path in ipairs(paths) do
      argv[#argv + 1] = path
   end
   if bumped then
      write_file(metadata_path, bumped)
      argv[#argv + 1] = "metadata.yaml"
   end
   local committed, commit_error = call(runner, root, table.unpack(argv))
   os.remove(message_path)
   if not committed then
      if bumped then
         write_file(metadata_path, original)
      end
      return nil, commit_error
   end

   local hash = call(runner, root, "git", "rev-parse", "--short", "HEAD")
   return { hash = hash, subject = message:match("^[^\n]*"), version = version }
end

local function current_branch(runner, root, branch)
   if branch then
      return branch
   end
   return call(runner, root, "git", "branch", "--show-current")
end

--- @return boolean|nil, string|nil: Refuses a delivery from a base branch or towards main.
local function check_direction(branch, base)
   if branch == "" then
      return nil, "HEAD is detached"
   end
   if base == "main" or branch == "main" then
      return nil, "never deliver from or to main"
   end
   if branch == base or branch == "develop" then
      return nil, "`" .. branch .. "` is a base branch, not a delivery branch"
   end
   return true
end

--- Merges the pull request of a branch, closes its issue and removes the branch.
-- @param options table: `{ root, base, branch?, issue?, runner? }`.
-- @return table|nil, string|nil: `{ branch, warnings }`; a warning is a cleanup step that failed
--   after the merge already happened.
function ship.merge(options)
   local runner, root, base = options.runner or ship.run, options.root, options.base
   local branch, err = current_branch(runner, root, options.branch)
   if not branch then
      return nil, err
   end
   local allowed, why = check_direction(branch, base)
   if not allowed then
      return nil, why
   end
   local issue = options.issue or ship.issue_number(branch)

   local fetched, fetch_error = call(runner, root, "git", "fetch", "-q", "--prune", "origin")
   if not fetched then
      return nil, fetch_error
   end
   local has_local = runner({ "git", "rev-parse", "--verify", "-q", "refs/heads/" .. branch }, root)
   if has_local then
      local local_tip = call(runner, root, "git", "rev-parse", "refs/heads/" .. branch)
      local remote_tip = call(runner, root, "git", "rev-parse", "refs/remotes/origin/" .. branch)
      if local_tip ~= remote_tip then
         return nil, "`" .. branch .. "` differs from origin; push or pull it before the merge"
      end
   end

   local merged, merge_error = call(runner, root, "gh", "pr", "merge", branch, "--merge")
   if not merged then
      return nil, merge_error
   end

   -- The merge is done: from here a failure is a warning, never an error.
   local warnings = {}
   local function try(...)
      local output, problem = call(runner, root, ...)
      if not output then
         warnings[#warnings + 1] = problem
      end
      return output
   end

   if issue and try("gh", "issue", "view", issue, "--json", "state", "-q", ".state") == "OPEN" then
      try("gh", "issue", "close", issue)
   end
   try("git", "fetch", "-q", "origin")
   if runner({ "git", "switch", "-q", base }, root) then
      try("git", "merge", "-q", "--ff-only", "origin/" .. base)
   elseif call(runner, root, "git", "branch", "--show-current") == branch then
      -- Another worktree holds the base. Leave the branch anyway, or git cannot delete it.
      try("git", "switch", "-q", "--detach", "origin/" .. base)
   end
   if try("git", "merge-base", "--is-ancestor", "origin/" .. branch, "origin/" .. base) then
      if has_local then
         try("git", "branch", "-q", "-D", branch)
      end
      local heads = try("git", "ls-remote", "--heads", "origin", "refs/heads/" .. branch)
      if heads and heads ~= "" then
         try("git", "push", "-q", "origin", "--delete", branch)
      end
   end
   return { branch = branch, warnings = warnings }
end

--- @return string: The commits that the branch adds to the base, oldest first, as a Markdown list;
--   subjects only when the full text would pass the size GitHub accepts. Empty when git cannot say.
local function describe(runner, root, base, branch)
   local range = "origin/" .. base .. ".." .. branch
   local function log(format)
      return call(runner, root, "git", "log", "--reverse", "--no-merges", "--format=" .. format, range)
   end
   local text = log("- %s%n%w(0,2,2)%b")
   if text and #text > 60000 then
      text = log("- %s")
   end
   if not text or text == "" then
      return ""
   end
   text = text:gsub("[ \t]+\n", "\n"):gsub("\n\n+%- ", "\n- ")
   return "## Changes\n\n" .. text
end

--- Pushes the current branch and opens its pull request; with `merge`, merges it at once.
-- @param options table: `{ root, base, title, body?, issue?, merge?, runner? }`.
-- @return table|nil, string|nil: `{ branch, url, warnings }`, or nil and why not.
function ship.open_pr(options)
   local runner, root, base = options.runner or ship.run, options.root, options.base
   local branch, err = current_branch(runner, root, nil)
   if not branch then
      return nil, err
   end
   local allowed, why = check_direction(branch, base)
   if not allowed then
      return nil, why
   end
   local issue = options.issue or ship.issue_number(branch)
   local pushed, push_error = call(runner, root, "git", "push", "-q", "-u", "origin", branch)
   if not pushed then
      return nil, push_error
   end
   local body = options.body or describe(runner, root, base, branch)
   if issue then
      body = (body == "" and "" or body .. "\n\n") .. "Resolves #" .. issue
   end
   local output, create_error = call(
      runner,
      root,
      "gh",
      "pr",
      "create",
      "--base",
      base,
      "--head",
      branch,
      "--title",
      options.title,
      "--body",
      body
   )
   if not output then
      return nil, create_error
   end
   local result = { branch = branch, url = output:match("https?://%S+"), warnings = {} }
   if options.merge then
      local merged, merge_error =
         ship.merge({ root = root, base = base, issue = issue, runner = options.runner })
      if not merged then
         return nil, "opened " .. tostring(result.url) .. " but did not merge it: " .. merge_error
      end
      result.warnings = merged.warnings
   end
   return result
end

return ship
