# Regression strategy

The regression gate is `./scripts/check.sh`. It checks C++ formatting, builds the development preset, and runs CTest. A tracked Git pre-commit hook runs the same gate after `.agents/skills/sevenzip-setup/scripts/setup.sh` configures the local checkout. GitHub Actions will run it on macOS, Linux, and Windows after the repository has a GitHub remote.

## What to test

- UI wiring: command availability, selected files, dialog inputs, and user-visible error handling.
- Backend: construct arguments without a shell, parse output and errors, handle progress and cancellation.
- Archive integration: make tiny disposable archives with `7zz`, then verify listing, extraction, creation, wrong passwords, corrupted data, Unicode filenames, spaces, and unsafe paths as each feature is implemented.
- Platform behavior: paths, permissions, encodings, and process launch on each supported OS.

Use CTest for tests and keep fixtures under `tests/`. Use temporary directories for generated archives. A test must assert observable behavior; merely launching a function or matching source text is insufficient. The existing `application_starts` test is only a Qt startup check. It does not verify archive behavior.

When a bug is found, first capture it with a focused regression test when practical, then fix it. Keep tests deterministic and independent of network access. Report the exact check command and result in the change summary. If a test cannot run on the current host, say why and rely on CI only after its result is available.
