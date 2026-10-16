# P3-03 — Current Windows whole gate and editor delivery diagnosis

[Index](../upstream-port-audit.md). Product revision
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
implementation change. Its result remains pending.

Evidence root `build/review/final-availability-8e9343b-20261014/`:
`windows-primitives-2fcc0e1-full/`, `windows-editor-phase-{v1,v2,minimal}-evidence/`,
`windows-editor-full-retained-fixture-postmortem.json`, source/stock-restoration
proofs and the preserved guarded drivers. Earlier accepted editor repetitions
retain their original609bf25 revision. Current P3-03.9 requires follow-up;
the P3 parents remain open.
