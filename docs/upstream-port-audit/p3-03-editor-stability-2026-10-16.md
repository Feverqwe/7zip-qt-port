# P3-03 — Current Windows whole gate and editor delivery diagnosis

[Index](../upstream-port-audit.md). Earlier gate product revision
`2fcc0e1ffc3c9e42765a11f014a75d676a484cac`. This receipt preserves a failed whole
gate and subsequent diagnostic evidence; it does not claim a stability fix.

Windows snapshot `20261016T004958Z-53dcb02f` completes338/339 executed tests in
2492.11s,5381Qt/1fail/20skip. One private-VHD test remains disabled and the two
existing no-VHD subsets remain selected; eleven slow suites remain excluded.
LastTest contains340 records because CTest writes the disabled empty-command
record as `Test Passed`; that record is excluded from the338/339 execution count.
All1047 committed product inputs match. Canonical CMake and stock generated
configuration are restored/reconfigured, stock targets rebuild successfully,
and the source snapshot is unmodified. Synthetic HEAD is
`68f175910e7a4a613886882e6d0c2721ab643843`.

The previous process_pause/external_codecs failures do not recur in this run;
codec_probe also passes. This does not establish the cause or resolution of
earlier cold delays. The sole failing suite is archive_external_edit:
`refusedUpdateRetainsEditedCopy(3-0-0-0-1)` does not observe its expected error
within the existing5s outcome wait. That row uses readonly TAR, pane0, English,
table view and alternative selection. Its last logged modal is the extraction
progress dialog with Cancel. The suite finishes in53.18s; all its other cases
pass. A read-only postmortem finds one retained disposable edited fixture with
the expected33-byte modified content. Only size/classification is exported,
without file contents or paths. The failure's cause remains unproven.

