# Step 2 of 4 - Check the license of the dependency and of its whole tree

Part of the [add-dependency](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Read the license files. Never infer a license from the name or the fame of the project.
- Stop and ask the user on any restrictive or unclear license, at any depth. Do not choose for the user.

Find every license that the dependency brings, at any depth, before you add anything. Read the real files. A missing or unreadable license is a failure of this step. You may read them from a scratch clone of the upstream in the scratchpad folder, before the submodule exists.

### What is accepted

| Accepted (permissive) | Refused (restrictive or unclear) |
| --- | --- |
| MIT, Apache-2.0, BSD-2-Clause, BSD-3-Clause, ISC, Zlib, Unlicense, 0BSD, CC0-1.0 | GPL, LGPL, AGPL, MPL, EPL, SSPL, BUSL, Commons Clause, "source available", a custom license, no license at all |

"Restrictive" means any license that forces the project to publish its own source, or to relink, or to accept a field-of-use limit. A weak copyleft such as MPL-2.0 is refused too. A case outside both columns is not decided by you: stop and ask the user.

### Which license to take when several are offered

An `A OR B` expression is a choice. Take the **most permissive** alternative, in this order (first wins):

1. Unlicense, CC0-1.0, 0BSD
2. MIT
3. ISC
4. Zlib
5. BSD-2-Clause, BSD-3-Clause
6. Apache-2.0 (accepted, but last: it adds patent and NOTICE obligations)

An `A AND B` expression is not a choice: every license of it applies, and every one must be accepted. `A WITH B` is one license with an exception: read the exception and ask the user when it is not plainly permissive. Write down which license you took and why.

### The tree below the dependency

A permissive license on the top is not enough. The dependency may carry something restrictive, and that obligation reaches this project when the code is compiled in.

- Read the top-level `LICENSE` or `COPYING` file, then every nested `external/`, `third_party/`, `3rdparty/` or `vendor/` directory and every nested git submodule that the library's own `CMakeLists.txt` pulls in with `add_subdirectory` or `FetchContent`, with the options this project sets. An option that looks off by default can be turned on by this project's CMake, and that decides whether a nested license matters. A nested directory that no build compiles carries no obligation.
- A library that links a system library at run time counts too: name it and ask the user.

### Record the result

Make this table in your notes. Step 03 and the report use it.

| Name | Version | Offered license | Selected license | License file | Compiled in where |
| --- | --- | --- | --- | --- | --- |

## Done when

- Every package of the tree (the dependency and all it compiles in) is in the table with its offered license.
- Every row has a selected license from the accepted column, and the most permissive one was taken when several were offered.
- No restrictive license remains in the tree. If one did, you stopped and asked the user.
- Each license file exists and was read (not a placeholder).

Next: [steps/03-add-3rdparty.md](03-add-3rdparty.md)
