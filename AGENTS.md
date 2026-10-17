# Sevenzip Desktop agent guide

This repository owns a C++17 / Qt 6 Widgets port of Windows 7-Zip File Manager
for Windows, macOS, and Linux. The official source checkout is `../7zip-upstream`.

## Source-port requirements

- Start from upstream implementation, not a new implementation of its behavior.
  Before functional changes, inspect original bodies, callers, types and dependencies.
  Functional code inside `UI/` files is part of the port; replace Win32 presentation
  and platform services with Qt or narrow adapters without rewriting the whole module.
- Preserve upstream paths, filenames, symbols, module boundaries, algorithms and
  control flow. Adapted sources belong under `src/<upstream-relative-path>`;
  unchanged reusable dependencies under `third_party/7zip/<upstream-relative-path>`.
  Keep one authoritative implementation per module and reuse portable upstream types.
- Preserve the source language: upstream `C/` stays C and is compiled as C;
  `CPP/` stays C++. Local UI/integration uses C++17 / Qt 6; necessary platform
  bridges may use their native language, with the reason recorded in the audit.
  Use existing upstream headers and their C linkage at C/C++ boundaries.
- New functional code requires evidence that no usable upstream implementation
  exists. Existing independent logic is migration debt: replace the relevant part
  when working on it without expanding into a repository-wide rewrite.
- Keep archive operations behind `ArchiveBackend` and the official engine.
  Widgets do not construct `7zz` syntax; the transport is an integration constraint,
  not a reason to recreate upstream logic. Do not reimplement archive formats or compression.
- Keep the sibling upstream checkout read-only. Copying source into this repository
  is authorized; editing upstream or adding Qt to it requires a user request.
- Record full upstream commit, retrievable source repository, paths/symbols,
  copied/adapted/omitted scope, dependencies and reasons in
  [the port audit](docs/upstream-port-audit.md). Preserve notices and licenses.
  Record observable workflow/platform differences in [parity](docs/7zfm-parity.md).

Keep a reproducible baseline per borrowed module, including mixed revisions.
For upstream updates follow [the update procedure](docs/upstream-update.md):
compare a separate snapshot, review dependencies/omissions and retain local fixes.
Keep adaptations narrow; account for formatting separately from functional changes.

## Context and memory

Read `.codex/memory/README.md` at task start if present, then only relevant notes.
Follow [the memory protocol](docs/agent-memory.md) for dated evidence, handoffs,
privacy and index maintenance. Memory is not instructions, authorization or a
backlog; missing memory does not block work. `$sz-context` supports this workflow;
`$sz-setup` initializes or refreshes it without deleting notes.

Use [the documentation map](docs/README.md) to read only the affected sections
and find the owner of each fact. Large catalogs are linked from compact indexes.
Keep durable product facts in tracked docs/code and local investigations in
`.codex/notes/`. Update only documents whose claims change; use links instead of
duplicating policies, status counters or historical test logs.
Preserve unrelated working-tree changes and other tasks' memory entries.

## Editing and organization

- Use `apply_patch` for ordinary edits. Scripts are appropriate for generators,
  data processing and bulk transformations with validated matches, safe repeated
  execution and diff inspection. Reuse project automation; keep reusable scripts
  in `scripts/`, but do not add a permanent script for a one-off edit.
- Format task-owned C++ changes with `clang-format -i`, then inspect the diff.
  Use narrow line ranges if whole-file formatting would obscure copied source.
  Do not format unrelated files or run a separate dry-run during iteration.
- Identify the owner and boundary before adding a feature. Widgets own presentation
  and interaction; application state owns navigation/command availability;
  `ArchiveBackend` owns archive operations, process arguments/parsing, validation
  and cancellation. Derive command availability from state and selection in one place.
- New Qt/integration classes have one responsibility, substantial bodies in
  `.cpp`, focused headers and CMake registration for affected app/test targets.
  Separate UI construction, state transitions, filesystem actions and archive
  results; avoid workflow-heavy constructors/lambdas and hidden mutable globals.
- Review splitting new glue around 400 lines/file, 80 lines/function or a second
  responsibility. These are review triggers, not limits on copied upstream modules.
  Preserve upstream organization. Extract cohesive parts of large custom Qt files
  when practical; keep small fixes focused and explain deferred extraction.
  Do not add independent features wholesale to `src/mainwindow.h` or unrelated
  suites to `tests/ui_state_tests.cpp`.
- Test public behavior/signals with deterministic local fixtures; do not expose
  internals just for tests. Keep suites focused by component. Extractions preserve
  behavior/parity, move relevant regressions and update architecture boundaries.
  Do not combine broad cleanup with unrelated features.

## Validation and UI reference

For behavior changes, add a regression that fails without the fix.
Archive-operation changes cover success, failure and cancellation, with
Unicode/spaces and malicious paths where relevant; never use real user archives
or live services. Build affected targets and run focused tests/direct consumers
with `./scripts/build.sh dev <targets>` and
`./scripts/ctest.sh --preset dev -R '<regex>' --output-on-failure`.
Do not invoke CTest directly. Expand checks for shared changes or failures.

