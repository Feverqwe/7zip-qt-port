---
name: sevenzip-regression
description: Design or review regression tests for Sevenzip Desktop bug fixes, archive edge cases, and platform-specific failures.
---

# Sevenzip Desktop regression work

Read `docs/testing.md` and the code around the changed behavior. Add a test that observes the user's expected outcome and fails before the fix when practical. Prefer small CTest tests and disposable archives created with the official `7zz` executable. Choose only failure cases relevant to the change, such as invalid archives, cancellation, passwords, unsafe paths, or Unicode filenames.

Run `./scripts/check.sh`; fix failures caused by the change. Explain what passed locally and which platform checks remain for CI. Do not replace behavior tests with source-text assertions or call a startup smoke test sufficient for archive features.
