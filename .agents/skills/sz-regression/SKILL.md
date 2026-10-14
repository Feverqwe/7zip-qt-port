---
name: sz-regression
description: Design or review regression tests for Sevenzip Desktop bug fixes, archive edge cases, and platform-specific failures.
---

# Sevenzip Desktop regression work

Use [testing](../../../docs/testing.md) to select checks; search
`docs/testing/` for the affected suite and inspect its current code/callers.
Follow `AGENTS.md` for coverage, focused build/CTest
wrappers, the full gate and reporting platform limits.

Prefer small public-behavior tests and disposable archives made by official
`7zz` or the affected native provider. Select relevant failure cases rather
than repeating a universal edge-case list. Matching output alone does not prove
source reuse: compare port bodies with the recorded upstream implementation.
Do not replace behavior tests with source-text assertions or treat a startup
smoke test as archive-feature coverage.

For layout work use [UI contracts](../../../docs/ui-layout-contracts.md) and
`tests/layout_inspection.h` for affected views/states and geometry reports.
