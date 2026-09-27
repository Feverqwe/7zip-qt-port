# Sevenzip Desktop agent guide

This repository owns a C++17 / Qt 6 Widgets desktop UI for Windows, macOS, and Linux. The official 7-Zip source is a separate sibling checkout at `../7zip-upstream`.

## Ownership and context

- Read and change this repository's code, tests, docs, and build tooling as needed for the user's task. Preserve unrelated changes.
- Use `README.md` for status, `docs/architecture.md` for backend boundaries, `docs/development.md` for the toolchain, and `docs/testing.md` for behavioral changes. Read only the context relevant to the task.
- Treat `../7zip-upstream` as read-only reference. Do not vendor or edit it unless the user asks for an upstream change. Keep archive operations behind an adapter; widgets should not know `7zz` command syntax.
- `.codex/` is a local Obsidian and agent workspace. Keep durable product facts in tracked `docs/` or code. Use `.codex/notes/` for local investigations or handoffs, not a second source of truth.

## Code organization

- Before adding a feature, identify its owner and the boundary it crosses. Keep widgets responsible for presentation and user interaction, application state for command availability and navigation, and `ArchiveBackend` for archive operations, process arguments, parsing, validation, and cancellation. Do not move backend decisions into widgets to save a file or class.
- Give each class and source file one coherent responsibility. Put substantial implementations in `.cpp` files; keep headers focused on interfaces, small inline functions, and Qt declarations. Register new source files in `CMakeLists.txt` and the affected test targets.
- Split code when a file approaches roughly 400 lines, a function approaches roughly 80 lines, or a change adds a second distinct responsibility. These are review triggers, not reasons to fragment cohesive code into trivial wrappers. Name extracted pieces for their domain role and give them narrow inputs and outputs.
- Existing large files are technical debt, not a pattern to copy. In particular, do not add another independent feature wholesale to `src/mainwindow.h` or another broad test suite to `tests/ui_state_tests.cpp`. For a related change, extract the relevant cohesive part when practical; for a small local fix, keep the diff focused and state why extraction was deferred.
- Keep UI construction, state transitions, filesystem actions, and archive result handling in separate functions or components. Avoid long constructors, nested action lambdas containing workflow logic, hidden mutable globals, and duplicate state checks. Make command availability derive from the current state and selection in one place.
- Organize tests by behavior and component. Put a new feature's cases in a focused test file when they would enlarge an unrelated suite. Test through public behavior or observable signals, with local deterministic fixtures; do not expose production internals solely to make a test pass.
- When extracting code, preserve behavior and platform parity, move or update the relevant regression tests, and update `docs/architecture.md` if ownership or component boundaries change. Do not combine a broad cleanup with an unrelated feature.

## Completion and regression checks

- For a behavior change or bug fix, add or update a test that would fail without the fix. Prefer deterministic CTest tests and local fixtures. Do not add a test that only repeats the implementation.
- Cover success, failure, and cancellation when changing archive operations. Include filenames with Unicode or spaces and malicious archive paths where relevant. Never use live services or real user archives in tests.
- Run `./scripts/check.sh` before completing code changes. It checks formatting, builds the app, and runs CTest. Fix failures caused by the change; report any platform that could not be checked locally.
- Treat the Windows 7-Zip File Manager (`7zFM`) as the product reference for window structure, menus, toolbar, file list, dialogs, command names, shortcuts, and behavior. Port those interactions to Qt Widgets step by step; do not invent a new file-manager design or rearrange core workflows without a documented reason and user direction. Relevant original UI code and resources are in `../7zip-upstream/CPP/7zip/UI/FileManager/`.
- Before a UI feature, inspect its Windows reference and record any necessary platform adaptation in `docs/7zfm-parity.md` or `docs/architecture.md`. Preserve the Windows-style layout across platforms where Qt permits; differences for native system dialogs, accessibility, scaling, or OS integration should be explicit. Disable unfinished commands. Update README, architecture, testing guidance, and the parity record when their claims change.
- Do not claim cross-platform verification from a single local OS. CI checks all three platforms after the repository is published to GitHub.

Project workflows are in `.agents/skills/`: `sevenzip-setup`, `sevenzip-context`, `sevenzip-navigation`, `sevenzip-cpp-qt`, and `sevenzip-regression`. Use them when their descriptions match the task. Do not publish or update upstream unless the user's task calls for it.
