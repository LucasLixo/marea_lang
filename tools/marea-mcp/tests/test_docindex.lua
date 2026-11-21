-- Copyright (c) 2026 Paulo Lucas Alves Franca
-- Licensed under the MIT License. See LICENSE.txt for full terms.

--- @script test_docindex
-- Position-aware symbol lookup against the fixtures/ mini header/source pair: exact doc-comment
-- and declaration spans, a function body, a bounded snippet, and every "never raise on a bad
-- name/range" error path. The exact line numbers below are load-bearing - they describe
-- fixtures/sample.h and fixtures/sample.c exactly as written, line for line.

local script_dir = (arg[0]:gsub("\\", "/")):match("(.*/)") or "./"
package.path = script_dir .. "../?.lua;" .. script_dir .. "../../marea-api-catalog/?.lua;" .. package.path

local docindex = require("docindex")

local root = script_dir .. "fixtures"

-- get_symbol: an object-like macro with no doc block above it.
do
   local symbol, err = docindex.get_symbol(root, "sample.h", "SAMPLE_MAGIC")
   assert(symbol, err)
   assert(symbol.kind == "macro", "kind: " .. tostring(symbol.kind))
   assert(symbol.start_line == 4 and symbol.end_line == 4, symbol.start_line .. ".." .. symbol.end_line)
   print("test_docindex: get_symbol macro OK")
end

-- get_symbol: an opaque typedef, doc block included.
do
   local symbol, err = docindex.get_symbol(root, "sample.h", "Sample")
   assert(symbol, err)
   assert(symbol.kind == "type", "kind: " .. tostring(symbol.kind))
   assert(symbol.start_line == 6 and symbol.end_line == 9, symbol.start_line .. ".." .. symbol.end_line)
   print("test_docindex: get_symbol opaque typedef OK")
end

-- get_symbol: a tagged enum typedef, doc block included, ending at "} SampleStatus;".
do
   local symbol, err = docindex.get_symbol(root, "sample.h", "SampleStatus")
   assert(symbol, err)
   assert(symbol.kind == "type", "kind: " .. tostring(symbol.kind))
   assert(symbol.start_line == 11 and symbol.end_line == 18, symbol.start_line .. ".." .. symbol.end_line)
   print("test_docindex: get_symbol tagged enum OK")
end

-- get_symbol: one enumerator, ending at its own comma, no doc block above it.
do
   local symbol, err = docindex.get_symbol(root, "sample.h", "SAMPLE_STATUS_ERROR")
   assert(symbol, err)
   assert(symbol.kind == "enumerator", "kind: " .. tostring(symbol.kind))
   assert(symbol.start_line == 17 and symbol.end_line == 17, symbol.start_line .. ".." .. symbol.end_line)
   print("test_docindex: get_symbol enumerator OK")
end

-- get_symbol: a function declaration, doc block included, text carries the brief line and ends
-- exactly at the declaration (never past line 25, the file's last line).
do
   local symbol, err = docindex.get_symbol(root, "sample.h", "SampleGetMagic")
   assert(symbol, err)
   assert(symbol.kind == "function", "kind: " .. tostring(symbol.kind))
   assert(symbol.start_line == 20 and symbol.end_line == 25, symbol.start_line .. ".." .. symbol.end_line)
   assert(symbol.text:find("@brief Returns the sample's magic number.", 1, true), symbol.text)
   assert(symbol.text:match("SampleGetMagic%(const Sample %*sample%);%s*$"), symbol.text)
   print("test_docindex: get_symbol function declaration OK")
end

-- get_symbol: an unknown name returns nil, err -- never raises.
do
   local ok, symbol, err = pcall(docindex.get_symbol, root, "sample.h", "NoSuchSymbol")
   assert(ok, "a missing symbol must not raise: " .. tostring(symbol))
   assert(symbol == nil)
   assert(type(err) == "string" and #err > 0)
   print("test_docindex: get_symbol unknown name OK")
end

-- get_function_body: the TAG comment counts as the adjacent doc block.
do
   local body, err = docindex.get_function_body(root, "sample.c", "SampleGetMagic")
   assert(body, err)
   assert(body.start_line == 4 and body.end_line == 12, body.start_line .. ".." .. body.end_line)
   assert(body.text:find("if (sample == 0)", 1, true), body.text)
   assert(body.text:match("}%s*$"), body.text)
   print("test_docindex: get_function_body OK")
end

-- get_function_body: a header-only prototype has no body.
do
   local ok, body, err = pcall(docindex.get_function_body, root, "sample.h", "SampleGetMagic")
   assert(ok, "a declaration-only symbol must not raise: " .. tostring(body))
   assert(body == nil)
   assert(type(err) == "string" and #err > 0)
   print("test_docindex: get_function_body declaration-only OK")
end

-- get_snippet: an explicit range is taken literally, no doc comment prepended.
do
   local snippet, err = docindex.get_snippet(root, "sample.c", 5, 7)
   assert(snippet, err)
   assert(
      snippet.text == "int MAREA_CALL SampleGetMagic(const Sample *sample)\n{\n    if (sample == 0)",
      snippet.text
   )
   print("test_docindex: get_snippet explicit range OK")
end

-- get_snippet: a range wider than an explicit max_lines is rejected, naming the cap.
do
   local ok, snippet, err = pcall(docindex.get_snippet, root, "sample.c", 1, 12, 5)
   assert(ok, "a too-wide range must not raise: " .. tostring(snippet))
   assert(snippet == nil)
   assert(type(err) == "string" and #err > 0)
   print("test_docindex: get_snippet cap exceeded OK")
end

-- get_snippet: start_line past the end of the file is rejected, not silently clamped.
do
   local ok, snippet, err = pcall(docindex.get_snippet, root, "sample.c", 50, 52)
   assert(ok, "start_line past the end of the file must not raise: " .. tostring(snippet))
   assert(snippet == nil)
   assert(type(err) == "string" and #err > 0)
   print("test_docindex: get_snippet past end of file OK")
end

print("test_docindex: ALL OK")
