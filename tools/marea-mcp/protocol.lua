-- Copyright (c) 2026 Paulo Lucas Alves Franca
-- Licensed under the MIT License. See LICENSE.txt for full terms.

--- @module protocol
-- The JSON-RPC line protocol itself: one `handle_line(line, state)` call per line of stdin,
-- returning a response string to write (or nil for a notification that needs no reply). Every
-- path returns a string or nil -- a malformed line, an unknown method, an unknown tool name or
-- a handler that raises all turn into a well-shaped response, never a crash of the caller's
-- stdio loop.

local json = require("json")

local PROTOCOL_VERSION_DEFAULT = "2025-06-18"
local PARSE_ERROR = -32700
local METHOD_NOT_FOUND = -32601

local protocol = {}

--- Builds a JSON-RPC success response line.
-- @param id any: The request's own id (echoed back).
-- @param result table: The result value.
-- @return string: The encoded response line.
local function result_response(id, result)
   return json.encode({ jsonrpc = "2.0", id = id, result = result })
end

--- Builds a JSON-RPC error response line.
-- @param id any|nil: The request's own id when known, nil otherwise (a line that failed to
--   decode at all has no id to echo).
-- @param code number: The JSON-RPC error code.
-- @param message string: A human-readable message.
-- @return string: The encoded response line.
local function error_response(id, code, message)
   return json.encode({ jsonrpc = "2.0", id = id, error = { code = code, message = message } })
end

--- Shapes one tool handler's outcome into an MCP `tools/call` result: a successful plain-text
-- result (`value.text`, when the value already carries one -- get_symbol/get_function_body/
-- get_snippet all do) is used as-is, so a symbol's own text is never duplicated inside a bigger
-- JSON wrapper; anything else is JSON-encoded (pretty, so a human or an agent reading the raw
-- response can follow it).
-- @param ok boolean: Whether the handler call succeeded (`pcall`'s own first return).
-- @param value any: The handler's return value on success, or the raised error on failure.
-- @return table: `{ content = {...} }` on success, `{ isError = true, content = {...} }` on
--   failure.
local function shape_tool_result(ok, value)
   if not ok then
      return { isError = true, content = json.array({ { type = "text", text = tostring(value) } }) }
   end
   local text
   if type(value) == "table" and type(value.text) == "string" then
      text = value.text
   else
      text = json.encode(value, { indent = 2 })
   end
   return { content = json.array({ { type = "text", text = text } }) }
end

--- Handles one line of stdin: decodes it, dispatches by method, and returns the response line
-- to write (or nil for a notification). Never raises.
-- @param line string: One raw line of input (no trailing newline required).
-- @param state table: `{ cat, root, tools }` -- the index built once at startup (`refresh_index`
--   is the only tool allowed to replace `state.cat`) and the static tool catalog.
-- @return string|nil: The response line to write, or nil when `line` was a notification.
function protocol.handle_line(line, state)
   local request, decode_err = json.decode(line)
   if not request then
      return error_response(nil, PARSE_ERROR, "Parse error: " .. tostring(decode_err))
   end

   local method = request.method
   local id = request.id

   if method == "initialize" then
      local requested_version = request.params and request.params.protocolVersion
      return result_response(id, {
         protocolVersion = requested_version or PROTOCOL_VERSION_DEFAULT,
         capabilities = { tools = {} },
         serverInfo = { name = "marea-mcp", version = state.version or "0.0.0.0" },
      })
   end

   if method == "notifications/initialized" then
      return nil
   end

   if method == "tools/list" then
      local listed = {}
      for _, tool in ipairs(state.tools) do
         listed[#listed + 1] =
            { name = tool.name, description = tool.description, inputSchema = tool.inputSchema }
      end
      return result_response(id, { tools = json.array(listed) })
   end

   if method == "tools/call" then
      local params = request.params or {}
      local tool
      for _, candidate in ipairs(state.tools) do
         if candidate.name == params.name then
            tool = candidate
            break
         end
      end
      if not tool then
         return result_response(id, shape_tool_result(false, "Unknown tool: " .. tostring(params.name)))
      end
      local ok, value = pcall(tool.handler, state, params.arguments or {})
      return result_response(id, shape_tool_result(ok, value))
   end

   return error_response(id, METHOD_NOT_FOUND, "Method not found: " .. tostring(method))
end

return protocol
