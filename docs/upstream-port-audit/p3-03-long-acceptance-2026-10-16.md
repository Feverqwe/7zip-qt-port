# P3-03/P3-05 — explicit long-suite acceptance, 2026-10-16

The user explicitly requested all eleven suites from `cmake/SlowTests.cmake`
on macOS, Linux and Windows. These results supplement the default gates,
which exclude the `slow` label. Final acceptance remains open.

## Initial complete run

Evidence root: `build/review/final-availability-8e9343b-20261014`.
macOS receipt: `mac-slow-acceptance-v51`; Linux snapshot
`20261016T194559Z-b43643e7`; Windows snapshot `20261016T194626Z-2efc99ae`.
The guest receipts are retained in the corresponding
`*-final-slow-acceptance-v51-evidence` directories. All source and generated
CTest configuration guards passed. Windows used the existing Limited runner
session; these suites do not require a general elevated gate.

| Suite | macOS | Linux | Windows |
| --- | --- | --- | --- |
| native_rename_2 | PASS | PASS | aggregate 360 s timeout |
| native_rename_301 | PASS | PASS | aggregate 360 s timeout |
| native_external_2 | PASS | PASS | aggregate 240 s timeout |
| native_external_301 | PASS | PASS | aggregate 240 s timeout |
| native_renderer_failure | PASS | PASS | aggregate 180 s timeout |
| native_renderer_copy | PASS | PASS | 53 Qt pass / 45 fail |
| native_nested_stream | PASS | PASS | 180 s timeout; one confirmed-Cancel failure |
| native_nested_memory | PASS | PASS | 180 s timeout; one confirmed-Cancel failure |
| native_nested_disk | PASS | PASS | aggregate 180 s timeout |
| interarchive_refresh | PASS | PASS | aggregate 120 s timeout |
| panel_creation_publication | 229 Qt pass / 32 fail | 229 Qt pass / 32 fail | 5 Qt pass / 256 fail |

Elapsed totals: macOS 1116.85 s, Linux 1182.61 s, Windows 2292.65 s.
Timeout suites have incomplete Qt totals and are not counted as passes.
Windows renderer Copy failed the peer enabled/redraw assertion; creation
failed its initial observation assertion. These require separate diagnosis.
The confirmed-Cancel failures cannot be attributed to an aggregate budget
without investigating their modal interaction.

## Correcting the filesystem provider fixture

Upstream repository: <https://github.com/ip7z/7zip.git>, commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`.
`CPP/7zip/UI/FileManager/PanelItems.cpp::OnReload/RefreshListCtrl` calls
`_folder->LoadItems()` on the bound provider. The current Qt filesystem
loader likewise receives and reloads the bound `CFSFolder`.
The creation fixture's expectation that a large filesystem Refresh replaces
that provider was obsolete. It now verifies initial provider identity and
retained identity after Refresh, preserving the rows, marks, focus, peer
and byte assertions. No product implementation changes for this correction;
no copied modules or dependencies.

Focused follow-ups pass all 261 Qt checks on macOS (`mac-creation-and-root-red-v53`,
35.54 s; repeated with direct consumers in `mac-persisted-root-green-v55`)
and Linux (`20261016T202215Z-b7568cad`, `final-creation-provider-v54`).
Windows fails earlier in the observation fixture and remains unaccepted.
The ten passing macOS/Linux matrices need no repeat for this fixture-only fix.

## Windows fixture diagnosis and affected follow-up

The bounded snapshot `20261016T205059Z-52166e6b` proves the creation callback
compares the same generated file with native backslashes against forward
slashes. It therefore never invokes `BeforeProbe` on Windows. The fixture now
normalizes separators on both operands before that comparison; it still
observes the original initial/late-panel ordering and retains all data rows.
The trace is in `windows-final-slow-diagnostic-v58-evidence`.

Renderer Copy previously waited for the active source's Refresh action, then
immediately asserted peer redraw. The existing peer filesystem worker completes
independently; `failFilesystemListing` clears rows before terminal state and
`refreshActions` restores each pane from its own scanning flag. The fixture now
waits for the peer's public enabled/redraw completion before retaining its
terminal assertions, provider, bytes, source selection and recovery checks.
The single bounded diagnostic row passed even without observing a disabled
peer, so it is not claimed as a deterministic reproduction of the whole-matrix
race. The original 45 whole-run failures remain the failing evidence.

Nested confirmed-Cancel uses the existing `WidgetDialogFixture`: queued timer
delivery and a dialog exposed/active/nested-loop readiness check. This prevents
the raw timer from clicking during native dialog presentation. Original
`ProgressDialog2.cpp::OnButtonClicked(IDCANCEL)` opens a nested Yes/No/Cancel
question, restores pause and then applies the confirmed stop policy. Product
control flow is unchanged; all six answers/panes per route, fifteen-second
watchdog and subsequent native recovery/bytes/lease assertions are retained.
Creation and interarchive tests also join the existing Windows native-font
plugin/desktop-lock list rather than relying on the offscreen font database.