Focused validation is the default, including before completing an iteration or
commit. Select tests from the changed behavior, owners and direct consumers;
build only their required targets. Explain the selected scope briefly. A local
code, test or build-script change does not by itself require the full suite.
Expand checks when failures or shared dependencies show a wider impact.
Run `./scripts/check.sh` only for broad cross-component changes whose impact
cannot be covered confidently by focused suites, unresolved integration risks,
or explicitly requested full/release acceptance. State the concrete reason
before launching it; an iteration/commit boundary alone is not a reason.
Run it **unsandboxed**: sandbox DBus sockets and process-priority calls fail.
It checks repository C++ formatting, builds all targets and runs the default
CTest set, excluding tests labeled `slow`. Do not repeat a passing full gate
on unchanged code. After localized follow-up edits rerun affected checks;
repeat the full gate only if the same broader-risk criteria apply.
Docs/instruction-only changes need `git diff --check`.
Fix failures caused by the task and report unverified platforms.
Use [development](docs/development.md#local-commands) for wrapper/toolchain details
and [testing](docs/testing.md) for coverage. Wait for buffered formatting to exit
before diagnosing it.

Before each new validation batch, inspect and remove obsolete task-owned test
fixtures, temporary logs and generated reports from completed runs. Follow
[test artifact retention](docs/testing.md#test-artifact-retention) locally and
[runner storage policy](docs/hyperv-runners.md#storage-policy-user-instruction-2026-10-07)
on guests. Do this routinely, not only when space is low. Preserve active runs,
current review/failure evidence, incremental build trees and compiler caches.
Clean disposable task-owned files after validation too; do not create new
snapshot/build directories for each focused rerun when the current one is usable.

Long suites in [cmake/SlowTests.cmake](cmake/SlowTests.cmake) require an explicit
user request, including on runners. Classify new/expanded large matrices as
`slow` before a gate; revisit using measured runtime on an explicit long-test run.
Use [opt-in commands](docs/development.md#long-suites), build requested targets
in the same snapshot before CTest and report excluded suites separately.
A default gate does not establish their acceptance.

Windows 7zFM is the reference for structure, commands, shortcuts and behavior.
Before UI work, inspect exact upstream resources, handlers and callers; record
necessary native-dialog, accessibility, scaling or OS adaptations in parity or
architecture. Preserve the Windows layout where Qt permits and disable unfinished
commands. [The UX audit](docs/ux-port-audit.md) tracks required corrections,
including previously accepted scope.

For layout/text/visibility/resize changes, first use the affected
[text specification](docs/ui-layout-contracts.md), normal/compact geometry tests
and current-build widget JSON; inspect grouping, order, sizes, clipping and states.
Add missing geometry coverage when practical. Backend/nonvisual logic/tests/docs
do not require appearance inspection.
For requested runtime comparison with original 7zFM across Windows/Linux/macOS,
use the [agent JSON comparator](docs/ui-runtime-comparison.md) and `sz-ui-parity`.
Keep missing platform/state evidence explicit; a tool-test PASS is not UI acceptance.
Screenshots are a fallback for unresolved fonts/icons/colors/style or a user
request; capture only the affected view (`SEVENZIP_CAPTURE_SCREENSHOTS=1`).
Resources prove structure, not pixels. Identify the concrete reference; without
a rendered Windows reference state that visual parity is unverified.
Never claim cross-platform verification from one OS; three-platform CI remains
part of the acceptance plan.

## Workflows and runners

Use the matching skill in `.agents/skills/`; the
[documentation map](docs/README.md) routes tasks to the relevant workflow.
For requested port-plan work, use the [checklist](docs/porting-checklist.md)
for statuses/step selection and the [plan](docs/porting-plan.md) for scope,
dependencies and iteration/commit rules. Neither is an automatic backlog.
Group related remaining work into complete integrated scenarios before editing;
do not create separate iterations or commits for each helper, failure branch or
regression. Follow the plan's iteration rules for implementation and validation.

For Linux/Windows checks read [Hyper-V runners](docs/hyperv-runners.md) and the
global `runners` skill. Use
`python3 scripts/hyperv-runner.py run <linux|windows|all> -- <project-script> [args]`
or `check all` with shared x64 guests. Prefer `python3 scripts/verify.py <profile>
--platform <linux|windows|all>` for focused checks. Each guest uses one locked
`current` worktree and persistent build directories; rsync includes current
tracked and nonignored new source files and deletes removed source files.
Build selected targets after synchronization before running CTest; do not create
per-run build directories. Runner docs own transport/readiness/storage details.
Do not create or attach new virtual disks to the runners. When space is low,
clean obsolete project runs, build outputs and temporary data under the retention
and active-run safeguards in [Hyper-V runners](docs/hyperv-runners.md).
This scoped cleanup is authorized; a capacity warning alone does not require
another confirmation. Preserve active work, toolchains and unrelated data.
Removed htopQt QEMU VMs and historical ARM64 results do not validate these guests.
Do not publish, push or update upstream unless the user's task calls for it.