Reviewed current worker/process wait, snapshot, changed-file/readonly branches,
QtExternalGuiCall delivery/readiness, MainWindow update guard and test event-loop
callers; source provenance remains the pinned
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` from https://github.com/ip7z/7zip.git.
No product behavior, source algorithm, deadline or required assertion changes.

The first guarded diagnostic `20261016T015200Z-cf71eda9` stops before tests:
its harness incorrectly expects `-silent`, while Windows already uses `-v1`.
All source inputs/configuration are restored and stock rebuilding passes.
Corrected snapshot `20261016T020353Z-aa95ad2e` passes the48-row refusal group
(50Qt) and the exact failing row separately (3Qt). The row's process/snapshot/
changed-file work completes in84/110ms; GUI readiness flags are clear, the
readonly error returns normally and its event loop returns0 after41/65ms.
The failure does not reproduce. All five temporarily instrumented files and
generated configuration are restored and the stock target rebuild passes.
These passing probes do not replace the failed whole gate.

Three complete native editor-suite repetitions with minimal test-only failure
diagnostics are started in `20261016T022640Z-e82ec463`. Production sources remain
uninstrumented. The same existing5s outcome and90s per-execution caps apply;
no new matrix or long-suite opt-in is introduced. Eight official engine/SFX
inputs are reused from the completed diagnostic after independent SHA256 and
full upstream revision checks. App/test targets build in the new snapshot;
this is neither a new engine build nor cold-codec acceptance. Exit codes are
0/8/0. The second execution fails readonly TAR row `(3-0-0-1-0)` instead:
pane0/English/icons/ordinary selection. Its event loop returns0 after4998ms,
the child finished marker and modified bytes are present, backend busy is false,
the window is enabled and no modal is active. No child-write error is recorded.
No application lifecycle/Quit event is captured by the test observer. This
reproduces the failure without production-source instrumentation; it does not
identify the stalled worker phase. Both test files are restored and the stock
target rebuild passes.

A following sparse diagnostic is started after this restoration. It logs native
waits/GUI delivery only when slow and records readiness refusals, without
changing deadlines or filtering process snapshots. An additional snapshot probe
reports only a boolean for creation-order inconsistencies in candidate parent/
child relations; no PID, process name, path or absolute creation timestamp is
exported. This is a hypothesis check, not an established PID-reuse cause or
implementation change. Snapshot `20261016T030055Z-1ce61fc1` subsequently
finishes with editor exits0/0/8. The failed ZIP row has four adopted processes
still being waited on after4002ms. Two candidate child/parent pairs have the
child created before its purported parent. The worker's native wait reaches
5090ms while the GUI remains enabled, idle and without a modal. All six
instrumented files are restored and the stock target rebuild passes. This
identifies false descendant adoption from stale Toolhelp parent PIDs after
PID reuse; the failure is not specific to readonly TAR.

## Windows snapshot identity adaptation

Reviewed upstream `CPP/7zip/UI/FileManager/PanelItemOpen.cpp::GetSnapshot`,
`CChildProcesses::Update`, `MyThreadFunction`, their callers, and
`CPP/Windows/ProcessUtils.h::{CProcess::Open,CProcess::GetTimes}` / `CHandle`
at full commit `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` from
https://github.com/ip7z/7zip.git. Upstream matches a parent by numeric PID and
contains no creation-order validator in this path. Reuse its existing process
query and scoped handle wrappers; adapt only `src/qtexternalprocess.cpp`'s
Windows OS snapshot service. The source descendant traversal, worker, GUI
publication, names, ordering and POSIX services remain unchanged.

The adapter caches optional creation times per snapshot with query-limited,
noninherited handles. A child older than its purported parent receives a zero
parent ID. Equal creation ticks remain valid; failed queries preserve the
original relation. All snapshot entries are checked, including grandchildren.
This narrows an OS identity ambiguity without changing source update/error/
cancellation flow or the existing5s/90s test deadlines. Microsoft documents the
[stale parent PID and creation-time check](https://devblogs.microsoft.com/oldnewthing/20150403-00/?p=44313)
and the [query permission for GetProcessTimes](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getprocesstimes).

Windows x64 `process_snapshot_tests` uses executable-local SDK fixtures through
the public native snapshot service, without real process creation or exposing
product internals. Five cases cover actual children, recycled parent IDs,
equal ticks and unavailable parent/child times; order, names, handle cleanup
and limited access are asserted. Snapshot `20261016T033218Z-31cd4125` executes
the unchanged product and fails only `recycled-parent-id` (6Qt PASS/1FAIL).
After the adapter fix, `20261016T033729Z-b6af1bc6` passes7Qt/0fail/0skip.
The earlier two RED launchers fail at SDK linking before tests and do not count
as regression evidence. Native integrated repetitions and the required full
gate are separate checks; Windows whole-gate acceptance remains pending.
Required unsandboxed macOS `scripts/check.sh` passes formatting, all-target
build and323/323 default tests in260.39s. Eleven slow suites remain excluded;
this does not establish Windows runtime acceptance.

Uninstrumented native Windows snapshot `20261016T033955Z-575d136f` builds the
snapshot regression and archive editor consumer, then passes three consecutive
executions of both suites:67Qt/0fail/0skip each (180 integrated editor checks
plus21 snapshot checks). All tracked source input hashes are independently
verified unchanged. The existing5s outcome/90s suite deadlines, real controlled
children, archive bytes, retained-copy assertions and update/failure/cancel
coverage remain in force. Session1/Highest launches the product tests only;
no original reference startup is run elevated. The first native launcher had
an invalid additional build target and stopped before tests; it does not count
as a product failure or passing native execution. Evidence:
`process-snapshot-native-evidence/` and `.tar.gz` under the common evidence root.
The following whole Windows gate is started separately with the existing
no-new-VHD guard and eleven slow suites excluded.

Current Linux snapshot `20261016T040220Z-6477ea9b` completes the default gate:
322/322 in509.94s,5144Qt/0fail/66skip. Its uploaded source inputs are
independently rehashed after the gate and stock Git is unchanged. Evidence is
`linux-process-snapshot-full/` and `.tar` under the common evidence root.
Current Windows whole snapshot `20261016T034626Z-7dab4581` completes339/340
executed tests in2550.18s,5388Qt/1fail/20skip. Its341 LastTest records include
one disabled empty-command VHD record, excluded from executed counts. All1623
uploaded inputs are independently rehashed, canonical source/configuration
restored and stock reconfigure/rebuild passes. Synthetic snapshot HEAD is
`17b442199fcf3817a0c7ae3ff0831cd05e21554e`. Evidence:
`windows-process-snapshot-full/` and `.tar.gz` under the common evidence root.

Archive_external_edit passes60Qt in46.47s, including the formerly failing
refused-update branches. Process_pause and codec_probe also pass. The sole
external_codecs failure reproduces the earlier independent startup timing problem:
`incompatibleModuleIsSkippedWithSourceError` observes its ready signal only
after6750ms against the original5000ms wait. Its12 other Qt checks pass. This
is not a passing whole Windows gate; the startup-delay cause remains unproven.
The editor lineage correction is accepted as the P3-03.9 component through
its actual RED/GREEN, three native repetitions and successful suite in the whole
gate. The separate P3-03.3 startup/stability requirement and P3 parents remain
open; no timeout, native loader algorithm or security setting is relaxed.

Evidence root `build/review/final-availability-8e9343b-20261014/`:
`windows-primitives-2fcc0e1-full/`, `windows-editor-phase-{v1,v2,minimal}-evidence/`,
`windows-editor-full-retained-fixture-postmortem.json`, source/stock-restoration
proofs and the preserved guarded drivers. Earlier accepted editor repetitions
retain their original609bf25 revision. Current P3-03.9 requires follow-up;
the P3 parents remain open.