The five affected full suites pass on macOS after these fixture changes:
`build/review/final-slow-fixtures-v59`, 210.81 s, 770 Qt pass / 0 fail / 0 skip
(nested stream/memory/disk 137 each, renderer Copy 98, creation 261).
Source/configuration guards pass. Linux snapshot `20261016T205608Z-a5da942d`
also passes all five suites, 238.31 s; Windows results follow below.

Windows follow-up `20261016T205609Z-0821b583` completes in 833.50 s:
renderer Copy passes all 98 Qt checks in 173.32 s. The three nested routes
time out on the old facade; creation reaches 122 of 256 matrix rows at its
120 s aggregate cap without an assertion failure. Its 243 generated reports
include a partial final row. All source/configuration guards pass; the complete
receipt is `windows-final-slow-fixtures-v59-evidence`. This establishes the
peer-wait correction separately from the outstanding native hold lifetime.

## Measured Windows aggregate budgets

Initial Windows JSON shows continued publication up to the cap for the six
matrices below, without a recorded Qt assertion failure in those suites.
Counts identify reached data rows, including a possibly partial last row;
they do not count completed assertions. Retained progress summaries are
`windows-long-progress-v51.json` and
`windows-long-distinct-final-v51.json` (including full hyphenated interarchive
route names).

| Matrix | Reached / total rows | Previous aggregate cap | Windows aggregate cap |
| --- | --- | --- | --- |
| native_rename_2 | 188 / 192 | 360 s | 720 s |
| native_rename_301 | 137 / 192 | 360 s | 1080 s |
| native_external_2 | 149 / 192 | 240 s | 660 s |
| native_external_301 | 102 / 192 | 240 s | 960 s |
| native_renderer_failure | 185 / 288 | 180 s | 600 s |
| interarchive_refresh | 91 / 256 | 120 s | 720 s |
| panel_creation_publication (corrected v59 fixture) | 122 / 256 | 120 s | 540 s |
| native_renderer_copy (corrected v59 fixture) | complete, 98 Qt pass in 173.32 s | 180 s | 360 s |

Caps allow about twice the extrapolated whole-matrix runtime, rounded up to a
minute. Only Windows aggregate CTest limits change. Application codec discovery,
individual state waits, cancellation watchdogs, required rows and assertions
remain unchanged. The last two limits follow the measured v59 result and
are included in both the root checkout and current Windows snapshot. Nested limits remain
180 s: their hold failures need verification of the actual fixture fix first.
Passing reruns are still required; a budget adjustment is not acceptance.

## Native facade control-handle lifetime

Queued modal delivery alone does not fix the Windows nested hold. Snapshot
`20261016T205609Z-0821b583` still fails No/Cancel and times out on the old DLL.
Its disposable active memory control file refuses an open requesting DELETE
access with `ERROR_SHARING_VIOLATION (32)`; the probe only opens/closes a handle,
does not delete or write the file, and is limited to this run's own fixture
paths (`windows-nested-control-handle-v59.json`).

`tests/external_format_fixture.cpp::CGlobalOnlyArchive::Open` kept its parsed
`std::ifstream` open across `SetCompleted` and sleep. On Windows this prevents
the GUI's `runtime_.clear()` from removing the artificial hold file after
answering No/Cancel. The writer has the same lifetime problem: its initial
stream remained open through delegated `UpdateItems`, and the polling stream
remained open through callbacks. All three parsed streams now close before
calling the unchanged official archive implementation or its callback. No
archive algorithm, product DLL, upstream source, control predicate or deadline
changes. This test-only control protocol has no upstream implementation to
reuse; the facade continues to delegate all archive work to the official module.

macOS direct consumers pass after the handle fix:
`build/review/final-native-facade-v61`, six suites, 149.23 s, 441 Qt pass /
0 fail / 0 skip (external codecs 13, Parent 7, Drop 10, nested 137 each).
Source/configuration guards pass. Linux snapshot `20261016T210612Z-bb8e5e60`
also passes these six suites, 142.01 s, 441 Qt pass / 0 fail / 0 skip.
Earlier v59 passes used the old facade. Current writer follow-ups run the four
Rename/External matrices on macOS/Linux. Windows snapshot
`20261016T211446Z-eb733735` runs all eleven long suites plus Parent, Drop and
external codecs with the corrected facade and measured Windows aggregate caps.
These active runs are not yet acceptance evidence.

The macOS writer follow-up subsequently passes all four full matrices:
`build/review/final-writer-matrices-v64`, 750.24 s, 778 Qt pass / 0 fail / 0 skip,
with source/configuration guards passing. Linux snapshot
`20261016T211131Z-bfe4769e` also passes all four writer matrices, 729.89 s,
778 Qt pass / 0 fail / 0 skip, with passing guards; its receipt is
`linux-final-writer-matrices-v64-evidence`. Windows complete acceptance remains
active. Early Windows v63 direct-consumer results are
Parent 7 Qt pass, Drop 10 Qt pass, external codecs 12 pass / 1 fail: the actual
global-only handler discovery times out again. It remains P3-03.8, independently
of the test facade's control-handle correction.

