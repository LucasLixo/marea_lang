-- Copyright (c) 2026 Paulo Lucas Alves Franca
-- Licensed under the MIT License. See LICENSE.txt for full terms.

--- @module cli
-- Spec-driven CLI parser and mise-style (`mise -h`) help renderer, shared by every tool under
-- tools/ so `-h`/`--help`, short/long flags and per-command help all come from one flag table
-- instead of a hand-written usage string per tool that can drift from what the parser actually
-- accepts -- that drift is the exact failure this module exists to prevent.
--
-- A multi-command spec is `{ name, summary, version, commands = { { name, summary, flags = {
-- {name, short, long, arg, help, default, required}, ... }, positional = { label, min }? }, ... }
-- }` (marea-docs-generator, marea-lua-generator, marea-make). A command's own `positional` is
-- optional -- one without it keeps the original behavior exactly: a bare, non-flag-prefixed token
-- is an unknown-argument usage error, same as before this field existed. A flat, subcommand-less
-- spec is `{ name, summary, flags = {...}, positional = { label, min } }` (marea-ascii-check).
--
-- A `required` flag needs an `arg`: a switch has no value to be missing, so it can never fail the
-- check. Only `--flag value` is read: neither `--flag=value` nor a bare `--` is supported.
--
-- `cli.parse` never writes to a stream or calls os.exit -- it always returns a plain result table
-- so tests can assert on it directly. Only `cli.finish`, the thin glue every main.lua calls right
-- after parsing, performs the actual print + exit.

local cli = {}

--- @param flag table: A flag spec.
-- @return string: The left column of its help/usage line, e.g. "-d, --docs <PATH>".
local function flag_left(flag)
   local left
   if flag.short and flag.long then
      left = "-" .. flag.short .. ", --" .. flag.long
   elseif flag.short then
      left = "-" .. flag.short
   else
      left = "    --" .. flag.long
   end
   if flag.arg then
      left = left .. " <" .. flag.arg .. ">"
   end
   return left
end

--- @param flag table: A flag spec.
-- @return string: Its help text, with "[default: X]" appended when it has a default value.
local function flag_right(flag)
   if flag.default ~= nil then
      return flag.help .. " [default: " .. tostring(flag.default) .. "]"
   end
   return flag.help
end

