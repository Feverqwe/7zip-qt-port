# P3-05/P3-06 — shared modal geometry and Copy presentation (2026-10-17)

Baseline: https://github.com/ip7z/7zip.git, full commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`. The sibling checkout stays read-only.

## Source scope

Original `CPP/Windows/Control/Dialog.cpp::CDialog::NormalizeSize` and
`NormalizePosition` bodies are adapted in the same mirrored module under
`src/CPP/Windows/Control/`. Win32 rectangle, monitor, parent, move and maximize
services become QWidget and the existing `IQtDialogScreenServices` boundary.
Branch ordering remains authoritative: size normalization ignores nonoverlapping
or fitting frames; full normalization maximizes oversized overlapping frames;
partial normalization clamps and centers each dimension. Position normalization
uses an entirely contained parent on the same monitor when the child fits.
With work-area placement, entirely inside and entirely outside windows remain
unchanged; only overlapping frames that fit are centered.
This is a scoped adaptation of those two bodies, alongside the existing
IsDialogSizeOK. Other Win32 dialog creation/message-dispatch helpers remain
the Qt presentation/event-loop boundary; no full Dialog.cpp port is claimed.

Reviewed callers: `CPP/7zip/UI/FileManager/{CopyDialog,EditDialog,ComboDialog,
SplitDialog,MessagesDialog,ListViewDialog,BrowseDialog,BrowseDialog2}.cpp`,
`ProgressDialog2.cpp`, `CPP/7zip/UI/GUI/BenchmarkDialog.cpp` and
`CPP/7zip/UI/GUI/LinkDialog.cpp`, their resources, shared `GuiCommon.rc` and
public operation callers. All eleven active size callers delegate to the shared
source method; Benchmark and Compress position callers do likewise. Existing
handlers, results, worker algorithms and archive boundaries remain. Notices and
source C++ language are preserved; no C module is changed or omitted.

Qt constructs these controls before native modal presentation. The narrow
`qtdialogscreenservices` adapter applies the source operation once at first Show,
after native frame creation, and removes its filter before applying the policy.
Later user resize/reopen is preserved. Maximization requests window state rather
than exposing a partially initialized modeless dialog. Client resize accounts
for native frame margins. Native window managers may impose additional limits.
`sevenzip_panel` owns this adapter once; standalone Progress/Compress consumers
link that owner instead of receiving duplicate per-executable definitions.

Copy's initial client follows `CopyDialog.rc`336×160DLU, including shared
eight-DLU margins, with Qt font mapping/minimum protection. Its source static
summary has no Tab stop. Copy, Combo, Split and Progress request source
`MY_MODAL_RESIZE_DIALOG_STYLE` minimize/maximize capabilities without context
help; Progress preserves flags on modal reparenting. Native modal chrome may
suppress controls. Compress retains its different original window style.

## Regression evidence and remaining acceptance

Evidence root: `build/review/final-availability-8e9343b-20261014`.
Actual pre-fix Copy regressions show eight frame failures (`v101`), eight extra
summary focus-stop failures (`v102`) and four oversized-font normalization
failures (`v105`). `v109` shows missing source chrome in all four affected views;
its geometry policy suite passes27Qt checks. A first expanded build exposed a
missing standalone Progress link dependency (`v106`), corrected at the common
owner. Sandbox priority failures (`v107`) are not passes; identical unsandboxed
consumers pass11/11 in `v108`, before position/chrome additions.

`dialog_size` covers source size/position branches through deterministic public
screen services and one-shot first-Show behavior. `copy_dialog` covers Copy/Move,
EN/RU,9/12/32pt, initial/compact JSON, selected initial path, allowed Tab stops,
Enter raw Unicode/space preservation and Escape. Combo/Split/Progress retain
normal/compact consumers and assert chrome; Progress worker checks modal parent
and flags, including the original fast path that never creates a dialog.

Expanded macOS `v111` passes23/26 suites. Three initial-frame tests retain
obsolete unbounded expectations on the800px offscreen display: source partial
normalization reduces the resource width, while Qt layout minima still protect
height. Their expectations now account for work area, native frame margins and
Qt minimum hints; all three pass in `v113`1.57s. Existing control visibility,
grouping, clipping and focus assertions remain. A new worker-parent assertion
also respects the source fast path that never creates a modal dialog.
All24 Copy/Move initial/compact reports were inspected (`v117`): five source
control groups are visible, enabled and unclipped, with fitted prompt labels.

Baseline verification `v117` passes412exact/198adapted/3local paths at one
revision. The302-owner candidate inventory records exactly eight changed
FileManager owners from7dadf6d;109-C++ dispositions remain8exact/96adapted/5absent.
The common Windows/Control source body is outside that UI ledger and covered
above. macOS full `v114` passes325/325268.61s,5233Qt/0fail/66platform skips,
with all1472 captured source/config inputs unchanged. Later edits affect only
the new lifecycle test. Cocoa `v118` passes22/23 suites,371Qt/1fail/0skip;
the new first-Show test incorrectly assumes a work-area-sized ordinary Cocoa
window cannot be reported as maximized. Its reduction fixture now exceeds only
screen width, leaving the native zoom convention outside that lifecycle check.
Source full-normalization/maximize branches remain in deterministic policy
tests and Copy32pt consumers. Corrected `dialog_size` passes27Qt in macOS
default `v122` and native Cocoa `v123`; the other22 Cocoa suites need no repeat
on unchanged product code.

Linux `v116` retains its failing receipt: default26 actual suites pass but the
unbuilt required archive_progress fixture and ui_parity_progress do not run;
native XCB21/22 pass,366Qt/1fail/0skip. The old lifecycle test hides/reopens
before asynchronous window restoration settles. The reduction lifecycle
fixture avoids that unrelated transition and still verifies first Show,
user resize and repeat Show. The next driver builds archive_progress in the
same snapshot. These failures do not change production policy or deadlines.
Corrected Linux `20261017T005510Z-98676908` (`v120`) passes default26suites385Qt
and native XCB22suites367Qt,0fail/0skip,324native JSON/0PNG, with source/config
guards passing. It precedes only the final width-only lifecycle fixture edit.
Fresh full Linux `20261017T010015Z-95a2bfd4` (`v124`) includes that edit:
324/324525.49s,5225Qt/0fail/66platform skips; exact native lifecycle replay
passes27Qt0.06s. All1472 current compiled source/configuration inputs match
the root bytes, source/config restoration guards pass. The default gate excludes
the eleven explicitly requested slow suites, whose results are recorded separately.

Windows Limited `v115` passes all22 executed focused suites in110.19s. The
unbuilt required archive_progress fixture and ui_parity_progress do not run;
they are not counted as passes. All11 long suites then run in the same sealed
snapshot. Completion and the targeted fixture follow-up remain pending.
Windows resource-fixture follow-up and final parent acceptance remain open.

These regressions
do not establish pixel equality or physical OS keyboard behavior. Existing
pinned Windows Copy rendering is the visual reference; no macOS screenshots
or OS input automation is used. Full P3 acceptance remains open.
