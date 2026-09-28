---
name: sz-navigation
description: Trace Sevenzip Desktop C++ code, Qt UI behavior, 7zz integration, and corresponding upstream 7-Zip implementation when locating symbols or planning a refactor.
---

# Navigate Sevenzip Desktop and upstream 7-Zip

Search this repository first with `rg -n` and `rg --files`. Use `build/dev/compile_commands.json` with clangd or an editor when compiler-aware definitions, references, or diagnostics help. Refresh it with `./scripts/build.sh dev` if build inputs have changed. `clangd --check=<source-file> --compile-commands-dir=build/dev` checks one translation unit.

For a source port, locate the original implementation and its dependencies in the relevant upstream area: `../7zip-upstream/CPP/7zip/UI/FileManager` for Windows UI, `UI/Console` for `7zz` behavior, `Archive` for format handling, and `Compress` or `Crypto` for algorithms. Keep upstream read-only. Use `git log -S'literal' -- path` or `git blame` when history helps explain a decision. Confirm a conclusion in current code or tests before editing; cite concrete paths and lines in the result.

Before implementing a feature, identify the upstream files, symbols, supporting types, and platform dependencies to copy. Read the relevant implementation, not just resources or observed behavior. Use `docs/upstream-port-audit.md` to distinguish actual source ports from existing custom implementations.

Follow `AGENTS.md`: preserve upstream relative paths, filenames, symbols, and functional code; adapt UI presentation to Qt and platform services only where necessary. Win32 presentation code needs adaptation, but functional code in the same UI file remains a source to copy. Keep the sibling checkout unmodified; copy into this repository. Keep archive operations behind `ArchiveBackend` and the official engine. In a plan or review, identify copied code and explain each necessary departure from upstream.
