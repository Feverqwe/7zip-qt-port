# Copy publication widget fixtures — 2026-10-17

> Архив завершённого Qt-портирования, 2026-10-17. Статусы и проверки
> относятся к указанным ревизиям. [Рабочая документация](../../../README.md).

P3-03/P3-06 component evidence; final acceptance remains open. Product sources
remain at `e3b3c652d4d2489d9143f3002c498057d6bd3c38`. This iteration changes
test presentation and readiness, not the source-port algorithms, archive engine,
application deadlines or native OS dialog behavior.

## Boundary and failure evidence

The existing `WidgetDialogFixture` now serves panel Copy reload, Copy publication
order, native Parent publication and native Drop publication. Their responders
inspect Qt widgets, so they explicitly use widget dialogs and wait until the
dialog's nested loop is running before answering. The separate Drop cancellation
timer remains able to answer the nested cancellation question. Parent no longer
waits for destruction of an already hidden, accepted widget error.

Before this correction, the Cocoa Drop exposure-only pilot reached
`NSAlert.runModal` and the original whole-suite limit. An own-process sample
records that boundary; no OS input was sent. Initial activation-only fixture
attempts also retained real failures. In v202 five of six suites passed, but
Column had ten activation failures. Its source guard failed only because two
documentation files changed during the run; test inputs were unchanged.

`WindowActivation::waitForActive` keeps exposure, raising and activation within
one original five-second deadline. It uses public Qt calls and reports public Qt
application/window/modal/popup state on activation failure. Qt 6.12's Cocoa
`QCocoaWindow::requestActivateWindow` requests the key window, whereas `raise`
also orders the window forward and activates its application. This supports the
fixture's explicit raise request; it does not establish the cause of every
earlier intermittent refusal. A sandboxed diagnostic failed before workflow
execution with “no screens available”; its unsandboxed replay passed all18 Qt
checks before adding raise.

## Focused validation

Only the six directly affected consumers are selected: `app_copy_routing`,
`panel_copy_reload`, `copy_publication_order`, `native_column_workflow`,
`native_parent_publication`, `native_drop_publication`. The build uses
`scripts/build.sh`; all test execution uses `scripts/ctest.sh`. This test-only
change does not require repeating the already recorded whole application gate.

macOS v205: Cocoa6/6 PASS75.89s,100Qt/0fail/0skip,320 widget JSON/0PNG;
source-input and configuration-restoration guards pass. Default v208:
6/6 PASS15.35s,100Qt/0fail/0skip. Existing normal/compact geometry and workflow
assertions remain in force. Earlier v199/v200 snapshots passed7/7 but precede
the final Parent and activation follow-ups.

Linux v206, run `20261017T081603Z-ab51c2fc`: default6/6 PASS21.30s and
XCB6/6 PASS24.84s, each100Qt/0fail/0skip;320 native JSON/0PNG.
Input and configuration guards pass. Collected evidence:
`linux-final-copy-activation-v206-evidence` under the evidence root below.

Windows v207, run `20261017T081606Z-104a9377`: native6/6 PASS94.22s,
100Qt/0fail/0skip,320JSON/0PNG. Input and configuration guards pass. Collected
evidence: `windows-final-copy-activation-v207-evidence`. All eight fixture and
consumer files match the current checkout in all three native receipts.
`python3 tests/ctest_selection_tests.py` also passes its disposable synthetic
slow-selection contract without executing the declined matrix.

Local evidence root:
`build/review/final-availability-8e9343b-20261014/`, with `mac-copy-six-v205`,
`mac-copy-six-default-v208`, earlier `mac-copy-six-v202` failure and
`mac-column-state-v203`/`v204` diagnostic receipts. Evidence retains the exact
input hashes; a passing replay does not erase earlier failures.

## Declined long matrix and remaining acceptance

`copy_archive_refresh` has192 native workflows. The separate proposed widget
fixture completed89 without a row failure before the original120s Cocoa cap.
It is now classified `slow`; the macOS aggregate budget is540s, twice the
extrapolated full time rounded to a minute. Windows retains360s. Per-state waits
and application deadlines are unchanged.

The user explicitly declined the new full three-platform replay on2026-10-17.
It was not selected in v205–v208. Its proposed responder changes remain outside
this verified fixture commit and its current full native matrix is unverified.
The previous11 explicitly requested long suites retain their own revisions and
three-platform receipts; they do not establish acceptance of this12th suite.
This fixture evidence also does not replace rendered Windows reference states,
physical input/native-panel acceptance or the open Windows cold-codec failure.
