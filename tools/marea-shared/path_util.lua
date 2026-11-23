-- Copyright (c) 2026 Paulo Lucas Alves Franca
-- Licensed under the MIT License. See LICENSE.txt for full terms.

--- @module path_util
-- Normalizes a path produced by an external POSIX tool (`find`, `realpath`) back into a form the
-- host platform's own file APIs accept. On Windows, the only `find`/`realpath` binaries generally
-- on PATH are Git-for-Windows' bundled MSYS coreutils, and they always emit MSYS-style paths
-- regardless of the invoking shell ("/c/Dev/project/lib" for "C:\Dev\project\lib") -- a form
-- libclang and Lua's own `io.open` cannot resolve, since it drops the drive letter's colon. Every
-- module that shells out to find/realpath pipes its output through this so the paths it hands back
-- stay usable.

local path_util = {}

--- @param path string: A path as printed by `find`/`realpath`.
-- @return string: The same path, rewritten from MSYS form ("/c/...") to Windows form ("C:/...")
--   when it matches that shape; anything else (a real POSIX path, an already-Windows path) passes
--   through unchanged.
function path_util.normalize(path)
   local drive, rest = path:match("^/([A-Za-z])/(.*)$")
   if not drive then
      return path
   end

   return drive:upper() .. ":/" .. rest
end

return path_util
