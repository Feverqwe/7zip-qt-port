# P3-06 — Source fallback and temporary-file browsers (2026-10-16)

Integrated correction after cadb215. Final checks are being collected; this
receipt does not close P3-06 or its parents. Original rendered/keyboard reference
and native consumers are verified below.

## Source boundary

Baseline https://github.com/ip7z/7zip.git,
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`:
`CPP/7zip/UI/FileManager/BrowseDialog.cpp/.h/.rc/BrowseDialogRes.h`,
`BrowseDialog2.cpp/.h/.rc/BrowseDialog2Res.h` and `CPP/7zip/GuiCommon.rc`.
Existing complete-body dispositions are in the
[Browse source closure](../omitted-scope-audit/filemanager.md#browse-path-dialog-disposition-2026-10-10)
and [temporary browser closure](../omitted-scope-audit/filemanager.md#temporary-browser-complete-bodycaller-disposition-2026-10-10).
This correction reinspects the original OnInit/OnSize, complete resource/shared
macros, MyBrowseForFolder/BrowseForFile/MyBrowseForTempFolder callers and
`CPP/Windows/Control/Dialog.cpp::NormalizeSize`. The latter constrains windows
outside the work area; it does not replace normal resource dimensions.

No new source module is copied or omitted. Adaptation stays in the existing
BrowseDialog.cpp, BrowseDialog2.cpp/.h and the established
`qttempbrowsepresentation.cpp` presentation boundary. Original OnOK is restored:
only actual list focus invokes OnItemEnter; readonly-path Enter does not end or
navigate the dialog. Qt accept/keyPress transport delegates to this same body.
Parent/root navigation, native enumeration, filtering/sorting,
raw acceptance/Cancel, ownership protection and deletion algorithms are unchanged.
The ordinary OS Save/Folder picker and its accepted manual checks are unaffected.

The custom fallback restores272×336DLU instead of560×640 pixels; temporary
browser restores466×344DLU instead of900×580 pixels. Eight-DLU root margins,
navigation buttons retain exact24DLU widths with16DLU minimum heights; public
style content rectangles prove caption fit. Standard64×16DLU buttons protect
native caption hints. Source5/3DLU command/path/list gaps in Browse2 and8DLU filter/footer
gaps are explicit. Native font/control hints can enlarge clients/buttons.
The existing temporary-browser500×320 compact minimum remains an accessibility
adaptation. Source minimize/maximize/resize capabilities are requested without
context-help; modal window managers can suppress requested chrome. Existing
native footer ordering remains. Temp has no permanent Enter default: focused
buttons activate on Return, list Enter navigates, readonly-path Enter is ignored.

These narrow Win32 resource/control services have no reusable upstream Qt
implementation; the existing Qt adapter provides them. The existing large
OnInit/BuildControls construction bodies remain within their established owner;
a broad extraction is deferred to keep this resource correction focused.

## Earlier checks and retained baselines

Actual pre-product RED has four failed EN/RU rows,29Qt passes: both browsers
have the wrong initial frame. After correction, focused `source_browse_dialog`
and `temp_cleanup` pass2/2 in4.25s,33Qt/0fail/0skip on macOS offscreen, including
their unchanged public navigation/create/selection/accept/Cancel and owned
deletion/protection consumers. Current-build JSON is retained.
Required unsandboxed macOS Qt6.12 full gate passes324/324 in266.27s,
5177Qt/0fail/66skip. Final native Cocoa passes2/2 in8.70s,33Qt/0fail/0skip; current
initial EN/RU JSON shows source frames521×672/892×688, exposed top/path/list/
footer controls and the expected hidden FolderMode filter. Native standard
button ordering, including Cocoa Help placement, remains a platform adaptation.
Inputs are unchanged, generated configuration restored, and no macOS screenshot
or OS input is used. The final temporary-browser report uses the public
MyBrowseForTempFolder/Create lifecycle, with loaded path/column labels and a
registered disposable fixture. Its suite uses a private owned-temp registry
for its whole lifetime and restores the previous environment afterwards.
The earlier constructor-only initial reports remain historical and are excluded
from original/current view comparison; they did not load the path/headers.
Focused checks after that fixture correction pass2/2 in1.29s. These results
precede the subsequent focus/column/control corrections below.

Linux first snapshot `20261016T101909Z-552dbc63` passes5/5 focused in1.41s,
80Qt/0fail/0skip, and2/2 native in0.97s,33Qt/0fail/0skip,16JSON/16PNG. Initial
Browse EN/RU views are reviewed; the constructor-only Temp initial PNGs are
excluded as above. Final native/whole follow-up uses the corrected public fixture.
Snapshot `20261016T102533Z-563e7798` passes5/5 focused and2/2 native,80Qt/33Qt
with zero failures/skips, summed times1.37/0.96s. All four affected owner/test
inputs match current files. Final native EN/RU Temp PNGs show the loaded path,
six translated columns, disposable fixture and protected active owned-resource
entry; controls fit. Private registry and generated configuration are restored.
Its subsequent full gate passes323/323 in550.70s,5169Qt/0fail/66skip, in the
same snapshot after restoring native fixture configuration. All1629 uploaded
inputs match terminal guest files and the HEAD diff is empty; synthetic snapshot
`b0980bfee2a4e2a2a0b2dd441a92c488b652b9ce`. The eight pinned runtime/SFX inputs
are reused with byte/pin/mode verification; this does not claim a fresh engine
build. Final full evidence is collected separately from the earlier native phase.
This whole snapshot precedes the subsequent corrections below. Eleven slow
suites remain excluded throughout.

## Final source corrections and reference

Both browsers now use the exact original demonstration strings for column
initialization, Qt content autosizing and sample removal before real reload.
An icon slot protects the name sample. Fixed130px Temp widths and Browse's name
width that omitted its icon are removed. Native header minimums protect labels;
stored names/values and compact horizontal scrolling remain unchanged.
Paths/filters have14DLU minimum heights. FolderMode retains the original vacant
filter row because source OnSize reserves both edit/filter slots even when hidden.
Cocoa uses the combo's actual widget rectangle for layout so optical margins do
not shrink that reserved row. The footer gap uses public layout-item geometry;
actual control visibility remains independently asserted.

Original Browse initially focuses edit102; Temp focuses Close8. Temp DM_GETDEFID
remains IDOK1 without an IDOK control, while focused Close has native default
styling. Source OnOK explicitly ignores non-list focus. Qt autoDefault permits
focused button Return; a scoped focusChanged callback clears fallback styling
after non-button focus and disconnects before QDialog teardown. No Qt private API
is used. Qt6.12's [button focus/default implementation](https://github.com/qt/qtbase/blob/v6.12.0/src/widgets/widgets/qpushbutton.cpp)
and [dialog fallback implementation](https://github.com/qt/qtbase/blob/v6.12.0/src/widgets/dialogs/qdialog.cpp)
explain why clearing the property before focus restoration was insufficient.
Source tab order is explicit. Ordinary Parent/Create also activate on Return
instead of falling through to OK; Create cancellation never creates a directory.

Private original producer `20261016T110650Z-39ec4a90` builds pinned FM sources
with a test entry calling original MyBrowseForFolder/MyBrowseForTempFolder.
Limited/Elevated=False is verified before startup/private key writes. Ten
original presenter/header/resource/shared/FM bodies are independently verified
against Git blobs; Windows formatting differs only by CRLF. Official26.03
language assets match their provenance. Adapter/binary/source hashes remain
stable and every normal exit removes its private key. No sibling-upstream,
authorization or OS policy change is made.

Controller `20261016T112315Z-de61b835` reuses that binary and supplies owned
extended long paths and private TEMP/TMP. Four populated initial EN/RU views
are captured, clear and reviewed. The7zO12345678 fixture follows the source root
admission rule:7z[E/O/S] plus exactly eight hex digits. Targeted native WM_KEYDOWN
probes confirm initial Close exit, readonly-path no-op, list navigation/Parent
return and focused Refresh publication. Only owned HWNDs are targeted; hardware/
global input is not claimed. Qt's accepted owned-temp membership boundary stays
explicit, with active/unconfirmed entries protected.

Earlier7zOwnedReference captures are valid empty states retained separately.
The first reference compiler attempt fails before startup on a shadowed adapter
variable; strict warnings remain. The second fixture hits Python MAX_PATH.
A standard extended path prefix fixes the fixture without changing OS policy.
Qt elides its static folder label and keeps the full tooltip/accessible name;
the original static label clips it. Native fonts, button ordering and this
accessibility adaptation preclude a pixel-equality claim.

Additional actual RED receipts cover navigation widths4, Temp focus/columns/
focused Return8, Browse sample/icon fitting2 and remaining height/top-button6
rows. Tests cover Return/keypad Enter, readonly-path/public accept guards,
Parent/Refresh/Close and Create cancellation, with a private owned-temp registry.
They wait for exposed/active windows before observing actual focus. Failed Qt
default/lifetime attempts are retained, including the destructor callback fatal.

Final native Cocoa v6 passes2/2,43Qt/0fail/0skip with unchanged inputs, restored
configuration and no PNG or OS input. The final unsandboxed macOS full gate
passes324/324 in265.65s,5187Qt/0fail/66skip after these corrections.
Guest v3 `20261016T122939Z-afd4e393` passes
5focused/2native: Linux90Qt/43Qt in summed1.61/1.37s; Windows92Qt/45Qt
in12.26/5.38s, all0fail/0skip,16JSON/16PNG each. It predates only the final combo
rectangle/footer metric. Exact final guest `20261016T123855Z-cdabe948` passes
5focused/2native: Linux90Qt/43Qt in summed1.81/1.34s, Windows92Qt/45Qt
in12.70/4.98s, all0fail/0skip. Both retain16JSON/16PNG; all six affected
source/header/test hashes match current inputs. Initial EN/RU grouping, order,
caption fitting and column presentation are reviewed against the original views.
Private configuration is restored and source inputs remain unchanged.
Eight pinned runtime/SFX inputs are reused with byte/pin/mode checks;
no fresh engine build is claimed. Private Linux Xvfb/Openbox is not physical
console proof. Final packages and whole native/workflow acceptance remain open.

Evidence root: ignored `build/review/final-availability-8e9343b-20261014`,
`source-browser-frame-{red,green}*`, `mac-source-browser-full-*` and
`mac-source-browsers-native-v1`/`v2`, `source-browser-public-frame-*`,
`linux-source-browsers-native-v1-evidence`/`v2-evidence`, and
`linux-source-browsers-full-v2-evidence`,
`{linux,windows}-source-browsers-native-v4-evidence`,
`windows-source-browsers-original-v4-evidence`, `mac-source-browsers-native-v6`,
`mac-source-browsers-final-full-gate.log`/`mac-source-browsers-final-full-LastTest.log`
and `source-browsers-final-input-proof.json`.
