---
name: sevenzip-cpp-qt
description: Implement or review Sevenzip Desktop C++17 and Qt 6 Widgets code, including UI state, asynchronous archive operations, and CMake integration.
---

# C++ and Qt in Sevenzip Desktop

Follow `AGENTS.md`. Read `docs/architecture.md` when changing the UI/backend boundary. This repository owns the Qt UI and archive adapter; `../7zip-upstream` remains an independent, read-only source checkout.

Port 7zFM workflows and operation rules into the Qt application while keeping the interface implementation native to Qt Widgets. Do not import its Win32 UI classes or reimplement archive codecs; route archive work through `ArchiveBackend` and the official 7-Zip engine.

When borrowing functional logic, retain the upstream module boundaries and recognizable names where practical. Map each ported functional area to its upstream source in `docs/7zfm-parity.md`; keep Win32 presentation code out of Qt components and archive-operation decisions inside `ArchiveBackend`.

- Keep widgets, archive operations, and process parsing separate. Widgets express user intent and display state; the backend owns `7zz` arguments, output, cancellation, and errors.
- Keep long archive operations off the UI event loop. Use asynchronous process signals or a worker when appropriate. Keep object lifetime and cancellation explicit; update widgets only on the GUI thread.
- Pass executable and arguments separately to `QProcess`. Do not run archive paths through a shell or build a single command string. Do not log passwords; assess process-argument exposure before implementing password support.
- Handle paths and names as Unicode. Use Qt path APIs rather than splitting archive paths on platform separators. Treat archive metadata as untrusted input, especially during extraction.
- Add new sources and tests to CMake. For behavior changes, use focused Qt/CTest tests with temporary directories and disposable archives. Use `$sevenzip-regression` when designing regression coverage or investigating a test failure.
- For visible UI changes, open the built window or dialog and inspect the rendered layout at normal and compact sizes. Review any generated screenshots rather than only checking that widgets exist; correct misaligned controls, uneven spacing, clipped labels, and awkward resizing before finishing. Compare grouping and control order with the relevant 7zFM reference.
- Format changed C++ files with the repository `.clang-format` rules. Run `./scripts/check.sh` for completed code changes.

Use `build/dev/compile_commands.json` with clangd for definitions and diagnostics. See `$sevenzip-navigation` when tracing behavior into upstream 7-Zip.
