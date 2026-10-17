# P3-03/P3-06 — Modal command ownership and native widget fixtures

## Source comparison and correction

Baseline: https://github.com/ip7z/7zip.git at
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`.
Reviewed complete CreateFile, context-menu dispatch, refresh and focused-panel
bodies and their consumers in `CPP/7zip/UI/FileManager/PanelOperations.cpp`,
`PanelMenu.cpp`, `PanelItems.cpp`, `App.cpp`, `App.h` and `PanelDrag.cpp`, with
the existing Qt reload/panel-projection adapters. Existing audit entries retain
their copied/adapted/omitted inventory; no additional upstream module is copied.

The original CreateFile invokes RefreshListCtrl on its captured panel. The Qt
reload callback also captures that panel, but its presentation consumers use
CApp's focused panel. Actual Cocoa execution shows a mismatch after modal close:
the command and Qt projection still own panel0, while native focus restoration
has selected panel1. CreateFile succeeds on disk, but the peer's cached catalog
is reloaded and the owner's list stays stale with automatic refresh disabled.

The narrow correction in `src/mainwindow_refresh.cpp` aligns the source focused
panel with the callback owner even when the Qt projection already names that
owner. Original command/refresh algorithms and the original SetFocusedPanel body
remain unchanged. No format, engine, language or archive-operation transport is
reimplemented. The existing both-pane EN/RU table/icon context-menu scenarios
protect the observable file/catalog/list result and peer isolation.

Three isolated Cocoa runs reproduce the original missing-row failure, including
the exact focus/projection mismatch. Temporary tracing is removed; the upstream
derived PanelOperations source is restored exactly. The corrected existing
context-menu suite passes its12 workflow rows on all three native platforms.

## Test-fixture scope

Widget responders explicitly use Qt dialogs, queue answers until their nested
loops are ready, and avoid reentering an answer for the same dialog while its
callback processes events. A distinct nested question still receives its answer.
The new public-dialog regression actually fails without the reentry guard
(parent answered twice), then passes after the correction. It does not inspect
source text or private fixture state.

Standalone Cocoa progress dialogs also need raising before requesting activation,
as in the existing WindowActivation helper. Existing progress/cancel scenarios
reproduce30s timeouts before this follow-up and pass afterwards without larger
budgets. Parent's test responder now waits for the displayed localized Close
caption and visible messages, rechecking ownership/state before a posted click;
it no longer treats a language-binding property as proof that Close is usable.
The original unexpected cancellation question remains in the failed receipt.

These are widget fixtures and public Qt event producers. They do not establish
OS-panel, physical-input or pixel acceptance. No macOS OS input or screenshots
are used. Existing byte, failure, cancellation, selection, geometry and timing
assertions remain; no matrix row or application deadline is removed or relaxed.

## Focused validation

All builds reuse existing incremental trees and compiler caches; all CTest runs
use the project wrappers. Nineteen suites cover the changed fixture owners and
direct Copy/refresh/Parent/Drop/checksum/progress consumers, including the new
responder regression. No slow suite is selected.

| Platform | Actual execution |
| --- | --- |
| macOS arm64, Qt6.12.0 | Default19/19 PASS,381Qt/0fail/0skip,60.75s. Initial Cocoa16/19 PASS387.23s; two progress timeouts and Parent's unexpected question remain failures. Final focused4/4 default12.68s / Cocoa39.51s PASS, each48Qt/0fail/0skip, covers all three failures and the responder regression. |
| Linux x64, Qt6.12.0 | Run `20261017T181512Z-4ed76103`: default19/19 and native XCB19/19 PASS, each381Qt/0fail/0skip;298 native JSON/0PNG. Private Xvfb/Openbox, without physical console acceptance. |
| Windows x64, Qt6.12.0 | Run `20261017T181511Z-624bf725`: Limited interactive runner session, default19/19 and native19/19 PASS, each381Qt/0fail/0skip;298 native JSON/0PNG. No administrator full gate. |

All recorded source/configuration guards pass. Guest runtime/SFX hash guards also
pass. The macOS executions form a composite across the activation/responder
follow-up; the initial19-suite Cocoa run is not relabelled PASS. A separate broad
Cocoa inventory run retains155/167 PASS and12 failures before these corrections.
Its actual LastTest is retained; a previously interrupted inventory's stale
LastTest copy is excluded from acceptance.

The other two broad failures are existing aggregate matrices. Default checks
pass2/2. Public `-datatags` inventories contain82 interarchive-recovery and80
drag-reload rows. All162 existing rows pass Cocoa in nine batches of at most20,
at the original per-execution limits, with488 JSON/0PNG and source/configuration
guards. Both original120s aggregate timeouts remain failures. No full aggregate
PASS, additional matrix, larger deadline or declined slow-suite acceptance is
inferred from batching.

Evidence: `build/review/goal-native-reconciliation-20261017/`, including
`mac-remaining`, isolated `mac-context-*` failures, `mac-owner-publication`,
`mac-progress-ready`, `mac-matrix-batches`, `linux-evidence` and
`windows-evidence`. The exact guest driver is archived there and removed from
scripts after both completed runs. Active/current builds, runtime producers,
compiler caches and current review/failure evidence are preserved.

Source completeness and runtime validation remain separate. This iteration
corrects one Qt boundary and validates its consumers; it does not close the
remaining parent acceptance automatically. Product56c01ba packages precede the
reload correction and require refresh before final delivery acceptance.