## Remaining acceptance

The complete Windows v63 run finishes8/11 long suites successfully in3215.20s
(3254.74s including three direct consumers). Rename2/301 pass195Qt each,
External2/301194Qt each, renderer failure294Qt, Copy98Qt, creation261Qt and
interarchive258Qt. All three nested routes still reach their180s caps; stream
and memory report spool-cleanup failures. The fixture retained an extra
`childProvider` reader until after Parent attempted deletion. The test now
releases that extra reader before Parent, after verifying recovered provider
identity. This respects original reader/lease ownership rather than changing
product deletion policy or suppressing cleanup assertions.

Current integrated snapshot `20261016T222256Z-67c427ba` then passes stream and
memory137Qt each in155.75/156.01s, with the extra reference released and current
proxy-lifetime correction. Disk still times out at180.03s, with no completed-row
JSON at all; a matrix-budget explanation is therefore not established. The
eight other complete Windows long passes remain retained; no disk cap change
is justified yet. Current Mac/Linux integrated fifteen suites pass683Qt each,
including all three corrected nested routes. Separate worker-deadline regression
passes on Windows18Qt while real global-only discovery still times out.

### Disk route metadata contract

The bounded first-row diagnostic `20261016T230009Z-37304817` reproduces the
disk stall with a45s diagnostic cap. Source phase follow-up
`20261016T230728Z-683fa672` shows original native Extract returnsS_OK before
the first GUI timer tick; it never reaches nested Open. The fixture's route
override keeps reporting2^63 as the archive item size. Original
`PanelItemOpen.cpp::OpenItemInArchive` needs that override only for its RAM/disk
decision; `ArchiveExtractCallback.cpp::GetStream` subsequently uses item size
for Windows's default native preallocation. POSIX defaults preallocation off.
This is a fixture protocol defect, not evidence of a slow matrix.

A public native-metadata assertion added to the existing disk rows fails on
macOS before the fixture correction:9Qt pass/128fail, actual size2^63 versus
the exact199/7111byte generated inner archives (`mac-disk-metadata-red-v93`).
The facade now consumes its artificial routing size once per archive instance;
subsequent properties delegate the true size to the official module. Extraction,
preallocation defaults, disk routing, required rows and original180s aggregate
cap remain unchanged. Current focused follow-ups are required before acceptance.

Exact Windows message follow-up `20261016T231416Z-6aaed205` confirms original
preallocation failure: output file length cannot be set because its file pointer
would precede the beginning of the file. The worker has returnedS_OK with a
message; original Progress retains the result for acknowledgement, while the
old fixture waits for nested Open and cannot acknowledge it. Its25s diagnostic
timeout is retained separately, not a new acceptance cap. No task-owned large
payload is required or created for the correction.

Current macOS six-suite follow-up passes441Qt/0fail/0skip in140.06s with source
guard passing (`mac-disk-metadata-v95`), including all three137Qt nested routes
and native external/Parent/Drop direct consumers. Linux snapshot
`20261016T231849Z-d2532428` also passes all six,441Qt/0fail/0skip in142.98s,
with source/configuration guards passing. Current Windows focused follow-up
`20261016T231906Z-0b4df445` passes all four suites,167Qt/0fail/0skip in197.84s;
disk completes137Qt in157.88s under its unchanged180s cap. Source/configuration
guards pass. External codecs13Qt also pass on this execution; this test-only
metadata change is not a causal cold-loading fix and does not erase earlier
default-deadline failures. Because the source Agent lifetime correction also
touches native writer/reopen consumers, final all-eleven runs on the current
product are required separately from earlier passing old-source matrices.

Resolve the Windows failures and assess aggregate budgets from measured
progress while retaining all rows and individual/application deadlines.
The repeated Windows codec-discovery 5000 ms failures are tracked separately
under P3-03.8; a passing long matrix would not close them. No final P3 parent
or three-platform acceptance is established by these receipts.

## All-eleven macOS/Linux acceptance (test83163a4 / product7dadf6d)

The complete current macOS run passes11/11 in1265.86s,2100Qt/0fail/0skip:
`mac-current-all-long-v98`. All1471 captured compiled source/configuration inputs
match committed test revision `83163a47f8c5895c8c92d89d96e29c262fe6531d`.
All eight official runtime/SFX hashes match before/after. The suite set exactly
matches all eleven entries in `cmake/SlowTests.cmake`; none is omitted or replaced
with a representative row. This supersedes earlier Mac long results for current
source acceptance, while preserving their failures and corrected-fixture history.
Linux snapshot `20261016T233608Z-f98e0528` also passes11/11 in1191.36s,
2100Qt/0fail/0skip,
with all1471 compiled inputs matching the same committed test revision and
source/configuration/runtime guards passing. Current Windows all-eleven acceptance
and the independent cold/native/rendered parent requirements remain open. These
receipts precede the subsequent shared CDialog/Copy presentation correction;
its focused/current/native consumers are required separately. Windows replay
`20261017T004810Z-11bf0cbe` is running with that product correction, all eleven
long suites and unchanged per-case/application deadlines.
