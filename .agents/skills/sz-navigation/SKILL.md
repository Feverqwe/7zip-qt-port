---
name: sz-navigation
description: Trace Sevenzip Desktop C++ code, Qt UI behavior, 7zz integration, and corresponding upstream 7-Zip implementation when locating symbols or planning a refactor.
---

# Navigate Sevenzip Desktop and upstream 7-Zip

Search with `rg -n` and `rg --files`. Use `build/dev/compile_commands.json` for
compiler-aware definitions/references. Refresh with `./scripts/build.sh dev`
when build inputs change; check a translation unit with
`clangd --check=<source-file> --compile-commands-dir=build/dev`.

Locate original code in the read-only `../7zip-upstream/CPP/7zip/`:
`UI/FileManager` for 7zFM, `UI/Console` for CLI behavior, `Archive` for formats,
`Compress`/`Crypto` for algorithms. Use `git log -S'literal' -- path` or
`git blame` when history matters.

Trace complete bodies, callers, supporting types and platform dependencies,
not just resources or names. Search `docs/archive/qt-port/upstream-port-audit/`
and `docs/archive/qt-port/omitted-scope-audit/` for baseline path/symbol decisions;
use `rg --no-ignore` for these opt-in archive directories. Current
records are linked from [the audit index](../../../docs/upstream-port-audit.md).
Follow `AGENTS.md` for port requirements.
In a plan/review identify source revision/symbols, copied code and necessary
adaptations; cite concrete paths/lines and verify findings in current code/tests.
