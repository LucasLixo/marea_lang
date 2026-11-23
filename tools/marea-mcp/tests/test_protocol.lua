-- Copyright (c) 2026 Paulo Lucas Alves Franca
-- Licensed under the MIT License. See LICENSE.txt for full terms.

--- @script test_protocol
-- The JSON-RPC line protocol itself: handle_line must never raise, whatever the input -- a
-- malformed line, an unknown method, an unknown tool or a handler that errors all turn into a
-- well-shaped response string, never a crash of the stdio loop. The freshness case proves
-- refresh_index rebuilds state.cat in place, visible to a later call in the SAME process.

local script_dir = (arg[0]:gsub("\\", "/")):match("(.*/)") or "./"
package.path = script_dir
   .. "../?.lua;"
   .. script_dir
   .. "../../marea-shared/?.lua;"
   .. script_dir
   .. "../../marea-api-catalog/?.lua;"
   .. package.path

local json = require("json")
local index = require("index")
local protocol = require("protocol")
local tools = require("tools")

local root = script_dir .. "fixtures/index-root"
local cat = index.build(root)
local state = { cat = cat, root = root, tools = tools }

-- initialize: echoes protocolVersion, carries capabilities.tools and serverInfo.name.
do
   local request = json.encode({
      jsonrpc = "2.0",
      id = 1,
      method = "initialize",
      params = { protocolVersion = "2025-06-18" },
   })
   local response = protocol.handle_line(request, state)
   assert(response, "initialize must return a response line")
   local decoded, err = json.decode(response)
   assert(decoded, err)
   assert(decoded.result.protocolVersion == "2025-06-18", decoded.result.protocolVersion)
   assert(decoded.result.capabilities.tools ~= nil, "capabilities.tools must be present")
   assert(
      decoded.result.serverInfo.name == "marea-mcp",
      tostring(decoded.result.serverInfo and decoded.result.serverInfo.name)
   )
   print("test_protocol: initialize OK")
end

-- notifications/initialized: a notification (no id) gets no response at all.
do
   local request = json.encode({ jsonrpc = "2.0", method = "notifications/initialized" })
   local response = protocol.handle_line(request, state)
   assert(response == nil, "a notification must get no response: " .. tostring(response))
   print("test_protocol: notifications/initialized OK")
end

-- tools/list: exactly 7 tools, every name present (order not asserted).
do
   local request = json.encode({ jsonrpc = "2.0", id = 2, method = "tools/list" })
   local response = protocol.handle_line(request, state)
   local decoded, err = json.decode(response)
   assert(decoded, err)
   assert(#decoded.result.tools == 7, "expected 7 tools, got " .. #decoded.result.tools)
   local names = {}
   for _, tool in ipairs(decoded.result.tools) do
      names[tool.name] = true
   end
   for _, expected in ipairs({
      "list_modules",
      "search_symbols",
      "list_symbols_in_file",
      "get_symbol",
      "get_function_body",
      "get_snippet",
      "refresh_index",
   }) do
      assert(names[expected], "missing tool: " .. expected)
   end
   print("test_protocol: tools/list OK")
end

-- tools/call get_symbol: the content is the single declaration, not a bigger excerpt.
do
   local request = json.encode({
      jsonrpc = "2.0",
      id = 3,
      method = "tools/call",
      params = {
         name = "get_symbol",
         arguments = { name = "WidgetGetMagic", file = "lib/include/marea/widget/MAREAWidget.h" },
      },
   })
   local response = protocol.handle_line(request, state)
   local decoded, err = json.decode(response)
   assert(decoded, err)
   local text = decoded.result.content[1].text
   local _, count = text:gsub("WidgetGetMagic", "")
   assert(count == 1, "expected exactly one occurrence of the name, got " .. count .. ": " .. text)
   print("test_protocol: tools/call get_symbol OK")
end

-- tools/call with an unknown tool name: isError, never a crash.
do
   local request = json.encode({
      jsonrpc = "2.0",
      id = 3,
      method = "tools/call",
      params = { name = "no_such_tool", arguments = {} },
   })
   local response = protocol.handle_line(request, state)
   local decoded, err = json.decode(response)
   assert(decoded, err)
   assert(decoded.result.isError == true)
   print("test_protocol: tools/call unknown tool OK")
end

-- A malformed line never raises and reports a JSON-RPC parse error.
do
   local ok, response = pcall(protocol.handle_line, "not json at all", state)
   assert(ok, "a malformed line must not raise: " .. tostring(response))
   local decoded, err = json.decode(response)
   assert(decoded, err)
   assert(decoded.error.code == -32700, tostring(decoded.error and decoded.error.code))
   print("test_protocol: malformed line OK")
end

-- An unknown method reports -32601, never a crash.
do
   local request = json.encode({ jsonrpc = "2.0", id = 4, method = "no/such/method" })
   local response = protocol.handle_line(request, state)
   local decoded, err = json.decode(response)
   assert(decoded, err)
   assert(decoded.error.code == -32601, tostring(decoded.error and decoded.error.code))
   print("test_protocol: unknown method OK")
end

-- Freshness: refresh_index picks up a file written to disk mid-session, visible in the SAME
-- process, through the SAME state table, with no restart.
do
   local before = protocol.handle_line(
      json.encode({
         jsonrpc = "2.0",
         id = 5,
         method = "tools/call",
         params = { name = "search_symbols", arguments = { query = "Sprocket" } },
      }),
      state
   )
   local before_decoded = json.decode(before)
   local before_hits = json.decode(before_decoded.result.content[1].text)
   assert(#before_hits == 0, "Sprocket must not exist yet")

   local sprocket_path = root .. "/lib/include/marea/widget/MAREASprocket.h"
   local file = assert(io.open(sprocket_path, "w"))
   file:write("// Copyright (c) 2026 Paulo Lucas Alves Franca\n")
   file:write("// Licensed under the MIT License. See LICENSE.txt for full terms.\n")
   file:write("\n")
   file:write("int MAREA_CALL SprocketPing(void);\n")
   file:close()

   local original_cat = state.cat
   local original_headers = original_cat.summary.headers

   local refreshed = protocol.handle_line(
      json.encode({
         jsonrpc = "2.0",
         id = 6,
         method = "tools/call",
         params = { name = "refresh_index", arguments = {} },
      }),
      state
   )
   local refreshed_decoded = json.decode(refreshed)
   local summary = json.decode(refreshed_decoded.result.content[1].text)
   assert(
      summary.headers == original_headers + 1,
      "expected " .. (original_headers + 1) .. ", got " .. summary.headers
   )
   assert(state.cat ~= original_cat, "refresh_index must replace state.cat in place")

   local after = protocol.handle_line(
      json.encode({
         jsonrpc = "2.0",
         id = 7,
         method = "tools/call",
         params = { name = "search_symbols", arguments = { query = "Sprocket" } },
      }),
      state
   )
   local after_decoded = json.decode(after)
   local after_hits = json.decode(after_decoded.result.content[1].text)
   assert(#after_hits == 1, "expected exactly one Sprocket hit, got " .. #after_hits)
   assert(after_hits[1].name == "SprocketPing", after_hits[1].name)

   os.remove(sprocket_path)
   print("test_protocol: refresh_index freshness OK")
end

print("test_protocol: ALL OK")
