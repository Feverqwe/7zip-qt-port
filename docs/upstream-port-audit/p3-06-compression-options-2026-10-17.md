# P3-06 — Add Options: source frame, captions and keyboard workflow

2026-10-17, parent `0c0d5d72e73cb3cc917ef9b9ca99d79dc109ecde`
(product `2a7303c`) plus this integrated presentation correction.
This closes the demonstrated COptionsDialog scope, not the P3 parents.

## Source boundary

Reviewed https://github.com/ip7z/7zip.git at
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`:
`CPP/7zip/UI/GUI/CompressOptionsDialog.rc`, `CompressDialogRes.h`,
`CompressDialog.cpp/.h`, shared `CPP/7zip/GuiCommon.rc`, and
`CPP/Windows/Control/Dialog.h`. Review includes the complete COptionsDialog
initialization, precision/time/checkbox handlers, OnOK/Help, caller
CCompressDialog::OnButtonClicked and their existing CArcInfoEx/CFormatOptions
owners. The desktop resource is256×248DLU, with a72DLU NTFS group, Time rows,
76DLU precision combo, explicit `:` checkboxes and64×16DLU footer buttons.

The original functional bodies remain in the authoritative adapted
`src/CPP/7zip/UI/GUI/CompressDialog.cpp`. Only the existing local
`qtcompressoptionscontrols.*` presentation adapter changes. It replaces the
independent520×400 frame and blank explicit captions, restores source spacing
through the existing dialog-unit service and retains native control minima.
Initial unused NTFS space can contract for native text minima/compact resizing.
No source format capability, timestamp algorithm, profile/persistence rule,
ArchiveBackend transport or archive operation changes. No module/dependency is
newly copied or omitted, no baseline revision changes, and source notices remain.

CModalDialog::OnInit returns true, requesting the native dialog manager's first
available tab stop. That platform service has no portable upstream Qt body.
The narrow adapter follows the existing Qt focus chain, filtering visible,
enabled tab stops for initial focus and Tab/Backtab. It adds no format-specific
focus list and changes no OS keyboard preference. Source checkbox/precision/OK
handlers still own the resulting values. Geometry and focus remain separate
small methods in the existing adapter; control construction stays with this
single dialog owner rather than introducing another workflow/controller.

Cocoa additionally needs one logical pixel below its group layout item and
control-dependent style spacing beside the precision combo. Actual widget
backings otherwise clip the group by1px and overlap the label by3px. Native
footer order, font metrics and compact minimum expansion remain documented Qt
adaptations. POSIX preserve-access stays disabled; ADS/security controls remain
hidden under the existing Windows-only capability policy.

## Concrete original reference

Protected source-built7zG SHA256
`a3f3743ea3b13bee0f2cb033091c5f86a0ea803362c8d28cd825223f69534c16`
is hash-verified with all manifest assets, including the same Russian language
file as the product. It runs under an actual Limited, non-elevated token before
startup/private-key writes. Run `20261017T142830Z-2f74afff` captures eight actual
EN/RU states:7z/ZIP/WIM initial and ZIP Unix precision. Initial client is
384×403 physical pixels,96DPI, MS Shell Dlg at height−11. Source extraction
independently establishes all22 controls and the resource dependencies.

GetGUIThreadInfo observations establish initial focus→first Tab:
7z203→204 (CTime→ATime explicit), ZIP201→203 (Precision→CTime explicit),
WIM4040→4041 (symbolic→hard links), identically in EN/RU. These are owned-window
native message observations, not a hardware-key claim. All dialogs finish with
Cancel; source bytes remain exact, outputs are absent, processes exit and the
private reference key is removed. Upstream and the protected reference stay
read-only. Disposable reference/fixture copies are removed after observation.

The earlier six-state capture uses a disabled combo programmatically; its Unix
labels are not accepted as a user workflow. V2 corrects this with BM_CLICK and
checked/enabled guards; V3 adds the actual focus/Tab observation. These attempts
remain separate evidence, not overwritten or counted as extra coverage.

## Regression and current three-platform validation

The old product gives actual RED12: six wrong initial frames and six missing
colon captions. Further public native RED6 proves the Cocoa backing defects
before their corrections, with exact source restoration/stock rebuild afterward.
Initial-focus RED6 then finds footer focus; the next RED6 finds Cocoa Tab staying
on the same checkbox. Final source-compatible focus/traversal passes.

Normal/compact EN/RU,7z/ZIP/WIM, explicit captions/accessibility names, source
initial focus, Tab/Backtab, Space/Down precision and conditional visibility,
Cancel/reopen and Return→OK profile publication are checked by the focused new
presentation suite. Existing advanced/timestamp/language consumers retain Help,
live-language pending values, OK/Cancel/persistence and real archive time
round-trips. Their compact requests expand to native minima; exposed-text and
control assertions remain. A new offscreen empty-space threshold is removed
after its placeholder font metrics disprove that assumption; actual resource
frame, native geometry relations, visibility and text-fit checks remain.

| Platform, Qt6.12 | Focused consumers | Native producer | Reports |
| --- | --- | --- | --- |
| macOS arm64 |5/5,95Qt/0fail/0skip,3.24s |14Qt/0fail/0skip,1.067s |14JSON,0PNG |
| Linux x64 |5/5,95Qt/0fail/0skip,3.18s |14Qt/0fail/0skip,1.087s |14JSON,0PNG |
| Windows x64 Limited |5/5,95Qt/0fail/0skip,20.24s |14Qt/0fail/0skip,1.013s |14JSON,0PNG |

Guests use run `20261017T143426Z-91434ff9`, persistent current/build/dev and
unchanged source guards. All six affected product/build/test inputs match the
final root. macOS uses public Cocoa Qt events, with no OS input, screenshots or
Computer Use. Linux uses private Xvfb/Openbox. Five direct consumers cover the
localized adapter; unchanged passing full-gate portions are not repeated solely
for this component. No slow suite or new engine build is selected. C++ is
clang-formatted; diff check passes.

Final review verifies all six product/build/test input hashes against the
accepted native/default evidence. The integrated `sevenzip_desktop` target
also builds incrementally on macOS and both guests; guest run
`20261017T144703Z-0697f406` finishes with exit0 on each OS. No build tree or
compiler cache is reset.

The runtime comparator maps all22 controls across eight states × three OS.
Windows passes8/8 pairs. Raw POSIX comparisons remain failed for exactly24
expected Windows-only state differences: preserve-access enabled in16 pairs,
and WIM ADS/security visible in eight comparisons. They are not relabelled exact
Windows-state passes. The source branches and public capability assertions
explain those differences; no other text/state/fit/relation error remains.
Independent normalized comparison checks346 visible captions with zero
differences, removing only whitespace wrapping and mnemonic markers. Numeric
bounds/deltas are retained; no arbitrary geometry tolerance or pixel-equivalence
claim is introduced. The comparator itself is unchanged.

Evidence: `build/review/goal-compression-options-20261017/`, including source
extraction, reviewed22-control contract, actual reference/focus records,
RED/GREEN logs, final source hashes and native reports, raw runtime comparison
and explicit platform-state review. Initial driver/helper/path failures remain
harness failures and are excluded from accepted checks. Persistent builds/caches
and failure/reference evidence are retained; completed one-shot source scripts
are archived and removed.

The preceding2a7303c private packages predate this correction; the subsequent
[three-platform7a8592f refresh](p3-04-compression-options-delivery-2026-10-17.md)
includes it. Windows cold timing and the final
native workflow matrix remain P3-03/P3-06 prerequisites. The count stays44/47;
no push, publication, upstream or VM/disk configuration change occurs.
