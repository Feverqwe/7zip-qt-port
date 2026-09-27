# Sevenzip Desktop agent guide

This repository owns a C++17 / Qt 6 Widgets desktop UI for Windows, macOS, and Linux. The official 7-Zip source is a separate sibling checkout at `../7zip-upstream`.

## Ownership and context

- Read and change this repository's code, tests, docs, and build tooling as needed for the user's task. Preserve unrelated changes.
- Use `README.md` for status, `docs/architecture.md` for backend boundaries, `docs/development.md` for the toolchain, and `docs/testing.md` for behavioral changes. Read only the context relevant to the task.
- Treat `../7zip-upstream` as read-only reference. Do not vendor or edit it unless the user asks for an upstream change. Keep archive operations behind an adapter; widgets should not know `7zz` command syntax.
- `.codex/` is a local Obsidian and agent workspace. Keep durable product facts in tracked `docs/` or code. Use `.codex/notes/` for local investigations or handoffs, not a second source of truth.

## Completion and regression checks

- For a behavior change or bug fix, add or update a test that would fail without the fix. Prefer deterministic CTest tests and local fixtures. Do not add a test that only repeats the implementation.
- Cover success, failure, and cancellation when changing archive operations. Include filenames with Unicode or spaces and malicious archive paths where relevant. Never use live services or real user archives in tests.
- Run `./scripts/check.sh` before completing code changes. It checks formatting, builds the app, and runs CTest. Fix failures caused by the change; report any platform that could not be checked locally.
- Keep the Windows 7-Zip workflow recognizable. Disable unfinished commands. Update README, architecture, testing guidance, and roadmap when their claims change.
- Do not claim cross-platform verification from a single local OS. CI checks all three platforms after the repository is published to GitHub.

Project workflows are in `.agents/skills/`: `sevenzip-setup`, `sevenzip-context`, and `sevenzip-regression`. Use them when their descriptions match the task. Do not publish or update upstream unless the user's task calls for it.
