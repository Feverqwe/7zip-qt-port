---
name: sz-cpp-qt
description: Implement or review Sevenzip Desktop C++17 and Qt 6 Widgets code, including UI state, asynchronous archive operations, and CMake integration.
---

# C++ and Qt in Sevenzip Desktop

Follow `AGENTS.md` for source reuse, module organization, provenance and validation.
Read [architecture](../../../docs/architecture.md) for the affected owner and
UI/backend boundary. Use `$sz-navigation` to locate original bodies and dependencies.

- Keep long archive operations off the UI event loop. Use asynchronous process
  signals or a worker; make lifetime/cancellation explicit and update widgets
  only on the GUI thread.
- Pass executable and arguments separately to `QProcess`. Never run archive paths
  through a shell or construct a single command string. Do not log passwords;
  assess process-argument exposure when implementing password support.
- Handle paths/names as Unicode. Use Qt path APIs instead of splitting on platform
  separators. Treat archive metadata as untrusted, particularly during extraction.
- Register sources and focused test targets in CMake. Use `$sz-regression` for
  behavioral coverage and [UI contracts](../../../docs/ui-layout-contracts.md)
  for affected visible controls/states.

Use `build/dev/compile_commands.json` with clangd for compiler diagnostics.