--- @param entries table: Array of `{ left, right }` pairs.
-- @return table: Array of rendered lines, each `"  " .. left, padded .. right`, aligned on the
--   longest `left` across the whole array.
local function render_two_column(entries)
   local max_len = 0
   for _, entry in ipairs(entries) do
      if #entry.left > max_len then
         max_len = #entry.left
      end
   end

   local lines = {}
   for _, entry in ipairs(entries) do
      lines[#lines + 1] = "  " .. entry.left .. string.rep(" ", max_len + 2 - #entry.left) .. entry.right
   end
   return lines
end

--- @param flags table: Array of flag specs.
-- @param token string: A raw CLI token, e.g. "-d" or "--docs".
-- @return table|nil: The matching flag spec.
local function find_flag(flags, token)
   for _, flag in ipairs(flags) do
      if (flag.short and token == "-" .. flag.short) or (flag.long and token == "--" .. flag.long) then
         return flag
      end
   end
   return nil
end

--- @param spec table: A multi-command program spec.
-- @param name string: A command name.
-- @return table|nil: The matching command spec.
local function find_command(spec, name)
   for _, command in ipairs(spec.commands) do
      if command.name == name then
         return command
      end
   end
   return nil
end

--- @param spec table: A multi-command program spec.
-- @return string: The full top-level help text, ending in a trailing newline.
local function render_top_help(spec)
   local lines = {
      spec.name .. " - " .. spec.summary,
      "",
      "Usage: " .. spec.name .. " [OPTIONS] <COMMAND>",
      "",
      "Commands:",
   }

   local command_entries = {}
   for _, command in ipairs(spec.commands) do
      command_entries[#command_entries + 1] = { left = command.name, right = command.summary }
   end
   command_entries[#command_entries + 1] = {
      left = "help",
      right = "Print this message or the help of the given subcommand",
   }
   for _, line in ipairs(render_two_column(command_entries)) do
      lines[#lines + 1] = line
   end

   lines[#lines + 1] = ""
   lines[#lines + 1] = "Options:"

   -- Flags are declared per-command, not globally, but the top-level help lists each distinct
   -- long flag once -- in the order it is first seen scanning commands top to bottom -- so a
   -- newcomer sees the whole recognized vocabulary without having to open every command's help.
   local seen = {}
   local option_entries = {}
   for _, command in ipairs(spec.commands) do
      for _, flag in ipairs(command.flags or {}) do
         if not seen[flag.long or flag.short] then
            seen[flag.long or flag.short] = true
            option_entries[#option_entries + 1] = { left = flag_left(flag), right = flag_right(flag) }
         end
      end
   end
   option_entries[#option_entries + 1] = { left = "-h, --help", right = "Print help" }
   if spec.version then
      option_entries[#option_entries + 1] = { left = "-V, --version", right = "Print version" }
   end
   for _, line in ipairs(render_two_column(option_entries)) do
      lines[#lines + 1] = line
   end

   return table.concat(lines, "\n") .. "\n"
end

--- @param spec table: A multi-command program spec.
-- @param command table: One of `spec.commands`.
-- @return string: That command's own help text, ending in a trailing newline.
local function render_command_help(spec, command)
   local usage = "Usage: " .. spec.name .. " " .. command.name .. " [OPTIONS]"
   if command.positional then
      usage = usage .. " " .. command.positional.label
   end
   local lines = {
      spec.name .. " " .. command.name .. " - " .. command.summary,
      "",
      usage,
      "",
      "Options:",
   }

   local option_entries = {}
   for _, flag in ipairs(command.flags or {}) do
      option_entries[#option_entries + 1] = { left = flag_left(flag), right = flag_right(flag) }
   end
   option_entries[#option_entries + 1] = { left = "-h, --help", right = "Print help" }
   for _, line in ipairs(render_two_column(option_entries)) do
      lines[#lines + 1] = line
   end

   return table.concat(lines, "\n") .. "\n"
end

--- @param spec table: A flat (subcommand-less) program spec.
-- @return string: The program's own help text, ending in a trailing newline.
local function render_flat_help(spec)
   local positional_label = spec.positional and spec.positional.label or ""
   local lines = {
      spec.name .. " - " .. spec.summary,
      "",
      "Usage: " .. spec.name .. " [OPTIONS] " .. positional_label,
      "",
      "Options:",
   }

   local option_entries = {}
   for _, flag in ipairs(spec.flags or {}) do
      option_entries[#option_entries + 1] = { left = flag_left(flag), right = flag_right(flag) }
   end
   option_entries[#option_entries + 1] = { left = "-h, --help", right = "Print help" }
   for _, line in ipairs(render_two_column(option_entries)) do
      lines[#lines + 1] = line
   end

   return table.concat(lines, "\n") .. "\n"
end

--- @param message string: One-line description of what went wrong.
-- @param help string: Full help text to show alongside it.
-- @return table: A terminal `cli.parse` result: exit code 1, printed to stderr.
local function usage_error(message, help)
   return { exit_code = 1, stream = "stderr", message = message .. "\n\n" .. help }
end

--- @param spec table: A multi-command program spec.
-- @param command table: The command being parsed. An optional `command.positional = { label,
--   min }` (the same shape a flat spec's own `spec.positional` uses) lets this ONE command also
--   collect bare, non-flag-prefixed tokens -- a command with no `positional` field keeps the
--   original behavior exactly: a bare token is an unknown-argument usage error.
-- @param argv table: The raw CLI argument array.
-- @param start number: Index of the first token past the command name.
-- @return table: `{ command = command.name, options = {...}, positional = {...} }`, or a
--   terminal help/error result.
local function parse_command(spec, command, argv, start)
   local flags = command.flags or {}
   local options = {}
   for _, flag in ipairs(flags) do
      if flag.default ~= nil then
         options[flag.name] = flag.default
      elseif not flag.arg then
         options[flag.name] = false
      end
   end

   local positional = {}
   local i = start
   while i <= #argv do
      local token = argv[i]

      if token == "-h" or token == "--help" then
         return { exit_code = 0, stream = "stdout", message = render_command_help(spec, command) }
      end

      if command.positional and token:sub(1, 1) ~= "-" then
         positional[#positional + 1] = token
         i = i + 1
      else
         local flag = find_flag(flags, token)
         if not flag then
            return usage_error(
               spec.name .. " " .. command.name .. ": unknown argument '" .. token .. "'",
               render_command_help(spec, command)
            )
         end

         if flag.arg then
            i = i + 1
            local value = argv[i]
            if value == nil then
               return usage_error(
                  spec.name .. " " .. command.name .. ": missing value for '" .. token .. "'",
                  render_command_help(spec, command)
               )
            end
            options[flag.name] = value
         else
            options[flag.name] = true
         end

         i = i + 1
      end
   end

   for _, flag in ipairs(flags) do
      if flag.required and options[flag.name] == nil then
         return usage_error(
            spec.name .. " " .. command.name .. ": missing required flag '--" .. flag.long .. "'",
            render_command_help(spec, command)
         )
      end
   end

   local min_positional = command.positional and command.positional.min or 0
   if #positional < min_positional then
      return usage_error(
         spec.name .. " " .. command.name .. ": expected at least " .. min_positional .. " argument(s)",
         render_command_help(spec, command)
      )
   end

   return { command = command.name, options = options, positional = positional }
end

--- @param spec table: A multi-command program spec.
-- @param argv table: The raw CLI argument array (`arg`).
-- @return table: `{ command, options }` on success, or a terminal help/error result.
local function parse_multi(spec, argv)
   local first = argv[1]

   if first == nil or first == "-h" or first == "--help" then
      return { exit_code = 0, stream = "stdout", message = render_top_help(spec) }
   end

   if spec.version and (first == "-V" or first == "--version") then
      return { exit_code = 0, stream = "stdout", message = spec.name .. " " .. spec.version .. "\n" }
   end

   if first == "help" then
      local target = argv[2]
      if target == nil then
         return { exit_code = 0, stream = "stdout", message = render_top_help(spec) }
      end
      local command = find_command(spec, target)
      if not command then
         return usage_error(spec.name .. ": unknown command '" .. target .. "'", render_top_help(spec))
      end
      return { exit_code = 0, stream = "stdout", message = render_command_help(spec, command) }
   end

   local command = find_command(spec, first)
   if not command then
      return usage_error(spec.name .. ": unknown command '" .. first .. "'", render_top_help(spec))
   end

   return parse_command(spec, command, argv, 2)
end

--- @param spec table: A flat (subcommand-less) program spec.
-- @param argv table: The raw CLI argument array (`arg`).
-- @return table: `{ options, positional }` on success, or a terminal help/error result.
local function parse_flat(spec, argv)
   local flags = spec.flags or {}
   local options = {}
   for _, flag in ipairs(flags) do
      if flag.default ~= nil then
         options[flag.name] = flag.default
      elseif not flag.arg then
         options[flag.name] = false
      end
   end

   local positional = {}
   local i = 1
   while i <= #argv do
      local token = argv[i]

      if token == "-h" or token == "--help" then
         return { exit_code = 0, stream = "stdout", message = render_flat_help(spec) }
      end

      if token:sub(1, 1) == "-" then
         local flag = find_flag(flags, token)
         if not flag then
            return usage_error(spec.name .. ": unknown argument '" .. token .. "'", render_flat_help(spec))
         end

         if flag.arg then
            i = i + 1
            local value = argv[i]
            if value == nil then
               return usage_error(
                  spec.name .. ": missing value for '" .. token .. "'",
                  render_flat_help(spec)
               )
            end
            options[flag.name] = value
         else
            options[flag.name] = true
         end
      else
         positional[#positional + 1] = token
      end

      i = i + 1
   end

   local min_positional = spec.positional and spec.positional.min or 0
   if #positional < min_positional then
      return usage_error(
         spec.name .. ": expected at least " .. min_positional .. " argument(s)",
         render_flat_help(spec)
      )
   end

   return { options = options, positional = positional }
end

--- @param spec table: A program spec, multi-command (`spec.commands`) or flat (`spec.flags`).
-- @param argv table: The raw CLI argument array (`arg`).
-- @return table: On success, `{ command, options }` (multi-command) or `{ options, positional }`
--   (flat). On help or a usage error, `{ exit_code, stream, message }` -- pass straight to
--   `cli.finish`.
function cli.parse(spec, argv)
   if spec.commands then
      return parse_multi(spec, argv)
   end
   return parse_flat(spec, argv)
end

--- Prints `result.message` to the stream it names and exits with `result.exit_code`, if `result`
-- is a terminal outcome (help printed, or a usage error). Returns (without exiting) when parsing
-- succeeded, so the caller can go on to read `result.command`/`result.options`/`result.positional`.
-- @param result table: Return value of `cli.parse`.
function cli.finish(result)
   if result.exit_code == nil then
      return
   end
   local stream = (result.stream == "stderr") and io.stderr or io.stdout
   stream:write(result.message)
   os.exit(result.exit_code)
end

return cli
