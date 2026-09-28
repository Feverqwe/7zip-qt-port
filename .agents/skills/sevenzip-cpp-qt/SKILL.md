---
name: sevenzip-cpp-qt
description: Implement or review Sevenzip Desktop C++17 and Qt 6 Widgets code, including UI state, asynchronous archive operations, and CMake integration.
---

# C++ and Qt in Sevenzip Desktop

Follow `AGENTS.md`. Read `docs/architecture.md` when changing the UI/backend boundary. This repository owns the Qt UI and archive adapter; `../7zip-upstream` remains an independent, read-only source checkout.

Start from the original upstream implementation, copying code rather than recreating its behavior. Follow the source-port rules in `AGENTS.md`: preserve relative paths, filenames, symbols, module boundaries, and algorithm/control flow. Adapted application code lives under `src/<upstream-relative-path>`; unchanged supporting code lives under `third_party/7zip/<upstream-relative-path>`.

Adapt the presentation in upstream UI modules to Qt Widgets while retaining their functional code. Replace Win32/platform dependencies with narrow adapters where required; do not rewrite a module because it lives under `UI/` or because Qt offers another implementation. Reuse portable upstream types and dependencies. New functional logic needs a documented reason why no upstream implementation can be reused.

Record source commit, paths/symbols, copied and adapted portions, and reasons for changes in `docs/upstream-port-audit.md`; preserve source notices and licenses. Record observable differences in `docs/7zfm-parity.md`. Keep archive operations behind `ArchiveBackend` and the official engine. The current `7zz` adapter and existing custom Qt logic do not override the source-port goal.

- Keep widgets, archive operations, and process parsing separate. Widgets express user intent and display state; the backend owns `7zz` arguments, output, cancellation, and errors.
- Keep long archive operations off the UI event loop. Use asynchronous process signals or a worker when appropriate. Keep object lifetime and cancellation explicit; update widgets only on the GUI thread.
- Pass executable and arguments separately to `QProcess`. Do not run archive paths through a shell or build a single command string. Do not log passwords; assess process-argument exposure before implementing password support.
- Handle paths and names as Unicode. Use Qt path APIs rather than splitting archive paths on platform separators. Treat archive metadata as untrusted input, especially during extraction.
- Add new sources and tests to CMake. For behavior changes, use focused Qt/CTest tests with temporary directories and disposable archives. Use `$sevenzip-regression` when designing regression coverage or investigating a test failure.
- For visible UI changes, open the built window or dialog and inspect the rendered layout at normal and compact sizes. Review any generated screenshots rather than only checking that widgets exist; correct misaligned controls, uneven spacing, clipped labels, and awkward resizing before finishing. Compare grouping and control order with the relevant 7zFM reference.
- Preserve copied upstream structure and avoid unrelated style rewrites. Apply repository formatting to changed files covered by its formatting gate; keep required formatting changes distinct from functional adaptations in the port record. Run `./scripts/check.sh` for completed code changes.

Use `build/dev/compile_commands.json` with clangd for definitions and diagnostics. See `$sevenzip-navigation` when tracing behavior into upstream 7-Zip.
