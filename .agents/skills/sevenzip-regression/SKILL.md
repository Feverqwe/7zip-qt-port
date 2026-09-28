---
name: sevenzip-regression
description: Design or review regression tests for Sevenzip Desktop bug fixes, archive edge cases, and platform-specific failures.
---

# Sevenzip Desktop regression work

Read `docs/testing.md` and the code around the changed behavior. Add a test that observes the user's expected outcome and fails before the fix when practical. Prefer small CTest tests and disposable archives created with the official `7zz` executable. Choose only failure cases relevant to the change, such as invalid archives, cancellation, passwords, unsafe paths, or Unicode filenames.

For a port or adaptation, compare the changed implementation with the cited upstream source as well as checking behavior. Matching output alone does not establish source reuse; document necessary differences in `docs/upstream-port-audit.md`. Keep regression tests behavioral rather than checking copied text.

For UI layout work, use `docs/ui-layout-contracts.md` and `tests/layout_inspection.h` for text reports and behavioral geometry checks. Test affected views and conditional states at normal and compact sizes; use screenshots only for unresolved rendering details or at the user's request.

Follow the staged validation rules in `AGENTS.md`. During implementation, build affected targets and run focused CTest cases covering the changed behavior and direct consumers; expand for shared components or failures. Run the full `./scripts/check.sh` once before completing code/test/build-tooling changes, without repeating it on unchanged code. Documentation and instruction edits need only `git diff --check`. Fix failures caused by the change. Explain what passed locally and which platform checks remain for CI. Do not replace behavior tests with source-text assertions or call a startup smoke test sufficient for archive features.
