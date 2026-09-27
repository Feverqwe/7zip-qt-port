---
name: sevenzip-regression
description: Add or review regression coverage for Sevenzip Desktop behavior changes, archive operations, or platform-specific fixes.
---

# Sevenzip Desktop regression work

Read `docs/testing.md` and the code around the changed behavior. Add a test that observes the user's expected outcome and fails before the fix when practical. Prefer small CTest tests and disposable archives created with the official `7zz` executable. Include failure cases that matter for the change, such as invalid archives, cancellation, passwords, unsafe paths, and Unicode filenames.

Run `./scripts/check.sh`; fix failures caused by the change. Explain what passed locally and which platform checks remain for CI. Do not replace behavior tests with source-text assertions or call a startup smoke test sufficient for archive features.
